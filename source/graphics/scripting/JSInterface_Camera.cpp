#include "precompiled.h"

#include "JSInterface_Camera.h"
#include "scripting/JSInterface_Vector3D.h"
#include "Camera.h"
#include "Vector3D.h"
#include "Matrix3D.h"
#include "Terrain.h"
#include "Game.h"

JSClass JSI_Camera::JSI_class = {
	"Camera", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JSI_Camera::getProperty, JSI_Camera::setProperty,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JSI_Camera::finalize,
	NULL, NULL, NULL, NULL
};

JSPropertySpec JSI_Camera::JSI_props[] =
{
	{ "position", JSI_Camera::vector_position, JSPROP_ENUMERATE },
	{ "orientation", JSI_Camera::vector_orientation, JSPROP_ENUMERATE },
	{ "up", JSI_Camera::vector_up, JSPROP_ENUMERATE },
	{ 0 },
};

JSFunctionSpec JSI_Camera::JSI_methods[] = 
{
    { "lookAt", JSI_Camera::lookAt, 2, 0, 0 },
	{ "getFocus", JSI_Camera::getFocus, 0, 0, 0 },
	{ 0 }
};

void JSI_Camera::init()
{
	g_ScriptingHost.DefineCustomObjectType( &JSI_class, JSI_Camera::construct, 0, JSI_props, JSI_methods, NULL, NULL );
}

JSI_Camera::Camera_Info::Camera_Info()
{
	CMatrix3D orient;
	orient.SetXRotation( DEGTORAD( 30 ) );
	orient.RotateY( DEGTORAD( -45 ) );
	orient.Translate( 100, 150, -100 );

	Camera_Info( orient.GetTranslation(), orient.GetIn(), orient.GetUp() );
}

JSI_Camera::Camera_Info::Camera_Info( const CVector3D& _position )
{
	Camera_Info();
	position = _position;
}

JSI_Camera::Camera_Info::Camera_Info( const CVector3D& _position, const CVector3D& _orientation )
{
	Camera_Info( _position, _orientation, CVector3D( 0.0f, 1.0f, 0.0f ) );
}

JSI_Camera::Camera_Info::Camera_Info( const CVector3D& _position, const CVector3D& _orientation, const CVector3D& _up )
{
	position = _position;
	orientation = _orientation;
	up = _up;
	copy = NULL;
}

JSI_Camera::Camera_Info::Camera_Info( CCamera* _copy )
{
	copy = _copy;
	position = copy->m_Orientation.GetTranslation();
	orientation = copy->m_Orientation.GetIn();
	up = copy->m_Orientation.GetUp();
}

void JSI_Camera::Camera_Info::update()
{
	if( copy )
		copy->LookAlong( position, orientation, up );	
}

JSBool JSI_Camera::getCamera( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	JSObject* camera = JS_NewObject( cx, &JSI_Camera::JSI_class, NULL, NULL );
	JS_SetPrivate( cx, camera, new Camera_Info( g_Game->GetView()->GetCamera() ) );
	*vp = OBJECT_TO_JSVAL( camera );
	return( JS_TRUE );
}

JSBool JSI_Camera::setCamera( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	JSObject* camera = JSVAL_TO_OBJECT( *vp );
	Camera_Info* cameraInfo;
	if( !JSVAL_IS_OBJECT( *vp ) || !( cameraInfo = (Camera_Info*)JS_GetInstancePrivate( cx, camera, &JSI_Camera::JSI_class, NULL ) ) )
	{
		JS_ReportError( cx, "[Camera] Invalid object" );
	}
	else
		g_Game->GetView()->GetCamera()->LookAlong( cameraInfo->position, cameraInfo->orientation, cameraInfo->up );	
	return( JS_TRUE );
}

JSBool JSI_Camera::getProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	if( !JSVAL_IS_INT( id ) )
		return( JS_TRUE );

	Camera_Info* cameraInfo = (Camera_Info*)JS_GetPrivate( cx, obj );
	if( !cameraInfo )
	{
		JS_ReportError( cx, "[Camera] Invalid Reference" );
		return( JS_TRUE );
	}
	
	CVector3D* d;

	switch( g_ScriptingHost.ValueToInt( id ) )
	{
	case vector_position: d = &cameraInfo->position; break;
	case vector_orientation: d = &cameraInfo->orientation; break;
	case vector_up: d = &cameraInfo->up; break;
	default: return( JS_FALSE );
	}

	JSObject* vector3d = JS_NewObject( g_ScriptingHost.getContext(), &JSI_Vector3D::JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.getContext(), vector3d, new JSI_Vector3D::Vector3D_Info( d, cameraInfo, ( void( IBoundPropertyOwner::* )() )&Camera_Info::update ) );
	*vp = OBJECT_TO_JSVAL( vector3d );
	return( JS_TRUE );
}

JSBool JSI_Camera::setProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	if( !JSVAL_IS_INT( id ) )
		return( JS_TRUE );

	Camera_Info* cameraInfo = (Camera_Info*)JS_GetPrivate( cx, obj );
	if( !cameraInfo )
	{
		JS_ReportError( cx, "[Camera] Invalid reference" );
		return( JS_TRUE );
	}
	
	JSObject* vector3d = JSVAL_TO_OBJECT( *vp );
	JSI_Vector3D::Vector3D_Info* v = NULL;

	if( JSVAL_IS_OBJECT( *vp ) && ( v = (JSI_Vector3D::Vector3D_Info*)JS_GetInstancePrivate( g_ScriptingHost.getContext(), vector3d, &JSI_Vector3D::JSI_class, NULL ) ) )
	{
		switch( g_ScriptingHost.ValueToInt( id ) )
		{
		case vector_position: cameraInfo->position = *( v->vector ); break;
		case vector_orientation: cameraInfo->orientation = *( v->vector ); break;
		case vector_up: cameraInfo->up = *( v->vector ); break;
		}

		cameraInfo->update();
	}

	return( JS_TRUE );
}

JSBool JSI_Camera::lookAt( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval )
{
	assert( argc >= 2 );

	JSI_Vector3D::Vector3D_Info* v = NULL;
	Camera_Info* cameraInfo = (Camera_Info*)JS_GetPrivate( cx, obj );

#define GETVECTOR( jv ) ( ( JSVAL_IS_OBJECT( jv ) && ( v = (JSI_Vector3D::Vector3D_Info*)JS_GetInstancePrivate( g_ScriptingHost.getContext(), JSVAL_TO_OBJECT( jv ), &JSI_Vector3D::JSI_class, NULL ) ) ) ? *(v->vector) : CVector3D() )

	if( argc <= 3 )
	{
		cameraInfo->position = GETVECTOR( argv[0] );
		cameraInfo->orientation = ( GETVECTOR( argv[1] ) - GETVECTOR( argv[0] ) );
		cameraInfo->orientation.Normalize();
	}
	else
	{
		JS_ReportError( cx, "[Camera] Too many arguments to lookAt" );
		*rval = JSVAL_FALSE;
		return( JS_TRUE );
	}
	
	if( argc == 2 )
	{
		cameraInfo->up = CVector3D( 0.0f, 1.0f, 0.0f );
	}
	else if( argc == 3 )
	{
		cameraInfo->up = GETVECTOR( argv[2] );
	}

#undef GETVECTOR

	cameraInfo->update();

	*rval = JSVAL_TRUE;
	return( JS_TRUE );
}

JSBool JSI_Camera::getFocus( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	// Largely copied from the equivalent method in CCamera

	Camera_Info* cameraInfo = (Camera_Info*)JS_GetPrivate( cx, obj );

	CHFTracer tracer( g_Game->GetWorld()->GetTerrain() ); int x, z;

	CVector3D currentTarget;
	
	if( !tracer.RayIntersect( cameraInfo->position, cameraInfo->orientation, x, z, currentTarget ) )
	{
		// Off the edge of the world?
		// Work out where it /would/ hit, if the map were extended out to infinity with average height.

		currentTarget = cameraInfo->position + cameraInfo->orientation * ( ( 50.0f - cameraInfo->position.Y ) / cameraInfo->orientation.Y );
	}

	JSObject* vector3d = JS_NewObject( g_ScriptingHost.getContext(), &JSI_Vector3D::JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.getContext(), vector3d, new JSI_Vector3D::Vector3D_Info( currentTarget ) );
	*rval = OBJECT_TO_JSVAL( vector3d );
	return( JS_TRUE );
}

JSBool JSI_Camera::construct( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	JSI_Vector3D::Vector3D_Info* v = NULL;

	JSObject* camera = JS_NewObject( cx, &JSI_Camera::JSI_class, NULL, NULL );

#define GETVECTOR( jv ) ( ( JSVAL_IS_OBJECT( jv ) && ( v = (JSI_Vector3D::Vector3D_Info*)JS_GetInstancePrivate( g_ScriptingHost.getContext(), JSVAL_TO_OBJECT( jv ), &JSI_Vector3D::JSI_class, NULL ) ) ) ? *(v->vector) : CVector3D() )

	if( argc == 0 )
	{
		JS_SetPrivate( cx, camera, new Camera_Info() );
	}
	else if( argc == 1 )
	{
		JS_SetPrivate( cx, camera, new Camera_Info( GETVECTOR( argv[0] ) ) );
	}
	else if( argc == 2 )
	{
		JS_SetPrivate( cx, camera, new Camera_Info( GETVECTOR( argv[0] ), GETVECTOR( argv[1] ) ) );
	}
	else if( argc == 3 )
	{
		JS_SetPrivate( cx, camera, new Camera_Info( GETVECTOR( argv[0] ), GETVECTOR( argv[1] ), GETVECTOR( argv[2] ) ) );
	}
	else
	{
		JS_ReportError( cx, "[Camera] Too many arguments to constructor" );
		*rval = JSVAL_NULL;
		return( JS_TRUE );
	}

#undef GET_VECTOR

	*rval = OBJECT_TO_JSVAL( camera );
	return( JS_TRUE );
}

void JSI_Camera::finalize( JSContext* cx, JSObject* obj )
{
	delete( (Camera_Info*)JS_GetPrivate( cx, obj ) );
}

