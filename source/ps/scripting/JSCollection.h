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

// JSCollection.h
//
// A Collection object for JavaScript to hold one specific type of 
// object.

#include "scripting/ScriptingHost.h"
#include "simulation/ScriptObject.h"
#include "scripting/JSConversions.h"

#ifndef INCLUDED_JSCOLLECTION
#define INCLUDED_JSCOLLECTION

template<typename T, JSClass* ScriptType> class CJSCollection
{
public:
	class CJSCollectionData
	{
	public:
		std::vector<T>* m_Data;
		bool m_EngineOwned;
		CJSCollectionData() { m_Data = new std::vector<T>; m_EngineOwned = false; }
		CJSCollectionData( const std::vector<T>& Copy ) { m_Data = new std::vector<T>( Copy ); m_EngineOwned = false; }
		CJSCollectionData( std::vector<T>* Reference ) { m_Data = Reference; m_EngineOwned = true; }
		~CJSCollectionData() { if( !m_EngineOwned ) delete( m_Data ); }
	};
	static JSClass JSI_class;
private:
	
	static JSPropertySpec JSI_props[];
	static JSFunctionSpec JSI_methods[];

	static JSBool ToString( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	static JSBool Subset( JSContext* cx, JSObject* obj, uintN argc, jsval* agv, jsval* rval );
	static JSBool Push( JSContext* cx, JSObject* obj, uintN argc, jsval* agv, jsval* rval );
	static JSBool Pop( JSContext* cx, JSObject* obj, uintN argc, jsval* agv, jsval* rval );
	static JSBool Remove( JSContext* cx, JSObject* obj, uintN argc, jsval* agv, jsval* rval );
	static JSBool GetLength( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	static JSBool IsEmpty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	static JSBool Clear( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	static JSBool Equals( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	static JSBool AddProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	static JSBool RemoveProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	static JSBool GetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	static JSBool SetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	static void Finalize( JSContext* cx, JSObject* obj );
	static bool GetNative( jsval m, T& Storage );

public:
	static void Init( const char* ClassName );
	static JSObject* Create( const std::vector<T>& Copy );
	static JSObject* CreateReference( std::vector<T>* Reference );
	static std::vector<T>* RetrieveSet( JSContext* cx, JSObject* obj );
};

template<typename T, JSClass* ScriptType> JSClass CJSCollection<T, ScriptType>::JSI_class =
{
	0, JSCLASS_HAS_PRIVATE,
	AddProperty, RemoveProperty,
	GetProperty, SetProperty,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, Finalize,
	NULL, NULL, NULL, NULL
};

template<typename T, JSClass* ScriptType> JSPropertySpec CJSCollection<T, ScriptType>::JSI_props[] =
{
	{ "length", 0, JSPROP_PERMANENT | JSPROP_READONLY, GetLength },
	{ "empty", 0, JSPROP_PERMANENT | JSPROP_READONLY, IsEmpty },
	{ 0 }
};

template<typename T, JSClass* ScriptType> JSFunctionSpec CJSCollection<T, ScriptType>::JSI_methods[] = 
{
	{ "toString", ToString, 0, 0, 0 },
	{ "subset", Subset, 1, 0, 0 },
	{ "push", Push, 1, 0, 0 },
	{ "pop", Pop, 0, 0, 0 },
	{ "remove", Remove, 1, 0, 0 },
	{ "clear", Clear, 0, 0, 0 },
	{ "equals", Equals, 1, 0, 0 },
	{ 0 },
};

template<typename T, JSClass* ScriptType> std::vector<T>* CJSCollection<T, ScriptType>::RetrieveSet( JSContext* cx, JSObject* obj )
{
	CJSCollectionData* Info = (CJSCollectionData*)JS_GetInstancePrivate( cx, obj, &JSI_class, NULL );
	if( !Info ) return( NULL );
	return( Info->m_Data );
}

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::AddProperty ( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	if( !JSVAL_IS_INT( id ) )
		return( JS_TRUE ); // Accessing a named property; nothing interesting.
	 
	int index = JSVAL_TO_INT( id ); 

	std::vector<T>* set = RetrieveSet( cx, obj );

	if( !set )
		return( JS_TRUE ); // That's odd; we've lost the pointer.

	if( ( index >= 0 ) && ( index < (int)set->size() ) )
		return( JS_TRUE );

	if( index != (int)set->size() )
	{
		// If you add something to the collection, it must be at the
		// next empty array element; i.e. set->size()
		JS_ReportError( cx, "Cannot create a property at that subscript" );
		return( JS_FALSE );
	}
	
	T m;

	if( !GetNative( *vp, m ) ) 
		return( JS_TRUE );

	set->resize( index + 1 );
	set->at( index ) = m;

	return( JS_TRUE );
}


template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::RemoveProperty (
	JSContext* cx, JSObject* obj, jsval id, jsval* UNUSED(vp) )
{
	if( !JSVAL_IS_INT( id ) )
		return( JS_TRUE ); // Accessing a named property; nothing interesting.
	 
	int index = JSVAL_TO_INT( id ); 

	std::vector<T>* set = RetrieveSet( cx, obj );

	if( !set )
		return( JS_TRUE ); // That's odd; we've lost the pointer.

	if( ( index < 0 ) || ( index >= (int)set->size() ) )
	{
		JS_ReportError( cx, "Index out of bounds." );
		return( JS_TRUE );
	}

	set->erase( set->begin() + index );

	return( JS_TRUE );
}

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::GetProperty ( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	if( !JSVAL_IS_INT( id ) )
		return( JS_TRUE ); // Accessing a named property; nothing interesting.
	 
	int index = JSVAL_TO_INT( id ); 
	std::vector<T>* set = RetrieveSet( cx, obj );

	if( !set )
		return( JS_FALSE ); // That's odd; we've lost the pointer.

	if( ( index < 0 ) || ( index >= (int)set->size() ) )
	{
		JS_ReportError( cx, "Index out of bounds." );
		return( JS_TRUE );
	}

	*vp = ToJSVal( set->at( index ) );
	
	return( JS_TRUE );
}

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::SetProperty ( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	if( !JSVAL_IS_INT( id ) )
		return( JS_TRUE ); // Accessing a named property; nothing interesting.
	 
	int index = JSVAL_TO_INT( id ); 

	std::vector<T>* set = RetrieveSet( cx, obj );

	if( !set )
		return( JS_FALSE ); // That's odd; we've lost the pointer.

	if( ( index < 0 ) || ( index >= (int)set->size() ) )
	{
		JS_ReportError( cx, "Index out of bounds." );
		return( JS_TRUE );
	}

	T m;

	if( !GetNative( *vp, m ) ) 
		return( JS_TRUE );

	set->at( index ) = m;
	
	return( JS_TRUE );
}

template<typename T, JSClass* ScriptType> void CJSCollection<T, ScriptType>::Finalize ( JSContext* cx, JSObject* obj )
{
	CJSCollectionData* info = (CJSCollectionData*)JS_GetPrivate( cx, obj );
	if( info )
		delete( info );
}

template<typename T, JSClass* ScriptType> JSObject* CJSCollection<T, ScriptType>::Create( const std::vector<T>& Copy )
{	
	JSObject* Collection = JS_NewObject( g_ScriptingHost.GetContext(), &JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.GetContext(), Collection, new CJSCollectionData( Copy ) );

	return( Collection );
}

template<typename T, JSClass* ScriptType> JSObject* CJSCollection<T, ScriptType>::CreateReference( std::vector<T>* Reference )
{
	JSObject* Collection = JS_NewObject( g_ScriptingHost.GetContext(), &JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.GetContext(), Collection, new CJSCollectionData( Reference ) );

	return( Collection );
}

template<typename T, JSClass* ScriptType> void CJSCollection<T, ScriptType>::Init( const char* ClassName )
{
	JSI_class.name = ClassName;
	g_ScriptingHost.DefineCustomObjectType( &JSI_class, NULL, 0, JSI_props, JSI_methods, NULL, NULL );	
}

template<typename T, JSClass* ScriptType> bool CJSCollection<T, ScriptType>::GetNative( jsval m, T& Storage )
{
	if( ToPrimitive( g_ScriptingHost.GetContext(), m, Storage ) )
		return( true );

	JS_ReportError( g_ScriptingHost.GetContext(), "Only objects of type %s can be stored in this collection.", ScriptType->name );
	return( false );
}


template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::ToString(
	JSContext* cx, JSObject* obj, uintN UNUSED(argc), jsval* UNUSED(argv), jsval* rval )
{
	std::vector<T>* set = RetrieveSet( cx, obj );

	if( !set )
		return( JS_FALSE ); // That's odd; we've lost the pointer.

	wchar_t buffer[256];
	int len=swprintf( buffer, 256, L"[object Collection: %hs: %d members]", ScriptType->name, set->size() );
	buffer[255] = 0;
	if (len < 0 || len > 255) len=255;
	utf16string u16str(buffer, buffer+len);
	*rval = STRING_TO_JSVAL( JS_NewUCStringCopyZ( cx, u16str.c_str() ) );
	return( JS_TRUE );
}

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::GetLength(
	JSContext* cx, JSObject* obj, jsval UNUSED(id), jsval* vp )
{
	std::vector<T>* set = RetrieveSet( cx, obj );

	if( !set )
		return( JS_FALSE ); // That's odd; we've lost the pointer.

	*vp = INT_TO_JSVAL( set->size() );
	return( JS_TRUE );
}

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::IsEmpty(
	JSContext* cx, JSObject* obj, jsval UNUSED(id), jsval* vp )
{
	std::vector<T>* set = RetrieveSet( cx, obj );

	if( !set )
		return( JS_FALSE ); // That's odd; we've lost the pointer.

	*vp = BOOLEAN_TO_JSVAL( set->empty() );
	return( JS_TRUE );
}

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::Equals( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	std::vector<T>* a = RetrieveSet( cx, obj );
	if( !a )
		return( JS_FALSE );
	if( ( argc == 0 ) || ( !JSVAL_IS_OBJECT( argv[0] ) ) ) return( JS_FALSE );
	std::vector<T>* b = RetrieveSet( cx, JSVAL_TO_OBJECT( argv[0] ) );
	if( !b )
		return( JS_FALSE );
	typename std::vector<T>::iterator ita, itb;
	
	size_t seek = a->size();
	for( ita = a->begin(); ita != a->end(); ita++ )
		for( itb = b->begin(); itb != b->end(); itb++ )
			if( *ita == *itb ) { seek--; break; }
		
	if( seek )
	{
		*rval = JSVAL_FALSE;
		return( JS_TRUE );
	}

	seek = b->size();
	for( itb = b->begin(); itb != b->end(); itb++ )
		for( ita = a->begin(); ita != a->end(); ita++ )
			if( *ita == *itb ) { seek--; break; }
		
	if( seek )
	{
		*rval = JSVAL_FALSE;
		return( JS_TRUE );
	}

	*rval = JSVAL_TRUE;
	return( JS_TRUE );
}

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::Subset(
	JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	debug_assert( argc > 0 );

	std::vector<T>* Set = RetrieveSet( cx, obj );
	if( !Set )
		return( JS_FALSE );

	CJSCollectionData* CollectionData = new CJSCollectionData();
	CollectionData->m_EngineOwned = false;

	typename std::vector<T>::iterator it;

	CScriptObject Predicate( argv[0] );

	JSObject* Collection = JS_NewObject( g_ScriptingHost.GetContext(), &JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.GetContext(), Collection, CollectionData );

	for( it = Set->begin(); it != Set->end(); it++ )
		if( Predicate.Run( ToScript( (T*)&( *it ) ) ) )
			CollectionData->m_Data->push_back( *it );

	*rval = OBJECT_TO_JSVAL( Collection );

	return( JS_TRUE );
}

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::Clear(
	JSContext* cx, JSObject* obj, uintN UNUSED(argc), jsval* UNUSED(argv), jsval* rval )
{
	std::vector<T>* Set = RetrieveSet( cx, obj );
	if( !Set )
		return( JS_FALSE );

	Set->clear();

	*rval = JSVAL_TRUE;

	return( JS_TRUE );
}

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::Push( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	debug_assert( argc > 0 );

	std::vector<T>* Set = RetrieveSet( cx, obj );
	if( !Set )
		return( JS_FALSE );

	for( int i = 0; i < (int)argc; i++ )
	{
		T m;
		if( GetNative( argv[i], m ) )
			Set->push_back( m );
	}

	*rval = INT_TO_JSVAL( Set->size() );
	return( JS_TRUE );
}

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::Pop(
	JSContext* cx, JSObject* obj, uintN UNUSED(argc), jsval* UNUSED(argv), jsval* rval )
{
	std::vector<T>* Set = RetrieveSet( cx, obj );
	if( !Set )
		return( JS_FALSE );

	if( !Set->size() )
	{
		JS_ReportError( cx, "Can't pop members from an empty Collection." );
		return( JS_TRUE );
	}

	*rval = ToJSVal( Set->back() );
	Set->pop_back();

	return( JS_TRUE );
}

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::Remove(
	JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* UNUSED(rval) )
{
	debug_assert( argc > 0 );

	
	std::vector<T>* Set = RetrieveSet( cx, obj );

	if( !Set )
		return( JS_TRUE ); // That's odd; we've lost the pointer.

	int index;
	try
	{
		index = ToPrimitive<int>( argv[0] );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Index must be an integer." );
		return( JS_TRUE );
	}

	if( ( index < 0 ) || ( index >= (int)Set->size() ) )
	{
		JS_ReportError( cx, "Index out of bounds." );
		return( JS_TRUE );
	}

	Set->erase( Set->begin() + index );

	return( JS_TRUE );
}

#endif
