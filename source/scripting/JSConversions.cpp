#include "precompiled.h"
#include "JSConversions.h"
#include "Entity.h"
#include "ObjectManager.h"
#include "scripting/JSInterface_Vector3D.h"

// HEntity

template<> HEntity* ToNative<HEntity>( JSContext* cx, JSObject* obj )
{
	CEntity* e = ToNative<CEntity>( cx, obj );
	return( &( e->me ) );
}

template<> JSObject* ToScript<HEntity>( HEntity* Native )
{
	return( ToScript<CEntity>( &( **Native ) ) );
}

// CBaseEntity*

template<> bool ToPrimitive<CBaseEntity*>( JSContext* cx, jsval v, CBaseEntity*& Storage )
{
	if( !JSVAL_IS_OBJECT( v ) ) return( false );
	CBaseEntity* Data = (CBaseEntity*)JS_GetInstancePrivate( cx, JSVAL_TO_OBJECT( v ), &CBaseEntity::JSI_class, NULL );
	if( !Data ) return( false );
	Storage = Data;
	return( true );
}

template<> JSObject* ToScript<CBaseEntity*>( CBaseEntity** Native )
{
	return( ToScript<CBaseEntity>( *Native ) );
}

// CObjectEntry*

template<> bool ToPrimitive<CObjectEntry*>( JSContext* cx, jsval v, CObjectEntry*& Storage )
{
	CStrW ActorName;
	if( !ToPrimitive<CStrW>( cx, v, ActorName ) )
		return( false );
	Storage = g_ObjMan.FindObject( (CStr)ActorName );
	return( true );
}

template<> jsval ToJSVal<CObjectEntry*>( CObjectEntry*& Native )
{
	if( !Native )
		return( ToJSVal<CStrW>( CStrW( L"[No actor]" ) ) );
	return( ToJSVal<CStrW>( CStrW( Native->m_Name ) ) );
}

// CVector3D

template<> CVector3D* ToNative<CVector3D>( JSContext* cx, JSObject* obj )
{
	JSI_Vector3D::Vector3D_Info* v = (JSI_Vector3D::Vector3D_Info*)JS_GetPrivate( cx, obj );
	return( v->vector );
}

template<> JSObject* ToScript<CVector3D>( CVector3D* Native )
{
	JSObject* Script = JS_NewObject( g_ScriptingHost.GetContext(), &JSI_Vector3D::JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.GetContext(), Script, new JSI_Vector3D::Vector3D_Info( *Native ) );
	return( Script );
}


// CScriptObject

template<> jsval ToJSVal<CScriptObject>( CScriptObject& Native )
{
	if( Native.Type == CScriptObject::FUNCTION )
		return( OBJECT_TO_JSVAL( JS_GetFunctionObject( Native.Function ) ) );
	return( JSVAL_NULL );
}

template<> bool ToPrimitive<CScriptObject>( JSContext* cx, jsval v, CScriptObject& Storage )
{
	Storage.SetJSVal( v );
	return( true );
}

// i32

template<> jsval ToJSVal<i32>( const i32& Native )
{
	return( INT_TO_JSVAL( Native ) );
}

template<> jsval ToJSVal<i32>( i32& Native )
{
	return( INT_TO_JSVAL( Native ) );
}

template<> bool ToPrimitive<i32>( JSContext* cx, jsval v, i32& Storage )
{
	try
	{
		Storage = g_ScriptingHost.ValueToInt( v );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		return( false );
	}
	return( true );
}

// double

template<> jsval ToJSVal<double>( const double& Native )
{
	return( DOUBLE_TO_JSVAL( JS_NewDouble( g_ScriptingHost.getContext(), Native ) ) );
}

template<> jsval ToJSVal<double>( double& Native )
{
	return( DOUBLE_TO_JSVAL( JS_NewDouble( g_ScriptingHost.getContext(), Native ) ) );
}

template<> bool ToPrimitive<double>( JSContext* cx, jsval v, double& Storage )
{
	try
	{
		Storage = g_ScriptingHost.ValueToDouble( v );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		return( false );
	}
	return( true );
}

// float

template<> jsval ToJSVal<float>( const float& Native )
{
	return( DOUBLE_TO_JSVAL( JS_NewDouble( g_ScriptingHost.getContext(), Native ) ) );
}

template<> jsval ToJSVal<float>( float& Native )
{
	return( DOUBLE_TO_JSVAL( JS_NewDouble( g_ScriptingHost.getContext(), Native ) ) );
}

template<> bool ToPrimitive<float>( JSContext* cx, jsval v, float& Storage )
{
	try
	{
		Storage = (float)g_ScriptingHost.ValueToDouble( v );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		return( false );
	}
	return( true );
}
	

// bool

template<> jsval ToJSVal<bool>( const bool& Native )
{
	return( BOOLEAN_TO_JSVAL( Native ) );
}

template<> jsval ToJSVal<bool>( bool& Native )
{
	return( BOOLEAN_TO_JSVAL( Native ) );
}

template<> bool ToPrimitive<bool>( JSContext* cx, jsval v, bool& Storage )
{
	try
	{
		Storage = g_ScriptingHost.ValueToBool( v );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		return( false );
	}
	return( true );
}

// CStrW
template<> bool ToPrimitive<CStrW>( JSContext* cx, jsval v, CStrW& Storage )
{
	try
	{
		Storage = g_ScriptingHost.ValueToUCString( v );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		return( false );
	}
	return( true );
}

template<> jsval ToJSVal<CStrW>( const CStrW& Native )
{
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( g_ScriptingHost.GetContext(), Native.c_str() ) ) );
}

template<> jsval ToJSVal<CStrW>( CStrW& Native )
{
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( g_ScriptingHost.GetContext(), Native.c_str() ) ) );
}

// jsval

template<> jsval ToJSVal<jsval>( const jsval& Native )
{
	return( Native );
}

