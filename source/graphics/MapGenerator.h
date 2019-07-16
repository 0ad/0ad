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

#ifndef INCLUDED_MAPGENERATOR
#define INCLUDED_MAPGENERATOR

#include "lib/posix/posix_pthread.h"
#include "ps/FileIo.h"
#include "ps/TemplateLoader.h"
#include "scriptinterface/ScriptInterface.h"

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
	shared_ptr<ScriptInterface::StructuredClone> GetResults();

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
 * - StructuredClone used to return JS map data - JS:Values can't be used across threads/runtimes.
 */
class CMapGeneratorWorker
{
public:
	CMapGeneratorWorker();
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
	shared_ptr<ScriptInterface::StructuredClone> GetResults();

private:

	/**
	 * Expose functions defined in this class to the script.
	 */
	void RegisterScriptFunctions();

	/**
	 * Load all scripts of the given library
	 *
	 * @param libraryName VfsPath specifying name of the library (subfolder of ../maps/random/)
	 * @return true if all scripts ran successfully, false if there's an error
	 */
	bool LoadScripts(const VfsPath& libraryName);

	/**
	 * Recursively load all script files in the given folder.
	 */
	static bool LoadLibrary(ScriptInterface::CxPrivate* pCxPrivate, const VfsPath& name);

	/**
	 * Finalize map generation and pass results from the script to the engine.
	 */
	static void ExportMap(ScriptInterface::CxPrivate* pCxPrivate, JS::HandleValue data);

	/**
	 * Load an image file and return it as a height array.
	 */
	static JS::Value LoadHeightmap(ScriptInterface::CxPrivate* pCxPrivate, const VfsPath& src);

	/**
	 * Load an Atlas terrain file (PMP) returning textures and heightmap.
	 */
	static JS::Value LoadMapTerrain(ScriptInterface::CxPrivate* pCxPrivate, const VfsPath& filename);

	/**
	 * Sets the map generation progress, which is one of multiple stages determining the loading screen progress.
	 */
	static void SetProgress(ScriptInterface::CxPrivate* pCxPrivate, int progress);

	/**
	 * Microseconds since the epoch.
	 */
	static double GetMicroseconds(ScriptInterface::CxPrivate* pCxPrivate);

	/**
	 * Return the template data of the given template name.
	 */
	static CParamNode GetTemplate(ScriptInterface::CxPrivate* pCxPrivate, const std::string& templateName);

	/**
	 * Check whether the given template exists.
	 */
	static bool TemplateExists(ScriptInterface::CxPrivate* pCxPrivate, const std::string& templateName);

	/**
	 * Returns all template names of simulation entity templates.
	 */
	static std::vector<std::string> FindTemplates(ScriptInterface::CxPrivate* pCxPrivate, const std::string& path, bool includeSubdirectories);

	/**
	 * Returns all template names of actors.
	 */
	static std::vector<std::string> FindActorTemplates(ScriptInterface::CxPrivate* pCxPrivate, const std::string& path, bool includeSubdirectories);

	/**
	 * Perform map generation in an independent thread.
	 */
	static void* RunThread(void* data);

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
	shared_ptr<ScriptInterface::StructuredClone> m_MapData;

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
	 * Holds the mapgeneration thread identifier.
	 */
	pthread_t m_WorkerThread;

	/**
	 * Avoids thread synchronization issues.
	 */
	std::mutex m_WorkerMutex;
};


#endif	//INCLUDED_MAPGENERATOR
