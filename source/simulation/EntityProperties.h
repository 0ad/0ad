// EntityProperties.h
//
// Last modified: 22 May 04, Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
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

#if( defined( _MSC_VER ) && ( _MSC_VER >= 1300 ) )

#define STL_HASH_MAP stdext::hash_map

#else

#define STL_HASH_MAP std::hash_map

#endif //( defined( _MSC_VER ) && ( _MSC_VER >= 1300 ) )

#include "CStr.h"
#include "Vector3D.h"

#include <hash_map>

class CGenericProperty
{
public:
	enum EPropTypes
	{
		PROP_INTRINSIC = 256,
		PROP_TYPELOCKED = 512,
		PROP_STRIPFLAGS = 255,
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
	EPropTypes m_type;
private:
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
	~CGenericProperty();
	void releaseData();

	/*
	CGenericProperty( i32 value ); // Create an integer property with a given value.
	CGenericProperty( i32* value );  // Create an integer property that points to the given variable.
	CGenericProperty( float value ); // Create a floating-point property with a given value.
	CGenericProperty( float* value ); // Create a floating-point property that points to the given variable.
	CGenericProperty( CStr& value ); // Create a string object property that's initialized to a copy of the given string.
	CGenericProperty( CStr* value ); // Create a string object property that points to the given variable.
	CGenericProperty( CVector3D& value ); // Create a vector object property that's initialized to a copy of the given vector.
	CGenericProperty( CVector3D* value ); // Create a vector object property that points to the given variable.
	CGenericProperty( void* value ); // Create a general property that points to the given value.
	*/

	// Associator functions: Links the property with the specified engine variable.
	void associate( i32* value ); 
	void associate( float* value ); 
	void associate( CStr* value );
	void associate( CVector3D* value );
	
	// Getter functions: Attempts to convert the property to the given type.
	operator i32(); // Convert to an integer if possible (integer, float, some strings), otherwise returns 0.
	operator float(); // Convert to a float if possible (integer, float, some strings), otherwise returns 0.0f.
	operator CStr(); // Convert to a string if possible (all except generic pointer), otherwise returns CStr().
	operator CVector3D(); // If this property is a vector, returns that vector, otherwise returns CVector3D().
	operator void*(); // If this property is a generic pointer, returns that pointer, otherwise returns NULL.

	// Setter functions: If this is a typelocked property, attempts to convert the given data
	//					 into the appropriate type, otherwise setting the associated value to 0, 0.0f, CStr() or CVector3D().
	//					 If this property is typeloose, converts this property into one of the same type
	//					 as the given value, then stores that value in this property.
	CGenericProperty& operator=( i32 value );
	CGenericProperty& operator=( float value );
	CGenericProperty& operator=( CStr& value );
	CGenericProperty& operator=( CVector3D& value );
	CGenericProperty& operator=( void* value ); // Be careful with this one. A lot of things will cast to void*.

	// Typelock functions. Use these when you want to make sure the property has the given type.
	void typelock( EPropTypes type );
	void typeloose();

private:
	// resolve-as functions. References the data, whereever it is.
	i32& asInteger();
	float& asFloat();
	CStr& asString();
	CVector3D& asVector();

	// to functions. Convert whatever this is now to the chosen type.
	i32 toInteger();
	float toFloat();
	CStr toString();
	CVector3D toVector();
	void* toVoid();

	// from functions. Convert the given value to whatever type this is now.
	void fromInteger( i32 value );
	void fromFloat( float value );
	void fromString( CStr& value );
	void fromVector( CVector3D& value );
	void fromVoid( void* value );
};

#endif

