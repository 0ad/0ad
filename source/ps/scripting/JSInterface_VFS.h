// JSInterface_VFS.h
//
// The JavaScript wrapper around useful snippets of the VFS

#include "scripting/ScriptingHost.h"

#ifndef JSI_VFS_INCLUDED
#define JSI_VFS_INCLUDED

namespace JSI_VFS
{
	JSBool BuildFileList( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool GetFileMTime( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool GetFileSize( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool ReadFile( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool ReadFileLines( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
};

#endif
