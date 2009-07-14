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

#include "JSConversions.h"
#include "simulation/Entity.h"
#include "graphics/ObjectManager.h"
#include "maths/scripting/JSInterface_Vector3D.h"
#include "ps/Parser.h"
#include "ps/Player.h"
#include "simulation/EntityTemplate.h"
#include "lib/sysdep/sysdep.h"	// isfinite
#include <math.h>
#include <cfloat>
#include "scripting/ScriptableComplex.inl"

// HEntity

template<> HEntity* ToNative<HEntity>( JSContext* cx, JSObject* obj )
{
	CEntity* e = ToNative<CEntity>( cx, obj );
	return( e ? &( e->me ) : NULL );
}

template<> JSObject* ToScript<HEntity>( HEntity* Native )
{
	return( ToScript<CEntity>( &( **Native ) ) );
}

// CPlayer*
template<> bool ToPrimitive<CPlayer*>( JSContext* cx, jsval v, CPlayer*& Storage )
{
	if( !JSVAL_IS_OBJECT( v ) || ( v == JSVAL_NULL ) ) return( false );
	CPlayer* Data = (CPlayer*)JS_GetInstancePrivate( cx, JSVAL_TO_OBJECT( v ), &CPlayer::JSI_class, NULL );
	if( !Data ) return( false );
	Storage = Data;
	return( true );
}

template<> JSObject* ToScript<CPlayer*>( CPlayer** Native )
{
	return( ToScript<CPlayer>( *Native ) );
}

// CEntityTemplate*

template<> bool ToPrimitive<CEntityTemplate*>( JSContext* cx, jsval v, CEntityTemplate*& Storage )
{
	if( !JSVAL_IS_OBJECT( v ) || ( v == JSVAL_NULL ) ) return( false );
	CEntityTemplate* Data = (CEntityTemplate*)JS_GetInstancePrivate( cx, JSVAL_TO_OBJECT( v ), &CEntityTemplate::JSI_class, NULL );
	if( !Data ) return( false );
	Storage = Data;
	return( true );
}

template<> JSObject* ToScript<CEntityTemplate*>( CEntityTemplate** Native )
{
	return( ToScript<CEntityTemplate>( *Native ) );
}

// CVector3D

template<> CVector3D* ToNative<CVector3D>( JSContext* cx, JSObject* obj )
{
	JSI_Vector3D::Vector3D_Info* v = (JSI_Vector3D::Vector3D_Info*)JS_GetInstancePrivate( cx, obj, &JSI_Vector3D::JSI_class, NULL );
	return( v ? v->vector : NULL );
}

template<> JSObject* ToScript<CVector3D>( CVector3D* Native )
{
	JSObject* Script = JS_NewObject( g_ScriptingHost.GetContext(), &JSI_Vector3D::JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.GetContext(), Script, new JSI_Vector3D::Vector3D_Info( *Native ) );
	return( Script );
}

template<> jsval ToJSVal<CVector3D>( const CVector3D& Native )
{
	JSObject* Script = JS_NewObject( g_ScriptingHost.GetContext(), &JSI_Vector3D::JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.GetContext(), Script, new JSI_Vector3D::Vector3D_Info( Native ) );
	return( OBJECT_TO_JSVAL( Script ) );
}

// CScriptObject

template<> jsval ToJSVal<CScriptObject>( CScriptObject& Native )
{
	return( OBJECT_TO_JSVAL( Native.GetFunctionObject() ) );
}

template<> bool ToPrimitive<CScriptObject>( JSContext* UNUSED(cx), jsval v, CScriptObject& Storage )
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
	JSBool ok = JS_ValueToInt32(cx, v, (int32*)&Storage);
	return ok == JS_TRUE;
}

// unsigned

template<> jsval ToJSVal<unsigned>( const unsigned& Native )
{
	return( INT_TO_JSVAL( Native ) );
}

template<> jsval ToJSVal<unsigned>( unsigned& Native )
{
	return( INT_TO_JSVAL( Native ) );
}

template<> bool ToPrimitive<unsigned>( JSContext* cx, jsval v, unsigned& Storage )
{
	int temp;
	if(!ToPrimitive<int>(cx, v, temp))
		return false;
	if(temp < 0)
		return false;
	Storage = (unsigned)temp;
	return true;
}

// long
template<> jsval ToJSVal<long>( const long& Native )
{
	return( INT_TO_JSVAL( (int)Native ) );
}

template<> jsval ToJSVal<long>( long& Native )
{
	return( INT_TO_JSVAL( (int)Native ) );
}

template<> bool ToPrimitive<long>( JSContext* cx, jsval v, long& Storage )
{
	int32 tmp;
	JSBool ok = JS_ValueToInt32(cx, v, &tmp);
	Storage = (long)tmp;
	return ok == JS_TRUE;
}

// unsigned long
template<> jsval ToJSVal<unsigned long>( const unsigned long& Native )
{
	return( INT_TO_JSVAL( (int)Native ) );
}

template<> jsval ToJSVal<unsigned long>( unsigned long& Native )
{
	return( INT_TO_JSVAL( (int)Native ) );
}

template<> bool ToPrimitive<unsigned long>( JSContext* cx, jsval v, unsigned long& Storage )
{
	int32 tmp;
	JSBool ok = JS_ValueToInt32(cx, v, &tmp);
	Storage = (unsigned long)tmp;
	return ok == JS_TRUE;
}

// see comment at declaration of specialization
#if !GCC_VERSION
#if ARCH_AMD64

template<> jsval ToJSVal<size_t>( const size_t& Native )
{
	return( INT_TO_JSVAL( (int)Native ) );
}

template<> jsval ToJSVal<size_t>( size_t& Native )
{
	return( INT_TO_JSVAL( (int)Native ) );
}

template<> bool ToPrimitive<size_t>( JSContext* cx, jsval v, size_t& Storage )
{
	int temp;
	if(!ToPrimitive<int>(cx, v, temp))
		return false;
	if(temp < 0)
		return false;
	Storage = (size_t)temp;
	return true;
}

#endif	// #if ARCH_AMD64
#endif	// #if !GCC_VERSION

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
	JSBool ok = JS_ValueToNumber(cx, v, &Storage);
	if (ok == JS_FALSE || !isfinite( Storage ) )
		return false;
	return true;
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
	double temp;
	if(!ToPrimitive<double>(cx, v, temp))
		return false;
	Storage = (float)temp;
	return true;
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
	JSBool temp;
	JSBool ok = JS_ValueToBoolean(cx, v, &temp);
	if(ok == JS_FALSE)
		return false;
	Storage = (temp == JS_TRUE);
	return true;
}

// CStrW
template<> bool ToPrimitive<CStrW>( JSContext* UNUSED(cx), jsval v, CStrW& Storage )
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

// CStr/CStr8

template<> bool ToPrimitive<CStr8>( JSContext* UNUSED(cx), jsval v, CStr8& Storage )
{
	try
	{
		Storage = g_ScriptingHost.ValueToString( v );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		return( false );
	}
	return( true );
}

template<> jsval ToJSVal<CStr8>( const CStr8& Native )
{
	return( STRING_TO_JSVAL( JS_NewStringCopyZ( g_ScriptingHost.GetContext(), Native.c_str() ) ) );
}

template<> jsval ToJSVal<CStr8>( CStr8& Native )
{
	return( STRING_TO_JSVAL( JS_NewStringCopyZ( g_ScriptingHost.GetContext(), Native.c_str() ) ) );
}

// jsval

template<> jsval ToJSVal<jsval_t>( const jsval_t& Native )
{
	return( Native.v );
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
