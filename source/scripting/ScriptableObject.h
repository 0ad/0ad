// ScriptableObject.h
//
// A quick way to add (mostly) sensibly-behaving JavaScript interfaces to native classes.
//
// Mark Thompson (mark@wildfiregames.com / mot20@cam.ac.uk)
//
// General idea:
//
// IJSProperty is the interface representing a property of an object.
// Objects contain a mapping of names->IJSProperties
// Some IJSProperties wrap C++ variables that are declared by the engine
// Others wrap pairs of getter/setter functions
// Most, however, wrap a jsval and are defined by scripts and XML files.
// Objects may also have a parent object. If an attempt is made to
// access a property that doesn't exist in this object, the parent's object
// is checked, and so on.
// To allow this parent system to work for C++ properties, when a C++ 
// is set on an object, it's also set on any object that inherits it
// (Unless that object specifies its own value for that property, or
// the property is marked as being uninheritable)

// Objects and properties may be flagged read-only, causing all attempts to
// set values to become no-ops. If an object is read-only, all properties are
// - even ones that are flagged as writable.

// Usage: Create a class CSomething inheriting CJSObject<CSomething>
// In CSomething's constructor, add properties to the new object with
// AddProperty( name, pointer-to-C++-variable ).
// Also, ScriptingInit( "Some Name" ) must be called at initialization
// - put it in main. There's also AddMethod<ReturnType, NativeFunction>( Name,
// MinArgs ) - call that at initialization, too.
// If you include data members or functions that return types that JSConversions.h
// doesn't handle sensibly, you need to make it do so.
// If you're looking for examples, DOMEvent.h is the simplest user of this class
// CBaseEntity and CEntity do also (and use other stuff not mentioned above)
// but are more complex.

#include "scripting/ScriptingHost.h"
#include "JSConversions.h"

#ifndef SCRIPTABLE_INCLUDED
#define SCRIPTABLE_INCLUDED

class IJSProperty
{
public:

	bool m_AllowsInheritance;
	bool m_Inherited;
	bool m_Intrinsic;
	
	// This is to make sure that all the fields are initialized at construction
	inline IJSProperty():
		m_AllowsInheritance(true),
		m_Inherited(true),
		m_Intrinsic(true)
	{}

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
	// Make copy constructor and assignment operator private - since copying of
	// these objects is unsafe unless done specially.
	// These will never be implemented (they are, after all, here to *prevent*
	// copying)
	IJSObject(const IJSObject &other);
	IJSObject& operator=(const IJSObject &other);

public:
	typedef STL_HASH_MAP<CStrW, IJSProperty*, CStrW_hash_compare> PropertyTable;
	typedef std::vector<IJSObject*> InheritorsList;

	// Used for freshen/update
	typedef void (IJSObject::*NotifyFn)();

	// Property getters and setters
	typedef jsval (IJSObject::*GetFn)();
	typedef void (IJSObject::*SetFn)( jsval );

	// Properties of this object
	PropertyTable m_Properties;

	// Parent object
	IJSObject* m_Parent;

	// Objects that inherit from this
	InheritorsList m_Inheritors;

	// Set the base, and rebuild
	void SetBase( IJSObject* m_Parent );

	// Rebuild any intrinsic (mapped-to-C++-variable) properties
	virtual void Rebuild() = 0;

	// Check for a property
	virtual IJSProperty* HasProperty( CStrW PropertyName ) = 0;

	// Retrieve the value of a property
	virtual void GetProperty( JSContext* cx, CStrW PropertyName, jsval* vp ) = 0;
	
	// Add a property (with immediate value)
	virtual void AddProperty( CStrW PropertyName, jsval Value ) = 0;
	virtual void AddProperty( CStrW PropertyName, CStrW Value ) = 0;
	
	inline IJSObject() {}
};

template<typename T, bool ReadOnly = false> class CJSObject;

template<typename T> class CJSPropertyAccessor
{
	T* m_Owner;
	CStrW m_PropertyRoot;
	template<typename Q, bool ReadOnly> friend class CJSObject;

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

		// Check all along the inheritance tree
		// Possible optimization: Store the hashed value over the lookups
		IJSObject* Target = Instance->m_Owner;
		IJSProperty* Property;

		while( Target )
		{
			Property = Target->HasProperty( Instance->m_PropertyRoot );
			if( Property )
			{
				*rval = Property->Get( cx );
				break;
			}
			Target = Target->m_Parent;
		}
		
		return( JS_TRUE );
	}
	static JSBool JSToString( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
	{
		CJSPropertyAccessor* Instance = (CJSPropertyAccessor*)JS_GetPrivate( cx, obj );
		if( !Instance ) return( JS_TRUE );

		// Check all along the inheritance tree
		// TODO: Optimization: Store the hashed value over the lookups
		IJSObject* Target = Instance->m_Owner;
		IJSProperty* Property;
		JSString* str;

		while( Target )
		{
			Property = Target->HasProperty( Instance->m_PropertyRoot );
			if( Property )
			{
				str = JS_ValueToString( cx, Property->Get( cx ) ); 
				break;
			}
			Target = Target->m_Parent;
		}
		
		*rval = STRING_TO_JSVAL( str );
		
		return( JS_TRUE );
	}
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

template<typename T, bool ReadOnly> class CJSProperty : public IJSProperty
{
	T* m_Data;

	IJSObject* m_Owner;

	// Function on Owner to call after value is changed
	IJSObject::NotifyFn m_Update;
	
	// Function on Owner to call before reading or writing the value
	IJSObject::NotifyFn m_Freshen;

public:
	CJSProperty( T* Data, IJSObject* Owner = NULL, bool AllowsInheritance = false, IJSObject::NotifyFn Update = NULL, IJSObject::NotifyFn Freshen = NULL )
	{
		assert( !( !Owner && ( Freshen || Update ) ) ); // Bad programmer.
		m_Data = Data;
		m_Owner = Owner;
		m_AllowsInheritance = AllowsInheritance;
		m_Update = Update;
		m_Freshen = Freshen;
		m_Intrinsic = true;
	}
	jsval Get( JSContext* cx )
	{
		if( m_Freshen ) (m_Owner->*m_Freshen)();
		return( ToJSVal( *m_Data ) );
	}
	void ImmediateCopy( IJSProperty* Copy )
	{
		*m_Data = *( ((CJSProperty<T, ReadOnly>*)Copy)->m_Data );
	}
	void Set( JSContext* cx, jsval Value )
	{
		if( !ReadOnly )
		{
			if( m_Freshen ) (m_Owner->*m_Freshen)();
			if( ToPrimitive( cx, Value, *m_Data ) )
				if( m_Update ) (m_Owner->*m_Update)();
		}
	}

};

class CJSReflector
{
	template<typename Q, bool ReadOnly> friend class CJSObject;
	JSObject* m_JSAccessor;
};

class CJSDynamicProperty : public IJSProperty
{
	template<typename Q, bool ReadOnly> friend class CJSObject;

	JSObject* m_JSAccessor;
public:
	CJSDynamicProperty()
	{
		m_JSAccessor = NULL;
		m_Intrinsic = false;
	}
};

class CJSValProperty : public CJSDynamicProperty
{
	template<typename Q, bool ReadOnly> friend class CJSObject;

	jsval m_Data;

public:
	CJSValProperty( jsval Data, bool Inherited )
	{
		m_Inherited = Inherited;
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
#ifndef NDEBUG
			JS_AddNamedRoot( g_ScriptingHost.GetContext(), (void*)&m_Data, "jsval property"  );
#else
			JS_AddRoot( g_ScriptingHost.GetContext(), (void*)&m_Data );
#endif
	}
	void Uproot()
	{
		if( JSVAL_IS_GCTHING( m_Data ) )
			JS_RemoveRoot( g_ScriptingHost.GetContext(), (void*)&m_Data );
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

class CJSFunctionProperty : public IJSProperty
{
	IJSObject* m_Owner;

	// Function on Owner to get the value
	IJSObject::GetFn m_Getter;
	
	// Function on Owner to set the value
	IJSObject::SetFn m_Setter;

public:
	CJSFunctionProperty( IJSObject* Owner, IJSObject::GetFn Getter, IJSObject::SetFn Setter )
	{
		m_Inherited = false;
		m_Intrinsic = true;
		m_Owner = Owner;
		m_Getter = Getter;
		m_Setter = Setter;
		// Must at least be able to read 
		assert( m_Owner && m_Getter );
	}
	jsval Get( JSContext* cx )
	{
		return( (m_Owner->*m_Getter)() );
	}
	void Set( JSContext* cx, jsval Value )
	{
		if( m_Setter )
			(m_Owner->*m_Setter)( Value );
	}
	void ImmediateCopy( IJSProperty* Copy )
	{
		assert( 0 && "ImmediateCopy called on a property wrapping getter/setter functions" );
	}
};


// Wrapper around native functions that are attached to CJSObjects

template<typename T, bool ReadOnly, typename RType, RType (T::*NativeFunction)( JSContext* cx, uintN argc, jsval* argv )> class CNativeFunction
{
public:
	static JSBool JSFunction( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
	{
		T* Native = (T*)ToNative< CJSObject<T, ReadOnly> >( cx, obj );
		if( !Native )
			return( JS_TRUE );

		*rval = ToJSVal<RType>( (Native->*NativeFunction)( cx, argc, argv ) );

		return( JS_TRUE );
	}
};

template<typename T, bool ReadOnly> class CJSObject : public IJSObject
{
	typedef STL_HASH_MAP<CStrW, CJSReflector*, CStrW_hash_compare> ReflectorTable;

	JSObject* m_JS;
	ReflectorTable m_Reflectors;

public:
	// Whether native code is responsible for managing this object.
	// Script constructors should clear this *BEFORE* creating a JS
	// mirror (otherwise it'll be rooted).

	bool m_EngineOwned;

	// JS Property access
	void GetProperty( JSContext* cx, CStrW PropertyName, jsval* vp );
	void SetProperty( JSContext* cx, CStrW PropertyName, jsval* vp )
	{
		if( !ReadOnly )
		{
			IJSProperty* prop = HasProperty( PropertyName );
			
			if( prop )
			{
				// Already exists
				prop->Set( cx, *vp );
				
				// If it's a C++ property, reflect this change in objects that inherit this.
				if( prop->m_AllowsInheritance && prop->m_Intrinsic )
				{
					InheritorsList UpdateSet( m_Inheritors );

					while( !UpdateSet.empty() )
					{
						IJSObject* UpdateObj = UpdateSet.back();
						UpdateSet.pop_back();
						IJSProperty* UpdateProp = UpdateObj->m_Properties[PropertyName];
						// Property must exist, also be a C++ property, and not have its value specified.
						if( UpdateProp && UpdateProp->m_Intrinsic && UpdateProp->m_Inherited )
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
	}

	// 
	// Functions that must be provided to JavaScript
	// 
	static JSBool JSGetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CJSObject<T, ReadOnly>* Instance = ToNative< CJSObject<T, ReadOnly> >( cx, obj );
		if( !Instance )
			return( JS_TRUE );

		CStrW PropName = g_ScriptingHost.ValueToUCString( id );

		Instance->GetProperty( cx, PropName, vp );
		
		return( JS_TRUE );
	}
	static JSBool JSSetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CJSObject<T, ReadOnly>* Instance = ToNative< CJSObject<T, ReadOnly> >( cx, obj );
		if( !Instance )
			return( JS_TRUE );

		CStrW PropName = g_ScriptingHost.ValueToUCString( id );

		Instance->SetProperty( cx, PropName, vp );

		return( JS_TRUE );
	}
	static void ScriptingInit( const char* ClassName, JSNative Constructor = NULL, uintN ConstructorMinArgs = 0 )
	{
		JSFunctionSpec* JSI_methods = new JSFunctionSpec[ m_Methods.size() + 1 ];
		unsigned int MethodID;
		for( MethodID = 0; MethodID < m_Methods.size(); MethodID++ )
			JSI_methods[MethodID] = m_Methods[MethodID];

		JSI_methods[MethodID].name = 0;

		JSI_class.name = ClassName;
		g_ScriptingHost.DefineCustomObjectType( &JSI_class, Constructor, ConstructorMinArgs, JSI_props, JSI_methods, NULL, NULL );

		delete[]( JSI_methods );
	}

	static void DefaultFinalize( JSContext *cx, JSObject *obj )
	{
		CJSObject<T, ReadOnly>* Instance = ToNative< CJSObject<T,ReadOnly> >( cx, obj );
		if( !Instance || Instance->m_EngineOwned )
			return;
		
		delete( Instance );
		JS_SetPrivate( cx, obj, NULL );
	}

public:
	static JSClass JSI_class;
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
			{
#ifndef NDEBUG // Name the GC roots something more useful than 'ScriptableObject.h'
				JS_AddNamedRoot( g_ScriptingHost.GetContext(), (void*)&m_JS, JSI_class.name );
#else
				JS_AddRoot( g_ScriptingHost.GetContext(), (void*)&m_JS );		
#endif
			}
			JS_SetPrivate( g_ScriptingHost.GetContext(), m_JS, this );
		}
	}
	void ReleaseScriptObject()
	{
		if( m_JS )
		{
			JS_SetPrivate( g_ScriptingHost.GetContext(), m_JS, NULL );
			if( m_EngineOwned )
				JS_RemoveRoot( g_ScriptingHost.GetContext(), &m_JS );
			m_JS = NULL;
		}
	}
private:
	static JSPropertySpec JSI_props[];
	static std::vector<JSFunctionSpec> m_Methods;
	
public:
	CJSObject()
	{
		m_Parent = NULL;
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
		for( it = m_Properties.begin(); it != m_Properties.end(); it++ )
		{
			if( !it->second->m_Intrinsic )
			{
				CJSDynamicProperty* extProp = (CJSValProperty*)it->second;
				if( extProp->m_JSAccessor )
				{
					CJSPropertyAccessor< CJSObject<T, ReadOnly> >* accessor = (CJSPropertyAccessor< CJSObject<T, ReadOnly> >*)JS_GetPrivate( g_ScriptingHost.GetContext(), extProp->m_JSAccessor );
					assert( accessor );
					delete( accessor );
					JS_SetPrivate( g_ScriptingHost.GetContext(), extProp->m_JSAccessor, NULL );
					JS_RemoveRoot( g_ScriptingHost.GetContext(), &( extProp->m_JSAccessor ) );
				}	
			}
			delete( it->second );
		}


		ReflectorTable::iterator it_a;
		for( it_a = m_Reflectors.begin(); it_a != m_Reflectors.end(); it_a++ )
		{
			CJSPropertyAccessor< CJSObject<T, ReadOnly> >* accessor = (CJSPropertyAccessor< CJSObject<T, ReadOnly> >*)JS_GetPrivate( g_ScriptingHost.GetContext(), it_a->second->m_JSAccessor );
			assert( accessor );
			delete( accessor );
			JS_SetPrivate( g_ScriptingHost.GetContext(), it_a->second->m_JSAccessor, NULL );
			JS_RemoveRoot( g_ScriptingHost.GetContext(), &( it_a->second->m_JSAccessor ) );
			delete( it_a->second );
		}	
		ReleaseScriptObject();
	}
	void SetBase( IJSObject* Parent )
{
		if( m_Parent )
		{
			// Remove this from the list of our parent's inheritors
			InheritorsList::iterator it;
			for( it = m_Parent->m_Inheritors.begin(); it != m_Parent->m_Inheritors.end(); it++ )
				if( (*it) == this )
				{
					m_Parent->m_Inheritors.erase( it );
					break;
				}
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
		// For each intrinsic property we have,
		for( it = m_Properties.begin(); it != m_Properties.end(); it++ )
		{
			if( !it->second->m_Intrinsic || !it->second->m_Inherited )
				continue;
			
			// Attempt to locate it in the parent
			IJSProperty* cp = m_Parent->HasProperty( it->first );

			// If it doesn't have it, we've inherited from an object of a different type
			// This isn't allowed at the moment; but I don't have an totally convincing
			// reason for forbidding it entirely. Mind, I can't think of any use for it,
			// either.
			// If it can be inherited, inherit it.
			if( cp && cp->m_AllowsInheritance )
			{
				assert( cp->m_Intrinsic );
				it->second->ImmediateCopy( cp );
			}
		}

		// Now recurse.
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

	void AddProperty( CStrW PropertyName, jsval Value )
	{
		assert( !HasProperty( PropertyName ) );
		CJSDynamicProperty* newProp = new CJSValProperty( Value, false ); 
		m_Properties[PropertyName] = newProp;

		ReflectorTable::iterator it;
		it = m_Reflectors.find( PropertyName );
		if( it != m_Reflectors.end() )
		{
			// We had an accessor pointing to this property before it was defined.
			newProp->m_JSAccessor = it->second->m_JSAccessor;
			JS_RemoveRoot( g_ScriptingHost.GetContext(), &( it->second->m_JSAccessor ) );
			JS_AddRoot( g_ScriptingHost.GetContext(), &( newProp->m_JSAccessor ) );
			delete( it->second );
			m_Reflectors.erase( it );
		}
	}

	void AddProperty( CStrW PropertyName, CStrW Value )
	{
		AddProperty( PropertyName, JSParseString( Value ) );
	}
	void AddProperty( CStrW PropertyName, GetFn Getter, SetFn Setter = NULL )
	{
		m_Properties[PropertyName] = new CJSFunctionProperty( this, Getter, Setter );
	}
	template<typename ReturnType, ReturnType (T::*NativeFunction)( JSContext* cx, uintN argc, jsval* argv )> 
		static void AddMethod( const char* Name, uintN MinArgs )
	{
		JSFunctionSpec FnInfo = { Name, CNativeFunction<T, ReadOnly, ReturnType, NativeFunction>::JSFunction, MinArgs, 0, 0 };
		T::m_Methods.push_back( FnInfo );
	}
	template<typename PropType> void AddProperty( CStrW PropertyName, PropType* Native, bool PropAllowInheritance = true, NotifyFn Update = NULL, NotifyFn Refresh = NULL )
	{
		m_Properties[PropertyName] = new CJSProperty<PropType, ReadOnly>( Native, this, PropAllowInheritance, Update, Refresh );
	}
	template<typename PropType> void AddReadOnlyProperty( CStrW PropertyName, PropType* Native, bool PropAllowInheritance = true, NotifyFn Update = NULL, NotifyFn Refresh = NULL )
	{
		m_Properties[PropertyName] = new CJSProperty<PropType, true>( Native, this, PropAllowInheritance, Update, Refresh );
	}
};

template<typename T, bool ReadOnly> JSClass CJSObject<T, ReadOnly>::JSI_class = {
	NULL, JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JSGetProperty, JSSetProperty,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, DefaultFinalize,
	NULL, NULL, NULL, NULL 
};

template<typename T, bool ReadOnly> JSPropertySpec CJSObject<T, ReadOnly>::JSI_props[] = {
	{ 0 },
};

template<typename T, bool ReadOnly> std::vector<JSFunctionSpec> CJSObject<T, ReadOnly>::m_Methods;

template<typename T, bool ReadOnly> void CJSObject<T, ReadOnly>::GetProperty( JSContext* cx, CStrW PropertyName, jsval* vp )
{
	IJSProperty* Property = HasProperty( PropertyName );
	if( Property && Property->m_Intrinsic )
	{
		*vp = Property->Get( cx );
	}
	else
	{
		CJSDynamicProperty* extProp;

		if( Property )
		{
			extProp = (CJSValProperty*)Property;

			if( !extProp->m_JSAccessor )
			{
				extProp->m_JSAccessor = CJSPropertyAccessor< CJSObject<T, ReadOnly> >::CreateAccessor( cx, this, PropertyName );
				JS_AddNamedRoot( cx, &extProp->m_JSAccessor, "property accessor" );
			}

			*vp = OBJECT_TO_JSVAL( extProp->m_JSAccessor );
		}
		else
		{
			// Check to see if it exists on a parent
			IJSObject* check = m_Parent;
			while( check )
			{
				if( check->HasProperty( PropertyName ) ) break;
				check = check->m_Parent;
			}

			if( !check )
				return;

			// FIXME: Fiddle a way so this /doesn't/ require multiple kilobytes
			// of memory. Can't think of any better way to do it yet. Problem is
			// that script may access a property that isn't defined locally, but
			// is defined by an ancestor. We can't return an accessor to the
			// ancestor's property, because then if it's altered it affects that
			// object, not this. At the moment, creating a 'reflector' property
			// accessor that references /this/ object to be returned to script.

			// (N.B. Can't just put JSObjects* in the table -> table entries can
			//  move -> root no longer refers to the JSObject.)

			ReflectorTable::iterator it;
			it = m_Reflectors.find( PropertyName );

			if( it == m_Reflectors.end() )
			{
				CJSReflector* reflector = new CJSReflector();
				reflector->m_JSAccessor = CJSPropertyAccessor< CJSObject<T, ReadOnly> >::CreateAccessor( cx, this, PropertyName );
				JS_AddRoot( cx, &reflector->m_JSAccessor );
				m_Reflectors.insert( std::pair<CStrW, CJSReflector*>( PropertyName, reflector ) );
				*vp = OBJECT_TO_JSVAL( reflector->m_JSAccessor );
			}
			else
				*vp = OBJECT_TO_JSVAL( it->second->m_JSAccessor );
		}
	}
}

#endif


