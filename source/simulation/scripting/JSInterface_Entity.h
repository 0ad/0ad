// JSInterface_Entity.h
//
// Last modified: 03 June 04, Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// The interface layer between JavaScript code and the actual CEntity object.
//
// Usage: Used when manipulating objects of class 'Entity' in JavaScript.
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com

#include "scripting/ScriptingHost.h"

#ifndef JSI_ENTITY_INCLUDED
#define JSI_ENTITY_INCLUDED

namespace JSI_Entity
{
	JSBool toString( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	extern JSClass JSI_class;
	extern JSPropertySpec JSI_props[];
	extern JSFunctionSpec JSI_methods[];
	JSBool addProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	JSBool delProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	JSBool getProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
    JSBool setProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	JSBool construct( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval );
	void finalize( JSContext* cx, JSObject* obj );
    void init();
};

#endif
