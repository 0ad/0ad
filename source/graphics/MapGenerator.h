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

#include "ps/CStr.h"
#include "ps/FileIo.h"
#include "scriptinterface/ScriptInterface.h"


class CMapGenerator
{

public:
	// constructor
	CMapGenerator();

	// destructor
	~CMapGenerator();

	// return success of map generation
	bool GenerateMap(const VfsPath& scriptFile, const CScriptValRooted& settings);

	// accessors
	ScriptInterface& GetScriptInterface();

	CScriptValRooted& GetMapData();

	// callbacks for script functions
	static bool LoadLibrary(void* cbdata, std::wstring name);

	static void ExportMap(void* cbdata, CScriptValRooted data);
	
private:

	bool LoadScripts(const std::wstring& libraryName);
	
	ScriptInterface m_ScriptInterface;
	CScriptValRooted m_MapData;
	std::set<std::wstring> m_LoadedLibraries;
};

#endif	//INCLUDED_MAPGENERATOR
