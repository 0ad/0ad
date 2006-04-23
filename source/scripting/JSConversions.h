// A general system of converting between native objects and their JavaScript representations
// Mark Thompson (mark@wildfiregames.com / mot20@cam.ac.uk)

#ifndef JSCONVERSIONS_INCLUDED
#define JSCONVERSIONS_INCLUDED

#include "scripting/ScriptingHost.h"

class CEntity;
class HEntity;
class CBaseEntity;
class CStrW;
class CScriptObject;
class CObjectEntry;
class CVector3D;
class CPlayer;

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

// CBaseEntity
template<> bool ToPrimitive<CBaseEntity*>( JSContext* cx, jsval v, CBaseEntity*& Storage );
template<> JSObject* ToScript<CBaseEntity*>( CBaseEntity** Native );

// CObjectEntry
template<> bool ToPrimitive<CObjectEntry>( JSContext* cx, jsval v, CObjectEntry*& Storage );
template<> jsval ToJSVal<CObjectEntry>( CObjectEntry*& Native );

// HEntity
template<> HEntity* ToNative<HEntity>( JSContext* cx, JSObject* obj );
template<> JSObject* ToScript<HEntity>( HEntity* Native );

// CPlayer*
template<> bool ToPrimitive<CPlayer*>( JSContext* cx, jsval v, CPlayer*& Storage );
template<> JSObject* ToScript<CPlayer*>( CPlayer** Native );

// CScriptObject
template<> bool ToPrimitive<CScriptObject>( JSContext* cx, jsval v, CScriptObject& Storage );
template<> jsval ToJSVal<CScriptObject>( CScriptObject& Native );

// int
template<> bool ToPrimitive<int>( JSContext* cx, jsval v, int& Storage );
template<> jsval ToJSVal<int>( const int& Native );
template<> jsval ToJSVal<int>( int& Native );

// uint
template<> bool ToPrimitive<uint>( JSContext* cx, jsval v, uint& Storage );
template<> jsval ToJSVal<uint>( const uint& Native );
template<> jsval ToJSVal<uint>( uint& Native );

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

// jsval

template<> jsval ToJSVal<jsval>( const jsval& Native );

// Intelligent CStrW->JSVal conversion

jsval JSParseString( const CStrW& String );

/* MT: Maybe:
#define JSCOPY_CAST( For, Use ) \
	template<> bool ToPrimitive<For>( JSContext* cx, jsval v, For& Storage ) \
	{ \
		Use temp; \
		if( !ToPrimitive<Use>( cx, v, temp ) ) \
			return( false ); \
		Storage = (For)temp; \
	} \
	template<> jsval ToJSVal<For>( const For& Native ) \
	{ \
		Use temp = (Use)Native; \
		return( ToJSVal<Use>( temp ) ); \
	} \
	template<> jsval ToJSVal<For>( For& Native ) \
	{ \
		Use temp = (Use)Native; \
		return( ToJSVal<Use>( temp ) ); \
	}
*/

#endif
