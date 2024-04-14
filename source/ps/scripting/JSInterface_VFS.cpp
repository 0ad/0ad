/* Copyright (C) 2024 Wildfire Games.
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
#include "scriptinterface/Object.h"

#include <algorithm>
#include <sstream>

namespace JSI_VFS
{
using namespace std::literals;

// Only allow engine compartments to read files they may be concerned about.
namespace PathRestriction
{
constexpr std::array<std::wstring_view, 8> GUI{L"gui/"sv, L"simulation/"sv, L"maps/"sv, L"campaigns/"sv,
	L"saves/campaigns/"sv, L"config/matchsettings.json"sv, L"config/matchsettings.mp.json"sv,
	L"moddata"sv};

constexpr std::array<std::wstring_view, 1> SIMULATION{L"simulation/"sv};

constexpr std::array<std::wstring_view, 2> MAPS{L"simulation/"sv, L"maps/"sv};
}

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
template<auto& restriction>
bool PathRestrictionMet(const ScriptRequest& rq, const std::wstring& filePath)
{
	if (std::any_of(restriction.begin(), restriction.end(), [&](const std::wstring_view allowedPath)
		{
			return filePath.find(allowedPath) == 0;
		}))
	{
		return true;
	}

	std::wstring allowedPaths;
	for (std::size_t i = 0; i < restriction.size(); ++i)
	{
		if (i != 0)
			allowedPaths += L", ";

		allowedPaths += L"\"" + static_cast<std::wstring>(restriction[i]) + L"\"";
	}

	ScriptException::Raise(rq, "Restricted access to %s. This part of the engine may only read from %s!", utf8_from_wstring(filePath).c_str(), utf8_from_wstring(allowedPaths).c_str());

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
template<auto& restriction>
JS::Value BuildDirEntList(const ScriptRequest& rq, const std::wstring& path, const std::wstring& filterStr,
	bool recurse)
{
	if (!PathRestrictionMet<restriction>(rq, path))
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
template<auto& restriction>
bool FileExists(const ScriptRequest& rq, const std::wstring& filename)
{
	return PathRestrictionMet<restriction>(rq, filename) && g_VFS->GetFileInfo(filename, 0) == INFO::OK;
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
template<auto& restriction>
JS::Value ReadFile(const ScriptRequest& rq, const std::wstring& filename)
{
	if (!PathRestrictionMet<restriction>(rq, filename))
		return JS::NullValue();

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
template<auto& restriction>
JS::Value ReadFileLines(const ScriptRequest& rq, const std::wstring& filename)
{
	if (!PathRestrictionMet<restriction>(rq, filename))
		return JS::NullValue();

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
template<auto& restriction>
JS::Value ReadJSONFile(const ScriptInterface& scriptInterface, const std::wstring& filePath)
{
	ScriptRequest rq(scriptInterface);
	if (!PathRestrictionMet<restriction>(rq, filePath))
		return JS::NullValue();

	JS::RootedValue out(rq.cx);
	Script::ReadJSONFile(rq, filePath, &out);
	return out;
}

// Save given JS Object to a JSON file
template<auto& restriction>
void WriteJSONFile(const ScriptInterface& scriptInterface, const std::wstring& filePath,
	JS::HandleValue val1)
{
	ScriptRequest rq(scriptInterface);
	if (!PathRestrictionMet<restriction>(rq, filePath))
		return;

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

void RegisterScriptFunctions_ReadWriteAnywhere(const ScriptRequest& rq,
	const u16 flags /*= JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT */)
{
	ScriptFunction::Register<&BuildDirEntList<PathRestriction::GUI>>(rq, "ListDirectoryFiles", flags);
	ScriptFunction::Register<&FileExists<PathRestriction::GUI>>(rq, "FileExists", flags);
	ScriptFunction::Register<&GetFileMTime>(rq, "GetFileMTime", flags);
	ScriptFunction::Register<&GetFileSize>(rq, "GetFileSize", flags);
	ScriptFunction::Register<&ReadFile<PathRestriction::GUI>>(rq, "ReadFile", flags);
	ScriptFunction::Register<&ReadFileLines<PathRestriction::GUI>>(rq, "ReadFileLines", flags);
	ScriptFunction::Register<&ReadJSONFile<PathRestriction::GUI>>(rq, "ReadJSONFile", flags);
	ScriptFunction::Register<&WriteJSONFile<PathRestriction::GUI>>(rq, "WriteJSONFile", flags);
	ScriptFunction::Register<&DeleteCampaignSave>(rq, "DeleteCampaignSave", flags);
}

void RegisterScriptFunctions_ReadOnlySimulation(const ScriptRequest& rq,
	const u16 flags /*= JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT */)
{
	ScriptFunction::Register<&BuildDirEntList<PathRestriction::SIMULATION>>(rq, "ListDirectoryFiles", flags);
	ScriptFunction::Register<&FileExists<PathRestriction::SIMULATION>>(rq, "FileExists", flags);
	ScriptFunction::Register<&ReadJSONFile<PathRestriction::SIMULATION>>(rq, "ReadJSONFile", flags);
}

void RegisterScriptFunctions_ReadOnlySimulationMaps(const ScriptRequest& rq,
	const u16 flags /*= JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT */)
{
	ScriptFunction::Register<&BuildDirEntList<PathRestriction::MAPS>>(rq, "ListDirectoryFiles", flags);
	ScriptFunction::Register<&FileExists<PathRestriction::MAPS>>(rq, "FileExists", flags);
	ScriptFunction::Register<&ReadJSONFile<PathRestriction::MAPS>>(rq, "ReadJSONFile", flags);
}
}
