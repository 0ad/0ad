// JSInterface_Camera.h
//
// A JavaScript wrapper around a camera object.
//
// Usage: When manipulating objects of type 'Camera' in JavaScript
//
// Mark Thompson (mot20@cam.ac.uk / mark@wildfiregames.com)

#include "scripting/ScriptingHost.h"
#include "EntityProperties.h"

#include "Vector3D.h"
#include "Camera.h"
#include "HFTracer.h"

#ifndef JSI_CAMERA_INCLUDED
#define JSI_CAMERA_INCLUDED

namespace JSI_Camera
{
	enum
	{
		vector_position,
		vector_orientation,
		vector_up
	};

	enum
	{
		lookat
	};

	struct Camera_Info : public IBoundPropertyOwner
	{
		CVector3D position;
		CVector3D orientation;
		CVector3D up;
		CCamera* copy;
		Camera_Info(); // Create a camera set to the initial view
		Camera_Info( const CVector3D& _position );
		Camera_Info( const CVector3D& _position, const CVector3D& _orientation );
		Camera_Info( const CVector3D& _position, const CVector3D& _orientation, const CVector3D& _up );
		Camera_Info( CCamera* copy );
		void update();
	};
	extern JSClass JSI_class;
	extern JSPropertySpec JSI_props[];
	extern JSFunctionSpec JSI_methods[];

	JSBool getProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	JSBool setProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );

	JSBool getCamera( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	JSBool setCamera( JSContext* cx, JSObject* obj, jsval id, jsval* vp );

	JSBool construct( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval );
	void finalize( JSContext* cx, JSObject* obj );

	JSBool lookAt( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval );
	JSBool getFocus( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval );

    void init();
};

#endif
