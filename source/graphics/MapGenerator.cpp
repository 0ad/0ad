/* Copyright (C) 2011 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "MapGenerator.h"

#include "lib/file/vfs/vfs_path.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"

// TODO: what's a good default? perhaps based on map size
#define RMS_RUNTIME_SIZE 96 * 1024 * 1024


CMapGeneratorWorker::CMapGeneratorWorker()
{
	// If something happens before we initialize, that's a failure
	m_Progress = -1;
}

CMapGeneratorWorker::~CMapGeneratorWorker()
{
	// Wait for thread to end
	pthread_join(m_WorkerThread, NULL);
}

void CMapGeneratorWorker::Initialize(const VfsPath& scriptFile, const std::string& settings)
{
	CScopeLock lock(m_WorkerMutex);

	// Set progress to positive value
	m_Progress = 1;
	m_ScriptPath = scriptFile;
	m_Settings = settings;

	// Preload random map and library scripts
	fs_util::ForEachFile(g_VFS, L"maps/random/", PreloadScript, (uintptr_t)this, L"*.js", fs_util::DIR_RECURSIVE);

	// Launch the worker thread
	int ret = pthread_create(&m_WorkerThread, NULL, &RunThread, this);
	debug_assert(ret == 0);
}

void* CMapGeneratorWorker::RunThread(void *data)
{
	debug_SetThreadName("MapGenerator");

	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(data);

	self->m_ScriptInterface = new ScriptInterface("RMS", "MapGenerator", ScriptInterface::CreateRuntime(RMS_RUNTIME_SIZE));

	// Run map generation scripts
	if ((!self->Run()) || (self->m_Progress > 0))
	{
		// Don't leave progress in an unknown state, if generator failed, set it to -1
		CScopeLock lock(self->m_WorkerMutex);
		self->m_Progress = -1;
	}

	// At this point the random map scripts are done running, so the thread has no further purpose
	//	and can die. The data will be stored in m_MapData already if successful, or m_Progress
	//	will contain an error value on failure.

	// Cleanup ScriptInterface
	SAFE_DELETE(self->m_ScriptInterface);

	return NULL;
}

bool CMapGeneratorWorker::Run()
{
	m_ScriptInterface->SetCallbackData(static_cast<void*> (this));

	// Replace RNG with a seeded deterministic function
	m_ScriptInterface->ReplaceNondeterministicFunctions(m_MapGenRNG);

	// Functions for RMS
	m_ScriptInterface->RegisterFunction<bool, std::wstring, CMapGeneratorWorker::LoadLibrary>("LoadLibrary");
	m_ScriptInterface->RegisterFunction<void, CScriptValRooted, CMapGeneratorWorker::ExportMap>("ExportMap");
	m_ScriptInterface->RegisterFunction<void, int, CMapGeneratorWorker::SetProgress>("SetProgress");
	m_ScriptInterface->RegisterFunction<void, CMapGeneratorWorker::MaybeGC>("MaybeGC");

	// Parse settings
	CScriptValRooted settingsVal = m_ScriptInterface->ParseJSON(m_Settings);
	if (settingsVal.undefined())
	{
		LOGERROR(L"CMapGeneratorWorker::Run: Failed to parse settings");
		return false;
	}

	// Init RNG seed
	uint32 seed;
	if (!m_ScriptInterface->GetProperty(settingsVal.get(), "Seed", seed))
	{	// No seed specified
		LOGWARNING(L"CMapGeneratorWorker::Run: No seed value specified - using 0");
		seed = 0;
	}

	m_MapGenRNG.seed(seed);

	// Copy settings to global variable
	if (!m_ScriptInterface->SetProperty(m_ScriptInterface->GetGlobalObject(), "g_MapSettings", settingsVal))
	{
		LOGERROR(L"CMapGeneratorWorker::Run: Failed to define g_MapSettings");
		return false;
	}

	// Find RMS file and run
	ScriptFilesMap::iterator it = m_ScriptFiles.find(m_ScriptPath);
	if (it != m_ScriptFiles.end())
	{
		LOGMESSAGE(L"CMapGeneratorWorker::Run: Loading RMS '%ls'", m_ScriptPath.string().c_str());
		// Note: we're not really accessing the file here
		if (m_ScriptInterface->LoadGlobalScript(m_ScriptPath, it->second))
		{
			return true;
		}
	}

	LOGERROR(L"CMapGeneratorWorker::Run: Failed to load RMS '%ls'", m_ScriptPath.string().c_str());
	return false;
}

int CMapGeneratorWorker::GetProgress()
{
	CScopeLock lock(m_WorkerMutex);
	return m_Progress;
}

shared_ptr<ScriptInterface::StructuredClone> CMapGeneratorWorker::GetResults()
{
	CScopeLock lock(m_WorkerMutex);
	return m_MapData;
}

bool CMapGeneratorWorker::LoadLibrary(void* cbdata, std::wstring name)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(cbdata);

	return self->LoadScripts(name);
}

void CMapGeneratorWorker::ExportMap(void* cbdata, CScriptValRooted data)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(cbdata);

	// Copy results
	CScopeLock lock(self->m_WorkerMutex);
	self->m_MapData = self->m_ScriptInterface->WriteStructuredClone(data.get());
	self->m_Progress = 0;
}

void CMapGeneratorWorker::SetProgress(void* cbdata, int progress)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(cbdata);

	// Copy data
	CScopeLock lock(self->m_WorkerMutex);
	self->m_Progress = progress;
}

void CMapGeneratorWorker::MaybeGC(void* cbdata)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(cbdata);
	self->m_ScriptInterface->MaybeGC();
}

bool CMapGeneratorWorker::LoadScripts(const std::wstring& libraryName)
{
	// Ignore libraries that are already loaded
	if (m_LoadedLibraries.find(libraryName) != m_LoadedLibraries.end())
	{
		return true;
	}

	// Mark this as loaded, to prevent it recursively loading itself
	m_LoadedLibraries.insert(libraryName);

	LOGMESSAGE(L"Loading '%ls' library", libraryName.c_str());

	std::wstring libraryPath = L"maps/random/" + libraryName + L"/";

	// Iterate preloaded script map, running library scripts
	for (ScriptFilesMap::iterator it = m_ScriptFiles.begin(); it != m_ScriptFiles.end(); ++it)
	{
		std::wstring path = it->first.string();
		if (path.find(libraryPath) == 0)
		{
			// This script is part of the library, so load it
			// Note: we're not really accessing the file here
			if (m_ScriptInterface->LoadGlobalScript(path, it->second))
			{
				LOGMESSAGE(L"Successfully loaded library script '%ls'", path.c_str());
			}
			else
			{
				// Script failed to run
				LOGERROR(L"Failed loading library script '%ls'", path.c_str());
				return false;
			}
		}
	}

	return true;
}

LibError CMapGeneratorWorker::PreloadScript(const VfsPath& pathname, const FileInfo& UNUSED(fileInfo), const uintptr_t cbData)
{
	CMapGeneratorWorker* self = (CMapGeneratorWorker*)cbData;

	if (!VfsFileExists(g_VFS, pathname))
	{
		// This should never happen
		LOGERROR(L"CMapGeneratorWorker::PreloadScript: File '%ls' does not exist", pathname.string().c_str());
		return ERR::VFS_FILE_NOT_FOUND;
	}

	CVFSFile file;

	PSRETURN ret = file.Load(g_VFS, pathname);
	if (ret != PSRETURN_OK)
	{
		LOGERROR(L"CMapGeneratorWorker::PreloadScript: Failed to load file '%ls', error=%hs", pathname.string().c_str(), GetErrorString(ret));
		return ERR::FAIL;
	}

	std::string content(file.GetBuffer(), file.GetBuffer() + file.GetBufferSize());

	self->m_ScriptFiles.insert(std::make_pair(pathname, wstring_from_utf8(content)));
	return INFO::OK;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////


CMapGenerator::CMapGenerator() : m_Worker(new CMapGeneratorWorker())
{
}

CMapGenerator::~CMapGenerator()
{
	delete m_Worker;
}

void CMapGenerator::GenerateMap(const VfsPath& scriptFile, const std::string& settings)
{
	m_Worker->Initialize(scriptFile, settings);
}

int CMapGenerator::GetProgress()
{
	return m_Worker->GetProgress();
}

shared_ptr<ScriptInterface::StructuredClone> CMapGenerator::GetResults()
{
	return m_Worker->GetResults();
}
