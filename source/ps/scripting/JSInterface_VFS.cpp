/* Copyright (C) 2022 Wildfire Games.
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

#include "JSInterface_VFS.h"

#include "lib/file/vfs/vfs_util.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Filesystem.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/JSON.h"

#include <sstream>

namespace JSI_VFS
{
// Only allow engine compartments to read files they may be concerned about.
#define PathRestriction_GUI {L""}
#define PathRestriction_Simulation {L"simulation/"}
#define PathRestriction_Maps {L"simulation/", L"maps/"}

// shared error handling code
#define JS_CHECK_FILE_ERR(err)\
	/* this is liable to happen often, so don't complain */\
	if (err == ERR::VFS_FILE_NOT_FOUND)\
	{\
		return 0; \
	}\
	/* unknown failure. We output an error message. */\
	else if (err < 0)\
		LOGERROR("Unknown failure in VFS %i", err );
	/* else: success */


// Tests whether the current script context is allowed to read from the given directory
bool PathRestrictionMet(const ScriptRequest& rq, const std::vector<CStrW>& validPaths, const CStrW& filePath)
{
	for (const CStrW& validPath : validPaths)
		if (filePath.find(validPath) == 0)
			return true;

	CStrW allowedPaths;
	for (std::size_t i = 0; i < validPaths.size(); ++i)
	{
		if (i != 0)
			allowedPaths += L", ";

		allowedPaths += L"\"" + validPaths[i] + L"\"";
	}

	ScriptException::Raise(rq, "This part of the engine may only read from %s!", utf8_from_wstring(allowedPaths).c_str());

	return false;
}


// state held across multiple BuildDirEntListCB calls; init by BuildDirEntList.
struct BuildDirEntListState
{
	const ScriptRequest& rq;
	JS::PersistentRootedObject filename_array;
	int cur_idx;

	BuildDirEntListState(const ScriptRequest& rq)
		: rq(rq),
		filename_array(rq.cx),
		cur_idx(0)
	{
		filename_array = JS::NewArrayObject(rq.cx, JS::HandleValueArray::empty());
	}
};

// called for each matching directory entry; add its full pathname to array.
static Status BuildDirEntListCB(const VfsPath& pathname, const CFileInfo& UNUSED(fileINfo), uintptr_t cbData)
{
	BuildDirEntListState* s = (BuildDirEntListState*)cbData;

	JS::RootedObject filenameArrayObj(s->rq.cx, s->filename_array);
	JS::RootedValue val(s->rq.cx);
	Script::ToJSVal(s->rq, &val, CStrW(pathname.string()) );
	JS_SetElement(s->rq.cx, filenameArrayObj, s->cur_idx++, val);
	return INFO::OK;
}


// Return an array of pathname strings, one for each matching entry in the
// specified directory.
//   filter_string: default "" matches everything; otherwise, see vfs_next_dirent.
//   recurse: should subdirectories be included in the search? default false.
JS::Value BuildDirEntList(const ScriptRequest& rq, const std::vector<CStrW>& validPaths, const std::wstring& path, const std::wstring& filterStr, bool recurse)
{
	if (!PathRestrictionMet(rq, validPaths, path))
		return JS::NullValue();

	// convert to const wchar_t*; if there's no filter, pass 0 for speed
	// (interpreted as: "accept all files without comparing").
	const wchar_t* filter = 0;
	if (!filterStr.empty())
		filter = filterStr.c_str();

	int flags = recurse ? vfs::DIR_RECURSIVE : 0;

	// build array in the callback function
	BuildDirEntListState state(rq);
	vfs::ForEachFile(g_VFS, path, BuildDirEntListCB, (uintptr_t)&state, filter, flags);

	return JS::ObjectValue(*state.filename_array);
}

// Return true iff the file exits
bool FileExists(const ScriptRequest& rq, const std::vector<CStrW>& validPaths, const CStrW& filename)
{
	return PathRestrictionMet(rq, validPaths, filename) && g_VFS->GetFileInfo(filename, 0) == INFO::OK;
}

// Return time [seconds since 1970] of the last modification to the specified file.
double GetFileMTime(const std::wstring& filename)
{
	CFileInfo fileInfo;
	Status err = g_VFS->GetFileInfo(filename, &fileInfo);
	JS_CHECK_FILE_ERR(err);

	return (double)fileInfo.MTime();
}

// Return current size of file.
unsigned int GetFileSize(const std::wstring& filename)
{
	CFileInfo fileInfo;
	Status err = g_VFS->GetFileInfo(filename, &fileInfo);
	JS_CHECK_FILE_ERR(err);

	return (unsigned int)fileInfo.Size();
}

// Return file contents in a string. Assume file is UTF-8 encoded text.
JS::Value ReadFile(const ScriptRequest& rq, const std::wstring& filename)
{
	CVFSFile file;
	if (file.Load(g_VFS, filename) != PSRETURN_OK)
		return JS::NullValue();

	CStr contents = file.DecodeUTF8(); // assume it's UTF-8

	// Fix CRLF line endings. (This function will only ever be used on text files.)
	contents.Replace("\r\n", "\n");

	// Decode as UTF-8
	JS::RootedValue ret(rq.cx);
	Script::ToJSVal(rq, &ret, contents.FromUTF8());
	return ret;
}

// Return file contents as an array of lines. Assume file is UTF-8 encoded text.
JS::Value ReadFileLines(const ScriptRequest& rq, const std::wstring& filename)
{
	CVFSFile file;
	if (file.Load(g_VFS, filename) != PSRETURN_OK)
		return JS::NullValue();

	CStr contents = file.DecodeUTF8(); // assume it's UTF-8

	// Fix CRLF line endings. (This function will only ever be used on text files.)
	contents.Replace("\r\n", "\n");

	// split into array of strings (one per line)
	std::stringstream ss(contents);

	JS::RootedValue line_array(rq.cx);
	Script::CreateArray(rq, &line_array);

	std::string line;
	int cur_line = 0;

	while (std::getline(ss, line))
	{
		// Decode each line as UTF-8
		JS::RootedValue val(rq.cx);
		Script::ToJSVal(rq, &val, CStr(line).FromUTF8());
		Script::SetPropertyInt(rq, line_array, cur_line++, val);
	}

	return line_array;
}

// Return file contents parsed as a JS Object
JS::Value ReadJSONFile(const ScriptInterface& scriptInterface, const std::vector<CStrW>& validPaths, const CStrW& filePath)
{
	ScriptRequest rq(scriptInterface);
	if (!PathRestrictionMet(rq, validPaths, filePath))
		return JS::NullValue();

	JS::RootedValue out(rq.cx);
	Script::ReadJSONFile(rq, filePath, &out);
	return out;
}

// Save given JS Object to a JSON file
void WriteJSONFile(const ScriptInterface& scriptInterface, const std::wstring& filePath, JS::HandleValue val1)
{
	ScriptRequest rq(scriptInterface);

	// TODO: This is a workaround because we need to pass a MutableHandle to StringifyJSON.
	JS::RootedValue val(rq.cx, val1);

	std::string str(Script::StringifyJSON(rq, &val, false));

	VfsPath path(filePath);
	WriteBuffer buf;
	buf.Append(str.c_str(), str.length());
	if (g_VFS->CreateFile(path, buf.Data(), buf.Size()) == INFO::OK)
	{
		OsPath realPath;
		g_VFS->GetRealPath(path, realPath, false);
		debug_printf("FILES| JSON data written to '%s'\n", realPath.string8().c_str());
	}
	else
		debug_printf("FILES| Failed to write JSON data to '%s'\n", path.string8().c_str());
}

bool DeleteCampaignSave(const CStrW& filePath)
{
	OsPath realPath;
	if (filePath.Left(16) != L"saves/campaigns/" || filePath.Right(12) != L".0adcampaign")
		return false;

	return VfsFileExists(filePath) &&
		g_VFS->GetRealPath(filePath, realPath) == INFO::OK &&
		g_VFS->RemoveFile(filePath) == INFO::OK &&
		wunlink(realPath) == 0;
}

#define VFS_ScriptFunctions(context)\
JS::Value Script_ReadJSONFile_##context(const ScriptInterface& scriptInterface, const std::wstring& filePath)\
{\
	return ReadJSONFile(scriptInterface, PathRestriction_##context, filePath);\
}\
JS::Value Script_ListDirectoryFiles_##context(const ScriptInterface& scriptInterface, const std::wstring& path, const std::wstring& filterStr, bool recurse)\
{\
	return BuildDirEntList(scriptInterface, PathRestriction_##context, path, filterStr, recurse);\
}\
bool Script_FileExists_##context(const ScriptInterface& scriptInterface, const std::wstring& filePath)\
{\
	return FileExists(scriptInterface, PathRestriction_##context, filePath);\
}\

VFS_ScriptFunctions(GUI);
VFS_ScriptFunctions(Simulation);
VFS_ScriptFunctions(Maps);
#undef VFS_ScriptFunctions

void RegisterScriptFunctions_ReadWriteAnywhere(const ScriptRequest& rq)
{
	ScriptFunction::Register<&Script_ListDirectoryFiles_GUI>(rq, "ListDirectoryFiles");
	ScriptFunction::Register<&Script_FileExists_GUI>(rq, "FileExists");
	ScriptFunction::Register<&GetFileMTime>(rq, "GetFileMTime");
	ScriptFunction::Register<&GetFileSize>(rq, "GetFileSize");
	ScriptFunction::Register<&ReadFile>(rq, "ReadFile");
	ScriptFunction::Register<&ReadFileLines>(rq, "ReadFileLines");
	ScriptFunction::Register<&Script_ReadJSONFile_GUI>(rq, "ReadJSONFile");
	ScriptFunction::Register<&WriteJSONFile>(rq, "WriteJSONFile");
	ScriptFunction::Register<&DeleteCampaignSave>(rq, "DeleteCampaignSave");
}

void RegisterScriptFunctions_ReadOnlySimulation(const ScriptRequest& rq)
{
	ScriptFunction::Register<&Script_ListDirectoryFiles_Simulation>(rq, "ListDirectoryFiles");
	ScriptFunction::Register<&Script_FileExists_Simulation>(rq, "FileExists");
	ScriptFunction::Register<&Script_ReadJSONFile_Simulation>(rq, "ReadJSONFile");
}

void RegisterScriptFunctions_ReadOnlySimulationMaps(const ScriptRequest& rq)
{
	ScriptFunction::Register<&Script_ListDirectoryFiles_Maps>(rq, "ListDirectoryFiles");
	ScriptFunction::Register<&Script_FileExists_Maps>(rq, "FileExists");
	ScriptFunction::Register<&Script_ReadJSONFile_Maps>(rq, "ReadJSONFile");
}
}
