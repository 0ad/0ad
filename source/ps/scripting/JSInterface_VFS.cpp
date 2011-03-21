/* Copyright (C) 2009 Wildfire Games.
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

#include "ps/CStr.h"
#include "ps/Filesystem.h"
//#include "lib/res/file/archive/vfs_optimizer.h"	// ArchiveBuilderCancel
#include "scripting/ScriptingHost.h"
#include "scripting/JSConversions.h"
#include "ps/scripting/JSInterface_VFS.h"

// shared error handling code
#define JS_CHECK_FILE_ERR(err)\
	/* this is liable to happen often, so don't complain */\
	if (err == ERR::VFS_FILE_NOT_FOUND)\
	{\
		JS_SET_RVAL(cx, vp, JSVAL_NULL);\
		return JS_TRUE;\
	}\
	/* unknown failure. we return an error (akin to an exception in JS) that
	   stops the script to make sure this error is noticed. */\
	else if (err < 0)\
		return JS_FALSE;\
	/* else: success */




// state held across multiple BuildDirEntListCB calls; init by BuildDirEntList.
struct BuildDirEntListState
{
	JSContext* cx;
	JSObject* filename_array;
	int cur_idx;

	BuildDirEntListState(JSContext* cx_)
		: cx(cx_)
	{
		filename_array = JS_NewArrayObject(cx, 0, NULL);
		JS_AddObjectRoot(cx, &filename_array);
		cur_idx = 0;
	}

	~BuildDirEntListState()
	{
		JS_RemoveObjectRoot(cx, &filename_array);
	}
};

// called for each matching directory entry; add its full pathname to array.
static LibError BuildDirEntListCB(const VfsPath& pathname, const FileInfo& UNUSED(fileINfo), uintptr_t cbData)
{
	BuildDirEntListState* s = (BuildDirEntListState*)cbData;

	jsval val = ToJSVal( CStrW(pathname) );
	JS_SetElement(s->cx, s->filename_array, s->cur_idx++, &val);
	return INFO::CB_CONTINUE;
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
JSBool JSI_VFS::BuildDirEntList(JSContext* cx, uintN argc, jsval* vp)
{
	//
	// get arguments
	//

	JSU_REQUIRE_MIN_PARAMS(1);

	CStrW path;
	if (!ToPrimitive<CStrW> (cx, JS_ARGV(cx, vp)[0], path))
		return JS_FALSE;

	CStrW filter_str = L"";
	if (argc >= 2)
	{
		if (!ToPrimitive<CStrW> (cx, JS_ARGV(cx, vp)[1], filter_str))
			return JS_FALSE;
	}
	// convert to const wchar_t*; if there's no filter, pass 0 for speed
	// (interpreted as: "accept all files without comparing").
	const wchar_t* filter = 0;
	if (!filter_str.empty())
		filter = filter_str.c_str();

	bool recursive = false;
	if (argc >= 3)
	{
		if (!ToPrimitive<bool> (cx, JS_ARGV(cx, vp)[2], recursive))
			return JS_FALSE;
	}
	int flags = recursive ? fs_util::DIR_RECURSIVE : 0;


	// build array in the callback function
	BuildDirEntListState state(cx);
	fs_util::ForEachFile(g_VFS, path, BuildDirEntListCB, (uintptr_t)&state, filter, flags);

	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(state.filename_array));
	return JS_TRUE;
}


// Return time [seconds since 1970] of the last modification to the specified file.
//
// mtime = getFileMTime(filename);
//   filename: VFS filename (may include path)
JSBool JSI_VFS::GetFileMTime(JSContext* cx, uintN argc, jsval* vp)
{
	JSU_REQUIRE_MIN_PARAMS(1);

	CStrW filename;
	if (!ToPrimitive<CStrW> (cx, JS_ARGV(cx, vp)[0], filename))
		return JS_FALSE;

	FileInfo fileInfo;
	LibError err = g_VFS->GetFileInfo(filename, &fileInfo);
	JS_CHECK_FILE_ERR(err);

	JS_SET_RVAL(cx, vp, ToJSVal((double)fileInfo.MTime()));
	return JS_TRUE;
}


// Return current size of file.
//
// size = getFileSize(filename);
//   filename: VFS filename (may include path)
JSBool JSI_VFS::GetFileSize(JSContext* cx, uintN argc, jsval* vp)
{
	JSU_REQUIRE_MIN_PARAMS(1);

	CStrW filename;
	if (!ToPrimitive<CStrW> (cx, JS_ARGV(cx, vp)[0], filename))
		return JS_FALSE;

	FileInfo fileInfo;
	LibError err = g_VFS->GetFileInfo(filename, &fileInfo);
	JS_CHECK_FILE_ERR(err);

	JS_SET_RVAL(cx, vp, ToJSVal( (unsigned)fileInfo.Size() ));
	return JS_TRUE;
}


// Return file contents in a string.
//
// contents = readFile(filename);
//   filename: VFS filename (may include path)
JSBool JSI_VFS::ReadFile(JSContext* cx, uintN argc, jsval* vp)
{
	JSU_REQUIRE_MIN_PARAMS(1);

	CStrW filename;
	if (!ToPrimitive<CStrW> (cx, JS_ARGV(cx, vp)[0], filename))
		return JS_FALSE;

	shared_ptr<u8> buf;
	size_t size;
	LibError err = g_VFS->LoadFile(filename, buf, size);
	JS_CHECK_FILE_ERR( err );

	CStr contents((const char*)buf.get(), size);

	// Fix CRLF line endings. (This function will only ever be used on text files.)
	contents.Replace("\r\n", "\n");

	// Decode as UTF-8
	JS_SET_RVAL(cx, vp, ToJSVal( contents.FromUTF8() ));
	return JS_TRUE;
}


// Return file contents as an array of lines.
//
// lines = readFileLines(filename);
//   filename: VFS filename (may include path)
JSBool JSI_VFS::ReadFileLines(JSContext* cx, uintN argc, jsval* vp)
{
	JSU_REQUIRE_MIN_PARAMS(1);

	CStrW filename;
	if (!ToPrimitive<CStrW> (cx, JS_ARGV(cx, vp)[0], filename))
		return (JS_FALSE);

	//
	// read file
	//

	shared_ptr<u8> buf;
	size_t size;
	LibError err = g_VFS->LoadFile(filename, buf, size);
	JS_CHECK_FILE_ERR( err );

	CStr contents((const char*)buf.get(), size);

	// Fix CRLF line endings. (This function will only ever be used on text files.)
	contents.Replace("\r\n", "\n");

	//
	// split into array of strings (one per line)
	//

	std::stringstream ss(contents);

	JSObject* line_array = JS_NewArrayObject(cx, 0, NULL);

	std::string line;
	int cur_line = 0;
	while (std::getline(ss, line))
	{
		// Decode each line as UTF-8
		jsval val = ToJSVal(CStr(line).FromUTF8());
		JS_SetElement(cx, line_array, cur_line++, &val);
	}

	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL( line_array ));
	return JS_TRUE ;
}


// vfs_optimizer

JSBool JSI_VFS::ArchiveBuilderCancel(JSContext* cx, uintN argc, jsval* vp)
{
	UNUSED2(cx);
	UNUSED2(argc);

//	vfs_opt_auto_build_cancel();

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}
