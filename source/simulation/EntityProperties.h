// EntityProperties.h
//
// Last modified: 22 May 04, Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Extended properties table, primarily intended for data-inheritable properties and those defined by JavaScript functions.
//
// Usage: Nothing yet.
//        These properties will be accessed via functions in CEntity/CBaseEntity
//
// Inefficiency warning.
// Will move frequently accessed properties (position, name, etc.) to native C++ primitives.
// Just playing around with this idea, will probably keep it for at least some of the more exotic and/or user-defined properties.

// TODO: Fix the silent failures of the conversion functions: need to work out what to do in these cases.

// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com

#ifndef ENTITY_PROPERTIES_INCLUDED
#define ENTITY_PROPERTIES_INCLUDED

#include "CStr.h"
#include "Vector3D.h"

#include <hash_map>

class CGenericProperty
{
public:
	enum EPropTypes
	{
		PROP_INTRINSIC = 256,
		PROP_INTEGER = 0,
		PROP_FLOAT,
		PROP_STRING,
		PROP_VECTOR,
		PROP_PTR,
		PROP_INTEGER_INTRINSIC = PROP_INTEGER | PROP_INTRINSIC,
		PROP_FLOAT_INTRINSIC = PROP_FLOAT | PROP_INTRINSIC,
		PROP_STRING_INTRINSIC = PROP_STRING | PROP_INTRINSIC,
		PROP_VECTOR_INTRINSIC = PROP_VECTOR | PROP_INTRINSIC
	};
private:
	EPropTypes m_type;
	union
	{
		i32 m_integer;
		i32* m_integerptr;
		float m_float;
		float* m_floatptr;
		CStr* m_string;
		CVector3D* m_vector;
		void* m_ptr;
	};
public:
	CGenericProperty(); // Create an integer property containing 0.
	CGenericProperty( i32 value ); // Create an integer property with a given value.
	CGenericProperty( i32* value );  // Create an integer property that points to the given variable.
	CGenericProperty( float value ); // Create a floating-point property with a given value.
	CGenericProperty( float* value ); // Create a floating-point property that points to the given variable.
	CGenericProperty( CStr& value ); // Create a string object property that's initialized to a copy of the given string.
	CGenericProperty( CStr* value ); // Create a string object property that points to the given variable.
	CGenericProperty( CVector3D& value ); // Create a vector object property that's initialized to a copy of the given vector.
	CGenericProperty( CVector3D* value ); // Create a vector object property that points to the given variable.
	CGenericProperty( void* value ); // Create a general property that points to the given value.
	~CGenericProperty();
	operator i32(); // Convert to an integer if possible (integer, float, some strings), otherwise returns 0.
	operator float(); // Convert to a float if possible (integer, float, some strings), otherwise returns 0.0f.
	operator CStr(); // Convert to a string if possible (all except generic pointer), otherwise returns CStr().
	operator CVector3D(); // If this property is a vector, returns that vector, otherwise returns CVector3D().
	operator void*(); // If this property is a generic pointer, returns that pointer, otherwise returns NULL.
};

#endif

