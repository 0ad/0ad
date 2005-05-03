#include "precompiled.h"

#include "ps/CStr.h"
#include "ps/VFSUtil.h"
#include "lib/res/res.h"
#include "scripting/ScriptingHost.h"
#include "scripting/JSConversions.h"
#include "scripting/JSInterface_VFS.h"


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

static void BuildFileListCB(const char* path, const vfsDirEnt* ent, void* context)
{
	BuildFileListState* s = (BuildFileListState*)context;

	jsval val = ToJSVal( CStr ( path ) );
		// note: <path> is already directory + name!

	JS_SetElement(s->cx, s->filename_array, s->cur_idx, &val);
	s->cur_idx++;
}


// array_of_filenames = buildFileList(start_path [, pattern [, recursive ] ]);
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
	VFSUtil::EnumDirEnts(path, filter, recursive, BuildFileListCB, &state);

	*rval = OBJECT_TO_JSVAL( state.filename_array );
	return( JS_TRUE );
}


// seconds_since_1970 = getFileMTime(filename);
JSBool JSI_VFS::GetFileMTime( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	assert( argc >= 1 );
	CStr filename;
	if( !ToPrimitive<CStr>( cx, argv[0], filename ) )
		return( JS_FALSE );

	struct stat s;
	int err = vfs_stat(filename.c_str(), &s);
	// this may happen; don't complain
	if(err == ERR_FILE_NOT_FOUND)
	{
		*rval = JSVAL_NULL;
		return( JS_TRUE );
	}
	// unknown failure - this will stop JS execution so the error is noticed.
	else if(err < 0)
		return( JS_FALSE );
	// else: success

	*rval = ToJSVal( (double)s.st_mtime );
	return( JS_TRUE );
}


// size = getFileSize(filename);
JSBool JSI_VFS::GetFileSize( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	assert( argc >= 1 );
	CStr filename;
	if( !ToPrimitive<CStr>( cx, argv[0], filename ) )
		return( JS_FALSE );

	struct stat s;
	int err = vfs_stat(filename.c_str(), &s);
	// this may happen; don't complain
	if(err == ERR_FILE_NOT_FOUND)
	{
		*rval = JSVAL_NULL;
		return( JS_TRUE );
	}
	// unknown failure - this will stop JS execution so the error is noticed.
	else if(err < 0)
		return( JS_FALSE );
	// else: success

	*rval = ToJSVal( (uint)s.st_size );
	return( JS_TRUE );
}


// contents_string = readFile(filename);
JSBool JSI_VFS::ReadFile( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	assert( argc >= 1 );
	CStr filename;
	if( !ToPrimitive<CStr>( cx, argv[0], filename ) )
		return( JS_FALSE );

	void* p;
	size_t size;
	Handle hm = vfs_load(filename.c_str(), p, size);
	// this may happen; don't complain
	if(hm == ERR_FILE_NOT_FOUND)
	{
		*rval = JSVAL_NULL;
		return( JS_TRUE );
	}
	// unknown failure - this will stop JS execution so the error is noticed.
	else if(hm <= 0)
		return( JS_FALSE );
	// else: success

	CStr contents((const char*)p, size);
	mem_free_h(hm);

	*rval = ToJSVal( CStr( contents ) );
	return( JS_TRUE );
}


// array_of_strings = readFileLines(filename);
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
	Handle hm = vfs_load(filename.c_str(), p, size);
	// this may happen; don't complain
	if(hm == ERR_FILE_NOT_FOUND)
	{
		*rval = JSVAL_NULL;
		return( JS_TRUE );
	}
	// unknown failure - this will stop JS execution so the error is noticed.
	else if(hm <= 0)
		return( JS_FALSE );
	// else: success

	CStr contents((const char*)p, size);
	mem_free_h(hm);


	//
	// split into array of strings (one per line)
	//

	std::stringstream ss(contents);
	JSObject* line_array = JS_NewArrayObject(cx, 0, NULL);
	std::string line;
	while(std::getline(ss, line))
	{
		jsval val = ToJSVal( CStr( line ) );
		JS_SetElement(cx, line_array, 0, &val);
	}

	*rval = OBJECT_TO_JSVAL( line_array);
	return( JS_TRUE );
}
