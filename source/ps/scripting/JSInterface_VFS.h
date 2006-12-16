// JSInterface_VFS.h
//
// The JavaScript wrapper around useful snippets of the VFS

#ifndef JSI_VFS_INCLUDED
#define JSI_VFS_INCLUDED

#include "scripting/ScriptingHost.h"

// [KEEP IN SYNC WITH TDD AND WIKI]

// these are registered in ScriptGlue.cpp, hence the need for a header.

namespace JSI_VFS
{
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
	JSBool BuildDirEntList( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );

	// Return time [seconds since 1970] of the last modification to the specified file.
	//
	// mtime = getFileMTime(filename);
	//   filename: VFS filename (may include path)
	JSBool GetFileMTime( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );

	// Return current size of file.
	//
	// size = getFileSize(filename);
	//   filename: VFS filename (may include path)
	JSBool GetFileSize( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );

	// Return file contents in a string.
	//
	// contents = readFile(filename);
	//   filename: VFS filename (may include path)
	JSBool ReadFile( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );

	// Return file contents as an array of lines.
	//
	// lines = readFileLines(filename);
	//   filename: VFS filename (may include path)
	JSBool ReadFileLines( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );

	// Abort the current archive build operation (no-op if not currently active).
	//
	// archiveBuilderCancel();
	JSBool ArchiveBuilderCancel(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
}

#endif
