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

#ifndef INCLUDED_MAPGENERATOR
#define INCLUDED_MAPGENERATOR

#include "ps/FileIo.h"
#include "ps/ThreadUtil.h"
#include "scriptinterface/ScriptInterface.h"

#include <boost/random/linear_congruential.hpp>
#include <boost/unordered_map.hpp>

#include <set>

typedef boost::unordered_map<VfsPath, std::wstring> ScriptFilesMap;

class CMapGeneratorWorker;

/**
 * Random map generator interface. Initialized by CMapReader and then checked
 * periodically during loading, until it's finished (progress value is 0).
 *
 * The actual work is performed by CMapGeneratorWorker in a separate thread.
 */
class CMapGenerator
{

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
 * - StructuredClone used to return JS map data - jsvals can't be used across threads/runtimes
 * - VFS is not threadsafe, so preload all random map scripts in the main thread for later use
 *		by the worker
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
	 * @return Progress percentage 1-100 if active, 0 when finished, or -1 when 
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
// Mapgen

	/**
	 * Load all scripts of the given library
	 * 
	 * @param libraryName String specifying name of the library (subfolder of ../maps/random/)
	 * @return true if all scripts ran successfully, false if there's an error
	 */
	bool LoadScripts(const std::wstring& libraryName);
	
	// callbacks for script functions
	static bool LoadLibrary(void* cbdata, std::wstring name);
	static void ExportMap(void* cbdata, CScriptValRooted data);
	static void SetProgress(void* cbdata, int progress);
	static void MaybeGC(void* cbdata);

	std::set<std::wstring> m_LoadedLibraries;
	shared_ptr<ScriptInterface::StructuredClone> m_MapData;
	boost::rand48 m_MapGenRNG;
	int m_Progress;
	VfsPath m_ScriptPath;
	ScriptInterface* m_ScriptInterface;
	std::string m_Settings;
	
	// Since VFS is not threadsafe, use this map to store preloaded scripts
	ScriptFilesMap m_ScriptFiles;

	// callback for VFS preloading
	static Status PreloadScript(const VfsPath& pathname, const FileInfo& fileInfo, const uintptr_t cbData);

// Thread
	static void* RunThread(void* data);
	bool Run();

	pthread_t m_WorkerThread;
	CMutex m_WorkerMutex;
};


#endif	//INCLUDED_MAPGENERATOR
