
#include "ScriptingHost.h"

// POINT2D

JSClass Point2dClass = 
{
	"Point2d", 0,
	JS_PropertyStub, JS_PropertyStub,
	JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JS_FinalizeStub
};

JSPropertySpec Point2dProperties[] = 
{
	{"x",	0,	JSPROP_ENUMERATE},
	{"y",	1,	JSPROP_ENUMERATE},
	{0}
};

JSBool Point2d_Constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	if (argc == 2)
	{
		g_ScriptingHost.SetObjectProperty(obj, "x", argv[0]);
		g_ScriptingHost.SetObjectProperty(obj, "y", argv[1]);
	}
	else
	{
		jsval zero = INT_TO_JSVAL(0);
		g_ScriptingHost.SetObjectProperty(obj, "x", zero);
		g_ScriptingHost.SetObjectProperty(obj, "y", zero);
	}

	return JS_TRUE;
}

