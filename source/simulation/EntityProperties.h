// EntityProperties.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Extended properties table, primarily intended for data-inheritable properties and those defined by JavaScript functions.
//
// Usage: Nothing yet.
//        These properties will be accessed via functions in CEntity/CBaseEntity
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

class IPropertyOwner;
class CBaseEntity;
class CProperty;
struct SProperty_NumericModifier;
struct SProperty_StringModifier;
struct SProperty_BooleanModifier;

// Property abstract.

class CProperty
{
protected:
	IPropertyOwner* m_owner;
	void (IPropertyOwner::*m_updateFn)();
	virtual void set( const jsval value ) = 0;
public:
	CProperty& operator=( const jsval value );
	virtual jsval tojsval() = 0;
	virtual bool rebuild( CProperty* parent, bool triggerFn = true ) = 0; // Returns true if the rebuild changed the value of this property.
	void associate( IPropertyOwner* owner, const CStr& name );
	void associate( IPropertyOwner* owner, const CStr& name, void (IPropertyOwner::*updateFn)() );
};

// Integer specialization

class CProperty_i32 : public CProperty
{
	i32 data;
	SProperty_NumericModifier* modifier;
public:
	CProperty_i32();
	~CProperty_i32();
	void set( const jsval value );
	jsval tojsval();
	bool rebuild( CProperty* parent, bool triggerFn = true );
	CProperty_i32& operator=( const i32 value );
	operator i32();
};

// Boolean specialization

class CProperty_bool : public CProperty
{
	bool data;
	SProperty_BooleanModifier* modifier;
public:
	CProperty_bool();
	~CProperty_bool();
	void set( const jsval value );
	jsval tojsval();
	bool rebuild( CProperty* parent, bool triggerFn = true );
	CProperty_bool& operator=( const bool value );
	operator bool();
};

// Floating-point specialization

class CProperty_float : public CProperty
{
	float data;
	SProperty_NumericModifier* modifier;
public:
	CProperty_float();
	~CProperty_float();
	void set( const jsval value );
	jsval tojsval();
	bool rebuild( CProperty* parent, bool triggerFn = true );
	CProperty_float& operator=( const float& value );
	operator float();
	operator bool();
	float operator+( float value );
	float operator-( float value );
	float operator*( float value );
	float operator/( float value );
	bool operator<( float value );
	bool operator>( float value );
	bool operator==( float value );
};

// String specialization

class CProperty_CStr : public CProperty, public CStr
{
	SProperty_StringModifier* modifier;
public:
	CProperty_CStr();
	~CProperty_CStr();
	void set( const jsval value );
	jsval tojsval();
	bool rebuild( CProperty* parent, bool triggerFn = true );
	CProperty_CStr& operator=( const CStr& value );
};

// 3-Vector specialization

class CProperty_CVector3D : public CProperty, public CVector3D
{
public:
	void set( const jsval value );
	jsval tojsval();
	bool rebuild( CProperty* parent, bool triggerFn = true );
	CProperty_CVector3D& operator=( const CVector3D& value );
};

// CBaseEntity* specialization

class CProperty_CBaseEntityPtr : public CProperty
{
	CBaseEntity* data;
public:
	void set( const jsval value );
	jsval tojsval();
	bool rebuild( CProperty* parent, bool triggerFn = true );
	operator CBaseEntity*();
	operator bool();
	CBaseEntity& operator*() const;
	CBaseEntity* operator->() const;
	CProperty_CBaseEntityPtr& operator=( CBaseEntity* value );
};

// e.g. Entities and their templates.
class IPropertyOwner
{
public:
	CProperty_CBaseEntityPtr m_base;
	STL_HASH_MAP<CStr,CProperty*,CStr_hash_compare> m_properties;
	std::vector<IPropertyOwner*> m_inheritors;
	void rebuild( CStr propName ); // Recursively rebuild just the named property over the inheritance tree.
	void rebuild(); // Recursively rebuild everything over the inheritance tree.
};

struct SProperty_NumericModifier
{
	float multiplicative;
	float additive;
	void operator=( float value )
	{
		multiplicative = 0.0f;
		additive = value;
	}
};

struct SProperty_StringModifier
{
	CStr replacement;
	void operator=( const CStr& value )
	{
		replacement = value;
	}
};

struct SProperty_BooleanModifier
{
	bool replacement;
	void operator=( const bool value )
	{
		replacement = value;
	}
};

#endif

