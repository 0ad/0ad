#include "precompiled.h"

#include "JSInterface_Vector3D.h"

JSClass JSI_Vector3D::JSI_class = {
	"Vector3D", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JSI_Vector3D::getProperty, JSI_Vector3D::setProperty,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JSI_Vector3D::finalize,
	NULL, NULL, NULL, NULL 
};

JSPropertySpec JSI_Vector3D::JSI_props[] = 
{
	{ "x", JSI_Vector3D::component_x, JSPROP_ENUMERATE },
	{ "y", JSI_Vector3D::component_y, JSPROP_ENUMERATE },
	{ "z", JSI_Vector3D::component_z, JSPROP_ENUMERATE },
	{ 0 }
};

JSFunctionSpec JSI_Vector3D::JSI_methods[] = 
{
	{ "toString", JSI_Vector3D::toString, 0, 0, 0 },
	{ 0 }
};

void JSI_Vector3D::init()
{
	g_ScriptingHost.DefineCustomObjectType( &JSI_class, JSI_Vector3D::construct, 0, JSI_props, JSI_methods, NULL, NULL );
}

JSI_Vector3D::Vector3D_Info::Vector3D_Info()
{
	owner = NULL;
	vector = new CVector3D();
}

JSI_Vector3D::Vector3D_Info::Vector3D_Info( float x, float y, float z )
{
	owner = NULL;
	vector = new CVector3D( x, y, z );
}

JSI_Vector3D::Vector3D_Info::Vector3D_Info( CVector3D* copy, IPropertyOwner* _owner )
{
	owner = _owner;
	updateFn = NULL;
	vector = copy;
}

JSI_Vector3D::Vector3D_Info::Vector3D_Info( CVector3D* copy, IPropertyOwner* _owner, void( IPropertyOwner::*_updateFn )(void) )
{
	owner = _owner;
	updateFn = _updateFn;
	vector = copy;
}

JSI_Vector3D::Vector3D_Info::~Vector3D_Info()
{
	if( !owner ) delete( vector );
}

JSBool JSI_Vector3D::getProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	if( !JSVAL_IS_INT( id ) )
		return( JS_TRUE );

	Vector3D_Info* vectorInfo = (Vector3D_Info*)JS_GetPrivate( cx, obj );
	if( !vectorInfo )
	{
		JS_ReportError( cx, "[Vector3D] Invalid reference" );
		return( JS_TRUE );
	}
 	CVector3D* vectorData = vectorInfo->vector;

	switch( g_ScriptingHost.ValueToInt( id ) )
	{
	case component_x: *vp = DOUBLE_TO_JSVAL( JS_NewDouble( cx, vectorData->X ) ); return( JS_TRUE );
	case component_y: *vp = DOUBLE_TO_JSVAL( JS_NewDouble( cx, vectorData->Y ) ); return( JS_TRUE );
	case component_z: *vp = DOUBLE_TO_JSVAL( JS_NewDouble( cx, vectorData->Z ) ); return( JS_TRUE );
	}

	return( JS_FALSE );
}

JSBool JSI_Vector3D::setProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	if( !JSVAL_IS_INT( id ) )
		return( JS_TRUE );

	Vector3D_Info* vectorInfo = (Vector3D_Info*)JS_GetPrivate( cx, obj );
	if( !vectorInfo )
	{
		JS_ReportError( cx, "[Vector3D] Invalid reference" );
		return( JS_TRUE );
	}
	CVector3D* vectorData = vectorInfo->vector;

	try
	{
		switch( g_ScriptingHost.ValueToInt( id ) )
		{
		case component_x: vectorData->X = (float)g_ScriptingHost.ValueToDouble( *vp ); break;
		case component_y: vectorData->Y = (float)g_ScriptingHost.ValueToDouble( *vp ); break;
		case component_z: vectorData->Z = (float)g_ScriptingHost.ValueToDouble( *vp ); break;
		}
	}
	catch (PSERROR_Scripting_ConversionFailed)
	{
		JS_ReportError( cx, "Invalid parameter value for Vector3D" );
		return( JS_TRUE );
	}
	
	if( vectorInfo->owner && vectorInfo->updateFn ) ( (vectorInfo->owner)->*(vectorInfo->updateFn) )();

	return( JS_TRUE );
}

JSBool JSI_Vector3D::construct( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	if( argc == 0 )
	{
		JS_SetPrivate( cx, obj, new Vector3D_Info() );
		return( JS_TRUE );
	}
	else if( argc == 3 )
	{
		try
		{
			float x = (float)g_ScriptingHost.ValueToDouble( argv[0] );
			float y = (float)g_ScriptingHost.ValueToDouble( argv[1] );
			float z = (float)g_ScriptingHost.ValueToDouble( argv[2] );
			JS_SetPrivate( cx, obj, new Vector3D_Info( x, y, z ) );
			return( JS_TRUE );
		}
		catch (PSERROR_Scripting_ConversionFailed)
		{
			// Invalid input (i.e. can't be coerced into doubles) - fail
			JS_ReportError( cx, "Invalid parameters to Vector3D constructor" );
			return( JS_TRUE );
		}
	}
	else
	{
		JS_ReportError( cx, "Invalid number of parameters to Vector3D constructor" );
		return( JS_FALSE );
	}
}

void JSI_Vector3D::finalize( JSContext* cx, JSObject* obj )
{
	delete( (Vector3D_Info*)JS_GetPrivate( cx, obj ) );
}

JSBool JSI_Vector3D::toString( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	char buffer[256];
	Vector3D_Info* vectorInfo = (Vector3D_Info*)JS_GetPrivate( cx, obj );
	if( !vectorInfo ) return( JS_TRUE );
 	CVector3D* vectorData = vectorInfo->vector;
	snprintf( buffer, 256, "[object Vector3D: ( %f, %f, %f )]", vectorData->X, vectorData->Y, vectorData->Z );
	buffer[255] = 0;
	*rval = STRING_TO_JSVAL( JS_NewStringCopyZ( cx, buffer ) );
	return( JS_TRUE );
}
