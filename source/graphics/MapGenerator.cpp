/* Copyright (C) 2019 Wildfire Games.
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

#include "graphics/MapIO.h"
#include "graphics/Patch.h"
#include "graphics/Terrain.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/status.h"
#include "lib/timer.h"
#include "lib/file/vfs/vfs_path.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/FileIo.h"
#include "ps/Profile.h"
#include "ps/scripting/JSInterface_VFS.h"
#include "scriptinterface/ScriptRuntime.h"
#include "scriptinterface/ScriptConversions.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/helpers/MapEdgeTiles.h"

#include <string>
#include <vector>

// TODO: what's a good default? perhaps based on map size
#define RMS_RUNTIME_SIZE 96 * 1024 * 1024

extern bool IsQuitRequested();

static bool
MapGeneratorInterruptCallback(JSContext* UNUSED(cx))
{
	// This may not use SDL_IsQuitRequested(), because it runs in a thread separate to SDL, see SDL_PumpEvents
	if (IsQuitRequested())
	{
		LOGWARNING("Quit requested!");
		return false;
	}

	return true;
}

CMapGeneratorWorker::CMapGeneratorWorker()
{
	// If something happens before we initialize, that's a failure
	m_Progress = -1;
}

CMapGeneratorWorker::~CMapGeneratorWorker()
{
	// Wait for thread to end
	m_WorkerThread.join();
}

void CMapGeneratorWorker::Initialize(const VfsPath& scriptFile, const std::string& settings)
{
	std::lock_guard<std::mutex> lock(m_WorkerMutex);

	// Set progress to positive value
	m_Progress = 1;
	m_ScriptPath = scriptFile;
	m_Settings = settings;

	// Launch the worker thread
	m_WorkerThread = std::thread(RunThread, this);
}

void* CMapGeneratorWorker::RunThread(CMapGeneratorWorker* self)
{
	debug_SetThreadName("MapGenerator");
	g_Profiler2.RegisterCurrentThread("MapGenerator");

	shared_ptr<ScriptRuntime> mapgenRuntime = ScriptInterface::CreateRuntime(g_ScriptRuntime, RMS_RUNTIME_SIZE);

	// Enable the script to be aborted
	JS_SetInterruptCallback(mapgenRuntime->m_rt, MapGeneratorInterruptCallback);

	self->m_ScriptInterface = new ScriptInterface("Engine", "MapGenerator", mapgenRuntime);

	// Run map generation scripts
	if (!self->Run() || self->m_Progress > 0)
	{
		// Don't leave progress in an unknown state, if generator failed, set it to -1
		std::lock_guard<std::mutex> lock(self->m_WorkerMutex);
		self->m_Progress = -1;
	}

	SAFE_DELETE(self->m_ScriptInterface);

	// At this point the random map scripts are done running, so the thread has no further purpose
	//	and can die. The data will be stored in m_MapData already if successful, or m_Progress
	//	will contain an error value on failure.

	return NULL;
}

bool CMapGeneratorWorker::Run()
{
	JSContext* cx = m_ScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	m_ScriptInterface->SetCallbackData(static_cast<void*> (this));

	// Replace RNG with a seeded deterministic function
	m_ScriptInterface->ReplaceNondeterministicRNG(m_MapGenRNG);

	RegisterScriptFunctions();

	// Parse settings
	JS::RootedValue settingsVal(cx);
	if (!m_ScriptInterface->ParseJSON(m_Settings, &settingsVal) && settingsVal.isUndefined())
	{
		LOGERROR("CMapGeneratorWorker::Run: Failed to parse settings");
		return false;
	}

	// Prevent unintentional modifications to the settings object by random map scripts
	if (!m_ScriptInterface->FreezeObject(settingsVal, true))
	{
		LOGERROR("CMapGeneratorWorker::Run: Failed to deepfreeze settings");
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
	if (!m_ScriptInterface->SetProperty(global, "g_MapSettings", settingsVal, true, true))
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

void CMapGeneratorWorker::RegisterScriptFunctions()
{
	// VFS
	JSI_VFS::RegisterScriptFunctions_Maps(*m_ScriptInterface);

	// Globalscripts may use VFS script functions
	m_ScriptInterface->LoadGlobalScripts();

	// File loading
	m_ScriptInterface->RegisterFunction<bool, VfsPath, CMapGeneratorWorker::LoadLibrary>("LoadLibrary");
	m_ScriptInterface->RegisterFunction<JS::Value, VfsPath, CMapGeneratorWorker::LoadHeightmap>("LoadHeightmapImage");
	m_ScriptInterface->RegisterFunction<JS::Value, VfsPath, CMapGeneratorWorker::LoadMapTerrain>("LoadMapTerrain");

	// Progression and profiling
	m_ScriptInterface->RegisterFunction<void, int, CMapGeneratorWorker::SetProgress>("SetProgress");
	m_ScriptInterface->RegisterFunction<double, CMapGeneratorWorker::GetMicroseconds>("GetMicroseconds");
	m_ScriptInterface->RegisterFunction<void, JS::HandleValue, CMapGeneratorWorker::ExportMap>("ExportMap");

	// Template functions
	m_ScriptInterface->RegisterFunction<CParamNode, std::string, CMapGeneratorWorker::GetTemplate>("GetTemplate");
	m_ScriptInterface->RegisterFunction<bool, std::string, CMapGeneratorWorker::TemplateExists>("TemplateExists");
	m_ScriptInterface->RegisterFunction<std::vector<std::string>, std::string, bool, CMapGeneratorWorker::FindTemplates>("FindTemplates");
	m_ScriptInterface->RegisterFunction<std::vector<std::string>, std::string, bool, CMapGeneratorWorker::FindActorTemplates>("FindActorTemplates");

	// Engine constants

	// Length of one tile of the terrain grid in metres.
	// Useful to transform footprint sizes to the tilegrid coordinate system.
	m_ScriptInterface->SetGlobal("TERRAIN_TILE_SIZE", static_cast<int>(TERRAIN_TILE_SIZE));

	// Number of impassable tiles at the map border
	m_ScriptInterface->SetGlobal("MAP_BORDER_WIDTH", static_cast<int>(MAP_EDGE_TILES));
}

int CMapGeneratorWorker::GetProgress()
{
	std::lock_guard<std::mutex> lock(m_WorkerMutex);
	return m_Progress;
}

double CMapGeneratorWorker::GetMicroseconds(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return JS_Now();
}

shared_ptr<ScriptInterface::StructuredClone> CMapGeneratorWorker::GetResults()
{
	std::lock_guard<std::mutex> lock(m_WorkerMutex);
	return m_MapData;
}

bool CMapGeneratorWorker::LoadLibrary(ScriptInterface::CxPrivate* pCxPrivate, const VfsPath& name)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(pCxPrivate->pCBData);
	return self->LoadScripts(name);
}

void CMapGeneratorWorker::ExportMap(ScriptInterface::CxPrivate* pCxPrivate, JS::HandleValue data)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(pCxPrivate->pCBData);

	// Copy results
	std::lock_guard<std::mutex> lock(self->m_WorkerMutex);
	self->m_MapData = self->m_ScriptInterface->WriteStructuredClone(data);
	self->m_Progress = 0;
}

void CMapGeneratorWorker::SetProgress(ScriptInterface::CxPrivate* pCxPrivate, int progress)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(pCxPrivate->pCBData);

	// Copy data
	std::lock_guard<std::mutex> lock(self->m_WorkerMutex);

	if (progress >= self->m_Progress)
		self->m_Progress = progress;
	else
		LOGWARNING("The random map script tried to reduce the loading progress from %d to %d", self->m_Progress, progress);
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

bool CMapGeneratorWorker::LoadScripts(const VfsPath& libraryName)
{
	// Ignore libraries that are already loaded
	if (m_LoadedLibraries.find(libraryName) != m_LoadedLibraries.end())
		return true;

	// Mark this as loaded, to prevent it recursively loading itself
	m_LoadedLibraries.insert(libraryName);

	VfsPath path = VfsPath(L"maps/random/") / libraryName / VfsPath();
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

JS::Value CMapGeneratorWorker::LoadHeightmap(ScriptInterface::CxPrivate* pCxPrivate, const VfsPath& filename)
{
	std::vector<u16> heightmap;
	if (LoadHeightmapImageVfs(filename, heightmap) != INFO::OK)
	{
		LOGERROR("Could not load heightmap file '%s'", filename.string8());
		return JS::UndefinedValue();
	}

	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(pCxPrivate->pCBData);
	JSContext* cx = self->m_ScriptInterface->GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue returnValue(cx);
	ToJSVal_vector(cx, &returnValue, heightmap);
	return returnValue;
}

// See CMapReader::UnpackTerrain, CMapReader::ParseTerrain for the reordering
JS::Value CMapGeneratorWorker::LoadMapTerrain(ScriptInterface::CxPrivate* pCxPrivate, const VfsPath& filename)
{
	CMapGeneratorWorker* self = static_cast<CMapGeneratorWorker*>(pCxPrivate->pCBData);
	JSContext* cx = self->m_ScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	if (!VfsFileExists(filename))
	{
		self->m_ScriptInterface->ReportError(
			("Terrain file \"" +  filename.string8() +  "\" does not exist!").c_str());

		return JS::UndefinedValue();
	}

	CFileUnpacker unpacker;
	unpacker.Read(filename, "PSMP");

	if (unpacker.GetVersion() < CMapIO::FILE_READ_VERSION)
	{
		self->m_ScriptInterface->ReportError(
			("Could not load terrain file \"" +  filename.string8() +  "\" too old version!").c_str());

		return JS::UndefinedValue();
	}

	// unpack size
	ssize_t patchesPerSide = (ssize_t)unpacker.UnpackSize();
	size_t verticesPerSide = patchesPerSide * PATCH_SIZE + 1;

	// unpack heightmap
	std::vector<u16> heightmap;
	heightmap.resize(SQR(verticesPerSide));
	unpacker.UnpackRaw(&heightmap[0], SQR(verticesPerSide) * sizeof(u16));

	// unpack texture names
	size_t textureCount = unpacker.UnpackSize();
	std::vector<std::string> textureNames;
	textureNames.reserve(textureCount);
	for (size_t i = 0; i < textureCount; ++i)
	{
		CStr texturename;
		unpacker.UnpackString(texturename);
		textureNames.push_back(texturename);
	}

	// unpack texture IDs per tile
	ssize_t tilesPerSide = patchesPerSide * PATCH_SIZE;
	std::vector<CMapIO::STileDesc> tiles;
	tiles.resize(size_t(SQR(tilesPerSide)));
	unpacker.UnpackRaw(&tiles[0], sizeof(CMapIO::STileDesc) * tiles.size());

	// reorder by patches and store and save texture IDs per tile
	std::vector<u16> textureIDs;
	for (ssize_t x = 0; x < tilesPerSide; ++x)
	{
		size_t patchX = x / PATCH_SIZE;
		size_t offX = x % PATCH_SIZE;
		for (ssize_t y = 0; y < tilesPerSide; ++y)
		{
			size_t patchY = y / PATCH_SIZE;
			size_t offY = y % PATCH_SIZE;
			// m_Priority and m_Tex2Index unused
			textureIDs.push_back(tiles[(patchY * patchesPerSide + patchX) * SQR(PATCH_SIZE) + (offY * PATCH_SIZE + offX)].m_Tex1Index);
		}
	}

	JS::RootedValue returnValue(cx);

	ScriptInterface::CreateObject(
		cx,
		&returnValue,
		"height", heightmap,
		"textureNames", textureNames,
		"textureIDs", textureIDs);

	return returnValue;
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
