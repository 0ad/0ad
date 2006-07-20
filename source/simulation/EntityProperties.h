// EntityProperties.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Extended properties table, primarily intended for data-inheritable properties and those defined by JavaScript functions.
//
// Usage: These properties are accessed via functions in CEntity/CEntityTemplate
//

// TODO: Fix the silent failures of the conversion functions: need to work out what to do in these cases.

// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com

/*
#ifndef ENTITY_PROPERTIES_INCLUDED
#define ENTITY_PROPERTIES_INCLUDED

#include "ps/CStr.h"
#include "maths/Vector3D.h"
#include "ScriptObject.h"

#include "simulation/scripting/JSInterface_Entity.h"
#include "maths/scripting/JSInterface_Vector3D.h"

#if !GCC_VERSION



class IBoundPropertyOwner;
class CEntityTemplate;

// Property interface

class IBoundProperty
{
protected:
	IBoundPropertyOwner* m_owner;
	void (IBoundPropertyOwner::*m_updateFn)();
	virtual void set( const jsval value ) = 0;
public:
	void fromjsval( const jsval value );
	virtual jsval tojsval() = 0;
	virtual bool rebuild( IBoundProperty* parent, bool triggerFn = true ) = 0; // Returns true if the rebuild changed the value of this property.
	void associate( IBoundPropertyOwner* owner, const CStrW& name );
	void associate( IBoundPropertyOwner* owner, const CStrW& name, void (IBoundPropertyOwner::*updateFn)() );
	virtual ~IBoundProperty() {}
};

// Specialize at least:
// - jsval conversion functions (set, tojsval)

// Generic primitive one:

template<typename T> class CBoundProperty : public IBoundProperty
{
	T m_data;
	bool m_inherited;

public:
	CBoundProperty() { m_inherited = true; }
	CBoundProperty( const T& copy ) { m_inherited = false; m_data = copy; }
	operator T&() { return( m_data ); }
	operator const T&() const { return( m_data ); }
	T& operator=( const T& copy ) { m_inherited = false; return( m_data = copy ); }

	void set( const jsval value );
	jsval tojsval();
	bool rebuild( IBoundProperty* parent, bool triggerFn = true )
	{
		if( m_inherited && parent )
		{
			*this = *( (CBoundProperty<T>*)parent );
			if( triggerFn && m_updateFn )
				(m_owner->*m_updateFn)();
		}
		return( !m_inherited );
	}
};

// Generic class one:

template<typename T> class CBoundObjectProperty : public IBoundProperty, public T
{
	bool m_inherited;

public:
	CBoundObjectProperty() { m_inherited = true; }
	CBoundObjectProperty( const T& copy ) : T( copy ) { m_inherited = false; }

	T& operator=( const T& copy )
	{
		IBoundPropertyOwner* sv_owner = m_owner;
		void (IBoundPropertyOwner::*sv_updateFn)() = m_updateFn;

		(T&)*this = copy;

		m_owner = sv_owner;
		m_updateFn = sv_updateFn;
		m_inherited = false;

		return( *this );
	}
	void set( const jsval value );
	jsval tojsval();
	bool rebuild( IBoundProperty* parent, bool triggerFn = true )
	{
		if( m_inherited && parent )
		{
			// Save some properties so they won't be overwritten
			IBoundPropertyOwner* sv_owner = m_owner;
			void (IBoundPropertyOwner::*sv_updateFn)() = m_updateFn;

			*this = *( (CBoundObjectProperty<T>*)parent );

			m_owner = sv_owner;
			m_updateFn = sv_updateFn;
			m_inherited = true;

			if( triggerFn && m_updateFn )
				(m_owner->*m_updateFn)();
		}
		return( !m_inherited );
	}
};

// And an explicit one:

template<> class CBoundProperty<CEntityTemplate*> : public IBoundProperty
{
	CEntityTemplate* m_data;

public:
	CBoundProperty() { m_data = NULL; }
	CBoundProperty( CEntityTemplate* copy ) { m_data = copy; }

	operator CEntityTemplate*&() { return( m_data ); }
	operator CEntityTemplate*() const { return( m_data ); }
	CEntityTemplate*& operator=( CEntityTemplate* copy ) { return( m_data = copy ); }

	// Standard pointerish things

	CEntityTemplate& operator*() { return( *m_data ); }
	CEntityTemplate* operator->() { return( m_data ); }

	void set( const jsval value );
	jsval tojsval();
	bool rebuild( IBoundProperty* parent, bool triggerFn = true )
	{
		return( false ); // Can't inherit a CEntityTemplate*
	}
};

// A jsval property
template<> class CBoundProperty<jsval> : public IBoundProperty
{
	jsval m_data;
	bool m_inherited;
public:
	CBoundProperty() { m_data = JSVAL_NULL; m_inherited = true; RootJSVal(); }
	CBoundProperty( const jsval value ) { set( value ); m_inherited = false; RootJSVal(); }
	CBoundProperty( const CStrW& value ) 
	{
		m_data = STRING_TO_JSVAL( JS_NewUCStringCopyZ( g_ScriptingHost.getContext(), value ) );
		RootJSVal();
	}
	~CBoundProperty()
	{
		UprootJSVal();
	}
	void set( const jsval value ) { UprootJSVal(); m_data = value; m_inherited = false; RootJSVal(); }
	jsval tojsval() { return( m_data ); }
	bool rebuild( IBoundProperty* parent, bool triggerFn = true )
	{
		if( m_inherited && parent )
		{
			UprootJSVal();
			m_data = ( (CBoundProperty<jsval>*)parent )->m_data;
			RootJSVal();
		}
		return( !m_inherited );
	}
	void RootJSVal() { if( JSVAL_IS_GCTHING( m_data ) ) JS_AddRoot( g_ScriptingHost.GetContext(), &m_data ); }
	void UprootJSVal() { if( JSVAL_IS_GCTHING( m_data ) ) JS_RemoveRoot( g_ScriptingHost.GetContext(), &m_data ); }
};

#endif
*/
