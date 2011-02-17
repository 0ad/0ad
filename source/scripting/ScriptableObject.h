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

// ScriptableObject.h
//
// A quick way to add (mostly) sensibly-behaving JavaScript interfaces to native classes.
//

#ifndef INCLUDED_SCRIPTABLEOBJECT
#define INCLUDED_SCRIPTABLEOBJECT

#include "scripting/ScriptingHost.h"
#include "JSConversions.h"

#include "lib/sysdep/stl.h"

#define ALLOW_NONSHARED_NATIVES

class IJSObject;

class IJSProperty
{
public:
	virtual ~IJSProperty() {};
	virtual jsval Get( JSContext* cx, IJSObject* obj ) = 0;
	virtual void Set( JSContext* cx, IJSObject* obj, jsval value ) = 0;
};

class IJSObject
{
public:
	typedef STL_HASH_MAP<CStrW, IJSProperty*, CStrW_hash_compare> PropertyTable;

	// Property getters and setters
	typedef jsval (IJSObject::*GetFn)( JSContext* cx );
	typedef void (IJSObject::*SetFn)( JSContext* cx, jsval value );

	// Return a pointer to a property, if it exists
	virtual IJSProperty* HasProperty( const CStrW& PropertyName ) = 0;

	// Retrieve the value of a property (returning false if that property is not defined)
	virtual bool GetProperty( JSContext* cx, const CStrW& PropertyName, jsval* vp ) = 0;
	
	// Add a property (with immediate value)
	virtual void AddProperty( const CStrW& PropertyName, jsval Value ) = 0;
	
	inline IJSObject() {}
	virtual ~IJSObject() {}
};

template<typename T, bool ReadOnly = false> class CJSObject;

template<typename T, bool ReadOnly> class CJSProperty : public IJSProperty
{
	T IJSObject::*m_Data;

public:
	CJSProperty( T IJSObject::*Data )
	{
		m_Data = Data;
	}
	jsval Get( JSContext* UNUSED(cx), IJSObject* owner )
	{
		return( ToJSVal( owner->*m_Data ) );
	}
	void Set( JSContext* cx, IJSObject* owner, jsval Value )
	{
		if( !ReadOnly )
			ToPrimitive( cx, Value, owner->*m_Data );
	}
};

#ifdef ALLOW_NONSHARED_NATIVES

template<typename T, bool ReadOnly> class CJSNonsharedProperty : public IJSProperty
{
	T* m_Data;

public:
	CJSNonsharedProperty( T* Data )
	{
		m_Data = Data;
	}
	jsval Get( JSContext* UNUSED(cx), IJSObject* UNUSED(owner) )
	{
		return( ToJSVal( *m_Data ) );
	}
	void Set( JSContext* cx, IJSObject* UNUSED(owner), jsval Value )
	{
		if( !ReadOnly )
			ToPrimitive( cx, Value, *m_Data );
	}
};

#endif /* ALLOW_NONSHARED_NATIVES */

class CJSFunctionProperty : public IJSProperty
{
	// Function on Owner to get the value
	IJSObject::GetFn m_Getter;
	
	// Function on Owner to set the value
	IJSObject::SetFn m_Setter;

public:
	CJSFunctionProperty( IJSObject::GetFn Getter, IJSObject::SetFn Setter )
	{
		m_Getter = Getter;
		m_Setter = Setter;
		// Must at least be able to read 
		debug_assert( m_Getter );
	}
	jsval Get( JSContext* cx, IJSObject* obj )
	{
		return( (obj->*m_Getter)(cx) );
	}
	void Set( JSContext* cx, IJSObject* obj, jsval value )
	{
		if( m_Setter )
			(obj->*m_Setter)( cx, value );
	}
};

class CJSValProperty : public IJSProperty
{
	template<typename Q, bool ReadOnly> friend class CJSObject;

	jsval m_Data;

public:
	CJSValProperty( jsval Data )
	{
		m_Data = Data;
		Root();
	}
	~CJSValProperty()
	{
		Uproot();
	}
	void Root()
	{
		if( JSVAL_IS_GCTHING( m_Data ) )
			JS_AddNamedValueRoot( g_ScriptingHost.GetContext(), &m_Data, "ScriptableObjectProperty" );
	}
	void Uproot()
	{
		if( JSVAL_IS_GCTHING( m_Data ))
			JS_RemoveValueRoot( g_ScriptingHost.GetContext(), &m_Data );
	}
	jsval Get( JSContext* UNUSED(cx), IJSObject* UNUSED(object))
	{
		return( m_Data );
	}
	void Set( JSContext* UNUSED(cx), IJSObject* UNUSED(owner), jsval value )
	{
		Uproot();
		m_Data = value;
		Root();
	}
};

// Wrapper around native functions that are attached to CJSObjects

template<typename T, bool ReadOnly, typename RType, RType (T::*NativeFunction)( JSContext* cx, uintN argc, jsval* argv )> class CNativeFunction
{
public:
	static JSBool JSFunction( JSContext* cx, uintN argc, jsval* vp )
	{
		T* Native = ToNative<T>( cx, JS_THIS_OBJECT( cx, vp ) );
		if( !Native )
			return( JS_FALSE );

		jsval rval = ToJSVal<RType>( (Native->*NativeFunction)( cx, argc, JS_ARGV(cx, vp) ) );
		JS_SET_RVAL( cx, vp, rval );
		return( JS_TRUE );
	}
};

// Special case for void functions
template<typename T, bool ReadOnly, void (T::*NativeFunction)( JSContext* cx, uintN argc, jsval* argv )>
class CNativeFunction<T, ReadOnly, void, NativeFunction>
{
public:
	static JSBool JSFunction( JSContext* cx, uintN argc, jsval* vp )
	{
		T* Native = ToNative<T>( cx, JS_THIS_OBJECT( cx, vp ) );
		if( !Native )
			return( JS_FALSE );

		(Native->*NativeFunction)( cx, argc, JS_ARGV(cx, vp) );

		JS_SET_RVAL( cx, vp, JSVAL_VOID );
		return( JS_TRUE );
	}
};

template<typename T, bool ReadOnly> class CJSObject : public IJSObject
{
	// This object
	JSObject* m_JS;

protected:
	// The properties defined by the engine
	static PropertyTable m_NativeProperties;
#ifdef ALLOW_NONSHARED_NATIVES
	PropertyTable m_NonsharedProperties;
#endif
	// Properties added by script
	PropertyTable m_ScriptProperties;

	// Whether native code is responsible for managing this object.
	// Script constructors should clear this *BEFORE* creating a JS
	// mirror (otherwise it'll be rooted).

	bool m_EngineOwned;

public:
	// Property getters and setters
	typedef jsval (T::*TGetFn)( JSContext* );
	typedef void (T::*TSetFn)( JSContext*, jsval value );

	static JSClass JSI_class;

	static void ScriptingInit( const char* ClassName, JSNative Constructor = NULL, uintN ConstructorMinArgs = 0 )
	{
		JSFunctionSpec* JSI_methods = new JSFunctionSpec[ m_Methods.size() + 1 ];
		size_t MethodID;
		for( MethodID = 0; MethodID < m_Methods.size(); MethodID++ )
			JSI_methods[MethodID] = m_Methods[MethodID];

		JSI_methods[MethodID].name = 0;

		JSI_class.name = ClassName;
		g_ScriptingHost.DefineCustomObjectType( &JSI_class, Constructor, ConstructorMinArgs, JSI_props, JSI_methods, NULL, NULL );

		delete[]( JSI_methods );

		atexit( ScriptingShutdown );
	}
	static void ScriptingShutdown()
	{
		PropertyTable::iterator it;
		for( it = m_NativeProperties.begin(); it != m_NativeProperties.end(); it++ )
			delete( it->second );
		m_NativeProperties.clear();
	}

	// JS Property access
	bool GetProperty( JSContext* cx, const CStrW& PropertyName, jsval* vp )
	{
		IJSProperty* Property = HasProperty( PropertyName );
		if( Property )
			*vp = Property->Get( cx, this );

		return( true );
	}
	void SetProperty( JSContext* cx, const CStrW& PropertyName, jsval* vp )
	{
		if( !ReadOnly )
		{
			IJSProperty* prop = HasProperty( PropertyName );
			
			if( prop )
			{
				// Already exists
				prop->Set( cx, this, *vp );
			}
			else
			{
				// Need to add it
				AddProperty( PropertyName, *vp );
			}
		}
	}

	IJSProperty* HasProperty( const CStrW& PropertyName )
	{
		PropertyTable::iterator it;

		// Engine-defined properties take precedence
		it = m_NativeProperties.find( PropertyName );
		if( it != m_NativeProperties.end() )
			return( it->second );
		// Next are the object-local engine-defined properties
		// (if they're compiled in)
#ifdef ALLOW_NONSHARED_NATIVES
		it = m_NonsharedProperties.find( PropertyName );
		if( it != m_NonsharedProperties.end() )
			return( it->second );
#endif
		// Then check in script properties
		it = m_ScriptProperties.find( PropertyName );
		if( it != m_ScriptProperties.end() )
			return( it->second );

		// Otherwise not found
		return( NULL );
	}

	void AddProperty( const CStrW& PropertyName, jsval Value )
	{
		debug_assert( !HasProperty( PropertyName ) );
		CJSValProperty* newProp = new CJSValProperty( Value ); 
		m_ScriptProperties[PropertyName] = newProp;
	}
	static void AddProperty( const CStrW& PropertyName, TGetFn Getter, TSetFn Setter = NULL )
	{
		m_NativeProperties[PropertyName] = new CJSFunctionProperty( (GetFn)Getter, (SetFn)Setter );
	}
	template<typename ReturnType, ReturnType (T::*NativeFunction)( JSContext* cx, uintN argc, jsval* argv )> 
		static void AddMethod( const char* Name, uintN MinArgs )
	{
		JSFunctionSpec FnInfo = { Name, CNativeFunction<T, ReadOnly, ReturnType, NativeFunction>::JSFunction, (uint8)MinArgs, 0 };
		m_Methods.push_back( FnInfo );
	}
	template<typename PropType> static void AddProperty( const CStrW& PropertyName, PropType T::*Native, bool PropReadOnly = ReadOnly )
	{
		IJSProperty* prop;
		if( PropReadOnly )
		{
			prop = new CJSProperty<PropType, true>( (PropType IJSObject::*)Native );
		}
		else
		{
			prop = new CJSProperty<PropType, ReadOnly>( (PropType IJSObject::*)Native );
		}
		m_NativeProperties[PropertyName] = prop;
	}
#ifdef ALLOW_NONSHARED_NATIVES
	template<typename PropType> void AddLocalProperty( const CStrW& PropertyName, PropType* Native, bool PropReadOnly = ReadOnly )
	{
		IJSProperty* prop;
		if( PropReadOnly )
		{
			prop = new CJSNonsharedProperty<PropType, true>( Native );
		}
		else
			prop = new CJSNonsharedProperty<PropType, ReadOnly>( Native );
		m_NonsharedProperties[PropertyName] = prop;
	}
#endif

	// Object operations
	JSObject* GetScript() 
	{
		if( !m_JS )
			CreateScriptObject();
		return( m_JS );
	}
	// Creating and releasing script objects is done automatically most of the time, but you
	// can do it explicitly. 
	void CreateScriptObject()
	{
		if( !m_JS )
		{
			m_JS = JS_NewObject( g_ScriptingHost.GetContext(), &JSI_class, NULL, NULL );
			if( m_EngineOwned )
				JS_AddNamedObjectRoot( g_ScriptingHost.GetContext(), &m_JS, JSI_class.name );
	
			JS_SetPrivate( g_ScriptingHost.GetContext(), m_JS, (T*)this );
		}
	}
	void ReleaseScriptObject()
	{
		if( m_JS )
		{
			JS_SetPrivate( g_ScriptingHost.GetContext(), m_JS, NULL );
			if( m_EngineOwned )
				JS_RemoveObjectRoot( g_ScriptingHost.GetContext(), &m_JS );
			m_JS = NULL;
		}
	}

	// 
	// Functions and data that must be provided to JavaScript
	// 
private:
	static JSBool JSGetProperty( JSContext* cx, JSObject* obj, jsid id, jsval* vp )
	{
		T* Instance = ToNative<T>( cx, obj );
		if( !Instance )
			return( JS_TRUE );

		jsval idval;
		if( !JS_IdToValue( cx, id, &idval ) )
			return( JS_FALSE );

		CStrW PropName = g_ScriptingHost.ValueToUCString( idval );

		if( !Instance->GetProperty( cx, PropName, vp ) )
			return( JS_TRUE );
		
		return( JS_TRUE );
	}
	static JSBool JSSetProperty( JSContext* cx, JSObject* obj, jsid id, jsval* vp )
	{
		T* Instance = ToNative<T>( cx, obj );
		if( !Instance )
			return( JS_TRUE );

		jsval idval;
		if( !JS_IdToValue( cx, id, &idval ) )
			return( JS_FALSE );

		CStrW PropName = g_ScriptingHost.ValueToUCString( idval );

		Instance->SetProperty( cx, PropName, vp );

		return( JS_TRUE );
	}
	static void DefaultFinalize( JSContext *cx, JSObject *obj )
	{
		T* Instance = ToNative<T>( cx, obj );
		if( !Instance || Instance->m_EngineOwned )
			return;
		
		delete( Instance );
		JS_SetPrivate( cx, obj, NULL );
	}

	
	static JSPropertySpec JSI_props[];
	static std::vector<JSFunctionSpec> m_Methods;
	
public:
	// Standard constructors/destructors
	CJSObject()
	{
		m_JS = NULL;
		m_EngineOwned = true;
	}
	virtual ~CJSObject()
	{
		Shutdown();
	}
	void Shutdown()
	{
		PropertyTable::iterator it;
		for( it = m_ScriptProperties.begin(); it != m_ScriptProperties.end(); it++ )
			delete( it->second );
		m_ScriptProperties.clear();
		ReleaseScriptObject();
#ifdef ALLOW_NONSHARED_NATIVES
		for( it = m_NonsharedProperties.begin(); it != m_NonsharedProperties.end(); it++ )
			delete( it->second );
		m_NonsharedProperties.clear();
#endif
	}
};

template<typename T, bool ReadOnly> JSClass CJSObject<T, ReadOnly>::JSI_class = {
	NULL, JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JSGetProperty, JSSetProperty,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, DefaultFinalize,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL
};

template<typename T, bool ReadOnly> JSPropertySpec CJSObject<T, ReadOnly>::JSI_props[] = {
	{ NULL, 0, 0, NULL, NULL },
};

template<typename T, bool ReadOnly> std::vector<JSFunctionSpec> CJSObject<T, ReadOnly>::m_Methods;

template<typename T, bool ReadOnly> typename CJSObject<T, ReadOnly>::PropertyTable CJSObject<T, ReadOnly>::m_NativeProperties;

#endif



