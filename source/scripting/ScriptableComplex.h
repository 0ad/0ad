// ScriptableComplex.h
//
// The version of CJSObject<> that retains the ability to use inheritance
// in its objects. Shouldn't be used any more for anything but entity code.

#include "scripting/ScriptingHost.h"
#include "JSConversions.h"

#include "lib/allocators.h"

#include <set>

#ifndef SCRIPTABLE_COMPLEX_INCLUDED
#define SCRIPTABLE_COMPLEX_INCLUDED

class IJSComplex;

class IJSComplexProperty
{
public:

	bool m_AllowsInheritance;
	bool m_Inherited;
	bool m_Intrinsic;
	
	// This is to make sure that all the fields are initialized at construction
	inline IJSComplexProperty():
		m_AllowsInheritance(true),
		m_Inherited(true),
		m_Intrinsic(true)
	{}

	virtual jsval Get( JSContext* cx, IJSComplex* owner ) = 0;
	virtual void Set( JSContext* cx, IJSComplex* owner, jsval Value ) = 0;

	// Copies the data directly out of a parent property
	// Warning: Don't use if you're not certain the properties are not of the same type.
	virtual void ImmediateCopy( IJSComplex* CopyTo, IJSComplex* CopyFrom, IJSComplexProperty* CopyProperty ) = 0;

	jsval Get( IJSComplex* owner ) { return( Get( g_ScriptingHost.GetContext(), owner ) ); }
	void Set( IJSComplex* owner, jsval Value ) { return( Set( g_ScriptingHost.GetContext(), owner, Value ) ); }

	virtual ~IJSComplexProperty() {}
};

class IJSComplex
{
	// Make copy constructor and assignment operator private - since copying of
	// these objects is unsafe unless done specially.
	// These will never be implemented (they are, after all, here to *prevent*
	// copying)
	IJSComplex(const IJSComplex &other);
	IJSComplex& operator=(const IJSComplex &other);

public:
	typedef STL_HASH_MAP<CStrW, IJSComplexProperty*, CStrW_hash_compare> PropertyTable;
	typedef std::vector<IJSComplex*> InheritorsList;
	typedef std::set<CStrW> StringTable;
	typedef std::pair<StringTable, StringTable::iterator> IteratorState;
		
	// Used for freshen/update
	typedef void (IJSComplex::*NotifyFn)();

	// Property getters and setters
	typedef jsval (IJSComplex::*GetFn)();
	typedef void (IJSComplex::*SetFn)( jsval );

	// Properties of this object
	PropertyTable m_Properties;

	// Parent object
	IJSComplex* m_Parent;

	// Objects that inherit from this
	InheritorsList m_Inheritors;

	// Destructor
	virtual ~IJSComplex() { }
	
	// Set the base, and rebuild
	void SetBase( IJSComplex* m_Parent );

	// Rebuild any intrinsic (mapped-to-C++-variable) properties
	virtual void Rebuild() = 0;

	// HACK: Doesn't belong here.
	virtual void rebuildClassSet() = 0;
	
	// Check for a property
	virtual IJSComplexProperty* HasProperty( CStrW PropertyName ) = 0;

	// Get all properties of an object
	virtual void FillEnumerateSet( IteratorState* it, CStrW* PropertyRoot = NULL ) = 0;

	// Retrieve the value of a property (returning false if that property is not defined)
	virtual bool GetProperty( JSContext* cx, CStrW PropertyName, jsval* vp ) = 0;
	
	// Add a property (with immediate value)
	virtual void AddProperty( CStrW PropertyName, jsval Value ) = 0;
	virtual void AddProperty( CStrW PropertyName, CStrW Value ) = 0;
	
	inline IJSComplex() {}
};

template<typename T, bool ReadOnly = false> class CJSComplex;

template<typename T> class CJSComplexPropertyAccessor
{
	T* m_Owner;
	CStrW m_PropertyRoot;
	template<typename Q, bool ReadOnly> friend class CJSComplex;

public:
	CJSComplexPropertyAccessor( T* Owner, CStrW PropertyRoot )
	{
		m_Owner = Owner;
		m_PropertyRoot = PropertyRoot;
	}
	static JSObject* CreateAccessor( JSContext* cx, T* Owner, CStrW PropertyRoot )
	{
		JSObject* Accessor = JS_NewObject( cx, &JSI_Class, NULL, NULL );
		JS_SetPrivate( cx, Accessor, new CJSComplexPropertyAccessor( Owner, PropertyRoot ) );

		return( Accessor );
	}
	static JSBool JSGetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CJSComplexPropertyAccessor* Instance = (CJSComplexPropertyAccessor*)JS_GetPrivate( cx, obj );
		if( !Instance ) return( JS_TRUE );

		CStrW PropName = Instance->m_PropertyRoot + CStrW( L"." ) + g_ScriptingHost.ValueToUCString( id );

		Instance->m_Owner->GetProperty( cx, PropName, vp );
		
		return( JS_TRUE );
	}
	static JSBool JSSetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CJSComplexPropertyAccessor* Instance = (CJSComplexPropertyAccessor*)JS_GetPrivate( cx, obj );
		if( !Instance ) return( JS_TRUE );

		CStrW PropName = g_ScriptingHost.ValueToUCString( id );

		Instance->m_Owner->SetProperty( cx, Instance->m_PropertyRoot + CStrW( L"." ) + PropName, vp );
		
		return( JS_TRUE );
	}
	static JSBool JSEnumerate( JSContext* cx, JSObject* obj, JSIterateOp enum_op, jsval* statep, jsid *idp )
	{
		IJSComplex::IteratorState* it;

		switch( enum_op )
		{
		case JSENUMERATE_INIT:
		{
			CJSComplexPropertyAccessor* Instance = (CJSComplexPropertyAccessor*)JS_GetPrivate( cx, obj );

			it = new IJSComplex::IteratorState;

			if( Instance )
			{
				size_t rlen = Instance->m_PropertyRoot.Length();

				IJSComplex::PropertyTable::iterator iit;
				for( iit = T::m_IntrinsicProperties.begin(); iit != T::m_IntrinsicProperties.end(); iit++ )
					if( ( iit->first.Length() > rlen ) && ( iit->first.Left( rlen ) == Instance->m_PropertyRoot ) && ( iit->first[rlen] == '.' ) )
						it->first.insert( iit->first.GetSubstring( rlen + 1, iit->first.Length() - rlen - 1 ).BeforeFirst( L"." ) );
					

				Instance->m_Owner->FillEnumerateSet( it, &( Instance->m_PropertyRoot ) );
			}
			it->second = it->first.begin();
			
			*statep = PRIVATE_TO_JSVAL( it );

			if( idp )
				*idp = INT_TO_JSVAL( it->first.size() );

			return( JS_TRUE );
		}
		case JSENUMERATE_NEXT:
			it = (IJSComplex::IteratorState*)JSVAL_TO_PRIVATE( *statep );
			if( it->second == it->first.end() )
			{
				delete( it );
				*statep = JSVAL_NULL;
				return( JS_TRUE );
			}
			
			// I think this is what I'm supposed to do... (cheers, Philip)
			if( !JS_ValueToId( g_ScriptingHost.GetContext(), ToJSVal<CStrW>( *( it->second ) ), idp ) )
				return( JS_FALSE );

			(it->second)++;

			*statep = PRIVATE_TO_JSVAL( it );
			return( JS_TRUE );
		case JSENUMERATE_DESTROY:
			it = (IJSComplex::IteratorState*)JSVAL_TO_PRIVATE( *statep );
			delete( it );
			*statep = JSVAL_NULL;
			return( JS_TRUE );
		}
		return( JS_FALSE );
	}
	static JSBool JSPrimitive( JSContext* cx, JSObject* obj, uintN UNUSED(argc), jsval* UNUSED(argv), jsval* rval )
	{
		CJSComplexPropertyAccessor* Instance = (CJSComplexPropertyAccessor*)JS_GetPrivate( cx, obj );
		if( !Instance ) return( JS_TRUE );

		// Check all along the inheritance tree
		// Possible optimization: Store the hashed value over the lookups
		IJSComplex* Target = Instance->m_Owner;
		IJSComplexProperty* Property;

		while( Target )
		{
			Property = Target->HasProperty( Instance->m_PropertyRoot );
			if( Property )
			{
				*rval = Property->Get( cx, Target );
				break;
			}
			Target = Target->m_Parent;
		}
		
		return( JS_TRUE );
	}
	static JSBool JSToString( JSContext* cx, JSObject* obj, uintN UNUSED(argc), jsval* UNUSED(argv), jsval* rval )
	{
		CJSComplexPropertyAccessor* Instance = (CJSComplexPropertyAccessor*)JS_GetPrivate( cx, obj );
		if( !Instance ) return( JS_TRUE );

		// Check all along the inheritance tree
		// TODO: Optimization: Store the hashed value over the lookups
		IJSComplex* Target = Instance->m_Owner;
		IJSComplexProperty* Property;
		JSString* str = NULL;

		while( Target )
		{
			Property = Target->HasProperty( Instance->m_PropertyRoot );
			if( Property )
			{
				str = JS_ValueToString( cx, Property->Get( cx, Target ) ); 
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

template<typename T> JSClass CJSComplexPropertyAccessor<T>::JSI_Class = {
	"Property", JSCLASS_HAS_PRIVATE | JSCLASS_NEW_ENUMERATE,
	JS_PropertyStub, JS_PropertyStub,
	JSGetProperty, JSSetProperty,
	(JSEnumerateOp)JSEnumerate, JS_ResolveStub,
	JS_ConvertStub, JS_FinalizeStub,
	NULL, NULL, NULL, NULL 
};


template<typename T, bool ReadOnly> class CJSSharedProperty : public IJSComplexProperty
{
	T IJSComplex::*m_Data;

	// Function on Owner to call after value is changed
	IJSComplex::NotifyFn m_Update;
	
	// Function on Owner to call before reading or writing the value
	IJSComplex::NotifyFn m_Freshen;

public:
	CJSSharedProperty( T IJSComplex::*Data, bool AllowsInheritance = false, IJSComplex::NotifyFn Update = NULL, IJSComplex::NotifyFn Freshen = NULL )
	{
		m_Data = Data;
		m_AllowsInheritance = AllowsInheritance;
		m_Update = Update;
		m_Freshen = Freshen;
		m_Intrinsic = true;
		m_Inherited = true;
	}
	jsval Get( JSContext* UNUSED(cx), IJSComplex* owner )
	{
		if( m_Freshen ) (owner->*m_Freshen)();
		return( ToJSVal( owner->*m_Data ) );
	}
	void ImmediateCopy( IJSComplex* UNUSED(CopyTo), IJSComplex* UNUSED(CopyFrom), IJSComplexProperty* UNUSED(CopyProperty) )
	{
		debug_warn( "Inheritance not supported for CJSSharedProperties" );
	}
	void Set( JSContext* cx, IJSComplex* owner, jsval Value )
	{
		if( !ReadOnly )
		{
			if( m_Freshen ) (owner->*m_Freshen)();
			if( ToPrimitive( cx, Value, owner->*m_Data ) )
				if( m_Update ) (owner->*m_Update)();
		}
	}

};

template<typename T, bool ReadOnly> class CJSComplexProperty : public IJSComplexProperty
{
	T* m_Data;

	// Function on Owner to call after value is changed
	IJSComplex::NotifyFn m_Update;
	
	// Function on Owner to call before reading or writing the value
	IJSComplex::NotifyFn m_Freshen;

public:
	CJSComplexProperty( T* Data, bool AllowsInheritance = false, IJSComplex::NotifyFn Update = NULL, IJSComplex::NotifyFn Freshen = NULL )
	{
		m_Data = Data;
		m_AllowsInheritance = AllowsInheritance;
		m_Update = Update;
		m_Freshen = Freshen;
		m_Intrinsic = true;
	}
	jsval Get( JSContext* UNUSED(cx), IJSComplex* owner )
	{
		if( m_Freshen ) (owner->*m_Freshen)();
		return( ToJSVal( *m_Data ) );
	}
	void ImmediateCopy( IJSComplex* UNUSED(CopyTo), IJSComplex* UNUSED(CopyFrom), IJSComplexProperty* CopyProperty )
	{
		*m_Data = *( ( (CJSComplexProperty*)CopyProperty )->m_Data );
	}
	void Set( JSContext* cx, IJSComplex* owner, jsval Value )
	{
		if( !ReadOnly )
		{
			if( m_Freshen ) (owner->*m_Freshen)();
			if( ToPrimitive( cx, Value, *m_Data ) )
				if( m_Update ) (owner->*m_Update)();
		}
	}

};

class CJSReflector
{
	template<typename Q, bool ReadOnly> friend class CJSComplex;
	JSObject* m_JSAccessor;
};

class CJSDynamicComplexProperty : public IJSComplexProperty
{
	template<typename Q, bool ReadOnly> friend class CJSComplex;

	JSObject* m_JSAccessor;
public:
	CJSDynamicComplexProperty()
	{
		m_JSAccessor = NULL;
		m_Intrinsic = false;
		m_Inherited = false;
	}
};

class CJSValComplexProperty : public CJSDynamicComplexProperty
{
	template<typename Q, bool ReadOnly> friend class CJSComplex;

	jsval m_Data;

public:
	CJSValComplexProperty( jsval Data, bool Inherited )
	{
		m_Inherited = Inherited;
		m_Data = Data;
		Root();
	}
	~CJSValComplexProperty()
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
	jsval Get( JSContext* UNUSED(cx), IJSComplex* UNUSED(owner) )
	{
		return( m_Data );
	}
	void Set( JSContext* UNUSED(cx), IJSComplex* UNUSED(owner), jsval Value )
	{
		Uproot();
		m_Data = Value;
		Root();
	}
	void ImmediateCopy( IJSComplex* UNUSED(CopyTo), IJSComplex* UNUSED(CopyFrom),
		IJSComplexProperty* UNUSED(CopyProperty) )
	{
		debug_warn("ImmediateCopy called on a CJSValComplexProperty (something's gone wrong with the inheritance on this object)" );
	}
};

class CJSFunctionComplexProperty : public IJSComplexProperty
{
	// Function on Owner to get the value
	IJSComplex::GetFn m_Getter;
	
	// Function on Owner to set the value
	IJSComplex::SetFn m_Setter;

public:
	CJSFunctionComplexProperty( IJSComplex::GetFn Getter, IJSComplex::SetFn Setter )
	{
		m_Inherited = false;
		m_Intrinsic = true;
		m_Getter = Getter;
		m_Setter = Setter;
		// Must at least be able to read 
		debug_assert( m_Getter );
	}
	jsval Get( JSContext* UNUSED(cx), IJSComplex* owner )
	{
		return( (owner->*m_Getter)() );
	}
	void Set( JSContext* UNUSED(cx), IJSComplex* owner, jsval Value )
	{
		if( m_Setter )
			(owner->*m_Setter)( Value );
	}
	void ImmediateCopy( IJSComplex* UNUSED(CopyTo), IJSComplex* UNUSED(CopyFrom), IJSComplexProperty* UNUSED(CopyProperty) )
	{
		debug_warn("ImmediateCopy called on a property wrapping getter/setter functions (something's gone wrong with the inheritance for this object)" );
	}
};


// Wrapper around native functions that are attached to CJSComplexs

template<typename T, bool ReadOnly, typename RType, RType (T::*NativeFunction)( JSContext* cx, uintN argc, jsval* argv )> class CNativeComplexFunction
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



extern void jscomplexproperty_suballoc_attach();
extern void jscomplexproperty_suballoc_detach();
extern void* jscomplexproperty_suballoc();
extern void jscomplexproperty_suballoc_free(IJSComplexProperty* p);

template<typename T, bool ReadOnly> class CJSComplex : public IJSComplex
{
	typedef STL_HASH_MAP<CStrW, CJSReflector*, CStrW_hash_compare> ReflectorTable;
	template<typename Q> friend class CJSComplexPropertyAccessor;
	JSObject* m_JS;

	std::vector<CScriptObject> m_Watches;

	ReflectorTable m_Reflectors;


public:
	static JSClass JSI_class;

	// Whether native code is responsible for managing this object.
	// Script constructors should clear this *BEFORE* creating a JS
	// mirror (otherwise it'll be rooted).

	bool m_EngineOwned;

	// JS Property access
	bool GetProperty( JSContext* cx, CStrW PropertyName, jsval* vp );
	void SetProperty( JSContext* cx, CStrW PropertyName, jsval* vp )
	{
		if( !ReadOnly )
		{
			IJSComplexProperty* prop = HasProperty( PropertyName );
			
			if( prop )
			{
				// Already exists
				WatchNotify( cx, PropertyName, vp );
				prop->Set( cx, this, *vp );
				
				prop->m_Inherited = false;

				// If it's a C++ property, reflect this change in objects that inherit this.
				if( prop->m_AllowsInheritance && prop->m_Intrinsic )
				{
					InheritorsList UpdateSet( m_Inheritors );

					while( !UpdateSet.empty() )
					{
						IJSComplex* UpdateObj = UpdateSet.back();
						UpdateSet.pop_back();
						IJSComplexProperty* UpdateProp = UpdateObj->HasProperty( PropertyName );
						// Property must exist, also be a C++ property, and not have its value specified.
						if( UpdateProp && UpdateProp->m_Intrinsic && UpdateProp->m_Inherited )
						{
							UpdateProp->Set( cx, this, *vp );
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
				WatchNotify( cx, PropertyName, vp );
				AddProperty( PropertyName, *vp );
			}
		}
	}
	void WatchNotify( JSContext* cx, CStrW PropertyName, jsval* newval )
	{
		if( m_Watches.empty() ) return;

		jsval oldval = JSVAL_VOID;
		GetProperty( cx, PropertyName, &oldval );

		jsval args[3] = { ToJSVal( PropertyName ), oldval, *newval };

		std::vector<CScriptObject>::iterator it;
		for( it = m_Watches.begin(); it != m_Watches.end(); it++ )
			if( it->Run( GetScript(), newval, 3, args ) )
				args[2] = *newval;
	}
	// 
	// Functions that must be provided to JavaScript
	// 
	static JSBool JSGetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		T* Instance = ToNative<T>( cx, obj );
		if( !Instance )
			return( JS_TRUE );

		CStrW PropName = g_ScriptingHost.ValueToUCString( id );

		if( !Instance->GetProperty( cx, PropName, vp ) )
			return( JS_TRUE );
		
		return( JS_TRUE );
	}
	static JSBool JSSetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		T* Instance = ToNative<T>( cx, obj );
		if( !Instance )
			return( JS_TRUE );

		CStrW PropName = g_ScriptingHost.ValueToUCString( id );

		Instance->SetProperty( cx, PropName, vp );

		return( JS_TRUE );
	}
	static JSBool JSEnumerate( JSContext* cx, JSObject* obj, JSIterateOp enum_op, jsval* statep, jsid *idp )
	{
		IteratorState* it;

		switch( enum_op )
		{
		case JSENUMERATE_INIT:
		{
			T* Instance = ToNative<T>( cx, obj );
		
			it = new IteratorState;
			if( Instance )
			{
				PropertyTable::iterator iit;
				for( iit = T::m_IntrinsicProperties.begin(); iit != T::m_IntrinsicProperties.end(); iit++ )
					it->first.insert( iit->first );

				Instance->FillEnumerateSet( it );
			}

			it->second = it->first.begin();
			*statep = PRIVATE_TO_JSVAL( it );

			if( idp )
				*idp = INT_TO_JSVAL( it->first.size() );

			return( JS_TRUE );
		}
		case JSENUMERATE_NEXT:
			it = (IteratorState*)JSVAL_TO_PRIVATE( *statep );
			if( it->second == it->first.end() )
			{
				delete( it );
				*statep = JSVAL_NULL;
				return( JS_TRUE );
			}

			// I think this is what I'm supposed to do... (cheers, Philip)
			if( !JS_ValueToId( g_ScriptingHost.GetContext(), ToJSVal<CStrW>( *( it->second ) ), idp ) )
				return( JS_FALSE );

			(it->second)++;

			*statep = PRIVATE_TO_JSVAL( it );
			return( JS_TRUE );
		case JSENUMERATE_DESTROY:
			it = (IteratorState*)JSVAL_TO_PRIVATE( *statep );
			delete( it );
			*statep = JSVAL_NULL;
			return( JS_TRUE );
		}
		return( JS_FALSE );
	}	
	static JSBool SetWatchAll( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
	{
		T* Native = ToNative<T>( cx, obj );
		if( !Native )
			return( JS_TRUE );

		debug_assert( argc >= 1 );

		CScriptObject watch( argv[0] );
		std::vector<CScriptObject>::iterator it;
		for( it = Native->m_Watches.begin(); it != Native->m_Watches.end(); it++ )
			if( *it == watch )
			{
				*rval = JSVAL_FALSE;
				return( JS_TRUE );
			}

		Native->m_Watches.push_back( watch );
		*rval = JSVAL_TRUE;
		return( JS_TRUE );
	}

	static JSBool UnWatchAll( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
	{
		T* Native = ToNative<T>( cx, obj );
		if( !Native )
			return( JS_TRUE );

		if( argc >= 1 )
		{
			CScriptObject watch( argv[0] );
			std::vector<CScriptObject>::iterator it;
			for( it = Native->m_Watches.begin(); it != Native->m_Watches.end(); it++ )
				if( *it == watch )
				{
					Native->m_Watches.erase( it );
					*rval = JSVAL_TRUE;
					return( JS_TRUE );
				}

			*rval = JSVAL_FALSE;
		}
		else
		{
			Native->m_Watches.clear();
			*rval = JSVAL_TRUE;
		}
		return( JS_TRUE );
	}

	static void ScriptingInit( const char* ClassName, JSNative Constructor = NULL, uintN ConstructorMinArgs = 0 )
	{
		JSFunctionSpec* JSI_methods = new JSFunctionSpec[ m_Methods.size() + 3 ];
		unsigned int MethodID;
		for( MethodID = 0; MethodID < m_Methods.size(); MethodID++ )
			JSI_methods[MethodID] = m_Methods[MethodID];

		JSFunctionSpec watchAll = { "watchAll", SetWatchAll, 1, 0, 0 };
		JSI_methods[MethodID] = watchAll;
		JSFunctionSpec unwatchAll = { "unwatchAll", UnWatchAll, 1, 0, 0 };
		JSI_methods[MethodID + 1] = unwatchAll;
		JSI_methods[MethodID + 2].name = 0;

		JSI_class.name = ClassName;
		g_ScriptingHost.DefineCustomObjectType( &JSI_class, Constructor, ConstructorMinArgs, JSI_props, JSI_methods, NULL, NULL );

		delete[]( JSI_methods );

		atexit( ScriptingShutdown );
	}
	static void ScriptingShutdown()
	{
		PropertyTable::iterator it;
		for( it = m_IntrinsicProperties.begin(); it != m_IntrinsicProperties.end(); it++ )
			delete( it->second );
	}
	static void DefaultFinalize( JSContext *cx, JSObject *obj )
	{
		T* Instance = ToNative<T>( cx, obj );
		if( !Instance || Instance->m_EngineOwned )
			return;
		
		delete( Instance );
		JS_SetPrivate( cx, obj, NULL );
	}

public:

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
			JS_SetPrivate( g_ScriptingHost.GetContext(), m_JS, (T*)this );
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
	static PropertyTable m_IntrinsicProperties;
	
	
public:
	CJSComplex()
	{
		jscomplexproperty_suballoc_attach();

		m_Parent = NULL;
		m_JS = NULL;
		m_EngineOwned = true;
	}
	virtual ~CJSComplex()
	{
		Shutdown();

		jscomplexproperty_suballoc_detach();
	}
	void Shutdown()
	{
		PropertyTable::iterator it;
		for( it = m_Properties.begin(); it != m_Properties.end(); it++ )
		{
			if( !it->second->m_Intrinsic )
			{
				CJSDynamicComplexProperty* extProp = (CJSValComplexProperty*)it->second;
				if( extProp->m_JSAccessor )
				{
					CJSComplexPropertyAccessor< CJSComplex<T, ReadOnly> >* accessor = (CJSComplexPropertyAccessor< CJSComplex<T, ReadOnly> >*)JS_GetPrivate( g_ScriptingHost.GetContext(), extProp->m_JSAccessor );
					debug_assert( accessor );
					delete( accessor );
					JS_SetPrivate( g_ScriptingHost.GetContext(), extProp->m_JSAccessor, NULL );
					JS_RemoveRoot( g_ScriptingHost.GetContext(), &( extProp->m_JSAccessor ) );
				}	
			}

			jscomplexproperty_suballoc_free(it->second);
		}


		ReflectorTable::iterator it_a;
		for( it_a = m_Reflectors.begin(); it_a != m_Reflectors.end(); it_a++ )
		{
			CJSComplexPropertyAccessor< CJSComplex<T, ReadOnly> >* accessor = (CJSComplexPropertyAccessor< CJSComplex<T, ReadOnly> >*)JS_GetPrivate( g_ScriptingHost.GetContext(), it_a->second->m_JSAccessor );
			debug_assert( accessor );
			delete( accessor );
			JS_SetPrivate( g_ScriptingHost.GetContext(), it_a->second->m_JSAccessor, NULL );
			JS_RemoveRoot( g_ScriptingHost.GetContext(), &( it_a->second->m_JSAccessor ) );
			delete( it_a->second );
		}	
		ReleaseScriptObject();
	}
	void SetBase( IJSComplex* Parent )
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
			IJSComplexProperty* cp = m_Parent->HasProperty( it->first );

			// If it doesn't have it, we've inherited from an object of a different type
			// This isn't allowed at the moment; but I don't have an totally convincing
			// reason for forbidding it entirely. Mind, I can't think of any use for it,
			// either.
			// If it can be inherited, inherit it.
			if( cp && cp->m_AllowsInheritance )
			{
				debug_assert( cp->m_Intrinsic );
				it->second->ImmediateCopy( this, m_Parent, cp );
			}
		}
		// Do the same for the shared properties table, too
		for( it = m_IntrinsicProperties.begin(); it != m_IntrinsicProperties.end(); it++ )
		{
			if( !it->second->m_Inherited )
				continue;
			
			IJSComplexProperty* cp = m_Parent->HasProperty( it->first );

			if( cp && cp->m_AllowsInheritance )
			{
				debug_assert( cp->m_Intrinsic );
				it->second->ImmediateCopy( this, m_Parent, cp );
			}
		}

		// Now recurse.
		InheritorsList::iterator c;
		for( c = m_Inheritors.begin(); c != m_Inheritors.end(); c++ )
			(*c)->Rebuild();
	
	}
	IJSComplexProperty* HasProperty( CStrW PropertyName )
	{
		PropertyTable::iterator it;
		it = T::m_IntrinsicProperties.find( PropertyName );
		if( it != T::m_IntrinsicProperties.end() )
			return( it->second );

		it = m_Properties.find( PropertyName );
		if( it != m_Properties.end() )
			return( it->second );

		return( NULL );
	}
	void FillEnumerateSet( IteratorState* it, CStrW* PropertyRoot = NULL )
	{
		PropertyTable::iterator iit;
		if( PropertyRoot )
		{
			size_t rlen = PropertyRoot->Length();
			for( iit = m_Properties.begin(); iit != m_Properties.end(); iit++ )
				if( ( iit->first.Length() > rlen ) && ( iit->first.Left( rlen ) == *PropertyRoot ) && ( iit->first[rlen] == '.' ) )
					it->first.insert( iit->first.GetSubstring( rlen + 1, iit->first.Length() - rlen - 1 ).BeforeFirst( L"." ) );
		}
		else
		{
			for( iit = m_Properties.begin(); iit != m_Properties.end(); iit++ )
				it->first.insert( iit->first.BeforeFirst( L"." ) );
		}
		if( m_Parent )
			m_Parent->FillEnumerateSet( it, PropertyRoot );
	}
	void AddProperty( CStrW PropertyName, jsval Value )
	{
		DeletePreviouslyAssignedProperty( PropertyName );
		void* mem = jscomplexproperty_suballoc();
#include "nommgr.h"
		CJSDynamicComplexProperty* newProp = new(mem) CJSValComplexProperty( Value, false ); 
#include "mmgr.h"
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
	static void AddClassProperty( CStrW PropertyName, GetFn Getter, SetFn Setter = NULL )
	{
		T::m_IntrinsicProperties[PropertyName] = new CJSFunctionComplexProperty( Getter, Setter );
	}
	template<typename ReturnType, ReturnType (T::*NativeFunction)( JSContext* cx, uintN argc, jsval* argv )> 
		static void AddMethod( const char* Name, uintN MinArgs )
	{
		JSFunctionSpec FnInfo = { Name, CNativeComplexFunction<T, ReadOnly, ReturnType, NativeFunction>::JSFunction, MinArgs, 0, 0 };
		T::m_Methods.push_back( FnInfo );
	}
	template<typename PropType> static void AddClassProperty( CStrW PropertyName, PropType T::*Native, bool PropAllowInheritance = true, NotifyFn Update = NULL, NotifyFn Refresh = NULL )
	{
		T::m_IntrinsicProperties[PropertyName] = new CJSSharedProperty<PropType, ReadOnly>( (PropType IJSComplex::*)Native, PropAllowInheritance, Update, Refresh );
	}
	template<typename PropType> static void AddReadOnlyClassProperty( CStrW PropertyName, PropType T::*Native, bool PropAllowInheritance = true, NotifyFn Update = NULL, NotifyFn Refresh = NULL )
	{
		T::m_IntrinsicProperties[PropertyName] = new CJSSharedProperty<PropType, true>( (PropType IJSComplex::*)Native, PropAllowInheritance, Update, Refresh );
	}

	// helper routine for Add*Property. Their interface requires the
	// property not already exist; we check for this (in non-final builds)
	// and if so, warn and free the previously new-ed memory in
	// m_Properties[PropertyName] (avoids mem leak).
	void DeletePreviouslyAssignedProperty( CStrW PropertyName )
	{
#ifndef FINAL
		PropertyTable::iterator it;
		it = m_Properties.find( PropertyName );
		if( it != m_Properties.end() )
		{
			debug_warn("BUG: CJSComplexProperty added but already existed!");
			jscomplexproperty_suballoc_free(it->second);
		}
#else
		UNUSED2(PropertyName);
#endif
	}

	// PropertyName must not already exist! (verified in non-final release)
	template<typename PropType> void AddProperty( CStrW PropertyName, PropType* Native, bool PropAllowInheritance = true, NotifyFn Update = NULL, NotifyFn Refresh = NULL )
	{
		DeletePreviouslyAssignedProperty( PropertyName );
		void* mem = jscomplexproperty_suballoc();
#include "nommgr.h"
		m_Properties[PropertyName] = new(mem) CJSComplexProperty<PropType, ReadOnly>( Native, PropAllowInheritance, Update, Refresh );
#include "mmgr.h"
	}
	// PropertyName must not already exist! (verified in non-final release)
	template<typename PropType> void AddReadOnlyProperty( CStrW PropertyName, PropType* Native, bool PropAllowInheritance = true, NotifyFn Update = NULL, NotifyFn Refresh = NULL )
	{
		DeletePreviouslyAssignedProperty( PropertyName );
		void* mem = jscomplexproperty_suballoc();
#include "nommgr.h"
		m_Properties[PropertyName] = new(mem) CJSComplexProperty<PropType, true>( Native, PropAllowInheritance, Update, Refresh );
#include "mmgr.h"
	}
};

template<typename T, bool ReadOnly> JSClass CJSComplex<T, ReadOnly>::JSI_class = {
	NULL, JSCLASS_HAS_PRIVATE | JSCLASS_NEW_ENUMERATE,
	JS_PropertyStub, JS_PropertyStub,
	JSGetProperty, JSSetProperty,
	(JSEnumerateOp)JSEnumerate, JS_ResolveStub,
	JS_ConvertStub, DefaultFinalize,
	NULL, NULL, NULL, NULL 
};

template<typename T, bool ReadOnly> JSPropertySpec CJSComplex<T, ReadOnly>::JSI_props[] = {
	{ 0 },
};

template<typename T, bool ReadOnly> std::vector<JSFunctionSpec> CJSComplex<T, ReadOnly>::m_Methods;
template<typename T, bool ReadOnly> typename CJSComplex<T, ReadOnly>::PropertyTable CJSComplex<T, ReadOnly>::m_IntrinsicProperties;

template<typename T, bool ReadOnly> bool CJSComplex<T, ReadOnly>::GetProperty( JSContext* cx, CStrW PropertyName, jsval* vp )
{
	IJSComplexProperty* Property = HasProperty( PropertyName );
	if( Property && Property->m_Intrinsic )
	{
		*vp = Property->Get( cx, this );
	}
	else
	{
		CJSValComplexProperty* extProp;

		if( Property )
		{
			extProp = (CJSValComplexProperty*)Property;
			
			// If it's already a JS object, there's no point in creating
			// a PropertyAccessor for it; it can manage far better on its
			// own (this was why valueOf() was necessary)

			if( !JSVAL_IS_OBJECT( extProp->m_Data ) )
			{
				if( !extProp->m_JSAccessor )
				{
					extProp->m_JSAccessor = CJSComplexPropertyAccessor< CJSComplex<T, ReadOnly> >::CreateAccessor( cx, this, PropertyName );
					JS_AddNamedRoot( cx, &extProp->m_JSAccessor, "property accessor" );
				}

				*vp = OBJECT_TO_JSVAL( extProp->m_JSAccessor );
			}
			else
				*vp = extProp->m_Data;
		}
		else
		{
			// Check to see if it exists on a parent
			IJSComplex* check = m_Parent;
			while( check )
			{
				if( check->HasProperty( PropertyName ) ) break;
				check = check->m_Parent;
			}

			if( !check )
				return( false );

			// FIXME: Fiddle a way so this /doesn't/ require multiple kilobytes
			// of memory. Can't think of any better way to do it yet. Problem is
			// that script may access a property that isn't defined locally, but
			// is defined by an ancestor. We can't return an accessor to the
			// ancestor's property, because then if it's altered it affects that
			// object, not this. At the moment, creating a 'reflector' property
			// accessor that references /this/ object to be returned to script.

			// (N.B. Can't just put JSComplexs* in the table -> table entries can
			//  move -> root no longer refers to the JSObject.)

			ReflectorTable::iterator it;
			it = m_Reflectors.find( PropertyName );

			if( it == m_Reflectors.end() )
			{
				CJSReflector* reflector = new CJSReflector();
				reflector->m_JSAccessor = CJSComplexPropertyAccessor< CJSComplex<T, ReadOnly> >::CreateAccessor( cx, this, PropertyName );
				JS_AddRoot( cx, &reflector->m_JSAccessor );
				m_Reflectors.insert( std::pair<CStrW, CJSReflector*>( PropertyName, reflector ) );
				*vp = OBJECT_TO_JSVAL( reflector->m_JSAccessor );
			}
			else
				*vp = OBJECT_TO_JSVAL( it->second->m_JSAccessor );
		}
	}
	return( true );
}

#endif


