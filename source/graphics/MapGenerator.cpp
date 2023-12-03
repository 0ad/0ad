/* Copyright (C) 2023 Wildfire Games.
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
#include "lib/status.h"
#include "lib/timer.h"
#include "lib/file/vfs/vfs_path.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/FileIo.h"
#include "ps/Profile.h"
#include "ps/scripting/JSInterface_VFS.h"
#include "ps/TemplateLoader.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/JSON.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptContext.h"
#include "scriptinterface/ScriptConversions.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/helpers/MapEdgeTiles.h"

#include <boost/random/linear_congruential.hpp>
#include <set>
#include <string>
#include <vector>

extern bool IsQuitRequested();

namespace
{
bool MapGenerationInterruptCallback(JSContext* UNUSED(cx))
{
	// This may not use SDL_IsQuitRequested(), because it runs in a thread separate to SDL, see SDL_PumpEvents
	if (IsQuitRequested())
	{
		LOGWARNING("Quit requested!");
		return false;
	}

	return true;
}

/**
 * Provides callback's for the JavaScript.
 */
class CMapGenerationCallbacks
{
public:
	// Only the constructor and the destructor are called by C++.

	CMapGenerationCallbacks(std::atomic<int>& progress, ScriptInterface& scriptInterface,
		Script::StructuredClone& mapData, const u16 flags) :
		m_Progress{progress},
		m_ScriptInterface{scriptInterface},
		m_MapData{mapData}
	{
		m_ScriptInterface.SetCallbackData(static_cast<void*>(this));

		// Enable the script to be aborted
		JS_AddInterruptCallback(m_ScriptInterface.GetGeneralJSContext(),
			&MapGenerationInterruptCallback);

		// Set initial seed, callback data.
		// Expose functions, globals and classes relevant to the map scripts.
#define REGISTER_MAPGEN_FUNC(func) \
	ScriptFunction::Register<&CMapGenerationCallbacks::func, \
		ScriptInterface::ObjectFromCBData<CMapGenerationCallbacks>>(rq, #func, flags);

		// VFS
		JSI_VFS::RegisterScriptFunctions_ReadOnlySimulationMaps(m_ScriptInterface, flags);

		// Globalscripts may use VFS script functions
		m_ScriptInterface.LoadGlobalScripts();

		// File loading
		ScriptRequest rq(m_ScriptInterface);
		REGISTER_MAPGEN_FUNC(LoadLibrary);
		REGISTER_MAPGEN_FUNC(LoadHeightmapImage);
		REGISTER_MAPGEN_FUNC(LoadMapTerrain);

		// Template functions
		REGISTER_MAPGEN_FUNC(GetTemplate);
		REGISTER_MAPGEN_FUNC(TemplateExists);
		REGISTER_MAPGEN_FUNC(FindTemplates);
		REGISTER_MAPGEN_FUNC(FindActorTemplates);

		// Progression and profiling
		REGISTER_MAPGEN_FUNC(SetProgress);
		REGISTER_MAPGEN_FUNC(GetMicroseconds);
		REGISTER_MAPGEN_FUNC(ExportMap);

		// Engine constants

		// Length of one tile of the terrain grid in metres.
		// Useful to transform footprint sizes to the tilegrid coordinate system.
		m_ScriptInterface.SetGlobal("TERRAIN_TILE_SIZE", static_cast<int>(TERRAIN_TILE_SIZE));

		// Number of impassable tiles at the map border
		m_ScriptInterface.SetGlobal("MAP_BORDER_WIDTH", static_cast<int>(MAP_EDGE_TILES));

#undef REGISTER_MAPGEN_FUNC
	}

	~CMapGenerationCallbacks()
	{
		JS_AddInterruptCallback(m_ScriptInterface.GetGeneralJSContext(), nullptr);
		m_ScriptInterface.SetCallbackData(nullptr);
	}

private:

	// These functions are called by JS.

	/**
	 * Load all scripts of the given library
	 *
	 * @param libraryName VfsPath specifying name of the library (subfolder of ../maps/random/)
	 * @return true if all scripts ran successfully, false if there's an error
	 */
	bool LoadLibrary(const VfsPath& libraryName)
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

				if (!m_ScriptInterface.LoadGlobalScriptFile(p))
				{
					LOGERROR("CMapGenerationCallbacks::LoadScripts: Failed to load script '%s'",
						p.string8());
					return false;
				}
			}
		}
		else
		{
			// Some error reading directory
			wchar_t error[200];
			LOGERROR(
				"CMapGenerationCallbacks::LoadScripts: Error reading scripts in directory '%s': %s",
				path.string8(),
				utf8_from_wstring(StatusDescription(ret, error, ARRAY_SIZE(error))));
			return false;
		}

		return true;
	}

	/**
	 * Finalize map generation and pass results from the script to the engine.
	 * The `data` has to be according to this format:
	 * https://trac.wildfiregames.com/wiki/Random_Map_Generator_Internals#Dataformat
	 */
	void ExportMap(JS::HandleValue data)
	{
		// Copy results
		m_MapData = Script::WriteStructuredClone(ScriptRequest(m_ScriptInterface), data);
	}

	/**
	 * Load an image file and return it as a height array.
	 */
	JS::Value LoadHeightmapImage(const VfsPath& filename)
	{
		std::vector<u16> heightmap;
		if (LoadHeightmapImageVfs(filename, heightmap) != INFO::OK)
		{
			LOGERROR("Could not load heightmap file '%s'", filename.string8());
			return JS::UndefinedValue();
		}

		ScriptRequest rq(m_ScriptInterface);
		JS::RootedValue returnValue(rq.cx);
		Script::ToJSVal(rq, &returnValue, heightmap);
		return returnValue;
	}

	/**
	 * Load an Atlas terrain file (PMP) returning textures and heightmap.
	 *
	 * See CMapReader::UnpackTerrain, CMapReader::ParseTerrain for the reordering
	 */
	JS::Value LoadMapTerrain(const VfsPath& filename)
	{
		ScriptRequest rq(m_ScriptInterface);

		if (!VfsFileExists(filename))
		{
			ScriptException::Raise(rq, "Terrain file \"%s\" does not exist!",
				filename.string8().c_str());
			return JS::UndefinedValue();
		}

		CFileUnpacker unpacker;
		unpacker.Read(filename, "PSMP");

		if (unpacker.GetVersion() < CMapIO::FILE_READ_VERSION)
		{
			ScriptException::Raise(rq, "Could not load terrain file \"%s\" too old version!",
				filename.string8().c_str());
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
				textureIDs.push_back(tiles[(patchY * patchesPerSide + patchX) * SQR(PATCH_SIZE) +
					(offY * PATCH_SIZE + offX)].m_Tex1Index);
			}
		}

		JS::RootedValue returnValue(rq.cx);

		Script::CreateObject(
			rq,
			&returnValue,
			"height", heightmap,
			"textureNames", textureNames,
			"textureIDs", textureIDs);

		return returnValue;
	}

	/**
	 * Sets the map generation progress, which is one of multiple stages
	 * determining the loading screen progress.
	 */
	void SetProgress(int progress)
	{
		// When the task is started, `m_Progress` is only mutated by this thread.
		const int currentProgress = m_Progress.load();
		if (progress >= currentProgress)
			m_Progress.store(progress);
		else
			LOGWARNING("The random map script tried to reduce the loading progress from %d to %d",
				currentProgress, progress);
	}

	/**
	 * Microseconds since the epoch.
	 */
	double GetMicroseconds() const
	{
		return JS_Now();
	}

	/**
	 * Return the template data of the given template name.
	 */
	CParamNode GetTemplate(const std::string& templateName)
	{
		const CParamNode& templateRoot =
			m_TemplateLoader.GetTemplateFileData(templateName).GetOnlyChild();
		if (!templateRoot.IsOk())
			LOGERROR("Invalid template found for '%s'", templateName.c_str());

		return templateRoot;
	}

	/**
	 * Check whether the given template exists.
	 */
	bool TemplateExists(const std::string& templateName) const
	{
		return m_TemplateLoader.TemplateExists(templateName);
	}

	/**
	 * Returns all template names of simulation entity templates.
	 */
	std::vector<std::string> FindTemplates(const std::string& path, bool includeSubdirectories)
	{
		return m_TemplateLoader.FindTemplates(path, includeSubdirectories, SIMULATION_TEMPLATES);
	}

	/**
	 * Returns all template names of actors.
	 */
	std::vector<std::string> FindActorTemplates(const std::string& path, bool includeSubdirectories)
	{
		return m_TemplateLoader.FindTemplates(path, includeSubdirectories, ACTOR_TEMPLATES);
	}

	/**
	 * Current map generation progress.
	 */
	std::atomic<int>& m_Progress;

	/**
	 * Provides the script context.
	 */
	ScriptInterface& m_ScriptInterface;

	/**
	 * Result of the mapscript generation including terrain, entities and environment settings.
	 */
	Script::StructuredClone& m_MapData;

	/**
	 * Currently loaded script librarynames.
	 */
	std::set<VfsPath> m_LoadedLibraries;

	/**
	 * Backend to loading template data.
	 */
	CTemplateLoader m_TemplateLoader;
};
} // anonymous namespace

Script::StructuredClone RunMapGenerationScript(std::atomic<int>& progress, ScriptInterface& scriptInterface,
	const VfsPath& script, const std::string& settings, const u16 flags)
{
	ScriptRequest rq(scriptInterface);

	// Parse settings
	JS::RootedValue settingsVal(rq.cx);
	if (!Script::ParseJSON(rq, settings, &settingsVal) && settingsVal.isUndefined())
	{
		LOGERROR("RunMapGenerationScript: Failed to parse settings");
		return nullptr;
	}

	// Prevent unintentional modifications to the settings object by random map scripts
	if (!Script::FreezeObject(rq, settingsVal, true))
	{
		LOGERROR("RunMapGenerationScript: Failed to deepfreeze settings");
		return nullptr;
	}

	// Init RNG seed
	u32 seed = 0;
	if (!Script::HasProperty(rq, settingsVal, "Seed") ||
		!Script::GetProperty(rq, settingsVal, "Seed", seed))
		LOGWARNING("RunMapGenerationScript: No seed value specified - using 0");

	boost::rand48 mapGenRNG{seed};
	scriptInterface.ReplaceNondeterministicRNG(mapGenRNG);

	Script::StructuredClone mapData;
	CMapGenerationCallbacks callbackData{progress, scriptInterface, mapData, flags};

	// Copy settings to global variable
	JS::RootedValue global(rq.cx, rq.globalValue());
	if (!Script::SetProperty(rq, global, "g_MapSettings", settingsVal, flags & JSPROP_READONLY,
		flags & JSPROP_ENUMERATE))
	{
		LOGERROR("RunMapGenerationScript: Failed to define g_MapSettings");
		return nullptr;
	}

	// Load RMS
	LOGMESSAGE("Loading RMS '%s'", script.string8());
	if (!scriptInterface.LoadGlobalScriptFile(script))
	{
		LOGERROR("RunMapGenerationScript: Failed to load RMS '%s'", script.string8());
		return nullptr;
	}

	return mapData;
}
