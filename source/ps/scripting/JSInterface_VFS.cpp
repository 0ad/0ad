#include "precompiled.h"

#include "ps/CStr.h"
#include "ps/VFSUtil.h"
#include "lib/res/res.h"
#include "scripting/ScriptingHost.h"
#include "scripting/JSConversions.h"
#include "scripting/JSInterface_VFS.h"


// shared error handling code
#define JS_CHECK_FILE_ERR(err)\
	/* this is liable to happen often, so don't complain */\
	if(err == ERR_FILE_NOT_FOUND)\
	{\
		*rval = JSVAL_NULL;\
		return( JS_TRUE );\
	}\
	/* unknown failure. we return an error (akin to an exception in JS) that
	   stops the script to make sure this error is noticed. */\
	else if(err < 0)\
		return( JS_FALSE );\
	/* else: success */




// state held across multiple BuildFileListCB calls; init by BuildFileList.
struct BuildFileListState
{
	JSContext* cx;
	JSObject* filename_array;
	int cur_idx;

	BuildFileListState(JSContext* cx_)
		: cx(cx_)
	{
		filename_array = JS_NewArrayObject(cx, 0, NULL);
		cur_idx = 0;
	}
};

// called for each matching directory entry; add its full pathname to array.
static void BuildFileListCB(const char* path, const vfsDirEnt* ent, void* context)
{
	BuildFileListState* s = (BuildFileListState*)context;

	jsval val = ToJSVal( CStr ( path ) );
		// note: <path> is already directory + name!

	JS_SetElement(s->cx, s->filename_array, s->cur_idx++, &val);
}


// Return an array of pathname strings, one for each matching entry in the
// specified directory.
//
// pathnames = buildFileList(start_path [, filter_string [, recursive ] ]);
//   directory: VFS path
//   filter_string: default "" matches everything; otherwise, see vfs_next_dirent.
//   recurse: should subdirectories be included in the search? default false.
//
// note: full pathnames of each file/subdirectory are returned,
// ready for use as a "filename" for the other functions.
JSBool JSI_VFS::BuildFileList( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	//
	// get arguments
	//

	assert( argc >= 1 );
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


	// build array in the callback function
	BuildFileListState state(cx);
	VFSUtil::EnumDirEnts( path, filter, recursive, BuildFileListCB, &state );

	*rval = OBJECT_TO_JSVAL( state.filename_array );
	return( JS_TRUE );
}


// Return time [seconds since 1970] of the last modification to the specified file.
//
// mtime = getFileMTime(filename);
//   filename: VFS filename (may include path)
JSBool JSI_VFS::GetFileMTime( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	assert( argc >= 1 );
	CStr filename;
	if( !ToPrimitive<CStr>( cx, argv[0], filename ) )
		return( JS_FALSE );

	struct stat s;
	int err = vfs_stat( filename.c_str(), &s );
	JS_CHECK_FILE_ERR( err );

	*rval = ToJSVal( (double)s.st_mtime );
	return( JS_TRUE );
}


// Return current size of file.
//
// size = getFileSize(filename);
//   filename: VFS filename (may include path)
JSBool JSI_VFS::GetFileSize( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	assert( argc >= 1 );
	CStr filename;
	if( !ToPrimitive<CStr>( cx, argv[0], filename ) )
		return( JS_FALSE );

	struct stat s;
	int err = vfs_stat( filename.c_str(), &s );
	JS_CHECK_FILE_ERR(err);

	*rval = ToJSVal( (uint)s.st_size );
	return( JS_TRUE );
}


// Return file contents in a string.
//
// contents = readFile(filename);
//   filename: VFS filename (may include path)
JSBool JSI_VFS::ReadFile( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	assert( argc >= 1 );
	CStr filename;
	if( !ToPrimitive<CStr>( cx, argv[0], filename ) )
		return( JS_FALSE );

	void* p;
	size_t size;
	Handle hm = vfs_load( filename.c_str(), p, size );
	JS_CHECK_FILE_ERR( hm );

	CStr contents( (const char*)p, size );
	mem_free_h( hm );

	*rval = ToJSVal( CStr( contents ) );
	return( JS_TRUE );
}


// Return file contents as an array of lines.
//
// lines = readFileLines(filename);
//   filename: VFS filename (may include path)
JSBool JSI_VFS::ReadFileLines( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	assert( argc >= 1 );
	CStr filename;
	if( !ToPrimitive<CStr>( cx, argv[0], filename ) )
		return( JS_FALSE );

	//
	// read file
	//

	void* p;
	size_t size;
	Handle hm = vfs_load( filename.c_str( ), p, size );
	JS_CHECK_FILE_ERR( hm );

	CStr contents( (const char*)p, size );
	mem_free_h( hm );


	//
	// split into array of strings (one per line)
	//

	std::stringstream ss( contents );
	JSObject* line_array = JS_NewArrayObject( cx, 0, NULL );
	std::string line;
	int cur_line = 0;
	while( std::getline( ss, line ) )
	{
		jsval val = ToJSVal( CStr( line ) );
		JS_SetElement( cx, line_array, cur_line++, &val );
	}

	*rval = OBJECT_TO_JSVAL( line_array );
	return( JS_TRUE );
}
