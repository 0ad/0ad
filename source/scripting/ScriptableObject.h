// ScriptableObject.h
//
// A quick way to add (mostly) sensibly-behaving JavaScript interfaces to native classes.
//
// Mark Thompson (mark@wildfiregames.com / mot20@cam.ac.uk)
//
// I really, really hope this is the last time I touch this code.

#include "scripting/ScriptingHost.h"
#include "JSConversions.h"

// The Last Redesign

#ifndef SCRIPTABLE_INCLUDED
#define SCRIPTABLE_INCLUDED

class IJSProperty
{
public:

	bool m_AllowsInheritance;
	bool m_Inherited;
	bool m_Intrinsic;

	virtual jsval Get( JSContext* cx ) = 0;
	virtual void Set( JSContext* cx, jsval Value ) = 0;

	// Copies the data directly out of a parent property
	// Warning: Don't use if you're not certain the properties are not of the same type.
	virtual void ImmediateCopy( IJSProperty* Copy ) = 0;

	jsval Get() { return( Get( g_ScriptingHost.GetContext() ) ); }
	void Set( jsval Value ) { return( Set( g_ScriptingHost.GetContext(), Value ) ); }
	
	virtual ~IJSProperty() {}
};

class IJSObject
{
public:
	typedef STL_HASH_MAP<CStrW, IJSProperty*, CStrW_hash_compare> PropertyTable;
	typedef std::vector<IJSObject*> InheritorsList;

	// Used for freshen/update
	typedef void (IJSObject::*NotifyFn)();

	// Properties of this object
	PropertyTable m_Properties;

	// Parent object
	IJSObject* m_Parent;

	// Objects that inherit from this
	InheritorsList m_Inheritors;

	// Set the base, and rebuild
	void SetBase( IJSObject* m_Parent );
	virtual void Rebuild() = 0;

	// Check for a property
	virtual IJSProperty* HasProperty( CStrW PropertyName ) = 0;

	// Add a property (inherits value from parent)
	virtual void ReplicateProperty( CStrW PropertyName, jsval Value ) = 0;
	
	// Add a property (with immediate value)
	virtual void AddProperty( CStrW PropertyName, jsval Value ) = 0;
	virtual void AddProperty( CStrW PropertyName, CStrW Value ) = 0;
};

template<typename T> class CJSObject;

template<typename T> class CJSPropertyAccessor
{
	T* m_Owner;
	CStrW m_PropertyRoot;
	template<typename Q> friend class CJSObject;

public:
	CJSPropertyAccessor( T* Owner, CStrW PropertyRoot )
	{
		m_Owner = Owner;
		m_PropertyRoot = PropertyRoot;
	}
	static JSObject* CreateAccessor( JSContext* cx, T* Owner, CStrW PropertyRoot )
	{
		JSObject* Accessor = JS_NewObject( cx, &JSI_Class, NULL, NULL );
		JS_SetPrivate( cx, Accessor, new CJSPropertyAccessor( Owner, PropertyRoot ) );

		return( Accessor );
	}
	static JSBool JSGetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CJSPropertyAccessor* Instance = (CJSPropertyAccessor*)JS_GetPrivate( cx, obj );
		if( !Instance ) return( JS_TRUE );

		CStrW PropName = Instance->m_PropertyRoot + CStrW( L"." ) + g_ScriptingHost.ValueToUCString( id );
		Instance->m_Owner->GetProperty( cx, PropName, vp );
		
		return( JS_TRUE );
	}
	static JSBool JSSetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CJSPropertyAccessor* Instance = (CJSPropertyAccessor*)JS_GetPrivate( cx, obj );
		if( !Instance ) return( JS_TRUE );

		CStrW PropName = g_ScriptingHost.ValueToUCString( id );

		Instance->m_Owner->SetProperty( cx, Instance->m_PropertyRoot + CStrW( L"." ) + PropName, vp );
		
		return( JS_TRUE );
	}
	static JSBool JSPrimitive( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
	{
		CJSPropertyAccessor* Instance = (CJSPropertyAccessor*)JS_GetPrivate( cx, obj );
		if( !Instance ) return( JS_TRUE );

		*rval = Instance->m_Owner->m_Properties[Instance->m_PropertyRoot]->Get( cx );
		
		return( JS_TRUE );
	}
	static JSBool JSToString( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
	{
		CJSPropertyAccessor* Instance = (CJSPropertyAccessor*)JS_GetPrivate( cx, obj );
		if( !Instance ) return( JS_TRUE );

		JSString* str = JS_ValueToString( cx, Instance->m_Owner->m_Properties[Instance->m_PropertyRoot]->Get( cx ) );
		*rval = STRING_TO_JSVAL( str );
		
		return( JS_TRUE );
	}
	/*
	static void JSFinalize( JSContext* cx, JSObject* obj )
	{
		CJSPropertyAccessor* Instance = (CJSPropertyAccessor*)JS_GetPrivate( cx, obj );
		if( !Instance )
			return;
	
		if( Instance ) delete( Instance );
	}
	*/
	static JSClass JSI_Class;
	static void ScriptingInit()
	{
		JSFunctionSpec JSI_methods[] = { { "valueOf", JSPrimitive, 0, 0, 0 }, { "toString", JSToString, 0, 0, 0 }, { 0 } };
		JSPropertySpec JSI_props[] = { { 0 } };

		g_ScriptingHost.DefineCustomObjectType( &JSI_Class, NULL, 0, JSI_props, JSI_methods, NULL, NULL );
	}
};

template<typename T> JSClass CJSPropertyAccessor<T>::JSI_Class = {
	"Property", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JSGetProperty, JSSetProperty,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JS_FinalizeStub,
	NULL, NULL, NULL, NULL 
};


template<typename T> class CJSProperty : public IJSProperty
{
	T* m_Data;

	IJSObject* m_Owner;

	// Function on Owner to call after value is changed
	IJSObject::NotifyFn m_Update;
	
	// Function on Owner to call before reading or writing the value
	IJSObject::NotifyFn m_Freshen;

public:
	CJSProperty( T* Data, IJSObject* Owner = NULL, bool AllowsInheritance = true, IJSObject::NotifyFn Update = NULL, IJSObject::NotifyFn Freshen = NULL )
	{
		assert( !( !m_Owner && ( Freshen || Update ) ) ); // Bad programmer.
		m_Data = Data;
		m_Owner = Owner;
		m_Update = Update;
		m_Freshen = Freshen;
		m_AllowsInheritance = AllowsInheritance;
		m_Intrinsic = true;
	}
	jsval Get( JSContext* cx )
	{
		if( m_Freshen ) (m_Owner->*m_Freshen)();
		return( ToJSVal<T>( *m_Data ) );
	}
	void ImmediateCopy( IJSProperty* Copy )
	{
		*m_Data = *( ((CJSProperty<T>*)Copy)->m_Data );
	}
	void Set( JSContext* cx, jsval Value )
	{
		if( m_Freshen ) (m_Owner->*m_Freshen)();
		if( ToPrimitive<T>( cx, Value, *m_Data ) )
			if( m_Update ) (m_Owner->*m_Update)();
	}
};

class CJSValProperty : public IJSProperty
{
	template<typename Q> friend class CJSObject;

	jsval m_Data;
	JSObject* m_JSAccessor;

public:
	CJSValProperty( jsval Data, bool Inherited )
	{
		m_Inherited = Inherited;
		m_Data = Data;
		m_Intrinsic = false;
		m_JSAccessor = NULL;
		Root();
	}
	~CJSValProperty()
	{
		Uproot();
	}
	void Root()
	{
		if( JSVAL_IS_GCTHING( m_Data ) )
			JS_AddRoot( g_ScriptingHost.GetContext(), &m_Data );
	}
	void Uproot()
	{
		if( JSVAL_IS_GCTHING( m_Data ) )
			JS_RemoveRoot( g_ScriptingHost.GetContext(), &m_Data );
	}
	jsval Get( JSContext* cx )
	{
		return( m_Data );
	}
	void Set( JSContext* cx, jsval Value )
	{
		Uproot();
		m_Data = Value;
		Root();
	}
	void ImmediateCopy( IJSProperty* Copy )
	{
		Uproot();
		m_Data = ((CJSValProperty*)Copy)->m_Data;
		Root();
	}
};

// Wrapper around native functions that are attached to CJSObjects

template<typename T, typename RType, RType (T::*NativeFunction)( JSContext* cx, uintN argc, jsval* argv )> class CNativeFunction
{
public:
	static JSBool JSFunction( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
	{
		T* Native = ToNative<T>( cx, obj );
		if( !Native )
			return( JS_TRUE );

		*rval = ToJSVal<RType>( (Native->*NativeFunction)( cx, argc, argv ) );

		return( JS_TRUE );
	}
};

template<typename T> class CJSObject : public IJSObject
{
	JSObject* m_JS;
public:

	// JS Property access
	void GetProperty( JSContext* cx, CStrW PropertyName, jsval* vp );
	void SetProperty( JSContext* cx, CStrW PropertyName, jsval* vp )
	{
		IJSProperty* prop = HasProperty( PropertyName );
		
		if( prop )
		{
			// Already exists
			prop->Set( cx, *vp );

			if( prop->m_AllowsInheritance )
			{
				// Run along and update this property in any inheritors
				InheritorsList UpdateSet( m_Inheritors );

				while( !UpdateSet.empty() )
				{
					IJSObject* UpdateObj = UpdateSet.back();
					UpdateSet.pop_back();
					IJSProperty* UpdateProp = UpdateObj->m_Properties[PropertyName];
					if( UpdateProp->m_Inherited )
					{
						UpdateProp->Set( cx, *vp );
						InheritorsList::iterator it2;
						for( it2 = UpdateObj->m_Inheritors.begin(); it2 != UpdateObj->m_Inheritors.end(); it2++ )
							UpdateSet.push_back( *it2 );
					}
				}
			}
		}
		else
		{
			// Need to add it
			AddProperty( PropertyName, *vp );
		}
	}

	// 
	// Functions that must be provided to JavaScript
	// 
	static JSBool JSGetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CJSObject<T>* Instance = ToNative<T>( cx, obj );
		if( !Instance )
			return( JS_TRUE );

		CStrW PropName = g_ScriptingHost.ValueToUCString( id );

		Instance->GetProperty( cx, PropName, vp );
		
		return( JS_TRUE );
	}
	static JSBool JSSetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CJSObject<T>* Instance = ToNative<T>( cx, obj );
		if( !Instance )
			return( JS_TRUE );

		CStrW PropName = g_ScriptingHost.ValueToUCString( id );

		Instance->SetProperty( cx, PropName, vp );

		return( JS_TRUE );
	}
	static void ScriptingInit( const char* ClassName, JSNative Constructor = NULL, uintN ConstructorMinArgs = 0 )
	{
		JSFunctionSpec* JSI_methods = new JSFunctionSpec[ m_Methods.size() + 1 ];
		for( unsigned int MethodID = 0; MethodID < m_Methods.size(); MethodID++ )
			JSI_methods[MethodID] = m_Methods[MethodID];

		JSI_methods[MethodID].name = 0;

		JSI_class.name = ClassName;
		g_ScriptingHost.DefineCustomObjectType( &JSI_class, Constructor, ConstructorMinArgs, JSI_props, JSI_methods, NULL, NULL );

		delete[]( JSI_methods );
	}
private:
	void CreateScriptObject()
	{
		assert( !m_JS );
		m_JS = JS_NewObject( g_ScriptingHost.GetContext(), &JSI_class, NULL, NULL );
		JS_AddRoot( g_ScriptingHost.GetContext(), m_JS );
		JS_SetPrivate( g_ScriptingHost.GetContext(), m_JS, this );
	}
public:
	static JSClass JSI_class;
	JSObject* GetScript() 
	{
		if( !m_JS )
			CreateScriptObject();
		return( m_JS );
	}
private:
	static JSPropertySpec JSI_props[];
	static std::vector<JSFunctionSpec> m_Methods;
	static void JSFinalize( JSContext* cx, JSObject* obj );
	
public:
	CJSObject()
	{
		m_Parent = NULL;
		m_JS = NULL;
	}
	~CJSObject()
	{
		PropertyTable::iterator it;
		for( it = m_Properties.begin(); it != m_Properties.end(); it++ )
		{
			if( !it->second->m_Intrinsic )
			{
				CJSValProperty* extProp = (CJSValProperty*)it->second;
				if( extProp->m_JSAccessor )
				{
					CJSPropertyAccessor< CJSObject<T> >* accessor = (CJSPropertyAccessor< CJSObject<T> >*)JS_GetPrivate( g_ScriptingHost.GetContext(), extProp->m_JSAccessor );
					assert( accessor );
					delete( accessor );
					JS_SetPrivate( g_ScriptingHost.GetContext(), extProp->m_JSAccessor, NULL );
					JS_RemoveRoot( g_ScriptingHost.GetContext(), extProp->m_JSAccessor );
				}
			}
			delete( it->second );
		}
		/*
		ReferrersSet::iterator rit;
		for( rit = m_Referring.begin(); rit != m_Referring.end(); rit++ )
		{
			JSObject* ref = *rit;
			if( JS_GetClass( ref ) == &JSI_class )
			{
				// Reference to this object directly.
				// Replace with null pointer.

				// - Make sure it refers to this object.
				assert( JS_GetPrivate( g_ScriptingHost.GetContext(), ref ) == this );

				JS_SetPrivate( g_ScriptingHost.GetContext(), ref, NULL );
			}
			else
			{
				// Possibly just deallocate the property object?

				// - Nothing else it should be.
				assert( JS_GetClass( ref ) == &CJSPropertyAccessor<T>::JSI_Class );
				
				CJSPropertyAccessor<T>* Accessor = (CJSPropertyAccessor<T>*)JS_GetPrivate( g_ScriptingHost.GetContext(), ref );

				// - Make sure it refers to this object.
				assert( Accessor->m_Owner == this );

				Accessor->m_Owner = NULL;
			}
		}
		*/
		if( m_JS )
		{
			JS_SetPrivate( g_ScriptingHost.GetContext(), m_JS, NULL );
			JS_RemoveRoot( g_ScriptingHost.GetContext(), m_JS );
		}
	}
	void SetBase( IJSObject* Parent )
	{
		if( m_Parent )
		{
			// Remove this from the list of our parent's inheritors
			InheritorsList::iterator it;
			for( it = m_Parent->m_Inheritors.begin(); it != m_Parent->m_Inheritors.end(); it++ )
				if( (*it) == this )
					m_Parent->m_Inheritors.erase( it );
			// TODO: Remove any properties we were inheriting from this parent that we didn't specify ourselves
		}
		m_Parent = Parent;
		if( m_Parent )
		{
			// Place this in the list of our parent's inheritors
			m_Parent->m_Inheritors.push_back( this );
			Rebuild();
		}
	}
	void Rebuild()
	{
		PropertyTable::iterator it;
		// For each property in the parent
		for( it = m_Parent->m_Properties.begin(); it != m_Parent->m_Properties.end(); it++ )
		{
			if( !it->second->m_AllowsInheritance )
				continue;
			PropertyTable::iterator cp;
			// Attempt to locate it in this object
			cp = m_Properties.find( it->first );
			if( cp != m_Properties.end() )
			{
				if( cp->second->m_Inherited )
					cp->second->ImmediateCopy( it->second );
			}
			else
				m_Properties[it->first] = new CJSValProperty( it->second->Get(), true );
		}
		InheritorsList::iterator c;
		for( c = m_Inheritors.begin(); c != m_Inheritors.end(); c++ )
			(*c)->Rebuild();
	}
	IJSProperty* HasProperty( CStrW PropertyName )
	{
		PropertyTable::iterator it;
		it = m_Properties.find( PropertyName );
		if( it == m_Properties.end() )
			return( NULL );
		return( it->second );
	}

	void ReplicateProperty( CStrW PropertyName, jsval Value )
	{
		m_Properties[PropertyName] = new CJSValProperty( Value, true );
		// Run through our descendants to add the property to all of them that don't
		// already have it.
		InheritorsList::iterator it;
		for( it = m_Inheritors.begin(); it != m_Inheritors.end(); it++ )
			if( !((*it)->HasProperty( PropertyName ) ) )
				(*it)->ReplicateProperty( PropertyName, Value );
	}

	void AddProperty( CStrW PropertyName, jsval Value )
	{
		assert( !HasProperty( PropertyName ) );
		m_Properties[PropertyName] = new CJSValProperty( Value, false );

		// Run through our descendants to add the property to all of them that don't
		// already have it.
		InheritorsList::iterator it;
		for( it = m_Inheritors.begin(); it != m_Inheritors.end(); it++ )
			if( !((*it)->HasProperty( PropertyName ) ) )
				(*it)->ReplicateProperty( PropertyName, Value );
	}

	void AddProperty( CStrW PropertyName, CStrW Value )
	{
		AddProperty( PropertyName, ToJSVal<CStrW>( Value ) );
	}
	template<typename ReturnType, ReturnType (T::*NativeFunction)( JSContext* cx, uintN argc, jsval* argv )> 
		static void AddMethod( const char* Name, uintN MinArgs )
	{
		JSFunctionSpec FnInfo = { Name, CNativeFunction<T, ReturnType, NativeFunction>::JSFunction, MinArgs, 0, 0 };
		T::m_Methods.push_back( FnInfo );
	}
	template<typename PropType> void AddProperty( CStrW PropertyName, PropType* Native, bool AllowInheritance = true, NotifyFn Update = NULL, NotifyFn Refresh = NULL )
	{
		m_Properties[PropertyName] = new CJSProperty<PropType>( Native, this, AllowInheritance, Update, Refresh );
	}
};

template<typename T> JSClass CJSObject<T>::JSI_class = {
	NULL, JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JSGetProperty, JSSetProperty,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JS_FinalizeStub,
	NULL, NULL, NULL, NULL 
};

template<typename T> JSPropertySpec CJSObject<T>::JSI_props[] = {
	{ 0 },
};

template<typename T> std::vector<JSFunctionSpec> CJSObject<T>::m_Methods;

template<typename T> void CJSObject<T>::GetProperty( JSContext* cx, CStrW PropertyName, jsval* vp )
{
	IJSProperty* Property = HasProperty( PropertyName );
	if( Property )
	{
		if( Property->m_Intrinsic )
		{
			*vp = Property->Get( cx );
		}
		else
		{
			CJSValProperty* extProp = (CJSValProperty*)Property;
			if( !extProp->m_JSAccessor )
			{
				extProp->m_JSAccessor = CJSPropertyAccessor< CJSObject<T> >::CreateAccessor( cx, this, PropertyName );
				JS_AddRoot( cx, extProp->m_JSAccessor );
			}

			*vp = OBJECT_TO_JSVAL( extProp->m_JSAccessor );
		}
	}
}

#endif


