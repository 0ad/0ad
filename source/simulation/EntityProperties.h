// EntityProperties.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Extended properties table, primarily intended for data-inheritable properties and those defined by JavaScript functions.
//
// Usage: These properties are accessed via functions in CEntity/CBaseEntity
//

// TODO: Fix the silent failures of the conversion functions: need to work out what to do in these cases.

// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com

#ifndef ENTITY_PROPERTIES_INCLUDED
#define ENTITY_PROPERTIES_INCLUDED

#include "CStr.h"
#include "Vector3D.h"
#include "scripting/ScriptingHost.h"
#include "scripting/JSInterface_Entity.h"
#include "scripting/JSInterface_Vector3D.h"

#ifndef __GNUC__

# include <hash_map>

# if( defined( _MSC_VER ) && ( _MSC_VER >= 1300 ) )
#  define STL_HASH_MAP stdext::hash_map
# else
#  define STL_HASH_MAP std::hash_map
# endif //( defined( _MSC_VER ) && ( _MSC_VER >= 1300 ) )

#else // #ifndef __GNUC__

# include <ext/hash_map>
# define STL_HASH_MAP __gnu_cxx::hash_map

#endif

class IBoundPropertyOwner;
class CBaseEntity;
class CBoundPropertyModifier;

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
};

// Specialize at least:
// - jsval conversion functions (set, tojsval)

// Generic primitive one:

template<typename T> class CBoundProperty : public IBoundProperty
{
	T m_data;
	bool m_inherited;
	void (IBoundPropertyOwner::*m_updateFn)();

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
	void (IBoundPropertyOwner::*m_updateFn)();

public:
	CBoundObjectProperty() { m_inherited = true; }
	CBoundObjectProperty( const T& copy ) : T( copy ) { m_inherited = false; }

	void set( const jsval value );
	jsval tojsval();
	bool rebuild( IBoundProperty* parent, bool triggerFn = true )
	{
		if( m_inherited && parent )
		{
			*this = *( (CBoundObjectProperty<T>*)parent );
			if( triggerFn && m_updateFn )
				(m_owner->*m_updateFn)();
		}
		return( !m_inherited );
	}
};

// And an explicit one:

template<> class CBoundProperty<CBaseEntity*> : public IBoundProperty
{
	CBaseEntity* m_data;
	void (IBoundPropertyOwner::*m_updateFn)();

public:
	CBoundProperty() { m_data = NULL; }
	CBoundProperty( CBaseEntity* copy ) { m_data = copy; }

	operator CBaseEntity*&() { return( m_data ); }
	operator CBaseEntity*() const { return( m_data ); }
	CBaseEntity*& operator=( CBaseEntity* copy ) { return( m_data = copy ); }

// Standard pointerish things

	CBaseEntity& operator*() { return( *m_data ); }
	CBaseEntity* operator->() { return( m_data ); }
	
	/*
	CBoundProperty( uintptr_t ptr ) { m_data = (CBaseEntity*)ptr; }
	CBaseEntity*& operator=( uintptr_t ptr ) { m_data = (CBaseEntity*)ptr; }
	operator uintptr_t() { return( (uintptr_t)m_data ); }
	*/

	void set( const jsval value );
	jsval tojsval();
	bool rebuild( IBoundProperty* parent, bool triggerFn = true )
	{
		return( false ); // Can't inherit a CBaseEntity*
	}
};

// e.g. Entities and their templates.

class IBoundPropertyOwner
{
public:
	CBoundProperty<CBaseEntity*> m_base;
	STL_HASH_MAP<CStrW,IBoundProperty*,CStrW_hash_compare> m_properties;
	std::vector<IBoundPropertyOwner*> m_inheritors;
	void rebuild( CStrW propName ); // Recursively rebuild just the named property over the inheritance tree.
	void rebuild(); // Recursively rebuild everything over the inheritance tree.
};

#endif

