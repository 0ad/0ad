/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "JSInterface_Vector3D.h"
#include "scripting/JSConversions.h"
#include "scripting/ScriptingHost.h"

namespace JSI_Vector3D
{
	static CVector3D* GetVector( JSContext* cx, JSObject* obj );
}

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
	{ "add", JSI_Vector3D::add, 1, 0, 0 },
	{ "subtract", JSI_Vector3D::subtract, 1, 0, 0 },
	{ "minus", JSI_Vector3D::subtract, 1, 0, 0 },
	{ "negate", JSI_Vector3D::negate, 1, 0, 0 },
	{ "scale", JSI_Vector3D::scale, 1, 0, 0 },
	{ "multiply", JSI_Vector3D::scale, 1, 0, 0 },
	{ "divide", JSI_Vector3D::divide, 1, 0, 0 },
	{ "dot", JSI_Vector3D::dot, 1, 0, 0 },
	{ "cross", JSI_Vector3D::cross, 1, 0, 0 },
	{ "length", JSI_Vector3D::length, 0, 0, 0 },
	{ "normalize", JSI_Vector3D::normalize, 0, 0, 0 },
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

JSI_Vector3D::Vector3D_Info::Vector3D_Info( const CVector3D& copy )
{
	owner = NULL;
	vector = new CVector3D( copy.X, copy.Y, copy.Z );
}

JSI_Vector3D::Vector3D_Info::Vector3D_Info( CVector3D* attach, IPropertyOwner* _owner )
{
	owner = _owner;
	updateFn = NULL;
	freshenFn = NULL;
	vector = attach;
}

JSI_Vector3D::Vector3D_Info::Vector3D_Info( CVector3D* attach, IPropertyOwner* _owner, void( IPropertyOwner::*_updateFn )(void) )
{
	owner = _owner;
	updateFn = _updateFn;
	freshenFn = NULL;
	vector = attach;
}

JSI_Vector3D::Vector3D_Info::Vector3D_Info( CVector3D* attach, IPropertyOwner* _owner, void( IPropertyOwner::*_updateFn )(void), void( IPropertyOwner::*_freshenFn )(void) )
{
	owner = _owner;
	updateFn = _updateFn;
	freshenFn = _freshenFn;
	vector = attach;
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

	if( vectorInfo->owner && vectorInfo->freshenFn ) ( (vectorInfo->owner)->*(vectorInfo->freshenFn) )();
	
	switch( ToPrimitive<int>( id ) )
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

	if( vectorInfo->owner && vectorInfo->freshenFn ) ( (vectorInfo->owner)->*(vectorInfo->freshenFn) )();

	switch( ToPrimitive<int>( id ) )
	{
	case component_x: vectorData->X = ToPrimitive<float>( *vp ); break;
	case component_y: vectorData->Y = ToPrimitive<float>( *vp ); break;
	case component_z: vectorData->Z = ToPrimitive<float>( *vp ); break;
	}

	
	if( vectorInfo->owner && vectorInfo->updateFn ) ( (vectorInfo->owner)->*(vectorInfo->updateFn) )();

	return( JS_TRUE );
}

JSBool JSI_Vector3D::construct( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	JSObject* vector = JS_NewObject( cx, &JSI_Vector3D::JSI_class, NULL, NULL );
	
	if( argc == 0 )
	{
		JS_SetPrivate( cx, vector, new Vector3D_Info() );
		*rval = OBJECT_TO_JSVAL( vector );
		return( JS_TRUE );
	}

	JSU_REQUIRE_PARAMS(3);
	try
	{
		float x = ToPrimitive<float>( argv[0] );
		float y = ToPrimitive<float>( argv[1] );
		float z = ToPrimitive<float>( argv[2] );
		JS_SetPrivate( cx, vector, new Vector3D_Info( x, y, z ) );
		*rval = OBJECT_TO_JSVAL( vector );
		return( JS_TRUE );
	}
	catch (PSERROR_Scripting_ConversionFailed)
	{
		// Invalid input (i.e. can't be coerced into doubles) - fail
		JS_ReportError( cx, "Invalid parameters to Vector3D constructor" );
		*rval = JSVAL_NULL;
		return( JS_FALSE );
	}
}

void JSI_Vector3D::finalize( JSContext* cx, JSObject* obj )
{
	delete( (Vector3D_Info*)JS_GetPrivate( cx, obj ) );
}

JSBool JSI_Vector3D::toString( JSContext* cx, JSObject* obj,
	uintN UNUSED(argc), jsval* UNUSED(argv), jsval* rval )
{
	char buffer[256];
	Vector3D_Info* vectorInfo = (Vector3D_Info*)JS_GetPrivate( cx, obj );

	if( !vectorInfo ) return( JS_TRUE );

	if( vectorInfo->owner && vectorInfo->freshenFn ) ( (vectorInfo->owner)->*(vectorInfo->freshenFn) )();

 	CVector3D* vectorData = vectorInfo->vector;
	snprintf( buffer, 256, "[object Vector3D: ( %f, %f, %f )]", vectorData->X, vectorData->Y, vectorData->Z );
	buffer[255] = 0;
	*rval = STRING_TO_JSVAL( JS_NewStringCopyZ( cx, buffer ) );
	return( JS_TRUE );
}

CVector3D* JSI_Vector3D::GetVector( JSContext* cx, JSObject* obj )
{
	Vector3D_Info* vectorInfo = (Vector3D_Info*)JS_GetInstancePrivate( cx, obj, &JSI_class, NULL );
	if( !vectorInfo )
	{
		JS_ReportError( cx, "[Vector3D] Invalid reference" );
		return( NULL );
	}
	if( vectorInfo->owner && vectorInfo->freshenFn ) ( (vectorInfo->owner)->*(vectorInfo->freshenFn) )();
	return( vectorInfo->vector );
}

#define GET_VECTORS\
	CVector3D* a = GetVector( cx, obj );\
	if(!a)\
		return( JS_TRUE );\
	if( ( argc == 0 ) || !JSVAL_IS_OBJECT( argv[0] ) )\
	{\
invalid_param:\
		JS_ReportError( cx, "[Vector3D] Invalid parameter" );\
		return( JS_TRUE );\
	}\
	CVector3D* b = GetVector( cx, JSVAL_TO_OBJECT( argv[0] ) );\
	if(!b)\
		goto invalid_param;

#define GET_VECTOR_AND_FLOAT\
	CVector3D* v = GetVector( cx, obj );\
	if(!v)\
		return( JS_TRUE );\
	float f;\
	if( ( argc == 0 ) || !ToPrimitive( cx, argv[0], f ) )\
	{\
		JS_ReportError( cx, "Invalid parameter" );\
		return( JS_TRUE );\
	}


JSBool JSI_Vector3D::add( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	GET_VECTORS;

	JSObject* vector3d = JS_NewObject( g_ScriptingHost.getContext(), &JSI_Vector3D::JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.getContext(), vector3d, new JSI_Vector3D::Vector3D_Info( *a + *b ) );
	*rval = OBJECT_TO_JSVAL( vector3d );

	return( JS_TRUE );
}

JSBool JSI_Vector3D::subtract( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	GET_VECTORS;

	JSObject* vector3d = JS_NewObject( g_ScriptingHost.getContext(), &JSI_Vector3D::JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.getContext(), vector3d, new JSI_Vector3D::Vector3D_Info( *a - *b ) );
	*rval = OBJECT_TO_JSVAL( vector3d );

	return( JS_TRUE );
}

JSBool JSI_Vector3D::negate( JSContext* cx, JSObject* obj,
	uintN UNUSED(argc), jsval* UNUSED(argv), jsval* rval )
{
	CVector3D* v = GetVector( cx, obj );
	if(!v)
		return( JS_TRUE );

	*rval = ToJSVal( -( *v ) );

	return( JS_TRUE );
}

JSBool JSI_Vector3D::scale( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	GET_VECTOR_AND_FLOAT;

	JSObject* vector3d = JS_NewObject( g_ScriptingHost.getContext(), &JSI_Vector3D::JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.getContext(), vector3d, new JSI_Vector3D::Vector3D_Info( *v * f ) );
	*rval = OBJECT_TO_JSVAL( vector3d );

	return( JS_TRUE );
}

JSBool JSI_Vector3D::divide( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	GET_VECTOR_AND_FLOAT;

	JSObject* vector3d = JS_NewObject( g_ScriptingHost.getContext(), &JSI_Vector3D::JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.getContext(), vector3d, new JSI_Vector3D::Vector3D_Info( *v * ( 1.0f / f ) ) );
	*rval = OBJECT_TO_JSVAL( vector3d );

	return( JS_TRUE );
}

JSBool JSI_Vector3D::dot( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	GET_VECTORS;

	*rval = ToJSVal( a->Dot( *b ) );
	return( JS_TRUE );
}

JSBool JSI_Vector3D::cross( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	GET_VECTORS;

	*rval = ToJSVal( a->Cross( *b ) );
	return( JS_TRUE );
}

JSBool JSI_Vector3D::length( JSContext* cx, JSObject* obj,
	uintN UNUSED(argc), jsval* UNUSED(argv), jsval* rval )
{
	CVector3D* v = GetVector( cx, obj );
	if(!v)
		return( JS_TRUE );

	*rval = ToJSVal( v->Length() );
	return( JS_TRUE );
}

JSBool JSI_Vector3D::normalize( JSContext* cx, JSObject* obj,
	uintN UNUSED(argc), jsval* UNUSED(argv), jsval* rval )
{
	CVector3D* v = GetVector( cx, obj );
	if(!v)
		return( JS_TRUE );

	CVector3D r( v->X, v->Y, v->Z );
	r.Normalize();

	*rval = ToJSVal( r );
	return( JS_TRUE );
}
