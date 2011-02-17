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

// A general system of converting between native objects and their JavaScript representations

#ifndef INCLUDED_JSCONVERSIONS
#define INCLUDED_JSCONVERSIONS

#include "scripting/ScriptingHost.h"

class CStrW;
class CScriptObject;
class CObjectEntry;
class CVector3D;

// -----
//
// Defaults
//
// -----

template<typename T> T* ToNative( JSContext* cx, JSObject* obj )
{
	return( (T*)JS_GetInstancePrivate( cx, obj, &T::JSI_class, NULL ) );
}

template<typename T> JSObject* ToScript( T* Native )
{
	if( !Native )
		return( (JSObject*)NULL );
	return( Native->GetScript() );
}

template<typename T> T* ToNative( jsval v )
{
	if( !JSVAL_IS_OBJECT( v ) ) return( NULL );
	if( v == JSVAL_NULL ) return( NULL );
	return( ToNative<T>( g_ScriptingHost.GetContext(), JSVAL_TO_OBJECT( v ) ) );
}

template<typename T> bool ToPrimitive( JSContext* UNUSED(cx), jsval v, T& Storage )
{
	T* Native = ToNative<T>( v );
	if( !Native ) return( false );
	Storage = *Native;
	return( true );
}

// Handle pointer-to-objects sensibly (by automatically dereferencing them one level)
template<typename T> bool ToPrimitive( JSContext* UNUSED(cx), jsval v, T*& Storage )
{
	T* Native = ToNative<T>( v );
	if( !Native ) return( false );
	Storage = Native;
	return( true );
}

// Throws PSERROR_Scripting_ConversionFailed on failure.
template<typename T> inline T ToPrimitive( JSContext* cx, jsval v )
{
	T Temp;
	bool ok = ToPrimitive( cx, v, Temp );
	if( !ok ) throw PSERROR_Scripting_ConversionFailed();
	return( Temp );
}

// Throws PSERROR_Scripting_ConversionFailed on failure.
template<typename T> inline T ToPrimitive( jsval v )
{
	return( ToPrimitive<T>( g_ScriptingHost.GetContext(), v ) );
}

template<typename T> jsval ToJSVal( T& Native )
{
	return( OBJECT_TO_JSVAL( ToScript<T>( &Native ) ) );
}

template<typename T> jsval ToJSVal( T*& Native )
{
	return( OBJECT_TO_JSVAL( ToScript<T>( Native ) ) );
}

template<typename T> jsval ToJSVal( const T& Native );

// -----
//
// Overrides
//
// -----

// CVector3D
template<> CVector3D* ToNative<CVector3D>( JSContext* cx, JSObject* obj );
template<> JSObject* ToScript<CVector3D>( CVector3D* Native );
template<> jsval ToJSVal<CVector3D>( const CVector3D& Native );

// CObjectEntry
template<> bool ToPrimitive<CObjectEntry>( JSContext* cx, jsval v, CObjectEntry*& Storage );
template<> jsval ToJSVal<CObjectEntry>( CObjectEntry*& Native );

// CScriptObject
template<> bool ToPrimitive<CScriptObject>( JSContext* cx, jsval v, CScriptObject& Storage );
template<> jsval ToJSVal<CScriptObject>( CScriptObject& Native );

// int
template<> bool ToPrimitive<int>( JSContext* cx, jsval v, int& Storage );
template<> jsval ToJSVal<int>( const int& Native );
template<> jsval ToJSVal<int>( int& Native );

// unsigned
template<> bool ToPrimitive<unsigned>( JSContext* cx, jsval v, unsigned& Storage );
template<> jsval ToJSVal<unsigned>( const unsigned& Native );
template<> jsval ToJSVal<unsigned>( unsigned& Native );

// long int
template<> bool ToPrimitive<long>( JSContext* cx, jsval v, long& Storage );
template<> jsval ToJSVal<long>( const long& Native );
template<> jsval ToJSVal<long>( long& Native );

// unsigned long int
template<> bool ToPrimitive<unsigned long>( JSContext* cx, jsval v, unsigned long& Storage );
template<> jsval ToJSVal<unsigned long>( const unsigned long& Native );
template<> jsval ToJSVal<unsigned long>( unsigned long& Native );

// (s)size_t are considered to be identical to (unsigned) int by GCC and
// their specializations would cause conflicts there. On x86_64 GCC, s/size_t
// is equivalent to (unsigned) long, but the same solution applies; use the
// long and unsigned long specializations instead of s/size_t.
#if !GCC_VERSION

// for some reason, x64 MSC treats size_t as distinct from unsigned long:
#if ARCH_AMD64

// size_t
template<> bool ToPrimitive<size_t>( JSContext* cx, jsval v, size_t& Storage );
template<> jsval ToJSVal<size_t>( const size_t& Native );
template<> jsval ToJSVal<size_t>( size_t& Native );

// ssize_t
template<> bool ToPrimitive<ssize_t>( JSContext* cx, jsval v, ssize_t& Storage );
template<> jsval ToJSVal<ssize_t>( const ssize_t& Native );
template<> jsval ToJSVal<ssize_t>( ssize_t& Native );

#endif

#endif

// double
template<> bool ToPrimitive<double>( JSContext* cx, jsval v, double& Storage );
template<> jsval ToJSVal<double>( const double& Native );
template<> jsval ToJSVal<double>( double& Native );

// float
template<> bool ToPrimitive<float>( JSContext* cx, jsval v, float& Storage );
template<> jsval ToJSVal<float>( const float& Native );
template<> jsval ToJSVal<float>( float& Native );

// bool
template<> bool ToPrimitive<bool>( JSContext* cx, jsval v, bool& Storage );
template<> jsval ToJSVal<bool>( const bool& Native );
template<> jsval ToJSVal<bool>( bool& Native );

/*
// char*
template<> bool ToPrimitive<char*>( JSContext* cx, jsval v, char*& Storage );
template<> jsval ToJSVal<char*>( const char* Native );
template<> jsval ToJSVal<char*>( char* Native );
*/

// CStrW
template<> bool ToPrimitive<CStrW>( JSContext* cx, jsval v, CStrW& Storage );
template<> jsval ToJSVal<CStrW>( const CStrW& Native );
template<> jsval ToJSVal<CStrW>( CStrW& Native );

// CStr(8)
template<> bool ToPrimitive<CStr8>( JSContext* cx, jsval v, CStr8& Storage );
template<> jsval ToJSVal<CStr8>( const CStr8& Native );
template<> jsval ToJSVal<CStr8>( CStr8& Native );

#endif
