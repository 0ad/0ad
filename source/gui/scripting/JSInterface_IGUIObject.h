// $Id: JSInterface_IGUIObject.h,v 1.2 2004/07/10 21:23:06 philip Exp $

#include "scripting/ScriptingHost.h"
#include "gui/GUI.h"

#ifndef JSI_IGUIOBJECT_INCLUDED
#define JSI_IGUIOBJECT_INCLUDED

namespace JSI_IGUIObject
{
	extern JSClass JSI_class;
	extern JSPropertySpec JSI_props[];
	extern JSFunctionSpec JSI_methods[];
	JSBool addProperty(JSContext* cx, JSObject* obj, jsval id, jsval* vp);
	JSBool delProperty(JSContext* cx, JSObject* obj, jsval id, jsval* vp);
	JSBool getProperty(JSContext* cx, JSObject* obj, jsval id, jsval* vp);
	JSBool setProperty(JSContext* cx, JSObject* obj, jsval id, jsval* vp);
	JSBool construct(JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval);
	JSBool getByName(JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval);
	JSBool toString(JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval);
	void init();
	void x();
}

#endif
