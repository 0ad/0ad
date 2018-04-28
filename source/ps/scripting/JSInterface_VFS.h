/* Copyright (C) 2018 Wildfire Games.
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

#ifndef INCLUDED_JSI_VFS
#define INCLUDED_JSI_VFS

#include "scriptinterface/ScriptInterface.h"

namespace JSI_VFS
{
	// Return an array of pathname strings, one for each matching entry in the
	// specified directory.
	JS::Value BuildDirEntList(ScriptInterface::CxPrivate* pCxPrivate, const std::vector<CStrW>& validPaths, const std::wstring& path, const std::wstring& filterStr, bool recurse);

	// Return true iff the file exists
	bool FileExists(ScriptInterface::CxPrivate* pCxPrivate, const std::vector<CStrW>& validPaths, const CStrW& filename);

	// Return time [seconds since 1970] of the last modification to the specified file.
	double GetFileMTime(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filename);

	// Return current size of file.
	unsigned int GetFileSize(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filename);

	// Return file contents in a string.
	JS::Value ReadFile(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filename);

	// Return file contents as an array of lines.
	JS::Value ReadFileLines(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filename);

	// Return file contents parsed as a JS Object
	JS::Value ReadJSONFile(ScriptInterface::CxPrivate* pCxPrivate, const std::vector<CStrW>& validPaths, const CStrW& filePath);

	// Save given JS Object to a JSON file
	void WriteJSONFile(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filePath, JS::HandleValue val1);

	// Tests whether the current script context is allowed to read from the given directory
	bool PathRestrictionMet(ScriptInterface::CxPrivate* pCxPrivate, const std::vector<CStrW>& validPaths, const CStrW& filePath);

	void RegisterScriptFunctions_GUI(const ScriptInterface& scriptInterface);
	void RegisterScriptFunctions_Simulation(const ScriptInterface& scriptInterface);
	void RegisterScriptFunctions_Maps(const ScriptInterface& scriptInterface);
}

#endif // INCLUDED_JSI_VFS
