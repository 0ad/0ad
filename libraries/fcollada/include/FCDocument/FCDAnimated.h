/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDAnimated.h
	This file contains the FCDAnimated class.
*/

#ifndef _FCD_ANIMATED_H_
#define _FCD_ANIMATED_H_

#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_

class FCDocument;
class FCDAnimated;
class FCDAnimationCurve;
class FCDAnimationChannel;
class FCDAnimationMultiCurve;

typedef fm::pvector<float> FloatPtrList; /**< A dynamically-sized array of floating-point value pointers. */
typedef fm::pvector<FCDAnimationCurve> FCDAnimationCurveList; /**< A dynamically-sized array of animation curves. */
typedef FUObjectList<FCDAnimationCurve> FCDAnimationCurveTrackList; /**< A dynamically-sized array of tracked animation curves. */
typedef fm::vector<FCDAnimationCurveTrackList> FCDAnimationCurveListList; /** A dynamically-sized array of animation curves. */
typedef fm::pvector<FCDAnimationChannel> FCDAnimationChannelList; /**< A dynamically-sized array of animation channels. */
typedef fm::pvector<FCDAnimated> FCDAnimatedList; /**< A dynamically-sized array of animated values. */

/**
	An animated element.
	An animated element encapsulates a set of floating-point values that are
	marked as animated.

	For this purpose, an animated element holds a list of floating-point values,
	their animation curves and their COLLADA qualifiers for the generation of
	COLLADA targets. For animated list elements, an animated element holds an array index.

	There are many classes built on top of this class. They represent
	the different element types that may be animated, such as 3D points,
	colors and matrices.

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDAnimated : public FCDObject
{
private:
	DeclareObjectType(FCDObject);

protected:
	/** The list of value pointers. */
	FloatPtrList values;

	/** The list of target qualifiers.
		There is always one qualifier for one value pointer. */
	StringList qualifiers; 

	/** The list of animation curves.
		There is always one curve for one value pointer, although
		that curve may be the NULL pointer to indicate a non-animated value. */
	FCDAnimationCurveListList curves; 

	/** The array index for animated element that belong
		to a list of animated elements. This value may be -1
		to indicate that the element does not belong to a list.
		Otherwise, the index should always be unsigned. */
	int32 arrayElement;

	/** [INTERNAL] The target pointer prefix. */
	fm::string pointer; 

public:
	/** Constructor.
		In most cases, it is preferable to create objects of the up-classes.
		@param document The COLLADA document that owns this animated element.
		@param valueCount The number of values inside the animated element. */
	FCDAnimated(FCDocument* document, size_t valueCount);

	/** Destructor. */
	virtual ~FCDAnimated();

	/** Retrieves the number of values contained within this animated element.
		@return The number of values. */
	inline size_t GetValueCount() const { return values.size(); }

	/** Retrieves the animation curve affecting the value of an animated element.
		@param index The value index.
		@param curveIndex The index of the curve within the list of curves affecting the
			value at the given index.
		@return The curve affecting the value at the given index. This pointer will
			be NULL if one of the index is out-of-bounds or if the value is not animated. */
	inline FCDAnimationCurve* GetCurve(size_t index, size_t curveIndex = 0) { FUAssert(index < GetValueCount(), return NULL); return curveIndex < curves.at(index).size() ? curves.at(index).at(curveIndex) : NULL; }
	inline const FCDAnimationCurve* GetCurve(size_t index, size_t curveIndex = 0) const { FUAssert(index < GetValueCount(), return NULL); return curveIndex < curves.at(index).size() ? curves.at(index).at(curveIndex) : NULL; } /**< See above. */

	/** Retrieves the list of the curves affecting the values of an animated element.
		This list may contain the NULL pointer, where a value is not animated.
		@return The list of animation curves. */
	inline FCDAnimationCurveListList& GetCurves() { return curves; }
	inline const FCDAnimationCurveListList& GetCurves() const { return curves; } /**< See above. */

	/** Assigns a curve to a value of the animated element.
		The previously assigned curve will be deleted.
		@param index The value index.
		@param curve The new curve(s) that will affect the value at the given index.
		@return Whether the curve was successfully assigned. Will return false if
			the index is out-of-bounds. */
	bool AddCurve(size_t index, FCDAnimationCurve* curve);
	bool AddCurve(size_t index, FCDAnimationCurveList& curve); /**< See above. */

	/** Removes the curves affecting a value of the animated element.
		@param index The value index.
		@return Whether a curve was successfully removed. Will return false
			if there was no curve to release or the index is out-of-bounds. */
	bool RemoveCurve(size_t index);

	/** Retrieves the value of an animated element.
		@param index The value index.
		@return The value at the given index. This pointer will
			be NULL if the index is out-of-boudns. */
	inline float* GetValue(size_t index) { FUAssert(index < GetValueCount(), return NULL); return values.at(index); }
	inline const float* GetValue(size_t index) const { FUAssert(index < GetValueCount(), return NULL); return values.at(index); } /**< See above. */

	/** Retrieves the qualifier of the value of an animated element.
		@param index The value index.
		@return The qualifier for the value. The value returned will be an
			empty string when the index is out-of-bounds. */
	const fm::string& GetQualifier(size_t index) const;

	/** Retrieves an animated value given a valid qualifier.
		@param qualifier A valid qualifier.
		@return The animated value for this qualifier. This pointer will be
			NULL if the given qualifier is not used within this animated element. */
	float* FindValue(const fm::string& qualifier);
	const float* FindValue(const fm::string& qualifier) const; /**< See above. */

	/** Retrieves an animation curve given a valid qualifier.
		@param qualifier A valid qualifier.
		@return The animation curve for this qualifier. This pointer will be
			NULL if the given qualifier is not used within this animated element
			or if the value for the given qualifier is not animated. */
	inline FCDAnimationCurve* FindCurve(const char* qualifier) { size_t index = FindQualifier(qualifier); return index < GetValueCount() ? GetCurve(index) : NULL; }
	inline FCDAnimationCurve* FindCurve(const fm::string& qualifier) { return FindCurve(qualifier.c_str()); } /**< See above. */
	inline const FCDAnimationCurve* FindCurve(const char* qualifier) const { size_t index = FindQualifier(qualifier); return index < GetValueCount() ? GetCurve(index) : NULL; } /**< See above. */
	inline const FCDAnimationCurve* FindCurve(const fm::string& qualifier) const { return FindCurve(qualifier.c_str()); } /**< See above. */

	/** Retrieves an animation curve given a value pointer.
		@param value A value pointer contained within the animated element.
		@return The animation curve for this qualifier. This pointer will be
			NULL if the value pointer is not contained by this animated element
			or if the value is not animated. */
	inline FCDAnimationCurve* FindCurve(const float* value) { size_t index = FindValue(value); return index < GetValueCount() ? GetCurve(index) : NULL; }
	inline const FCDAnimationCurve* FindCurve(const float* value) const { size_t index = FindValue(value); return index < GetValueCount() ? GetCurve(index) : NULL; } /**< See above. */

	/** Retrieves the value index for a given qualifier.
		@param qualifier A valid qualifier.
		@return The value index. This value will be -1 to indicate that the
			qualifier does not belong to this animated element. */
	size_t FindQualifier(const char* qualifier) const;
	inline size_t FindQualifier(const fm::string& qualifier) const { return FindQualifier(qualifier.c_str()); } /**< See above. */

	/** Retrieves the value index for a given value pointer.
		@param value A value pointer contained within the animated element.
		@return The value index. This value will be -1 to indicate that the
			value pointer is not contained by this animated element. */
	size_t FindValue(const float* value) const;

	/** Retrieves the array index for an animated element.
		This value is used only for animated elements that belong
		to a list of animated elements within the COLLADA document.
		@return The array index. This value will be -1 to indicate that
			the animated element does not belong to a list. */
	inline int32 GetArrayElement() const { return arrayElement; }

	/** Sets the array index for an animated element.
		This value is used only for animated elements that belong
		to a list of animated elements within the COLLADA document.
		@param index The array index. This value should be -1 to indicate that
			the animated element does not belong to a list. */
	inline void SetArrayElement(int32 index) { arrayElement = index; SetDirtyFlag(); }

	/** Retrieves whether this animated element has any animation curves
		affecting its values.
		@return Whether any curves affect this animated element. */
	bool HasCurve() const;

	/** Creates one multi-dimensional animation curve from this animated element.
		This function is useful is your application does not handle animations
		per-values, but instead needs one animation per-element.
		@return The multi-dimensional animation curve. */
	FCDAnimationMultiCurve* CreateMultiCurve() const;

	/** Creates one multi-dimensional animation curve from a list of animated element.
		This function is useful is your application does not handle animations
		per-values. For example, we use this function is ColladaMax for animated scale values,
		where one scale value is two rotations for the scale rotation pivot and one
		3D point for the scale factors.
		@param toMerge The list of animated elements to merge
		@return The multi-dimensional animation curve. */
	static FCDAnimationMultiCurve* CreateMultiCurve(const FCDAnimatedList& toMerge);

	/** Evaluates the animated element at a given time.
		This function directly and <b>permanently</b> modifies the values
		of the animated element according to the curves affecting them.
		@param time The evaluation time. */
	void Evaluate(float time);

	/** [INTERNAL] Clones an animated element.
		@param document The COLLADA document that owns the cloned animated element.
		@param animatedValue One animated value contained within the original animated element.
		@param newAnimatedValues The list of value pointers to be contained by the cloned animated element.
		@return The cloned animated element. */
	static FCDAnimated* Clone(FCDocument* document, const float* animatedValue, FloatPtrList& newAnimatedValues);

	/** [INTERNAL] Clones an animated element.
		@param document The COLLADA document that owns the cloned animated element.
		@return The cloned animated element. */
	FCDAnimated* Clone(FCDocument* document) const;

	/** [INTERNAL] Clones an animated element.
		@param animated A clone.
		@return The clone. */
	FCDAnimated* Clone(FCDAnimated* clone) const;

	/** [INTERNAL] Retrieves the target pointer that prefixes the
		fully-qualified target for the element.
		@return The target pointer prefix. */
	inline const fm::string& GetTargetPointer() const { return pointer; }

	/** [INTERNAL] Sets the target pointer that prefixes the
		fully-qualified target for the element. This function is used on export,
		for the support of driven-key animations.
		@param _pointer The target pointer. */
	inline void SetTargetPointer(const fm::string& _pointer) { pointer = _pointer; }

	/** [INTERNAL] Links this animated element with a given XML tree node.
		This function is solely used within the import of a COLLADA document.
		The floating-point values held within the XML tree node will be linked
		with the list of floating-point value pointers held by the animated entity.
		@param node The XML tree node.
		@return Whether there was any linkage done. */
	bool Link(xmlNode* node);

	/** [INTERNAL] Links the animated element with the imported animation curves.
		This compares the animation channel targets with the animated element target
		and qualifiers to assign curves unto the value pointers.
		@param channels A list of animation channels with the correct target pointer.
		@return Whether any animation curves were assigned to the animation element. */
	bool ProcessChannels(FCDAnimationChannelList& channels);
};

/** A COLLADA animated single floating-point value element.
	Use this animated element class for all generic-purpose single floating-point values.
	For angles, use the FCDAnimatedAngle class.
	@ingroup FCDocument */
class FCOLLADA_EXPORT FCDAnimatedFloat : public FCDAnimated
{
private:
	DeclareObjectType(FCDAnimated);

	// Don't build directly, use the Create function instead
	FCDAnimatedFloat(FCDocument* document, float* value, int32 arrayElement);

public:
	/** Sets a different default qualifier for this animated element.
		Don't forget the '.'!
		@param qualifier The default qualifier for this animated element. */
	void SetQualifier(const char* qualifier);

	/** Creates a new animated element.
		@param document The COLLADA document that owns the animated element.
		@param value The value pointer for the single floating-point value.
		@param arrayElement The optional array index for animated element
			that belong to an animated element list.
		@return The new animated element. */
	static FCDAnimatedFloat* Create(FCDocument* document, float* value, int32 arrayElement=-1);

	/** [INTERNAL] Creates a new animated element.
		This function is used during the import of a COLLADA document.
		@param document The COLLADA document that owns the animated element.
		@param node The XML tree node that contains the animated values.
		@param value The value pointer for the single floating-point value.
		@param arrayElement The optional array index for animated element
			that belong to an animated element list.
		@return The new animated element. */
	static FCDAnimatedFloat* Create(FCDocument* document, xmlNode* node, float* value, int32 arrayElement=-1);

	/** [INTERNAL] Clones an animated element.
		@param document The COLLADA document that owns the cloned animated element.
		@param oldValue The single floating-point value pointer contained within the original animated element.
		@param newValue The single floating-point value pointer for the cloned animated element.
		@return The cloned animated value. */
	static FCDAnimated* Clone(FCDocument* document, const float* oldValue, float* newValue);
};

/** A COLLADA animated 3D vector element.
	@ingroup FCDocument */
class FCOLLADA_EXPORT FCDAnimatedPoint3 : public FCDAnimated
{
private:
	DeclareObjectType(FCDAnimated);

	// Don't build directly, use the Create function instead
	FCDAnimatedPoint3(FCDocument* document, FMVector3* value, int32 arrayElement);

public:
	/** Creates a new animated element.
		@param document The COLLADA document that owns the animated element.
		@param value The value pointer for the 3D vector.
		@param arrayElement The optional array index for animated element
			that belong to an animated element list.
		@return The new animated element. */
	static FCDAnimatedPoint3* Create(FCDocument* document, FMVector3* value, int32 arrayElement=-1);

	/** [INTERNAL] Creates a new animated element.
		This function is used during the import of a COLLADA document.
		@param document The COLLADA document that owns the animated element.
		@param node The XML tree node that contains the animated values.
		@param value The value pointer for the 3D vector.
		@param arrayElement The optional array index for animated element
			that belong to an animated element list.
		@return The new animated element. */
	static FCDAnimatedPoint3* Create(FCDocument* document, xmlNode* node, FMVector3* value, int32 arrayElement=-1);

	/** [INTERNAL] Clones an animated element.
		@param document The COLLADA document that owns the cloned animated element.
		@param oldValue The 3D vector contained within the original animated element.
		@param newValue The 3D vector for the cloned animated element.
		@return The cloned animated value. */
	static FCDAnimated* Clone(FCDocument* document, const FMVector3* oldValue, FMVector3* newValue);
};

/** A COLLADA animated RGB color element.
	@ingroup FCDocument */
class FCOLLADA_EXPORT FCDAnimatedColor : public FCDAnimated
{
private:
	DeclareObjectType(FCDAnimated);

	// Don't build directly, use the Create function instead
	FCDAnimatedColor(FCDocument* document, FMVector3* value, int32 arrayElement);
	FCDAnimatedColor(FCDocument* document, FMVector4* value, int32 arrayElement);

public:
	/** Creates a new animated element.
		@param document The COLLADA document that owns the animated element.
		@param value The value pointer for the RGB color.
		@param arrayElement The optional array index for animated element
			that belong to an animated element list.
		@return The new animated element. */
	static FCDAnimatedColor* Create(FCDocument* document, FMVector3* value, int32 arrayElement=-1);

	/** Creates a new animated element.
		@param document The COLLADA document that owns the animated element.
		@param value The value pointer for the RGBA color.
		@param arrayElement The optional array index for animated element
			that belong to an animated element list.
		@return The new animated element. */
	static FCDAnimatedColor* Create(FCDocument* document, FMVector4* value, int32 arrayElement=-1);

	/** [INTERNAL] Creates a new animated element.
		This function is used during the import of a COLLADA document.
		@param document The COLLADA document that owns the animated element.
		@param node The XML tree node that contains the animated values.
		@param value The value pointer for the RGB color.
		@param arrayElement The optional array index for animated element
			that belong to an animated element list.
		@return The new animated element. */
	static FCDAnimatedColor* Create(FCDocument* document, xmlNode* node, FMVector3* value, int32 arrayElement=-1);

	/** [INTERNAL] Creates a new animated element.
		This function is used during the import of a COLLADA document.
		@param document The COLLADA document that owns the animated element.
		@param node The XML tree node that contains the animated values.
		@param value The value pointer for the RGBA color.
		@param arrayElement The optional array index for animated element
			that belong to an animated element list.
		@return The new animated element. */
	static FCDAnimatedColor* Create(FCDocument* document, xmlNode* node, FMVector4* value, int32 arrayElement=-1);

	/** [INTERNAL] Clones an animated element.
		@param document The COLLADA document that owns the cloned animated element.
		@param oldValue The RGB color contained within the original animated element.
		@param newValue The RGB color for the cloned animated element.
		@return The cloned animated value. */
	static FCDAnimated* Clone(FCDocument* document, const FMVector3* oldValue, FMVector3* newValue);

	/** [INTERNAL] Clones an animated element.
		@param document The COLLADA document that owns the cloned animated element.
		@param oldValue The RGBA color contained within the original animated element.
		@param newValue The RGBA color for the cloned animated element.
		@return The cloned animated value. */
	static FCDAnimated* Clone(FCDocument* document, const FMVector4* oldValue, FMVector4* newValue);
};

/** A COLLADA floating-point value that represents an angle.
	@ingroup FCDocument */
class FCOLLADA_EXPORT FCDAnimatedAngle : public FCDAnimated
{
private:
	DeclareObjectType(FCDAnimated);

	// Don't build directly, use the Create function instead
	FCDAnimatedAngle(FCDocument* document, float* value, int32 arrayElement);

public:
	/** Creates a new animated element.
		@param document The COLLADA document that owns the animated element.
		@param value The value pointer for the angle.
		@param arrayElement The optional array index for animated element
			that belong to an animated element list.
		@return The new animated element. */
	static FCDAnimatedAngle* Create(FCDocument* document, float* value, int32 arrayElement=-1);

	/** [INTERNAL] Creates a new animated element.
		This function is used during the import of a COLLADA document.
		@param document The COLLADA document that owns the animated element.
		@param node The XML tree node that contains the animated values.
		@param value The value pointer for the angle.
		@param arrayElement The optional array index for animated element
			that belong to an animated element list.
		@return The new animated element. */
	static FCDAnimatedAngle* Create(FCDocument* document, xmlNode* node, float* value, int32 arrayElement=-1);

	/** [INTERNAL] Clones an animated element.
		@param document The COLLADA document that owns the cloned animated element.
		@param oldValue The angle value pointer contained within the original animated element.
		@param newValue The angle value pointer for the cloned animated element.
		@return The cloned animated value. */
	static FCDAnimated* Clone(FCDocument* document, const float* oldValue, float* newValue);
};

/** A COLLADA animated angle-axis.
	Used for rotations, takes in a 3D vector for the axis and
	a single floating-point value for the angle.
	@ingroup FCDocument */
class FCOLLADA_EXPORT FCDAnimatedAngleAxis : public FCDAnimated
{
private:
	DeclareObjectType(FCDAnimated);

	// Don't build directly, use the Create function instead
	FCDAnimatedAngleAxis(FCDocument* document, FMVector3* axis, float* angle, int32 arrayElement);

public:
	/** Creates a new animated element.
		@param document The COLLADA document that owns the animated element.
		@param value The value pointer for the axis.
		@param angle The value pointer for the angle.
		@param arrayElement The optional array index for animated element
			that belong to an animated element list.
		@return The new animated element. */
	static FCDAnimatedAngleAxis* Create(FCDocument* document, FMVector3* value, float* angle, int32 arrayElement=-1);

	/** [INTERNAL] Creates a new animated element.
		This function is used during the import of a COLLADA document.
		@param document The COLLADA document that owns the animated element.
		@param node The XML tree node that contains the animated values.
		@param axis The value pointer for the axis.
		@param angle The value pointer for the angle.
		@param arrayElement The optional array index for animated element
			that belong to an animated element list.
		@return The new animated element. */
	static FCDAnimatedAngleAxis* Create(FCDocument* document, xmlNode* node, FMVector3* axis, float* angle, int32 arrayElement=-1);

	/** [INTERNAL] Clones an animated element.
		@param document The COLLADA document that owns the cloned animated element.
		@param oldAngle The angle value pointer contained within the original animated element.
		@param newAxis The axis value pointer for the cloned animated element.
		@param newAngle The angle value pointer for the cloned animated element.
		@return The cloned animated value. */
	static FCDAnimated* Clone(FCDocument* document, const float* oldAngle, FMVector3* newAxis, float* newAngle);
};

/** A COLLADA animated matrix.
	Used for animated transforms, takes in a 16 floating-point values.
	@ingroup FCDocument */
class FCOLLADA_EXPORT FCDAnimatedMatrix : public FCDAnimated
{
private:
	DeclareObjectType(FCDAnimated);

	// Don't build directly, use the Create function instead
	FCDAnimatedMatrix(FCDocument* document, FMMatrix44* value, int32 arrayElement);

public:
	/** Creates a new animated element.
		@param document The COLLADA document that owns the animated element.
		@param value The value pointer for the matrix.
		@param arrayElement The optional array index for animated element
			that belong to an animated element list.
		@return The new animated element. */
	static FCDAnimatedMatrix* Create(FCDocument* document, FMMatrix44* value, int32 arrayElement=-1);

	/** [INTERNAL] Creates a new animated element.
		This function is used during the import of a COLLADA document.
		@param document The COLLADA document that owns the animated element.
		@param node The XML tree node that contains the animated values.
		@param value The value pointer for the matrix.
		@param arrayElement The optional array index for animated element
			that belong to an animated element list.
		@return The new animated element. */
	static FCDAnimatedMatrix* Create(FCDocument* document, xmlNode* node, FMMatrix44* value, int32 arrayElement=-1);

	/** [INTERNAL] Clones an animated element.
		@param document The COLLADA document that owns the cloned animated element.
		@param oldMx The matrix value pointer contained within the original animated element.
		@param newMx The matrix value pointer for the cloned animated element.
		@return The cloned animated value. */
	static FCDAnimated* Clone(FCDocument* document, const FMMatrix44* oldMx, FMMatrix44* newMx);
};

/** A COLLADA custom animated value.
	Used for animated extra elements. A single value is used multiple times to hold
	as many value pointers are necessary to hold the animation curves.
	@ingroup FCDocument */
class FCOLLADA_EXPORT FCDAnimatedCustom : public FCDAnimated
{
private:
	DeclareObjectType(FCDAnimated);
	float dummy;

	// Don't build directly, use the Create function instead
	FCDAnimatedCustom(FCDocument* document);

	bool Link(xmlNode* node);

public:
	/** Creates a new animated element.
		@param document The COLLADA document that owns the animated element.
		@param node [INTERNAL] The XML tree node that contains the animated values.
		@return The new animated element. */
	static FCDAnimatedCustom* Create(FCDocument* document, xmlNode* node = NULL);

	/** [INTERNAL] Initialized a custom animated element from
		another animated element. The custom animated element will
		be resized to copy the given animated element.
		@param copy The animated element to copy. */
	void Copy(const FCDAnimated* copy);

	/** Retrieves the floating-point value used for all the value pointers.
		@return The dummy floating-point value. */
	float& GetDummy() { return dummy; }
	const float& GetDummy() const { return dummy; } /**< See above. */

	/** Resizes the wanted qualifiers.
		Using the FUDaeAccessor types is recommended.
		@param count The new size of the animated element.
		@param qualifiers The new qualifiers for the animated element.
		@param prependDot Whether to prepend the '.' character for all the qualifiers of the animated element. */
	void Resize(size_t count, const char** qualifiers = NULL, bool prependDot = true);
};

#endif // _FCD_ANIMATED_H_

