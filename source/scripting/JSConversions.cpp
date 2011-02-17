/* Copyright (C) 2010 Wildfire Games.
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
#include "graphics/ObjectManager.h"
#include "maths/scripting/JSInterface_Vector3D.h"
#include "lib/sysdep/sysdep.h"	// isfinite
#include "scriptinterface/ScriptInterface.h"
#include <math.h>
#include <cfloat>

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


template<> jsval ToJSVal<ssize_t>( const ssize_t& Native )
{
	return( INT_TO_JSVAL( (int)Native ) );
}

template<> jsval ToJSVal<ssize_t>( ssize_t& Native )
{
	return( INT_TO_JSVAL( (int)Native ) );
}

template<> bool ToPrimitive<ssize_t>( JSContext* cx, jsval v, ssize_t& Storage )
{
	int temp;
	if(!ToPrimitive<int>(cx, v, temp))
		return false;
	if(temp < 0)
		return false;
	Storage = (ssize_t)temp;
	return true;
}

#endif	// #if ARCH_AMD64
#endif	// #if !GCC_VERSION

// double

template<> jsval ToJSVal<double>( const double& Native )
{
	jsval val = JSVAL_VOID;
	JS_NewNumberValue( g_ScriptingHost.getContext(), Native, &val );
	return val;
}

template<> jsval ToJSVal<double>( double& Native )
{
	jsval val = JSVAL_VOID;
	JS_NewNumberValue( g_ScriptingHost.getContext(), Native, &val );
	return val;
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
	jsval val = JSVAL_VOID;
	JS_NewNumberValue( g_ScriptingHost.getContext(), Native, &val );
	return val;
}

template<> jsval ToJSVal<float>( float& Native )
{
	jsval val = JSVAL_VOID;
	JS_NewNumberValue( g_ScriptingHost.getContext(), Native, &val );
	return val;
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
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( g_ScriptingHost.GetContext(), reinterpret_cast<const jschar*>(Native.utf16().c_str()) ) ) );
}

template<> jsval ToJSVal<CStrW>( CStrW& Native )
{
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( g_ScriptingHost.GetContext(), reinterpret_cast<const jschar*>(Native.utf16().c_str()) ) ) );
}

// CStr/CStr8

template<> bool ToPrimitive<CStr8>( JSContext* cx, jsval v, CStr8& Storage )
{
	std::string str;
	if (!ScriptInterface::FromJSVal(cx, v, str))
		return false;
	Storage = str;
	return true;
}

template<> jsval ToJSVal<CStr8>( const CStr8& Native )
{
	return( STRING_TO_JSVAL( JS_NewStringCopyZ( g_ScriptingHost.GetContext(), Native.c_str() ) ) );
}

template<> jsval ToJSVal<CStr8>( CStr8& Native )
{
	return( STRING_TO_JSVAL( JS_NewStringCopyZ( g_ScriptingHost.GetContext(), Native.c_str() ) ) );
}
