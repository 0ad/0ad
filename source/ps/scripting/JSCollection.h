// JSCollection.h
//
// A Collection object for JavaScript to hold one specific type of 
// object.

#include "scripting/ScriptingHost.h"
#include "scripting/JSInterface_Entity.h"
#include "simulation/ScriptObject.h"
#include "scripting/JSConversions.h"

#ifndef JS_COLLECTION_INCLUDED
#define JS_COLLECTION_INCLUDED

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
	static JSBool AddProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	static JSBool RemoveProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	static JSBool GetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	static JSBool SetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	static void Finalize( JSContext* cx, JSObject* obj );
	static T* GetNative( jsval m );

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
	{ 0 }
};

template<typename T, JSClass* ScriptType> JSFunctionSpec CJSCollection<T, ScriptType>::JSI_methods[] = 
{
	{ "toString", ToString, 0, 0, 0 },
	{ "subset", Subset, 1, 0, 0 },
	{ "push", Push, 1, 0, 0 },
	{ "pop", Pop, 0, 0, 0 },
	{ "remove", Remove, 1, 0, 0 },
	{ 0 },
};

template<typename T, JSClass* ScriptType> std::vector<T>* CJSCollection<T, ScriptType>::RetrieveSet( JSContext* cx, JSObject* obj )
{
	CJSCollectionData* Info = (CJSCollectionData*)JS_GetPrivate( cx, obj );
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

	if( index != set->size() )
	{
		// If you add something to the collection, it must be at the
		// next empty array element; i.e. set->size()
		JS_ReportError( cx, "Cannot create a property at that subscript" );
		return( JS_FALSE );
	}
		
	T* m = GetNative( *vp );

	if( !m )
		return( JS_TRUE ); // GetNative will report the error if it can't be put in this collection.

	set->resize( index + 1 );
	set->at( index ) = *m;

	return( JS_TRUE );
}


template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::RemoveProperty ( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
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

	*vp = ToJSVal<T>( set->at( index ) );
	
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

	T* m = GetNative( *vp );
	if( !m )
		return( JS_TRUE ); // GetNative will report the error if that can't be put in this collection.
	
	set->at( index ) = *m;
	
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

template<typename T, JSClass* ScriptType> T* CJSCollection<T, ScriptType>::GetNative( jsval m )
{
	T* Native = ToNative<T>( m );
	if( !Native )
		JS_ReportError( g_ScriptingHost.GetContext(), "Only objects of type %s can be stored in this collection.", ScriptType->name );
	return( Native );
}


template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::ToString( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
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

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::GetLength( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	std::vector<T>* set = RetrieveSet( cx, obj );

	if( !set )
		return( JS_FALSE ); // That's odd; we've lost the pointer.

	*vp = INT_TO_JSVAL( set->size() );
	return( JS_TRUE );
}

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::Subset( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	assert( argc > 0 );

	std::vector<T>* Set = RetrieveSet( cx, obj );
	if( !Set )
		return( JS_FALSE );

	CJSCollectionData* CollectionData = new CJSCollectionData();
	CollectionData->m_EngineOwned = false;

	typename std::vector<T>::iterator it;

	CScriptObject Predicate( argv[0] );

	JSObject* Collection = JS_NewObject( g_ScriptingHost.GetContext(), &JSI_class, NULL, NULL );
	JS_SetPrivate( g_ScriptingHost.GetContext(), Collection, CollectionData );

	int i = 0;

	for( it = Set->begin(); it != Set->end(); it++ )
		if( Predicate.Run( ToScript<T>( &( *it ) ) ) )
			CollectionData->m_Data->push_back( *it );

	*rval = OBJECT_TO_JSVAL( Collection );

	return( JS_TRUE );
}

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::Push( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	assert( argc > 0 );

	std::vector<T>* Set = RetrieveSet( cx, obj );
	if( !Set )
		return( JS_FALSE );

	for( int i = 0; i < (int)argc; i++ )
	{
		T* m = GetNative( argv[i] );
		if( m )
			Set->push_back( *m );
	}

	*rval = INT_TO_JSVAL( Set->size() );
	return( JS_TRUE );
}

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::Pop( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
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

template<typename T, JSClass* ScriptType> JSBool CJSCollection<T, ScriptType>::Remove( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	assert( argc > 0 );

	
	std::vector<T>* Set = RetrieveSet( cx, obj );

	if( !Set )
		return( JS_TRUE ); // That's odd; we've lost the pointer.

	int index;
	try
	{
		index = g_ScriptingHost.ValueToInt( argv[0] );
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

#define EntityCollection CJSCollection<HEntity, &CEntity::JSI_class>
	
#endif
