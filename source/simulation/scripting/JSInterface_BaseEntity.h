// JSInterface_BaseEntity.h
//
// Last modified: 08 June 04, Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// An interface between CBaseEntity and the JavaScript class EntityTemplate
//
// Usage: Used when manipulating objects of class 'EntityTemplate' in JavaScript.
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com

#include "scripting/ScriptingHost.h"

#ifndef JSI_BASEENTITY_INCLUDED
#define JSI_BASEENTITY_INCLUDED

namespace JSI_BaseEntity
{
	JSBool toString( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	extern JSClass JSI_class;
	extern JSPropertySpec JSI_props[];
	extern JSFunctionSpec JSI_methods[];
	JSBool addProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	JSBool delProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	JSBool getProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
    JSBool setProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	void finalize( JSContext* cx, JSObject* obj );
    void init();
};

#endif
