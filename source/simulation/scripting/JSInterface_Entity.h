// JSInterface_Entity.h
//
// The interface layer between JavaScript code and the actual CEntity object.
//
// Usage: Used when manipulating objects of class 'Entity' in JavaScript.
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com

#include "scripting/ScriptingHost.h"

#ifndef JSI_ENTITY_INCLUDED
#define JSI_ENTITY_INCLUDED

/*
namespace JSI_Entity
{
	JSBool toString( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool order( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval, bool queueOrder );
	JSBool orderSingle( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool orderQueued( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	extern JSClass JSI_class;
	extern JSPropertySpec JSI_props[];
	extern JSFunctionSpec JSI_methods[];
	JSBool getProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
    JSBool setProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	JSBool construct( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval );
	void finalize( JSContext* cx, JSObject* obj );
    void init();
}
*/

#endif
