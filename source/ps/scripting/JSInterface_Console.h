// JSInterface_Console.h
//
// The JavaScript wrapper around the console system

#include "scripting/ScriptingHost.h"

#ifndef JSI_CONSOLE_INCLUDED
#define JSI_CONSOLE_INCLUDED

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

	JSBool writeConsole( JSContext* context, JSObject* obj, uint argc, jsval* argv, jsval* rval );
};

#endif
