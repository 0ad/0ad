// JSInterface_Entity.h
//
// 
// A JavaScript class representing a Pyrogenesis CVector3D object.
//
// Usage: Used when manipulating objects of class 'Vector3D' in JavaScript.
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com

#include "scripting/ScriptingHost.h"
#include "maths/Vector3D.h"

#ifndef JSI_VECTOR3_INCLUDED
#define JSI_VECTOR3_INCLUDED

namespace JSI_Vector3D
{
	enum 
	{
		component_x,
		component_y,
		component_z
	};
	JSBool toString( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool add( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool subtract( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool negate( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool scale( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool divide( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool dot( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool cross( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool length( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool normalize( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );

	struct Vector3D_Info
	{
		IPropertyOwner* owner;
		void ( IPropertyOwner::*updateFn )();
		void ( IPropertyOwner::*freshenFn )();
		CVector3D* vector;
		Vector3D_Info();
		Vector3D_Info( float x, float y, float z );
		Vector3D_Info( const CVector3D& copy );
		Vector3D_Info( CVector3D* attach, IPropertyOwner* _owner );
		Vector3D_Info( CVector3D* attach, IPropertyOwner* _owner, void (IPropertyOwner::*_updateFn)() );
		Vector3D_Info( CVector3D* attach, IPropertyOwner* _owner, void (IPropertyOwner::*_updateFn)(), void (IPropertyOwner::*_freshenFn)() );
		~Vector3D_Info();
	};
	extern JSClass JSI_class;
	extern JSPropertySpec JSI_props[];
	extern JSFunctionSpec JSI_methods[];

	JSBool getProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
    JSBool setProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	void finalize( JSContext* cx, JSObject* obj );
	JSBool construct( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
    void init();
}

#endif

