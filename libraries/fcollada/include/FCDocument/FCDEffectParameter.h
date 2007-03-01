/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDEffectParameter.h
	This file contains the FCDEffectParameter interface and the simpler of its derivate classes:
	FCDEffectParameterString, FCDEffectParameterFloat, FCDEffectParameterVector...
*/

#ifndef _FCD_EFFECT_PARAMETER_H_
#define _FCD_EFFECT_PARAMETER_H_

#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_

class FCDocument;
class FCDEffectParameterAnnotation;
class FCDEffectParameterList;
typedef fm::pvector<FCDEffectParameterAnnotation> FCDEffectParameterAnnotationList;

/**
	A COLLADA effect parameter.

	This interface class is used to define all the valid
	ColladaFX parameter types. There are many types of
	parameters: integers, booleans, floating-point
	values, 2D, 3D and 4D vectors of floating-point values,
	matrices, strings, surfaces and their samplers.

	A COLLADA effect parameter may generate a new
	effect parameter, in which case it will declare a semantic
	and a reference: to represent it within the COLLADA document.

	@ingroup FCDEffect
*/
class FCOLLADA_EXPORT FCDEffectParameter : public FCDObject
{
public:
	/** The type of the effect parameter class. */
	enum Type
	{
		SAMPLER, /**< A sampler effect parameter. Points towards a surface parameter and adds extra texturing parameters. */
		INTEGER, /**< A single integer effect parameter. */
		BOOLEAN, /**< A single boolean effect parameter. */
		FLOAT, /**< A single floating-pointer value effect parameter. */
		FLOAT2, /**< A 2D vector of floating-pointer values. */
		FLOAT3, /**< A 3D vector of floating-pointer values. */
		VECTOR, /**< A 4D vector of floating-pointer values. */
		MATRIX, /**< A 4x4 matrix. */
		STRING, /**< A string effect parameter. */
		SURFACE /**< A surface effect parameter. Contains a COLLADA image pointer. */
	};

private:
	DeclareObjectType(FCDObject);
	bool isGenerator; // whether this effect parameter structure generates a new value or modifies an existing value (is <newparam>?)
	fm::string reference;
	fm::string semantic; // this is a COLLADA Semantic, not a Cg semantic
	FCDEffectParameterAnnotationList annotations;
	
public:
	/** Constructor: do not use directly.
		Instead, use the FCDEffectParameterList::AddParameter function.
		@param document The COLLADA document that owns the effect parameter. */
	FCDEffectParameter(FCDocument* document);

	/** Destructor. */
	virtual ~FCDEffectParameter();

	/** Retrieves the type of effect parameter class.
		@return The type of the effect parameter class.*/
	virtual Type GetType() const = 0;

	/** Retrieves the reference for this effect parameter.
		In the case of generators, the reference string contains the sub-id.
		@return The reference. */
	const fm::string& GetReference() const { return reference; }

	/** Sets the reference for the effect parameter.
		In the case of generators, the reference string contains the sub-id.
		@param _reference The reference. */
	void SetReference(const char* _reference) { reference = _reference; SetDirtyFlag(); }
	void SetReference(const fm::string& _reference) { reference = _reference; SetDirtyFlag(); } /**< See above. */

	/** Retrieves the semantic for this effect parameter.
		@return The semantic. */
	const fm::string& GetSemantic() const { return semantic; }

	/** Sets the semantic for this effect parameter.
		@param _semantic The semantic. */
	void SetSemantic(const fm::string& _semantic) { semantic = _semantic; SetDirtyFlag(); } 

	/** Retrieves whether this effect parameter is a parameter generator.
		A ColladaFX parameter must be generated to be modified or bound at
		higher abstraction levels.
		@return Whether this is a generator. */
	bool IsGenerator() const { return isGenerator; }

	/** Sets this effect parameter as a generator. */
	void SetGenerator() { isGenerator = true; SetDirtyFlag(); }

	/** Retrieves whether this effect parameter is a parameter modifier.
		A ColladaFX parameter must be generated to be modified or bound at
		higher abstraction levels.
		@return Whether this is a modifier. */
	bool IsModifier() const { return !isGenerator; }

	/** Sets this effect parameter as a modified. */
	void SetModifier() { isGenerator = false; SetDirtyFlag(); }

	/**	Retrieves the list of annotations for this parameter.
		@return The list of annotations. */
	FCDEffectParameterAnnotationList& GetAnnotations() { return annotations; }
	const FCDEffectParameterAnnotationList& GetAnnotations() const { return annotations; }
	
	/** Retrieves the number of annotations for this parameter.
		@return The number of annotations. */
	size_t GetAnnotationCount() const { return annotations.size(); }

	/** Retrieves an annotation of this parameter.
		@param index The index of the annotation.
		@return The annotation for the given index. This pointer will be NULL
			if the index is out-of-bounds. */
	FCDEffectParameterAnnotation* GetAnnotation(size_t index) { FUAssert(index < GetAnnotationCount(), return NULL); return annotations.at(index); }
	const FCDEffectParameterAnnotation* GetAnnotation(size_t index) const { FUAssert(index < GetAnnotationCount(), return NULL); return annotations.at(index); } /**< See above. */

	/** Adds a blank annotation to this parameter.
		@return The blank annotation. */
	FCDEffectParameterAnnotation* AddAnnotation();

	/** Adds an annotation to this parameter.
		@param name The name of the annotation.
		@param value The value of the annotation. */
	void AddAnnotation(const fchar* name, FCDEffectParameter::Type type, const fchar* value);
	void AddAnnotation(const fstring& name, FCDEffectParameter::Type type, const fchar* value) { AddAnnotation(name.c_str(), type, value); } /**< See above. */
	void AddAnnotation(const fchar* name, FCDEffectParameter::Type type, const fstring& value) { AddAnnotation(name, type, value.c_str()); } /**< See above. */
	void AddAnnotation(const fstring& name, FCDEffectParameter::Type type, const fstring& value) { AddAnnotation(name.c_str(), type, value.c_str()); } /**< See above. */
	template <class T> void AddAnnotation(const fchar* name, FCDEffectParameter::Type type, const T& value) { globalBuilder.set(value); AddAnnotation(name, type, globalBuilder.ToCharPtr()); } /**< See above. */
	template <class T> void AddAnnotation(const fstring& name, FCDEffectParameter::Type type, const T& value) { globalBuilder.set(value); AddAnnotation(name.c_str(), type, globalBuilder.ToCharPtr()); } /**< See above. */

	/** Releases an annotation of this parameter.
		@param annotation The annotation to release. */
	void ReleaseAnnotation(FCDEffectParameterAnnotation* annotation) { annotations.release(annotation); SetDirtyFlag(); }

	/** Compares this parameter's value with another
		@param parameter The given parameter to compare with.
		@return true if the values are equal */
	virtual bool IsValueEqual( FCDEffectParameter *parameter ) = 0;

	/** Creates a full copy of the effect parameter.
		@param clone The cloned effect parameter. If this pointer is NULL,
			a new effect parameter will be created and you
			will need to delete this pointer.
		@return The cloned effect parameter. */
	virtual FCDEffectParameter* Clone(FCDEffectParameter* clone = NULL) const;

	/** [INTERNAL] Overwrites the target parameter with this parameter.
		This function is used during the flattening of materials.
		@param target The target parameter to overwrite. */
	virtual void Overwrite(FCDEffectParameter* target);

	/** [INTERNAL] Reads in the effect parameter from a given COLLADA XML tree node.
		@param parameterNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the parameter.*/
	virtual bool LoadFromXML(xmlNode* parameterNode);

	/** [INTERNAL] Links the parameter. This is done after
		the whole COLLADA document has been processed once.
		@param parameters The list of parameters available at this abstraction level. */
	virtual void Link(FCDEffectParameterList& parameters);

	/** [INTERNAL] Writes out the effect parameter to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the parameter.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

/**
	A COLLADA integer effect parameter.
	Contains a single, non-animated integer.
*/
class FCOLLADA_EXPORT FCDEffectParameterInt : public FCDEffectParameter
{
private:
	DeclareObjectType(FCDEffectParameter);
	int value;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDEffectParameterList::AddParameter function.
		@param document The COLLADA document that owns the effect parameter. */
	FCDEffectParameterInt(FCDocument* document);

	/** Destructor. */
	virtual ~FCDEffectParameterInt();

	/** Retrieves the type of effect parameter class.
		@return The parameter class type: INTEGER. */
	virtual Type GetType() const { return INTEGER; }

	/** Retrieves the value of the effect parameter.
		@return The integer value. */
	int GetValue() const { return value; }

	/** Sets the integer value of the effect parameter.
		@param _value The integer value. */
	void SetValue(int _value) { value = _value; SetDirtyFlag(); }

	/** Compares this parameter's value with another
		@param parameter The given parameter to compare with.
		@return true if the values are equal */
	virtual bool IsValueEqual( FCDEffectParameter *parameter );

	/** Creates a full copy of the effect parameter.
		@param clone The cloned effect parameter. If this pointer is NULL,
			a new effect parameter will be created and you
			will need to delete this pointer.
		@return The cloned effect parameter. */
	virtual FCDEffectParameter* Clone(FCDEffectParameter* clone = NULL) const;

	/** [INTERNAL] Overwrites the target parameter with this parameter.
		This function is used during the flattening of materials.
		@param target The target parameter to overwrite. */
	virtual void Overwrite(FCDEffectParameter* target);

	/** [INTERNAL] Reads in the effect parameter from a given COLLADA XML tree node.
		@param parameterNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false'
			it may be dangerous to extract information from the parameter.*/
	virtual bool LoadFromXML(xmlNode* parameterNode);

	/** [INTERNAL] Writes out the effect parameter to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the parameter.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

/**
	A COLLADA boolean effect parameter.
	Contains a single, unanimated boolean.
*/
class FCOLLADA_EXPORT FCDEffectParameterBool : public FCDEffectParameter
{
private:
	DeclareObjectType(FCDEffectParameter);
	bool value;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDEffectParameterList::AddParameter function.
		@param document The COLLADA document that owns the effect parameter. */
	FCDEffectParameterBool(FCDocument* document);

	/** Destructor. */
	virtual ~FCDEffectParameterBool();

	/** Retrieves the type of effect parameter class.
		@return The parameter class type: BOOLEAN. */
	virtual Type GetType() const { return BOOLEAN; }

	/** Retrieves the boolean value of the effect parameter.
		@return The boolean value. */
	bool GetValue() const { return value; }

	/** Sets the boolean value of the effect parameter.
		@param _value The boolean value. */
	void SetValue(bool _value) { value = _value; SetDirtyFlag(); }

	/** Compares this parameter's value with another
		@param parameter The given parameter to compare with.
		@return true if the values are equal */
	virtual bool IsValueEqual( FCDEffectParameter *parameter );

	/** Creates a full copy of the effect parameter.
		@param clone The cloned effect parameter. If this pointer is NULL,
			a new effect parameter will be created and you
			will need to delete this pointer.
		@return The cloned effect parameter. */
	virtual FCDEffectParameter* Clone(FCDEffectParameter* clone = NULL) const;

	/** [INTERNAL] Overwrites the target parameter with this parameter.
		This function is used during the flattening of materials.
		@param target The target parameter to overwrite. */
	virtual void Overwrite(FCDEffectParameter* target);

	/** [INTERNAL] Reads in the effect parameter from a given COLLADA XML tree node.
		@param parameterNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the parameter.*/
	virtual bool LoadFromXML(xmlNode* parameterNode);

	/** [INTERNAL] Writes out the effect parameter to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the parameter.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

/**
	A COLLADA string effect parameter.
	Contains a single, non-animated string.
*/
class FCOLLADA_EXPORT FCDEffectParameterString : public FCDEffectParameter
{
private:
	DeclareObjectType(FCDEffectParameter);
	fm::string value;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDEffectParameterList::AddParameter function.
		@param document The COLLADA document that owns the effect parameter. */
	FCDEffectParameterString(FCDocument* document);

	/** Destructor. */
	virtual ~FCDEffectParameterString();

	/** Retrieves the type of effect parameter class.
		@return The parameter class type: STRING. */
	virtual Type GetType() const { return STRING; }

	/** Retrieves the string contained in the effect parameter.
		@return The string. */
	const fm::string& GetValue() const { return value; }

	/** Sets the string contained in the effect parameter.
		@param _value The string. */
	void SetValue(const fm::string& _value) { value = _value; SetDirtyFlag(); }

	/** Compares this parameter's value with another
		@param parameter The given parameter to compare with.
		@return true if the values are equal */
	virtual bool IsValueEqual(FCDEffectParameter *parameter);

	/** Creates a full copy of the effect parameter.
		@param clone The cloned effect parameter. If this pointer is NULL,
			a new effect parameter will be created and you
			will need to delete this pointer.
		@return The cloned effect parameter. */
	virtual FCDEffectParameter* Clone(FCDEffectParameter* clone = NULL) const;

	/** [INTERNAL] Overwrites the target parameter with this parameter.
		This function is used during the flattening of materials.
		@param target The target parameter to overwrite. */
	virtual void Overwrite(FCDEffectParameter* target);

	/** [INTERNAL] Reads in the effect parameter from a given COLLADA XML tree node.
		@param parameterNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the parameter.*/
	virtual bool LoadFromXML(xmlNode* parameterNode);

	/** [INTERNAL] Writes out the effect parameter to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the parameter.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

/**
	A COLLADA float effect parameter.
	Contains a single, possibly animated, floating-point value.
	The type of the floating-point value may be HALF or FLOAT.
*/
class FCOLLADA_EXPORT FCDEffectParameterFloat : public FCDEffectParameter
{
public:
	/** The supported types of float-point values. */
	enum FloatType
	{
		FLOAT, /** A full-fledged floating-point value. This is the default. */
		HALF /** Probably implies a 16-bit floating-point value. */
	};

private:
	DeclareObjectType(FCDEffectParameter);
	FloatType floatType;
	float value;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDEffectParameterList::AddParameter function.
		@param document The COLLADA document that owns the effect parameter. */
	FCDEffectParameterFloat(FCDocument* document);

	/** Destructor. */
	virtual ~FCDEffectParameterFloat();

	/** Retrieves the type of effect parameter class.
		@return The parameter class type: FLOAT. */
	virtual FCDEffectParameter::Type GetType() const { return FCDEffectParameter::FLOAT; }

	/** Retrieves the type of floating-point value held by this effect parameter.
		@return The type of floating-point value. */
	FloatType GetFloatType() const { return floatType; }

	/** Sets the type of floating-point value held by this effect parameter.
		@param type The type of floating-point value. */
	void SetFloatType(FloatType type) { floatType = type; SetDirtyFlag(); }

	/** Retrieves the floating-point value of the effect parameter.
		@return The floating-point value. */
	float& GetValue() { return value; }
	const float& GetValue() const { return value; } /**< See above. */

	/** Sets the floating-point value of the effect parameter.
		@param _value The floating-point value. */
	void SetValue(float _value) { value = _value; SetDirtyFlag(); }

	/** Compares this parameter's value with another
		@param parameter The given parameter to compare with.
		@return true if the values are equal */
	virtual bool IsValueEqual( FCDEffectParameter *parameter );

	/** Creates a full copy of the effect parameter.
		@param clone The cloned effect parameter. If this pointer is NULL,
			a new effect parameter will be created and you
			will need to delete this pointer.
		@return The cloned effect parameter. */
	virtual FCDEffectParameter* Clone(FCDEffectParameter* clone = NULL) const;

	/** [INTERNAL] Overwrites the target parameter with this parameter.
		This function is used during the flattening of materials.
		@param target The target parameter to overwrite. */
	virtual void Overwrite(FCDEffectParameter* target);

	/** [INTERNAL] Reads in the effect parameter from a given COLLADA XML tree node.
		@param parameterNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the parameter.*/
	virtual bool LoadFromXML(xmlNode* parameterNode);

	/** [INTERNAL] Writes out the effect parameter to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the parameter.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

/**
	A COLLADA 2D vector of floats.
	Contains two, possibly animated, floating-point values.
	The type of the floating-point values may be HALF or FLOAT.
*/
class FCOLLADA_EXPORT FCDEffectParameterFloat2 : public FCDEffectParameter
{
public:
	/** The supported types of float-point values. */
	enum FloatType
	{
		FLOAT, /** A full-fledged floating-point value. This is the default. */
		HALF /** Probably implies a 16-bit floating-point value. */
	};

private:
	DeclareObjectType(FCDEffectParameter);
	FloatType floatType;
	float value_x;
	float value_y;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDEffectParameterList::AddParameter function.
		@param document The COLLADA document that owns the effect parameter. */
	FCDEffectParameterFloat2(FCDocument* document);

	/** Destructor. */
	virtual ~FCDEffectParameterFloat2();

	/** Retrieves the type of effect parameter class.
		@return The parameter class type: FLOAT2. */
	virtual Type GetType() const { return FLOAT2; }

	/** Retrieves the type of floating-point value held by this effect parameter.
		@return The type of floating-point value. */
	FloatType GetFloatType() const { return floatType; }

	/** Sets the type of floating-point value held by this effect parameter.
		@param type The type of floating-point value. */
	void SetFloatType(FloatType type) { floatType = type; SetDirtyFlag(); }

	/** Retrieves the first floating-point value of the effect parameter.
		@return The first floating-point value. */
	float& GetValueX() { return value_x; }
	const float& GetValueX() const { return value_x; } /**< See above. */

	/** Sets the first floating-point value of the effect parameter.
		@param value The first floating-point value. */
	void SetValueX(float value) { value_x = value; SetDirtyFlag(); }

	/** Retrieves the second floating-point value of the effect parameter.
		@return The second floating-point value. */
	float& GetValueY() { return value_y; }
	const float& GetValueY() const { return value_y; } /**< See above. */

	/** Sets the second floating-point value of the effect parameter.
		@param value The second floating-point value. */
	void SetValueY(float value) { value_y = value; SetDirtyFlag(); }

	/** Compares this parameter's value with another
		@param parameter The given parameter to compare with.
		@return true if the values are equal */
	virtual bool IsValueEqual( FCDEffectParameter *parameter );

	/** Creates a full copy of the effect parameter.
		@param clone The cloned effect parameter. If this pointer is NULL,
			a new effect parameter will be created and you
			will need to delete this pointer.
		@return The cloned effect parameter. */
	virtual FCDEffectParameter* Clone(FCDEffectParameter* clone = NULL) const;

	/** [INTERNAL] Overwrites the target parameter with this parameter.
		This function is used during the flattening of materials.
		@param target The target parameter to overwrite. */
	virtual void Overwrite(FCDEffectParameter* target);

	/** [INTERNAL] Reads in the effect parameter from a given COLLADA XML tree node.
		@param parameterNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the parameter.*/
	virtual bool LoadFromXML(xmlNode* parameterNode);

	/** [INTERNAL] Writes out the effect parameter to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the parameter.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

/**
	A COLLADA 3D vector of floats.
	Contains three, possibly animated, floating-point values.
	The type of the floating-point values may be HALF or FLOAT.
*/
class FCOLLADA_EXPORT FCDEffectParameterFloat3 : public FCDEffectParameter
{
public:
	/** The supported types of float-point values. */
	enum FloatType
	{
		FLOAT, /** A full-fledged floating-point value. This is the default. */
		HALF /** Probably implies a 16-bit floating-point value. */
	};

private:
	DeclareObjectType(FCDEffectParameter);
	FloatType floatType;
	FMVector3 value;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDEffectParameterList::AddParameter function.
		@param document The COLLADA document that owns the effect parameter. */
	FCDEffectParameterFloat3(FCDocument* document);

	/** Destructor. */
	virtual ~FCDEffectParameterFloat3();

	/** Retrieves the type of effect parameter class.
		@return The parameter class type: FLOAT3. */
	virtual Type GetType() const { return FLOAT3; }

	/** Retrieves the type of floating-point value held by this effect parameter.
		@return The type of floating-point value. */
	FloatType GetFloatType() const { return floatType; }

	/** Sets the type of floating-point value held by this effect parameter.
		@param type The type of floating-point value. */
	void SetFloatType(FloatType type) { floatType = type; SetDirtyFlag(); }

	/** Retrieves the value vector of the effect parameter.
		@return The value vector. */
	FMVector3& GetValue() { return value; }
	const FMVector3& GetValue() const { return value; } /**< See above. */

	/** Sets the value vector of the effect parameter.
		@param _value The value vector. */
	void SetValue(const FMVector3& _value) { value = _value; SetDirtyFlag(); }

	/** Retrieves the first floating-point value of the effect parameter.
		@return The first floating-point value. */
	float& GetValueX() { return value.x; }
	const float& GetValueX() const { return value.x; } /**< See above. */

	/** Sets the first floating-point value of the effect parameter.
		@param _value The first floating-point value. */
	void SetValueX(float _value) { value.x = _value; SetDirtyFlag(); }

	/** Retrieves the second floating-point value of the effect parameter.
		@return The second floating-point value. */
	float& GetValueY() { return value.y; }
	const float& GetValueY() const { return value.y; } /**< See above. */

	/** Sets the second floating-point value of the effect parameter.
		@param _value The second floating-point value. */
	void SetValueY(float _value) { value.y = _value; SetDirtyFlag(); }

	/** Retrieves the third floating-point value of the effect parameter.
		@return The third floating-point value. */
	float& GetValueZ() { return value.z; }
	const float& GetValueZ() const { return value.z; } /**< See above. */

	/** Sets the third floating-point value of the effect parameter.
		@param _value The third floating-point value. */
	void SetValueZ(float _value) { value.z = _value; SetDirtyFlag(); }

	/** Compares this parameter's value with another
		@param parameter The given parameter to compare with.
		@return true if the values are equal */
	virtual bool IsValueEqual( FCDEffectParameter *parameter );

	/** Creates a full copy of the effect parameter.
		@param clone The cloned effect parameter. If this pointer is NULL,
			a new effect parameter will be created and you
			will need to delete this pointer.
		@return The cloned effect parameter. */
	virtual FCDEffectParameter* Clone(FCDEffectParameter* clone = NULL) const;

	/** [INTERNAL] Overwrites the target parameter with this parameter.
		This function is used during the flattening of materials.
		@param target The target parameter to overwrite. */
	virtual void Overwrite(FCDEffectParameter* target);

	/** [INTERNAL] Reads in the effect parameter from a given COLLADA XML tree node.
		@param parameterNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the parameter.*/
	virtual bool LoadFromXML(xmlNode* parameterNode);

	/** [INTERNAL] Writes out the effect parameter to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the parameter.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

/**
	A COLLADA 4D vector of floats.
	Contains four, possibly animated, floating-point values.
	The type of the floating-point values may be HALF or FLOAT.
*/
class FCOLLADA_EXPORT FCDEffectParameterVector : public FCDEffectParameter
{
public:
	/** The supported types of float-point values. */
	enum FloatType
	{
		FLOAT, /** A full-fledged floating-point value. This is the default. */
		HALF /** Probably implies a 16-bit floating-point value. */
	};

private:
	DeclareObjectType(FCDEffectParameter);
	FloatType floatType;

	FMVector4 value;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDEffectParameterList::AddParameter function.
		@param document The COLLADA document that owns the effect parameter. */
	FCDEffectParameterVector(FCDocument* document);

	/** Destructor. */
	virtual ~FCDEffectParameterVector();

	/** Retrieves the type of effect parameter class.
		@return The parameter class type: VECTOR. */
	virtual Type GetType() const { return VECTOR; }

	/** Retrieves the type of floating-point value held by this effect parameter.
		@return The type of floating-point value. */
	FloatType GetFloatType() const { return floatType; }

	/** Sets the type of floating-point value held by this effect parameter.
		@param type The type of floating-point value. */
	void SetFloatType(FloatType type) { floatType = type; SetDirtyFlag(); }

	/** Sets the vector value of the effect parameter.
		@return The vector value. */
	float* GetVector() { return (float*)value; }
	const float* GetVector() const { return (const float*)value; } /**< See above. */

	/** Retrieves the first floating-point value of the effect parameter.
		@return The first floating-point value. */
	float& GetValueX() { return value.x; }
	const float& GetValueX() const { return value.x; } /**< See above. */

	/** Sets the first floating-point value of the effect parameter.
		@param _value The first floating-point value. */
	void SetValueX(float _value) { value.x = _value; SetDirtyFlag(); }

	/** Retrieves the second floating-point value of the effect parameter.
		@return The second floating-point value. */
	float& GetValueY() { return value.y; }
	const float& GetValueY() const { return value.y; } /**< See above. */

	/** Sets the second floating-point value of the effect parameter.
		@param _value The second floating-point value. */
	void SetValueY(float _value) { value.y = _value; SetDirtyFlag(); }

	/** Retrieves the third floating-point value of the effect parameter.
		@return The third floating-point value. */
	float& GetValueZ() { return value.z; }
	const float& GetValueZ() const { return value.z; } /**< See above. */

	/** Sets the third floating-point value of the effect parameter.
		@param _value The third floating-point value. */
	void SetValueZ(float _value) { value.z = _value; SetDirtyFlag(); }

	/** Retrieves the fourth floating-point value of the effect parameter.
		@return The fourth floating-point value. */
	float& GetValueW() { return value.w; }
	const float& GetValueW() const { return value.w; } /**< See above. */

	/** Sets the fourth floating-point value of the effect parameter.
		@param _value The fourth floating-point value. */
	void SetValueW(float _value) { value.w = _value; SetDirtyFlag(); }

	/** Get the Vector value.
		@return The FMVector4 value of the effect parameter. */
	FMVector4& GetValue(){ return value; }

	/** Sets the vector value.
		@param value The 4D vector value for the effect parameter. */
	void SetValue(const FMVector4& _value) { value = _value; SetDirtyFlag(); }

	/** Compares this parameter's value with another
		@param parameter The given parameter to compare with.
		@return true if the values are equal */
	virtual bool IsValueEqual( FCDEffectParameter *parameter );

	/** Creates a full copy of the effect parameter.
		@param clone The cloned effect parameter. If this pointer is NULL,
			a new effect parameter will be created and you
			will need to delete this pointer.
		@return The cloned effect parameter. */
	virtual FCDEffectParameter* Clone(FCDEffectParameter* clone = NULL) const;

	/** [INTERNAL] Overwrites the target parameter with this parameter.
		This function is used during the flattening of materials.
		@param target The target parameter to overwrite. */
	virtual void Overwrite(FCDEffectParameter* target);

	/** [INTERNAL] Reads in the effect parameter from a given COLLADA XML tree node.
		@param parameterNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the parameter.*/
	virtual bool LoadFromXML(xmlNode* parameterNode);

	/** [INTERNAL] Writes out the effect parameter to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the parameter.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

/**
	A COLLADA 4x4 matrix.
	Contains 16 floating-point values that represent a COLLADA column-major 4x4 matrix.
	The type of the floating-point values may be HALF or FLOAT.
*/
class FCOLLADA_EXPORT FCDEffectParameterMatrix : public FCDEffectParameter
{
public:
	/** The supported types of float-point values. */
	enum FloatType
	{
		FLOAT, /** A full-fledged floating-point value. This is the default. */
		HALF /** Probably implies a 16-bit floating-point value. */
	};

private:
	DeclareObjectType(FCDEffectParameter);
	FloatType floatType;
	FMMatrix44 matrix;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDEffectParameterList::AddParameter function.
		@param document The COLLADA document that owns the effect parameter. */
	FCDEffectParameterMatrix(FCDocument* document);

	/** Destructor. */
	virtual ~FCDEffectParameterMatrix();

	/** Retrieves the type of effect parameter class.
		@return The parameter class type: MATRIX. */
	virtual Type GetType() const { return MATRIX; }

	/** Retrieves the type of floating-point value held by this effect parameter.
		@return The type of floating-point value. */
	FloatType GetFloatType() const { return floatType; }

	/** Sets the type of floating-point value held by this effect parameter.
		@param type The type of floating-point value. */
	void SetFloatType(FloatType type) { floatType = type; }

	/** Retrieves the matrix contained within this effect parameter.
		@return The matrix. */
	FMMatrix44& GetMatrix() { return matrix; }
	const FMMatrix44& GetMatrix() const { return matrix; } /**< See above. */

	/** Sets the matrix contained within this effect parameter.
		@deprecated Use SetValue instead, for consistency.
		@param mx The matrix. */
	void SetMatrix(const FMMatrix44& mx) { matrix = mx; SetDirtyFlag(); }

	/** Sets the matrix contained within this effect parameter.
		Added for consistency-sake.
		@param mx The matrix. */
	void SetValue(const FMMatrix44& mx) { matrix = mx; SetDirtyFlag(); } /**< See above. */

	/** Compares this parameter's value with another
		@param parameter The given parameter to compare with.
		@return true if the values are equal */
	virtual bool IsValueEqual( FCDEffectParameter *parameter );

	/** Creates a full copy of the effect parameter.
		@param clone The cloned effect parameter. If this pointer is NULL,
			a new effect parameter will be created and you
			will need to delete this pointer.
		@return The cloned effect parameter. */
	virtual FCDEffectParameter* Clone(FCDEffectParameter* clone = NULL) const;

	/** [INTERNAL] Overwrites the target parameter with this parameter.
		This function is used during the flattening of materials.
		@param target The target parameter to overwrite. */
	virtual void Overwrite(FCDEffectParameter* target);

	/** [INTERNAL] Reads in the effect parameter from a given COLLADA XML tree node.
		@param parameterNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the parameter.*/
	virtual bool LoadFromXML(xmlNode* parameterNode);

	/** [INTERNAL] Writes out the effect parameter to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the parameter.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

/**
	A ColladaFX annotation.

	ColladaFX annotations are used mainly to describe the user
	interface necessary to modify a parameter. Common annotations
	are "UIMin", "UIMax" and "UIWidget".
*/
class FCOLLADA_EXPORT FCDEffectParameterAnnotation
{
public:
	FCDEffectParameter::Type type; /**< The annotation value type. */
	fstring name; /**< The annotation name. */
	fstring value; /**< The annotation value. */
};

#endif // _FCD_EFFECT_PARAMETER_H_

