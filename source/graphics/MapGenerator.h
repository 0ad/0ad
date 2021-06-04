/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_MAPGENERATOR
#define INCLUDED_MAPGENERATOR

#include "ps/FileIo.h"
#include "ps/Future.h"
#include "ps/TemplateLoader.h"
#include "scriptinterface/StructuredClone.h"

#include <boost/random/linear_congruential.hpp>
#include <mutex>
#include <set>
#include <string>

class CMapGeneratorWorker;

/**
 * Random map generator interface. Initialized by CMapReader and then checked
 * periodically during loading, until it's finished (progress value is 0).
 *
 * The actual work is performed by CMapGeneratorWorker in a separate thread.
 */
class CMapGenerator
{
	NONCOPYABLE(CMapGenerator);

public:
	CMapGenerator();
	~CMapGenerator();

	/**
	 * Start the map generator thread
	 *
	 * @param scriptFile The VFS path for the script, e.g. "maps/random/latium.js"
	 * @param settings JSON string containing settings for the map generator
	 */
	void GenerateMap(const VfsPath& scriptFile, const std::string& settings);

	/**
	 * Get status of the map generator thread
	 *
	 * @return Progress percentage 1-100 if active, 0 when finished, or -1 on error
	 */
	int GetProgress();

	/**
	 * Get random map data, according to this format:
	 * http://trac.wildfiregames.com/wiki/Random_Map_Generator_Internals#Dataformat
	 *
	 * @return StructuredClone containing map data
	 */
	Script::StructuredClone GetResults();

private:
	CMapGeneratorWorker* m_Worker;

};

/**
 * Random map generator worker thread.
 * (This is run in a thread so that the GUI remains responsive while loading)
 *
 * Thread-safety:
 * - Initialize and constructor/destructor must be called from the main thread.
 * - ScriptInterface created and destroyed by thread
 * - StructuredClone used to return JS map data - JS:Values can't be used across threads/contexts.
 */
class CMapGeneratorWorker
{
public:
	CMapGeneratorWorker(ScriptInterface* scriptInterface);
	~CMapGeneratorWorker();

	/**
	 * Start the map generator thread
	 *
	 * @param scriptFile The VFS path for the script, e.g. "maps/random/latium.js"
	 * @param settings JSON string containing settings for the map generator
	 */
	void Initialize(const VfsPath& scriptFile, const std::string& settings);

	/**
	 * Get status of the map generator thread
	 *
	 * @return Progress percentage 1-100 if active, 0 when finished, or -1 on error
	 */
	int GetProgress();

	/**
	 * Get random map data, according to this format:
	 * http://trac.wildfiregames.com/wiki/Random_Map_Generator_Internals#Dataformat
	 *
	 * @return StructuredClone containing map data
	 */
	Script::StructuredClone GetResults();

	/**
	 * Set initial seed, callback data.
	 * Expose functions, globals and classes defined in this class relevant to the map and test scripts.
	 */
	void InitScriptInterface(const u32 seed);

private:

	/**
	 * Expose functions defined in this class that are relevant to mapscripts but not the tests.
	 */
	void RegisterScriptFunctions_MapGenerator();

	/**
	 * Load all scripts of the given library
	 *
	 * @param libraryName VfsPath specifying name of the library (subfolder of ../maps/random/)
	 * @return true if all scripts ran successfully, false if there's an error
	 */
	bool LoadScripts(const VfsPath& libraryName);

	/**
	 * Finalize map generation and pass results from the script to the engine.
	 */
	void ExportMap(JS::HandleValue data);

	/**
	 * Load an image file and return it as a height array.
	 */
	JS::Value LoadHeightmap(const VfsPath& src);

	/**
	 * Load an Atlas terrain file (PMP) returning textures and heightmap.
	 */
	JS::Value LoadMapTerrain(const VfsPath& filename);

	/**
	 * Sets the map generation progress, which is one of multiple stages determining the loading screen progress.
	 */
	void SetProgress(int progress);

	/**
	 * Microseconds since the epoch.
	 */
	double GetMicroseconds();

	/**
	 * Return the template data of the given template name.
	 */
	CParamNode GetTemplate(const std::string& templateName);

	/**
	 * Check whether the given template exists.
	 */
	bool TemplateExists(const std::string& templateName);

	/**
	 * Returns all template names of simulation entity templates.
	 */
	std::vector<std::string> FindTemplates(const std::string& path, bool includeSubdirectories);

	/**
	 * Returns all template names of actors.
	 */
	std::vector<std::string> FindActorTemplates(const std::string& path, bool includeSubdirectories);

	/**
	 * Perform the map generation.
	 */
	bool Run();

	/**
	 * Currently loaded script librarynames.
	 */
	std::set<VfsPath> m_LoadedLibraries;

	/**
	 * Result of the mapscript generation including terrain, entities and environment settings.
	 */
	Script::StructuredClone m_MapData;

	/**
	 * Deterministic random number generator.
	 */
	boost::rand48 m_MapGenRNG;

	/**
	 * Current map generation progress.
	 */
	int m_Progress;

	/**
	 * Provides the script context.
	 */
	ScriptInterface* m_ScriptInterface;

	/**
	 * Map generation script to run.
	 */
	VfsPath m_ScriptPath;

	/**
	 * Map and simulation settings chosen in the gamesetup stage.
	 */
	std::string m_Settings;

	/**
	 * Backend to loading template data.
	 */
	CTemplateLoader m_TemplateLoader;

	/**
	 * Holds the completion result of the asynchronous map generation.
	 * TODO: this whole class could really be a future on its own.
	 */
	Future<void> m_WorkerThread;

	/**
	 * Avoids thread synchronization issues.
	 */
	std::mutex m_WorkerMutex;
};


#endif	//INCLUDED_MAPGENERATOR
