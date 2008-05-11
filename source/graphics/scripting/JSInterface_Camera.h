// JSInterface_Camera.h
//
// A JavaScript wrapper around a camera object.
//
// Usage: When manipulating objects of type 'Camera' in JavaScript

#include "scripting/ScriptingHost.h"

#include "maths/Vector3D.h"
#include "graphics/Camera.h"
#include "graphics/HFTracer.h"

#ifndef INCLUDED_JSI_CAMERA
#define INCLUDED_JSI_CAMERA

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

	struct Camera_Info : public IPropertyOwner
	{
		CCamera* m_Data;
		bool m_EngineOwned;
		CVector3D m_sv_Position;
		CVector3D m_sv_Orientation;
		CVector3D m_sv_Up;
		CVector3D m_sv_Target;

		Camera_Info(); // Create a camera set to the initial view

		Camera_Info( const CVector3D& Position );
		Camera_Info( const CVector3D& Position, const CVector3D& Orientation );
		Camera_Info( const CVector3D& Position, const CVector3D& Orientation, const CVector3D& Up );
		Camera_Info( const CMatrix3D& Orientation );
		Camera_Info( CCamera* copy );

		void Initialise();
		void Initialise( const CMatrix3D& Orientation );

		void Freshen();
		void Update();
		void FreshenTarget();

		~Camera_Info();
/*

		Camera_Info( const CVector3D& _position );
		Camera_Info( const CVector3D& _position, const CVector3D& _orientation );
		Camera_Info( const CVector3D& _position, const CVector3D& _orientation, const CVector3D& _up );
		Camera_Info( CCamera* _copy );
		void update();

*/
	};
	extern JSClass JSI_class;
	extern JSPropertySpec JSI_props[];
	extern JSFunctionSpec JSI_methods[];

	JSBool getProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	JSBool setProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );

	JSBool getCamera( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	JSBool setCamera( JSContext* cx, JSObject* obj, jsval id, jsval* vp );

	JSBool construct( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	void finalize( JSContext* cx, JSObject* obj );

	JSBool lookAt( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	JSBool getFocus( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );

    void init();
}

#endif
