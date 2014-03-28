/* Copyright (C) 2012 Wildfire Games.
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

#include "lib/timer.h"
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

	// Launch the worker thread
	int ret = pthread_create(&m_WorkerThread, NULL, &RunThread, this);
	ENSURE(ret == 0);
}

void* CMapGeneratorWorker::RunThread(void *data)
{
	debug_SetThreadName("MapGenerator");
	g_Profiler2.RegisterCurrentThread("MapGenerator");

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

	return NULL;
}

bool CMapGeneratorWorker::Run()
{
	m_ScriptInterface->SetCallbackData(static_cast<void*> (this));

	// Replace RNG with a seeded deterministic function
	m_ScriptInterface->ReplaceNondeterministicRNG(m_MapGenRNG);
	m_ScriptInterface->LoadGlobalScripts();

	// Functions for RMS
	m_ScriptInterface->RegisterFunction<bool, std::wstring, CMapGeneratorWorker::LoadLibrary>("LoadLibrary");
	m_ScriptInterface->RegisterFunction<void, CScriptValRooted, CMapGeneratorWorker::ExportMap>("ExportMap");
	m_ScriptInterface->RegisterFunction<void, int, CMapGeneratorWorker::SetProgress>("SetProgress");
	m_ScriptInterface->RegisterFunction<void, CMapGeneratorWorker::MaybeGC>("MaybeGC");
	m_ScriptInterface->RegisterFunction<std::vector<std::string>, CMapGeneratorWorker::GetCivData>("GetCivData");

	// TODO: This code is a bit ugly because we have to ensure that CScriptValRooted gets destroyed before the ScriptInterface.
	// In the future we should work more with the standard JSAPI types for rooting on the stack, which should avoid such problems.
	bool ret = true;
	{
		// Parse settings
		CScriptValRooted settingsVal = m_ScriptInterface->ParseJSON(m_Settings);
		if (settingsVal.undefined())
		{
			LOGERROR(L"CMapGeneratorWorker::Run: Failed to parse settings");
			ret = false;
		} 
		else
		{
			// Init RNG seed
			u32 seed;
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
				ret = false;
			}
			else
			{
				// Load RMS
				LOGMESSAGE(L"Loading RMS '%ls'", m_ScriptPath.string().c_str());
				if (!m_ScriptInterface->LoadGlobalScriptFile(m_ScriptPath))
				{
					LOGERROR(L"CMapGeneratorWorker::Run: Failed to load RMS '%ls'", m_ScriptPath.string().c_str());
					ret = false;
				}
			}
		}
	}
	
	// We must destroy the ScriptInterface in the same thread because the JSAPI requires that!
	SAFE_DELETE(m_ScriptInterface);

	return ret;
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

bool CMapGeneratorWorker::LoadLibrary(ScriptInterface::CxPrivate* pCxPrivate, std::wstring name)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(pCxPrivate->pCBData);
	return self->LoadScripts(name);
}

void CMapGeneratorWorker::ExportMap(ScriptInterface::CxPrivate* pCxPrivate, CScriptValRooted data)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(pCxPrivate->pCBData);

	// Copy results
	CScopeLock lock(self->m_WorkerMutex);
	self->m_MapData = self->m_ScriptInterface->WriteStructuredClone(data.get());
	self->m_Progress = 0;
}

void CMapGeneratorWorker::SetProgress(ScriptInterface::CxPrivate* pCxPrivate, int progress)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(pCxPrivate->pCBData);

	// Copy data
	CScopeLock lock(self->m_WorkerMutex);
	self->m_Progress = progress;
}

void CMapGeneratorWorker::MaybeGC(ScriptInterface::CxPrivate* pCxPrivate)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(pCxPrivate->pCBData);
	self->m_ScriptInterface->MaybeGC();
}

std::vector<std::string> CMapGeneratorWorker::GetCivData(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	VfsPath path(L"civs/");
	VfsPaths pathnames;

	std::vector<std::string> data;

	// Load all JSON files in civs directory
	Status ret = vfs::GetPathnames(g_VFS, path, L"*.json", pathnames);
	if (ret == INFO::OK)
	{
		for (VfsPaths::iterator it = pathnames.begin(); it != pathnames.end(); ++it)
		{
			// Load JSON file
			CVFSFile file;
			PSRETURN ret = file.Load(g_VFS, *it);
			if (ret != PSRETURN_OK)
			{
				LOGERROR(L"CMapGeneratorWorker::GetCivData: Failed to load file '%ls': %hs", path.string().c_str(), GetErrorString(ret));
			}
			else
			{
				data.push_back(file.DecodeUTF8()); // assume it's UTF-8
			}
		}
	}
	else
	{
		// Some error reading directory
		wchar_t error[200];
		LOGERROR(L"CMapGeneratorWorker::GetCivData: Error reading directory '%ls': %ls", path.string().c_str(), StatusDescription(ret, error, ARRAY_SIZE(error)));
	}

	return data;

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

	VfsPath path = L"maps/random/" + libraryName + L"/";
	VfsPaths pathnames;

	// Load all scripts in mapgen directory
	Status ret = vfs::GetPathnames(g_VFS, path, L"*.js", pathnames);
	if (ret == INFO::OK)
	{
		for (VfsPaths::iterator it = pathnames.begin(); it != pathnames.end(); ++it)
		{
			LOGMESSAGE(L"Loading map generator script '%ls'", it->string().c_str());

			if (!m_ScriptInterface->LoadGlobalScriptFile(*it))
			{
				LOGERROR(L"CMapGeneratorWorker::LoadScripts: Failed to load script '%ls'", it->string().c_str());
				return false;
			}
		}
	}
	else
	{
		// Some error reading directory
		wchar_t error[200];
		LOGERROR(L"CMapGeneratorWorker::LoadScripts: Error reading scripts in directory '%ls': %ls", path.string().c_str(), StatusDescription(ret, error, ARRAY_SIZE(error)));
		return false;
	}

	return true;
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
