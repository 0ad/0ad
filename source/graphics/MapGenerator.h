/* Copyright (C) 2014 Wildfire Games.
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
#include "ps/TemplateLoader.h"
#include "scriptinterface/ScriptInterface.h"

#include <boost/random/linear_congruential.hpp>

#include <set>

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
 * - StructuredClone used to return JS map data - jsvals can't be used across threads/runtimes
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
// Mapgen

	/**
	 * Load all scripts of the given library
	 * 
	 * @param libraryName String specifying name of the library (subfolder of ../maps/random/)
	 * @return true if all scripts ran successfully, false if there's an error
	 */
	bool LoadScripts(const std::wstring& libraryName);
	
	// callbacks for script functions
	static bool LoadLibrary(ScriptInterface::CxPrivate* pCxPrivate, std::wstring name);
	static void ExportMap(ScriptInterface::CxPrivate* pCxPrivate, CScriptValRooted data);
	static void SetProgress(ScriptInterface::CxPrivate* pCxPrivate, int progress);
	static void MaybeGC(ScriptInterface::CxPrivate* pCxPrivate);
	static std::vector<std::string> GetCivData(ScriptInterface::CxPrivate* pCxPrivate);
	static CParamNode GetTemplate(ScriptInterface::CxPrivate* pCxPrivate, std::string templateName);
	static std::vector<std::string> FindTemplates(ScriptInterface::CxPrivate* pCxPrivate, std::string path, bool includeSubdirectories);
	static std::vector<std::string> FindActorTemplates(ScriptInterface::CxPrivate* pCxPrivate, std::string path, bool includeSubdirectories);

	std::set<std::wstring> m_LoadedLibraries;
	shared_ptr<ScriptInterface::StructuredClone> m_MapData;
	boost::rand48 m_MapGenRNG;
	int m_Progress;
	ScriptInterface* m_ScriptInterface;
	VfsPath m_ScriptPath;
	std::string m_Settings;
	CTemplateLoader m_TemplateLoader;

// Thread
	static void* RunThread(void* data);
	bool Run();

	pthread_t m_WorkerThread;
	CMutex m_WorkerMutex;
};


#endif	//INCLUDED_MAPGENERATOR
