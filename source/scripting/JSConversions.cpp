#include "precompiled.h"
#include "JSConversions.h"
#include "Entity.h"
#include "ObjectManager.h"
#include "scripting/JSInterface_Vector3D.h"
#include "Parser.h"
#include "BaseEntity.h"

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

// CPlayer*
template<> bool ToPrimitive<CPlayer*>( JSContext* cx, jsval v, CPlayer*& Storage )
{
	if( !JSVAL_IS_OBJECT( v ) ) return( false );
	CPlayer* Data = (CPlayer*)JS_GetInstancePrivate( cx, JSVAL_TO_OBJECT( v ), &CPlayer::JSI_class, NULL );
	if( !Data ) return( false );
	Storage = Data;
	return( true );
}

template<> JSObject* ToScript<CPlayer*>( CPlayer** Native )
{
	return( ToScript<CPlayer>( *Native ) );
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

// CObjectEntry

template<> bool ToPrimitive<CObjectEntry>( JSContext* cx, jsval v, CObjectEntry*& Storage )
{
	CStrW ActorName;
	if( !ToPrimitive<CStrW>( cx, v, ActorName ) )
		return( false );
	Storage = g_ObjMan.FindObject( (CStr)ActorName );
	return( true );
}

template<> jsval ToJSVal<CObjectEntry>( CObjectEntry*& Native )
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
	return( OBJECT_TO_JSVAL( Native.GetFunctionObject() ) );
}

template<> bool ToPrimitive<CScriptObject>( JSContext* cx, jsval v, CScriptObject& Storage )
{
	Storage.SetJSVal( v );
	return( true );
}

// int

template<> jsval ToJSVal<int>( const int& Native )
{
	return( INT_TO_JSVAL( Native ) );
}

template<> jsval ToJSVal<int>( int& Native )
{
	return( INT_TO_JSVAL( Native ) );
}

template<> bool ToPrimitive<int>( JSContext* cx, jsval v, int& Storage )
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

// uint

template<> jsval ToJSVal<uint>( const uint& Native )
{
	return( INT_TO_JSVAL( Native ) );
}

template<> jsval ToJSVal<uint>( uint& Native )
{
	return( INT_TO_JSVAL( Native ) );
}

template<> bool ToPrimitive<uint>( JSContext* cx, jsval v, uint& Storage )
{
	try
	{
		int t = g_ScriptingHost.ValueToInt( v );
		if( t < 0 ) return( false );
		Storage = t;
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
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( g_ScriptingHost.GetContext(), Native.utf16().c_str() ) ) );
}

template<> jsval ToJSVal<CStrW>( CStrW& Native )
{
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( g_ScriptingHost.GetContext(), Native.utf16().c_str() ) ) );
}

// jsval

template<> jsval ToJSVal<jsval>( const jsval& Native )
{
	return( Native );
}

// String->JSVal

jsval JSParseString( const CStrW& Native )
{
	CParser stringParser;
	stringParser.InputTaskType( "string", "_$value_" );
	CParserLine result;
	result.ParseString( stringParser, (CStr)Native );
	bool boolResult; int intResult; float floatResult;

	if( result.GetArgFloat( 0, floatResult ) )
	{
		if( floatResult == floor( floatResult ) )
		{
			intResult = (int)floatResult;
			if( INT_FITS_IN_JSVAL( intResult ) )
				return( ToJSVal( intResult ) );
		}
		return( ToJSVal( floatResult ) );
	}
	if( result.GetArgBool( 0, boolResult ) )
		return( BOOLEAN_TO_JSVAL( boolResult ) ); 
	
	return( ToJSVal( Native ) );
}
