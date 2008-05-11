#include "scripting/ScriptingHost.h"

#ifndef INCLUDED_JSI_IGUIOBJECT
#define INCLUDED_JSI_IGUIOBJECT

namespace JSI_IGUIObject
{
	extern JSClass JSI_class;
	extern JSPropertySpec JSI_props[];
	extern JSFunctionSpec JSI_methods[];
	JSBool addProperty(JSContext* cx, JSObject* obj, jsval id, jsval* vp);
	JSBool delProperty(JSContext* cx, JSObject* obj, jsval id, jsval* vp);
	JSBool getProperty(JSContext* cx, JSObject* obj, jsval id, jsval* vp);
	JSBool setProperty(JSContext* cx, JSObject* obj, jsval id, jsval* vp);
	JSBool construct(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);
	JSBool getByName(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);
	JSBool toString(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);
	void init();
}

#endif
