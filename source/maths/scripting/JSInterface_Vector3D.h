// JSInterface_Entity.h
//
// Last modified: 03 June 04, Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// A JavaScript class representing a Prometheus CVector3D object.
//
// Usage: Used when manipulating objects of class 'Vector3D' in JavaScript.
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com

#include "scripting/ScriptingHost.h"
#include "Vector3D.h"

#ifndef JSI_VECTOR3_INCLUDED
#define JSI_VECTOR3_INCLUDED

class IPropertyOwner;

namespace JSI_Vector3D
{
	enum 
	{
		component_x,
		component_y,
		component_z
	};
	JSBool toString( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	struct Vector3D_Info
	{
		IPropertyOwner* owner;
		void ( IPropertyOwner::*updateFn )();
		CVector3D* vector;
		Vector3D_Info();
		Vector3D_Info( float x, float y, float z );
		Vector3D_Info( CVector3D* copy, IPropertyOwner* _owner );
		Vector3D_Info( CVector3D* copy, IPropertyOwner* _owner, void (IPropertyOwner::*_updateFn)() );
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
};

#endif

