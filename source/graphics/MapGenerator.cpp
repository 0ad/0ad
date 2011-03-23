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

#include "lib/timer.h"
#include "ps/CLogger.h"


// TODO: what's a good default? perhaps based on map size
#define RMS_RUNTIME_SIZE 96 * 1024 * 1024


CMapGenerator::CMapGenerator() : m_ScriptInterface("RMS", "MapGenerator", ScriptInterface::CreateRuntime(RMS_RUNTIME_SIZE))
{
	m_ScriptInterface.SetCallbackData(static_cast<void*> (this));

	// Replace RNG with a seeded deterministic function
	m_ScriptInterface.ReplaceNondeterministicFunctions(m_MapGenRNG);

	// functions for RMS
	m_ScriptInterface.RegisterFunction<bool, std::wstring, CMapGenerator::LoadLibrary>("LoadLibrary");
	m_ScriptInterface.RegisterFunction<void, CScriptValRooted, CMapGenerator::ExportMap>("ExportMap");
}

bool CMapGenerator::GenerateMap(const VfsPath& scriptFile, const CScriptValRooted& settings)
{
	TIMER(L"GenerateMap");

	// Init RNG seed
	uint32 seed;
	if (!m_ScriptInterface.GetProperty(settings.get(), "Seed", seed))
	{	// No seed specified
		LOGWARNING(L"GenerateMap: No seed value specified - using 0");
		seed = 0;
	}

	m_MapGenRNG.seed(seed);

	// Copy settings to script context
	if (!m_ScriptInterface.SetProperty(m_ScriptInterface.GetGlobalObject(), "g_MapSettings", settings))
		return false;

	// Load RMS
	LOGMESSAGE(L"Loading RMS '%ls'", scriptFile.c_str());
	if (!m_ScriptInterface.LoadGlobalScriptFile(scriptFile))
	{
		LOGERROR(L"Failed to load RMS '%ls'", scriptFile.c_str());
		return false;
	}

	return true;
}

ScriptInterface& CMapGenerator::GetScriptInterface()
{
	return m_ScriptInterface;
}

CScriptValRooted& CMapGenerator::GetMapData()
{
	return m_MapData;
}

bool CMapGenerator::LoadLibrary(void* cbdata, std::wstring name)
{
	CMapGenerator* self = static_cast<CMapGenerator*> (cbdata);

	return self->LoadScripts(name);
}

void CMapGenerator::ExportMap(void* cbdata, CScriptValRooted data)
{
	CMapGenerator* self = static_cast<CMapGenerator*> (cbdata);

	// Copy results
	self->m_MapData = data;
}

bool CMapGenerator::LoadScripts(const std::wstring& libraryName)
{
	// Ignore libraries that are already loaded
	if (m_LoadedLibraries.find(libraryName) != m_LoadedLibraries.end())
		return true;

	// Mark this as loaded, to prevent it recursively loading itself
	m_LoadedLibraries.insert(libraryName);

	VfsPath path = L"maps/random/" + libraryName + L"/";
	VfsPaths pathnames;

	// Load all scripts in mapgen directory
	if (fs_util::GetPathnames(g_VFS, path, L"*.js", pathnames) < 0)
	{
		LOGERROR(L"Error reading scripts in directory '%ls'", path.c_str());
		return false;
	}

	for (VfsPaths::iterator it = pathnames.begin(); it != pathnames.end(); ++it)
	{
		LOGMESSAGE(L"Loading map generator script '%ls'", it->c_str());

		if (!m_ScriptInterface.LoadGlobalScriptFile(*it))
		{
			LOGERROR(L"Failed to load script '%ls'", it->c_str());
			return false;
		}
	}

	return true;
}
