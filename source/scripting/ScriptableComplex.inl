/*
Implementation of CJSComplex's functions and related helper functions and classes.
This file must be #included in any CPP file that accesses these functions directly,
but may be omitted in those that don't.

rationale:
we are changing CJSComplex often for purposes of optimization. this triggers
close to a full rebuild, which is unacceptable.
ideally, we would move the method implementations into a cpp file, and
then only that need be recompiled.
unfortunately that is exactly what the export keyword enables,
which is not likely to be implemented in VC.

several workarounds have been investigated.
1) reduce symptoms of the problem by reducing #includes of entity.h, which is
   what pulls CJSComplex in. done; it's been removed entirely from headers and
   is currently only left in ~30 cpp files (could probably be
   further reduced)

2) de-templatize CJSComplex. result would be enabling the normal split of
   interface vs. implementation in cpp file.

   this would be mostly feasible because it's really only used by
   CEntity and CEntityTemplate. however, since there are 2 users, the
   static class data (notably m_Methods) would have to be duplicated for each.
   one possibility would be an 'array' of each, indexed via class id.

   also, static data could be reduced by having property lookup be done
   via per-property hash tables, instead of the root object holding one
   large table of the entire property names (e.g. traits.health.max).
   in retrospect, this would probably have been better, but is probably
   too much work to change now.

3) instead of CEntity IS-A CJSComplex, use composition and pImpl. this would
   decouple entity.h from changes in scriptablecomplex.h.
   however, we'd have to dynamically allocate the CJSComplex in each entity
   (required by pImpl and less efficient/more annoying). also, there would
   potentially be trouble with ToJSVal, since we no longer derive from
   CJSComplex.
   this is not deemed worth the effort due to steps taken in #1.

We decided to split off the implementation of CJSComplex as well as many of its
helper classes into a separate header, ScriptableComplex.inl. This way, 
ScriptableComplex.h does not need to be modified unless we change the API,
but the implementations of CJSComplex's methods can be changed. However, this
also means that this header (ScriptableComplex.inl) must be #included in any
CPP file that directly accesses ScriptableComplex's methods - otherwise, the
linker won't find the definitions of these functions. Right now this is only
5 files, which results in much faster rebuilds after modifying this code.
*/

#ifndef SCRIPTABLE_COMPLEX_INL_INCLUDED
#define SCRIPTABLE_COMPLEX_INL_INCLUDED

#include "ScriptableComplex.h"

//-----------------------------------------------------------------------------
// CJSComplexPropertyAccessor
//-----------------------------------------------------------------------------


template<typename T, bool ReadOnly> class CJSComplex;

template<typename T> class CJSComplexPropertyAccessor
{
	T* m_Owner;
	CStrW m_PropertyRoot;
	template<typename Q, bool ReadOnly> friend class CJSComplex;

public:
	CJSComplexPropertyAccessor( T* Owner, const CStrW& PropertyRoot )
	{
		m_Owner = Owner;
		m_PropertyRoot = PropertyRoot;
	}

	static JSObject* CreateAccessor( JSContext* cx, T* Owner, const CStrW& PropertyRoot )
	{
		JSObject* Accessor = JS_NewObject( cx, &JSI_Class, NULL, NULL );
		JS_SetPrivate( cx, Accessor, new CJSComplexPropertyAccessor( Owner, PropertyRoot ) );

		return( Accessor );
	}

	// JW: ugly, but more efficient than previous approach
	// (string = root + "." + id;). saves 50ms during init.
	// made macro to ensure there is no extra overhead.
#define BUILD_PROPNAME(root, id)\
	PropName.reserve(50);\
	PropName += root;\
	PropName += '.';\
	PropName += id;

	static JSBool JSGetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CJSComplexPropertyAccessor* Instance = (CJSComplexPropertyAccessor*)JS_GetPrivate( cx, obj );
		if( !Instance ) return( JS_TRUE );

		CStrW PropName;
		BUILD_PROPNAME(Instance->m_PropertyRoot, g_ScriptingHost.ValueToUCString(id));

		Instance->m_Owner->GetProperty( cx, PropName, vp );

		return( JS_TRUE );
	}

	static JSBool JSSetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CJSComplexPropertyAccessor* Instance = (CJSComplexPropertyAccessor*)JS_GetPrivate( cx, obj );
		if( !Instance ) return( JS_TRUE );

		CStrW PropName;
		BUILD_PROPNAME(Instance->m_PropertyRoot, g_ScriptingHost.ValueToUCString(id));

		Instance->m_Owner->SetProperty( cx, PropName, vp );

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
					size_t rlen = Instance->m_PropertyRoot.length();

					IJSComplex::PropertyTable::iterator iit;
					for( iit = T::m_IntrinsicProperties.begin(); iit != T::m_IntrinsicProperties.end(); iit++ )
						if( ( iit->first.length() > rlen ) && ( iit->first.Left( rlen ) == Instance->m_PropertyRoot ) && ( iit->first[rlen] == '.' ) )
							it->first.insert( CStrW( iit->first.substr( rlen + 1 ) ).BeforeFirst( L"." ) );


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
			if( !JS_ValueToId( cx, ToJSVal<CStrW>( *( it->second ) ), idp ) )
				return( JS_FALSE );

			// EVIL HACK: since https://bugzilla.mozilla.org/show_bug.cgi?id=261887 (which is in
			// the SpiderMonkey 1.6 release, and not in 1.5), you can't enumerate properties that
			// don't actually exist on the object. This should probably be fixed by defining a custom
			// Resolve function to make them look like they exist, but for now we just define the
			// property on the object here so that it will exist by the time the JS iteration code
			// does its checks.
			JS_DefineProperty(cx, obj, CStr(*it->second).c_str(), JSVAL_VOID, NULL, NULL, 0);

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
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL
};


template<typename T>
void ScriptableComplex_InitComplexPropertyAccessor()
{
	CJSComplexPropertyAccessor<T>::ScriptingInit();
}


//-----------------------------------------------------------------------------
// various property types
//-----------------------------------------------------------------------------

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
	void ImmediateCopy( IJSComplex* CopyTo, IJSComplex* CopyFrom, IJSComplexProperty* CopyProperty )
	{
		CJSSharedProperty* otherProp = (CJSSharedProperty*) CopyProperty;
		T IJSComplex::*otherData = otherProp->m_Data;
		CopyTo->*m_Data = CopyFrom->*otherData;
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

//-----------------------------------------------------------------------------
// CJSComplex implementation
//-----------------------------------------------------------------------------

template<typename T, bool ReadOnly>
void CJSComplex<T, ReadOnly>::SetProperty( JSContext* cx, const CStrW& PropertyName, jsval* vp )
{
	if( !ReadOnly )
	{
		IJSComplexProperty* prop = HasProperty( PropertyName );

		if( prop )
		{
			// Already exists
			WatchNotify( cx, PropertyName, vp );
			prop->Set( cx, this, *vp );

			if(!prop->m_Intrinsic)
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


template<typename T, bool ReadOnly>
void CJSComplex<T, ReadOnly>::WatchNotify( JSContext* cx, const CStrW& PropertyName, jsval* newval )
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

template<typename T, bool ReadOnly>
JSBool CJSComplex<T, ReadOnly>::JSGetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	T* Instance = ToNative<T>( cx, obj );
	if( !Instance )
		return( JS_TRUE );

	CStrW PropName = g_ScriptingHost.ValueToUCString( id );

	if( !Instance->GetProperty( cx, PropName, vp ) )
		return( JS_TRUE );

	return( JS_TRUE );
}


template<typename T, bool ReadOnly>
JSBool CJSComplex<T, ReadOnly>::JSSetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	T* Instance = ToNative<T>( cx, obj );
	if( !Instance )
		return( JS_TRUE );

	CStrW PropName = g_ScriptingHost.ValueToUCString( id );

	Instance->SetProperty( cx, PropName, vp );

	return( JS_TRUE );
}


template<typename T, bool ReadOnly> 
JSBool CJSComplex<T, ReadOnly>::JSEnumerate( JSContext* cx, JSObject* obj, JSIterateOp enum_op, jsval* statep, jsid *idp )
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
		if( !JS_ValueToId( cx, ToJSVal<CStrW>( *( it->second ) ), idp ) )
			return( JS_FALSE );

		// EVIL HACK: see the comment in the other JSEnumerate
		JS_DefineProperty(cx, obj, CStr(*it->second).c_str(), JSVAL_VOID, NULL, NULL, 0);

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


template<typename T, bool ReadOnly>
JSBool CJSComplex<T, ReadOnly>::SetWatchAll( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
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


template<typename T, bool ReadOnly> 
JSBool CJSComplex<T, ReadOnly>::UnWatchAll( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
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


template<typename T, bool ReadOnly> 
void CJSComplex<T, ReadOnly>::ScriptingInit( const char* ClassName, JSNative Constructor, uintN ConstructorMinArgs )
{
	JSFunctionSpec* JSI_methods = new JSFunctionSpec[ m_Methods.size() + 3 ];
	size_t MethodID;
	for( MethodID = 0; MethodID < m_Methods.size(); MethodID++ )
		JSI_methods[MethodID] = m_Methods[MethodID];

	JSFunctionSpec watchAll = { "watchAll", SetWatchAll, 1, 0, 0 };
	JSI_methods[MethodID + 0] = watchAll;

	JSFunctionSpec unwatchAll = { "unwatchAll", UnWatchAll, 1, 0, 0 };
	JSI_methods[MethodID + 1] = unwatchAll;

	JSI_methods[MethodID + 2].name = 0;

	JSI_class.name = ClassName;

	g_ScriptingHost.DefineCustomObjectType( &JSI_class, Constructor, ConstructorMinArgs, JSI_props, JSI_methods, NULL, NULL );

	delete[]( JSI_methods );

	atexit( ScriptingShutdown );
}

template<typename T, bool ReadOnly>
void CJSComplex<T, ReadOnly>::ScriptingShutdown()
{
	PropertyTable::iterator it;
	for( it = m_IntrinsicProperties.begin(); it != m_IntrinsicProperties.end(); it++ )
		delete( it->second );
}


template<typename T, bool ReadOnly> 
void CJSComplex<T, ReadOnly>::DefaultFinalize( JSContext *cx, JSObject *obj )
{
	T* Instance = ToNative<T>( cx, obj );
	if( !Instance || Instance->m_EngineOwned )
		return;

	delete( Instance );
	JS_SetPrivate( cx, obj, NULL );
}

// Creating and releasing script objects is done automatically most of the time, but you
// can do it explicitly. 
template<typename T, bool ReadOnly>
void CJSComplex<T, ReadOnly>::CreateScriptObject()
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


template<typename T, bool ReadOnly> 
void CJSComplex<T, ReadOnly>::ReleaseScriptObject()
{
	if( m_JS )
	{
		JS_SetPrivate( g_ScriptingHost.GetContext(), m_JS, NULL );
		if( m_EngineOwned )
			JS_RemoveRoot( g_ScriptingHost.GetContext(), &m_JS );
		m_JS = NULL;
	}
}


template<typename T, bool ReadOnly> 
CJSComplex<T, ReadOnly>::CJSComplex()
{
	jscomplexproperty_suballoc_attach();

	m_Parent = NULL;
	m_JS = NULL;
	m_EngineOwned = true;
}


template<typename T, bool ReadOnly>
CJSComplex<T, ReadOnly>::~CJSComplex()
{
	Shutdown();

	jscomplexproperty_suballoc_detach();
}


template<typename T, bool ReadOnly> 
void CJSComplex<T, ReadOnly>::Shutdown()
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


template<typename T, bool ReadOnly>
void CJSComplex<T, ReadOnly>::SetBase( IJSComplex* Parent )
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

template<typename T, bool ReadOnly> 
void CJSComplex<T, ReadOnly>::Rebuild()
{
	PropertyTable::iterator it;
	// For each intrinsic property we have,
	for( it = m_Properties.begin(); it != m_Properties.end(); it++ )
	{
		const CStrW& prop_name = it->first;
		IJSComplexProperty* prop = it->second;

		if( !prop->m_Intrinsic || !prop->m_Inherited )
			continue;

		// Attempt to locate it in the parent
		IJSComplexProperty* parent_prop = m_Parent->HasProperty( prop_name );

		// If it doesn't have it, we've inherited from an object of a different type
		// This isn't allowed at the moment; but I don't have an totally convincing
		// reason for forbidding it entirely. Mind, I can't think of any use for it,
		// either.
		// If it can be inherited, inherit it.
		if( parent_prop && parent_prop->m_AllowsInheritance )
		{
			debug_assert( parent_prop->m_Intrinsic );
			prop->ImmediateCopy( this, m_Parent, parent_prop );
		}
	}
	// Do the same for the shared properties table, too
	for( it = m_IntrinsicProperties.begin(); it != m_IntrinsicProperties.end(); it++ )
	{
		const CStrW& prop_name = it->first;
		IJSComplexProperty* prop = it->second;

		if( !prop->m_Inherited )
			continue;

		IJSComplexProperty* parent_prop = m_Parent->HasProperty( prop_name );

		if( parent_prop && parent_prop->m_AllowsInheritance )
		{
			debug_assert( parent_prop->m_Intrinsic );
			prop->ImmediateCopy( this, m_Parent, parent_prop );
		}
	}

	// Now recurse.
	InheritorsList::iterator c;
	for( c = m_Inheritors.begin(); c != m_Inheritors.end(); c++ )
		(*c)->Rebuild();

}


template<typename T, bool ReadOnly>
IJSComplexProperty* CJSComplex<T, ReadOnly>::HasProperty( const CStrW& PropertyName )
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


template<typename T, bool ReadOnly>
void CJSComplex<T, ReadOnly>::FillEnumerateSet( IteratorState* it, CStrW* PropertyRoot)
{
	PropertyTable::iterator iit;
	if( PropertyRoot )
	{
		size_t rlen = PropertyRoot->length();
		for( iit = m_Properties.begin(); iit != m_Properties.end(); iit++ )
			if( ( iit->first.length() > rlen ) && ( iit->first.Left( rlen ) == *PropertyRoot ) && ( iit->first[rlen] == '.' ) )
				it->first.insert( CStrW( iit->first.substr( rlen + 1 ) ).BeforeFirst( L"." ) );
	}
	else
	{
		for( iit = m_Properties.begin(); iit != m_Properties.end(); iit++ )
			it->first.insert( iit->first.BeforeFirst( L"." ) );
	}
	if( m_Parent )
		m_Parent->FillEnumerateSet( it, PropertyRoot );
}


template<typename T, bool ReadOnly>
void CJSComplex<T, ReadOnly>::AddProperty( const CStrW& PropertyName, jsval Value )
{
	DeletePreviouslyAssignedProperty( PropertyName );
	void* mem = jscomplexproperty_suballoc();
	CJSDynamicComplexProperty* newProp = new(mem) CJSValComplexProperty( Value, false ); 
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

template<typename T, bool ReadOnly>
void CJSComplex<T, ReadOnly>::AddProperty( const CStrW& PropertyName, const CStrW& Value )
{
	AddProperty( PropertyName, JSParseString( Value ) );
}

template<typename T, bool ReadOnly>
void CJSComplex<T, ReadOnly>::AddClassProperty( const CStrW& PropertyName, GetFn Getter, SetFn Setter )
{
	T::m_IntrinsicProperties[PropertyName] = new CJSFunctionComplexProperty( Getter, Setter );
}

// helper routine for Add*Property. Their interface requires the
// property not already exist; we check for this (in debug builds)
// and if so, warn and free the previously new-ed memory in
// m_Properties[PropertyName] (avoids mem leak).
template<typename T, bool ReadOnly>
void CJSComplex<T, ReadOnly>::DeletePreviouslyAssignedProperty( const CStrW& PropertyName )
{
#ifdef NDEBUG
	UNUSED2(PropertyName);
#else
	PropertyTable::iterator it;
	it = m_Properties.find( PropertyName );
	if( it != m_Properties.end() )
	{
		debug_warn("BUG: CJSComplexProperty added but already existed!");
		jscomplexproperty_suballoc_free(it->second);
	}
#endif
}


template<typename T, bool ReadOnly> 
bool CJSComplex<T, ReadOnly>::GetProperty( JSContext* cx, const CStrW& PropertyName, jsval* vp )
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

template<typename T, bool ReadOnly, typename ReturnType, ReturnType (T::*NativeFunction)( JSContext* cx, uintN argc, jsval* argv )> 
void AddMethodImpl( const char* Name, uintN MinArgs )
{
	JSFunctionSpec FnInfo = { Name, CNativeComplexFunction<T, ReadOnly, ReturnType, NativeFunction>::JSFunction, MinArgs, 0, 0 };
	T::m_Methods.push_back( FnInfo );
}

template<typename T, bool ReadOnly, typename PropType>
void AddClassPropertyImpl( const CStrW& PropertyName, PropType T::*Native, bool PropAllowInheritance, IJSComplex::NotifyFn Update, IJSComplex::NotifyFn Refresh )
{
	T::m_IntrinsicProperties[PropertyName] = new CJSSharedProperty<PropType, ReadOnly>( (PropType IJSComplex::*)Native, PropAllowInheritance, Update, Refresh );
}

template<typename T, bool ReadOnly, typename PropType>
void AddReadOnlyClassPropertyImpl( const CStrW& PropertyName, PropType T::*Native, bool PropAllowInheritance, IJSComplex::NotifyFn Update, IJSComplex::NotifyFn Refresh )
{
	T::m_IntrinsicProperties[PropertyName] = new CJSSharedProperty<PropType, true>( (PropType IJSComplex::*)Native, PropAllowInheritance, Update, Refresh );
}

// PropertyName must not already exist! (verified in debug build)
template<typename T, bool ReadOnly, typename PropType>
void MemberAddPropertyImpl( IJSComplex* obj, const CStrW& PropertyName, PropType* Native, bool PropAllowInheritance, IJSComplex::NotifyFn Update, IJSComplex::NotifyFn Refresh )
{
	((T*)obj)->DeletePreviouslyAssignedProperty( PropertyName );
	void* mem = jscomplexproperty_suballoc();
	obj->m_Properties[PropertyName] = new(mem) CJSComplexProperty<PropType, ReadOnly>( Native, PropAllowInheritance, Update, Refresh );
}

// PropertyName must not already exist! (verified in debug build)
template<typename T, bool ReadOnly, typename PropType>
void MemberAddReadOnlyPropertyImpl( IJSComplex* obj, const CStrW& PropertyName, PropType* Native, bool PropAllowInheritance, IJSComplex::NotifyFn Update, IJSComplex::NotifyFn Refresh )
{
	((T*)obj)->DeletePreviouslyAssignedProperty( PropertyName );
	void* mem = jscomplexproperty_suballoc();
	obj->m_Properties[PropertyName] = new(mem) CJSComplexProperty<PropType, true>( Native, PropAllowInheritance, Update, Refresh );
}

#endif
