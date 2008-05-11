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
	if(err == ERR::VFS_FILE_NOT_FOUND)\
	{\
		*rval = JSVAL_NULL;\
		return( JS_TRUE );\
	}\
	/* unknown failure. we return an error (akin to an exception in JS) that
	   stops the script to make sure this error is noticed. */\
	else if(err < 0)\
		return( JS_FALSE );\
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
		JS_AddRoot(cx, &filename_array);
		cur_idx = 0;
	}

	~BuildDirEntListState()
	{
		JS_RemoveRoot(cx, &filename_array);
	}
};

// called for each matching directory entry; add its full pathname to array.
static LibError BuildDirEntListCB(const VfsPath& pathname, const FileInfo& UNUSED(fileINfo), uintptr_t cbData)
{
	BuildDirEntListState* s = (BuildDirEntListState*)cbData;

	jsval val = ToJSVal( CStr(pathname.string()) );
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
JSBool JSI_VFS::BuildDirEntList( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	//
	// get arguments
	//

	debug_assert( argc >= 1 );
	CStr path;
	if( !ToPrimitive<CStr>( cx, argv[0], path ) )
		return( JS_FALSE );

	CStr filter_str = "";
	if(argc >= 2)
	{
		if( !ToPrimitive<CStr>( cx, argv[1], filter_str ) )
			return( JS_FALSE );
	}
	// convert to const char*; if there's no filter, pass 0 for speed
	// (interpreted as: "accept all files without comparing").
	const char* filter = 0;
	if(!filter_str.empty())
		filter = filter_str.c_str();

	bool recursive = false;
	if(argc >= 3)
	{
		if( !ToPrimitive<bool>( cx, argv[2], recursive ) )
			return( JS_FALSE );
	}
	int flags = recursive? DIR_RECURSIVE : 0;


	// build array in the callback function
	BuildDirEntListState state(cx);
	fs_ForEachFile(g_VFS, path, BuildDirEntListCB, (uintptr_t)&state, filter, flags);

	*rval = OBJECT_TO_JSVAL( state.filename_array );
	return( JS_TRUE );
}


// Return time [seconds since 1970] of the last modification to the specified file.
//
// mtime = getFileMTime(filename);
//   filename: VFS filename (may include path)
JSBool JSI_VFS::GetFileMTime( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	debug_assert( argc >= 1 );
	CStr filename;
	if( !ToPrimitive<CStr>( cx, argv[0], filename ) )
		return( JS_FALSE );

	FileInfo fileInfo;
	LibError err = g_VFS->GetFileInfo(filename.c_str(), &fileInfo);
	JS_CHECK_FILE_ERR( err );

	*rval = ToJSVal( (double)fileInfo.MTime() );
	return( JS_TRUE );
}


// Return current size of file.
//
// size = getFileSize(filename);
//   filename: VFS filename (may include path)
JSBool JSI_VFS::GetFileSize( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	debug_assert( argc >= 1 );
	CStr filename;
	if( !ToPrimitive<CStr>( cx, argv[0], filename ) )
		return( JS_FALSE );

	FileInfo fileInfo;
	LibError err = g_VFS->GetFileInfo(filename.c_str(), &fileInfo);
	JS_CHECK_FILE_ERR(err);

	*rval = ToJSVal( (unsigned)fileInfo.Size() );
	return( JS_TRUE );
}


// Return file contents in a string.
//
// contents = readFile(filename);
//   filename: VFS filename (may include path)
JSBool JSI_VFS::ReadFile( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	debug_assert( argc >= 1 );
	CStr filename;
	if( !ToPrimitive<CStr>( cx, argv[0], filename ) )
		return( JS_FALSE );

	shared_ptr<u8> buf; size_t size;
	LibError err = g_VFS->LoadFile( filename.c_str(), buf, size );
	JS_CHECK_FILE_ERR( err );

	CStr contents( (const char*)buf.get(), size );

	// Fix CRLF line endings. (This function will only ever be used on text files.)
	contents.Replace("\r\n", "\n");

	*rval = ToJSVal( contents );
	return( JS_TRUE );
}


// Return file contents as an array of lines.
//
// lines = readFileLines(filename);
//   filename: VFS filename (may include path)
JSBool JSI_VFS::ReadFileLines( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	debug_assert( argc >= 1 );
	CStr filename;
	if( !ToPrimitive<CStr>( cx, argv[0], filename ) )
		return( JS_FALSE );

	//
	// read file
	//

	shared_ptr<u8> buf; size_t size;
	LibError err = g_VFS->LoadFile( filename.c_str( ), buf, size );
	JS_CHECK_FILE_ERR( err );

	CStr contents( (const char*)buf.get(), size );

	// Fix CRLF line endings. (This function will only ever be used on text files.)
	contents.Replace("\r\n", "\n");

	//
	// split into array of strings (one per line)
	//

	std::stringstream ss( contents );

	JSObject* line_array = JS_NewArrayObject( cx, 0, NULL );
	JS_AddRoot(cx, &line_array);

	std::string line;
	int cur_line = 0;
	while( std::getline( ss, line ) )
	{
		jsval val = ToJSVal( CStr( line ) );
		JS_SetElement( cx, line_array, cur_line++, &val );
	}

	JS_RemoveRoot(cx, &line_array);

	*rval = OBJECT_TO_JSVAL( line_array );
	return( JS_TRUE );
}


// vfs_optimizer

JSBool JSI_VFS::ArchiveBuilderCancel(JSContext* UNUSED(cx), JSObject* UNUSED(obj), uintN argc, jsval* UNUSED(argv), jsval* UNUSED(rval) )
{
	debug_assert( argc == 0 );
//	vfs_opt_auto_build_cancel();
	return( JS_TRUE );
}
