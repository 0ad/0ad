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

// JSInterface_VFS.h
//
// The JavaScript wrapper around useful snippets of the VFS

#ifndef INCLUDED_JSI_VFS
#define INCLUDED_JSI_VFS

#include "scriptinterface/ScriptInterface.h"

// these are registered in ScriptFunctions.cpp, hence the need for a header.

namespace JSI_VFS
{
	// Return an array of pathname strings, one for each matching entry in the
	// specified directory.
	//
	// pathnames = buildDirEntList(start_path [, filter_string [, recursive ] ]);
	//   directory: VFS path
	//   filter_string: see match_wildcard; "" matches everything.
	//   recurse: should subdirectories be included in the search? default false.
	//
	// note: full pathnames of each file/subdirectory are returned,
	// ready for use as a "filename" for the other functions.
	JS::Value BuildDirEntList(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& path, const std::wstring& filterStr, bool recurse);

	// Return true iff the file exists
	bool FileExists(ScriptInterface::CxPrivate* pCxPrivate, const CStrW& filename);

	// Return time [seconds since 1970] of the last modification to the specified file.
	double GetFileMTime(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filename);

	// Return current size of file.
	unsigned int GetFileSize(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filename);

	// Return file contents in a string.
	JS::Value ReadFile(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filename);

	// Return file contents as an array of lines.
	JS::Value ReadFileLines(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filename);

	// Return file contents parsed as a JS Object
	JS::Value ReadJSONFile(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filePath);

	// Save given JS Object to a JSON file
	void WriteJSONFile(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filePath, JS::HandleValue val1);

	void RegisterReadOnlyScriptFunctions(const ScriptInterface& scriptInterface);
	void RegisterWriteScriptFunctions(const ScriptInterface& scriptInterface);
}

#endif
