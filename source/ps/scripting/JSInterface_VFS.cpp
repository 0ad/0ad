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

#include "precompiled.h"

#include <sstream>

#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Filesystem.h"
#include "scriptinterface/ScriptVal.h"
#include "scriptinterface/ScriptInterface.h"
#include "ps/scripting/JSInterface_VFS.h"
#include "lib/file/vfs/vfs_util.h"

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


// state held across multiple BuildDirEntListCB calls; init by BuildDirEntList.
struct BuildDirEntListState
{
	JSContext* cx;
	JS::PersistentRootedObject filename_array;
	int cur_idx;

	BuildDirEntListState(JSContext* cx_)
		: cx(cx_),
		filename_array(cx, JS_NewArrayObject(cx, JS::HandleValueArray::empty())),
		cur_idx(0)
	{
	}
};

// called for each matching directory entry; add its full pathname to array.
static Status BuildDirEntListCB(const VfsPath& pathname, const CFileInfo& UNUSED(fileINfo), uintptr_t cbData)
{
	BuildDirEntListState* s = (BuildDirEntListState*)cbData;
	JSAutoRequest rq(s->cx);

	JS::RootedObject filenameArrayObj(s->cx, s->filename_array);
	JS::RootedValue val(s->cx);
	ScriptInterface::ToJSVal( s->cx, &val, CStrW(pathname.string()) );
	JS_SetElement(s->cx, filenameArrayObj, s->cur_idx++, val);
	return INFO::OK;
}


// Return an array of pathname strings, one for each matching entry in the
// specified directory.
//
// pathnames = buildDirEntList(start_path [, filter_string [, recursive ] ]);
//   directory: VFS path
//   filter_string: default "" matches everything; otherwise, see vfs_next_dirent.
//   recurse: should subdirectories be included in the search? default false.
//
// note: full pathnames of each file/subdirectory are returned,
// ready for use as a "filename" for the other functions.
JS::Value JSI_VFS::BuildDirEntList(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& path, const std::wstring& filterStr, bool recurse)
{
	// convert to const wchar_t*; if there's no filter, pass 0 for speed
	// (interpreted as: "accept all files without comparing").
	const wchar_t* filter = 0;
	if (!filterStr.empty())
		filter = filterStr.c_str();

	int flags = recurse ? vfs::DIR_RECURSIVE : 0;

	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	// build array in the callback function
	BuildDirEntListState state(cx);
	vfs::ForEachFile(g_VFS, path, BuildDirEntListCB, (uintptr_t)&state, filter, flags);

	return OBJECT_TO_JSVAL(state.filename_array);
}

// Return true iff the file exits
bool JSI_VFS::FileExists(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const CStrW& filename)
{
	return (g_VFS->GetFileInfo(filename, 0) == INFO::OK);
}

// Return time [seconds since 1970] of the last modification to the specified file.
double JSI_VFS::GetFileMTime(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& filename)
{
	CFileInfo fileInfo;
	Status err = g_VFS->GetFileInfo(filename, &fileInfo);
	JS_CHECK_FILE_ERR(err);

	return (double)fileInfo.MTime();
}

// Return current size of file.
unsigned int JSI_VFS::GetFileSize(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& filename)
{
	CFileInfo fileInfo;
	Status err = g_VFS->GetFileInfo(filename, &fileInfo);
	JS_CHECK_FILE_ERR(err);

	return (unsigned int)fileInfo.Size();
}

// Return file contents in a string. Assume file is UTF-8 encoded text.
JS::Value JSI_VFS::ReadFile(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filename)
{
	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	CVFSFile file;
	if (file.Load(g_VFS, filename) != PSRETURN_OK)
		return JS::NullValue();

	CStr contents = file.DecodeUTF8(); // assume it's UTF-8

	// Fix CRLF line endings. (This function will only ever be used on text files.)
	contents.Replace("\r\n", "\n");

	// Decode as UTF-8
	JS::RootedValue ret(cx);
	ScriptInterface::ToJSVal(cx, &ret, contents.FromUTF8());
	return ret;
}

// Return file contents as an array of lines. Assume file is UTF-8 encoded text.
JS::Value JSI_VFS::ReadFileLines(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filename)
{
	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	CVFSFile file;
	if (file.Load(g_VFS, filename) != PSRETURN_OK)
		return JSVAL_NULL;

	CStr contents = file.DecodeUTF8(); // assume it's UTF-8

	// Fix CRLF line endings. (This function will only ever be used on text files.)
	contents.Replace("\r\n", "\n");

	// split into array of strings (one per line)
	std::stringstream ss(contents);
	JS::RootedObject line_array(cx, JS_NewArrayObject(cx, JS::HandleValueArray::empty()));
	std::string line;
	int cur_line = 0;

	while (std::getline(ss, line))
	{
		// Decode each line as UTF-8
		JS::RootedValue val(cx);
		ScriptInterface::ToJSVal(cx, &val, CStr(line).FromUTF8());
		JS_SetElement(cx, line_array, cur_line++, val);
	}

	return JS::ObjectValue(*line_array);
}

JS::Value JSI_VFS::ReadJSONFile(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filePath)
{
	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue out(cx);
	pCxPrivate->pScriptInterface->ReadJSONFile(filePath, &out);
	return out;
}

void JSI_VFS::WriteJSONFile(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& filePath, JS::HandleValue val1)
{
	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	// TODO: This is a workaround because we need to pass a MutableHandle to StringifyJSON.
	JS::RootedValue val(cx, val1);

	std::string str(pCxPrivate->pScriptInterface->StringifyJSON(&val, false));

	VfsPath path(filePath);
	WriteBuffer buf;
	buf.Append(str.c_str(), str.length());
	g_VFS->CreateFile(path, buf.Data(), buf.Size());
}

void JSI_VFS::RegisterReadOnlyScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<JS::Value, std::wstring, std::wstring, bool, &JSI_VFS::BuildDirEntList>("BuildDirEntList");
	scriptInterface.RegisterFunction<bool, CStrW, JSI_VFS::FileExists>("FileExists");
	scriptInterface.RegisterFunction<double, std::wstring, &JSI_VFS::GetFileMTime>("GetFileMTime");
	scriptInterface.RegisterFunction<unsigned int, std::wstring, &JSI_VFS::GetFileSize>("GetFileSize");
	scriptInterface.RegisterFunction<JS::Value, std::wstring, &JSI_VFS::ReadFile>("ReadFile");
	scriptInterface.RegisterFunction<JS::Value, std::wstring, &JSI_VFS::ReadFileLines>("ReadFileLines");
	scriptInterface.RegisterFunction<JS::Value, std::wstring, &ReadJSONFile>("ReadJSONFile");
}

void JSI_VFS::RegisterWriteScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<void, std::wstring, JS::HandleValue, &WriteJSONFile>("WriteJSONFile");
}
