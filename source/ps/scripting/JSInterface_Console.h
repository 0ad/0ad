// JSInterface_Console.h
//
// The JavaScript wrapper around the console system

#include "scripting/ScriptingHost.h"

#ifndef INCLUDED_JSI_CONSOLE
#define INCLUDED_JSI_CONSOLE

namespace JSI_Console
{
	enum
	{
		console_visible
	};
	extern JSClass JSI_class;
	extern JSPropertySpec JSI_props[];
	extern JSFunctionSpec JSI_methods[];

	JSBool getProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	JSBool setProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );

	JSBool getConsole( JSContext* context, JSObject* obj, jsval id, jsval* vp );

	void init();

	JSBool writeConsole( JSContext* context, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
}

#endif
