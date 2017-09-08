/* Copyright (C) 2017 Wildfire Games.
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
#include "ps/Profile.h"


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

	self->m_ScriptInterface = new ScriptInterface("RMS", "MapGenerator", ScriptInterface::CreateRuntime(g_ScriptRuntime, RMS_RUNTIME_SIZE));

	// Run map generation scripts
	if (!self->Run() || self->m_Progress > 0)
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
	// We must destroy the ScriptInterface in the same thread because the JSAPI requires that!
	// Also we must not be in a request when calling the ScriptInterface destructor, so the autoFree object
	// must be instantiated before the request (destructors are called in reverse order of instantiation)
	struct AutoFree {
		AutoFree(ScriptInterface* p) : m_p(p) {}
		~AutoFree() { SAFE_DELETE(m_p); }
		ScriptInterface* m_p;
	} autoFree(m_ScriptInterface);

	JSContext* cx = m_ScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	m_ScriptInterface->SetCallbackData(static_cast<void*> (this));

	// Replace RNG with a seeded deterministic function
	m_ScriptInterface->ReplaceNondeterministicRNG(m_MapGenRNG);
	m_ScriptInterface->LoadGlobalScripts();

	// Functions for RMS
	m_ScriptInterface->RegisterFunction<bool, std::wstring, CMapGeneratorWorker::LoadLibrary>("LoadLibrary");
	m_ScriptInterface->RegisterFunction<void, JS::HandleValue, CMapGeneratorWorker::ExportMap>("ExportMap");
	m_ScriptInterface->RegisterFunction<void, int, CMapGeneratorWorker::SetProgress>("SetProgress");
	m_ScriptInterface->RegisterFunction<void, CMapGeneratorWorker::MaybeGC>("MaybeGC");
	m_ScriptInterface->RegisterFunction<bool, std::wstring, CMapGeneratorWorker::FileExists>("FileExists");
	m_ScriptInterface->RegisterFunction<JS::Value, std::wstring, CMapGeneratorWorker::ReadJSONFile> ("ReadJSONFile");
	m_ScriptInterface->RegisterFunction<std::vector<std::string>, CMapGeneratorWorker::GetCivData>("GetCivData");
	m_ScriptInterface->RegisterFunction<CParamNode, std::string, CMapGeneratorWorker::GetTemplate>("GetTemplate");
	m_ScriptInterface->RegisterFunction<bool, std::string, CMapGeneratorWorker::TemplateExists>("TemplateExists");
	m_ScriptInterface->RegisterFunction<std::vector<std::string>, std::string, bool, CMapGeneratorWorker::FindTemplates>("FindTemplates");
	m_ScriptInterface->RegisterFunction<std::vector<std::string>, std::string, bool, CMapGeneratorWorker::FindActorTemplates>("FindActorTemplates");

	// Parse settings
	JS::RootedValue settingsVal(cx);
	if (!m_ScriptInterface->ParseJSON(m_Settings, &settingsVal) && settingsVal.isUndefined())
	{
		LOGERROR("CMapGeneratorWorker::Run: Failed to parse settings");
		return false;
	}

	// Init RNG seed
	u32 seed = 0;
	if (!m_ScriptInterface->HasProperty(settingsVal, "Seed") ||
	    !m_ScriptInterface->GetProperty(settingsVal, "Seed", seed))
		LOGWARNING("CMapGeneratorWorker::Run: No seed value specified - using 0");

	m_MapGenRNG.seed(seed);

	// Copy settings to global variable
	JS::RootedValue global(cx, m_ScriptInterface->GetGlobalObject());
	if (!m_ScriptInterface->SetProperty(global, "g_MapSettings", settingsVal))
	{
		LOGERROR("CMapGeneratorWorker::Run: Failed to define g_MapSettings");
		return false;
	}

	// Load RMS
	LOGMESSAGE("Loading RMS '%s'", m_ScriptPath.string8());
	if (!m_ScriptInterface->LoadGlobalScriptFile(m_ScriptPath))
	{
		LOGERROR("CMapGeneratorWorker::Run: Failed to load RMS '%s'", m_ScriptPath.string8());
		return false;
	}

	return true;
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

bool CMapGeneratorWorker::LoadLibrary(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& name)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(pCxPrivate->pCBData);
	return self->LoadScripts(name);
}

void CMapGeneratorWorker::ExportMap(ScriptInterface::CxPrivate* pCxPrivate, JS::HandleValue data)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(pCxPrivate->pCBData);

	// Copy results
	CScopeLock lock(self->m_WorkerMutex);
	self->m_MapData = self->m_ScriptInterface->WriteStructuredClone(data);
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

bool CMapGeneratorWorker::FileExists(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& filePath)
{
	return g_VFS->GetFileInfo(filePath, 0) == INFO::OK;
}

JS::Value CMapGeneratorWorker::ReadJSONFile(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filePath)
{
	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue out(cx);
	pCxPrivate->pScriptInterface->ReadJSONFile(filePath, &out);
	return out;
}

std::vector<std::string> CMapGeneratorWorker::GetCivData(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	VfsPath path(L"simulation/data/civs/");
	VfsPaths pathnames;

	std::vector<std::string> data;

	// Load all JSON files in civs directory
	Status ret = vfs::GetPathnames(g_VFS, path, L"*.json", pathnames);
	if (ret == INFO::OK)
	{
		for (const VfsPath& p : pathnames)
		{
			// Load JSON file
			CVFSFile file;
			PSRETURN ret = file.Load(g_VFS, p);
			if (ret != PSRETURN_OK)
				LOGERROR("CMapGeneratorWorker::GetCivData: Failed to load file '%s': %s", p.string8(), GetErrorString(ret));
			else
				data.push_back(file.DecodeUTF8()); // assume it's UTF-8
		}
	}
	else
	{
		// Some error reading directory
		wchar_t error[200];
		LOGERROR("CMapGeneratorWorker::GetCivData: Error reading directory '%s': %s", path.string8(), utf8_from_wstring(StatusDescription(ret, error, ARRAY_SIZE(error))));
	}

	return data;

}

CParamNode CMapGeneratorWorker::GetTemplate(ScriptInterface::CxPrivate* pCxPrivate, const std::string& templateName)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(pCxPrivate->pCBData);
	const CParamNode& templateRoot = self->m_TemplateLoader.GetTemplateFileData(templateName).GetChild("Entity");
	if (!templateRoot.IsOk())
		LOGERROR("Invalid template found for '%s'", templateName.c_str());

	return templateRoot;
}

bool CMapGeneratorWorker::TemplateExists(ScriptInterface::CxPrivate* pCxPrivate, const std::string& templateName)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(pCxPrivate->pCBData);
	return self->m_TemplateLoader.TemplateExists(templateName);
}

std::vector<std::string> CMapGeneratorWorker::FindTemplates(ScriptInterface::CxPrivate* pCxPrivate, const std::string& path, bool includeSubdirectories)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(pCxPrivate->pCBData);
	return self->m_TemplateLoader.FindTemplates(path, includeSubdirectories, SIMULATION_TEMPLATES);
}

std::vector<std::string> CMapGeneratorWorker::FindActorTemplates(ScriptInterface::CxPrivate* pCxPrivate, const std::string& path, bool includeSubdirectories)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(pCxPrivate->pCBData);
	return self->m_TemplateLoader.FindTemplates(path, includeSubdirectories, ACTOR_TEMPLATES);
}

bool CMapGeneratorWorker::LoadScripts(const std::wstring& libraryName)
{
	// Ignore libraries that are already loaded
	if (m_LoadedLibraries.find(libraryName) != m_LoadedLibraries.end())
		return true;

	// Mark this as loaded, to prevent it recursively loading itself
	m_LoadedLibraries.insert(libraryName);

	VfsPath path = L"maps/random/" + libraryName + L"/";
	VfsPaths pathnames;

	// Load all scripts in mapgen directory
	Status ret = vfs::GetPathnames(g_VFS, path, L"*.js", pathnames);
	if (ret == INFO::OK)
	{
		for (const VfsPath& p : pathnames)
		{
			LOGMESSAGE("Loading map generator script '%s'", p.string8());

			if (!m_ScriptInterface->LoadGlobalScriptFile(p))
			{
				LOGERROR("CMapGeneratorWorker::LoadScripts: Failed to load script '%s'", p.string8());
				return false;
			}
		}
	}
	else
	{
		// Some error reading directory
		wchar_t error[200];
		LOGERROR("CMapGeneratorWorker::LoadScripts: Error reading scripts in directory '%s': %s", path.string8(), utf8_from_wstring(StatusDescription(ret, error, ARRAY_SIZE(error))));
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
