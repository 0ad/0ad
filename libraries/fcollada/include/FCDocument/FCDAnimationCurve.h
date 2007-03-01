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
	@file FCDAnimationCurve.h
	This file contains the FCDAnimationCurve class and the conversion functions/functors.
*/

#ifndef _FCD_ANIMATION_CURVE_H_
#define _FCD_ANIMATION_CURVE_H_

#ifndef _FU_DAE_ENUM_H_
#include "FUtils/FUDaeEnum.h"
#endif // _FU_DAE_ENUM_H_
#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_

class FCDAnimated;
class FCDAnimationClip;
class FCDAnimationChannel;
class FCDConversionFunctor;

typedef fm::pvector<FCDAnimationClip> FCDAnimationClipList; /**< A dynamically-sized array of animation clips. */
typedef float (*FCDConversionFunction)(float v); /**< A simple conversion function. */

/**
	A COLLADA single-dimensional animation curve.
	An animation curve holds the keyframes necessary
	to animate an animatable floating-point value.

	There are multiple interpolation mechanisms supported by COLLADA.
	FCollada supports the CONSTANT, LINEAR and BEZIER interpolations.

	@see FUDaeInterpolation FUDaeInfinity
	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDAnimationCurve : public FCDObject
{
private:
	DeclareObjectType(FCDObject);
	FCDAnimationChannel* parent;

	// Targeting information
	int32 targetElement;
	fm::string targetQualifier;

	// Input information
	FloatList keys, keyValues;
	FMVector2List inTangents, outTangents;
	FMVector3List tcbParameters;
	FMVector2List easeInOuts;
	FUDaeInfinity::Infinity preInfinity, postInfinity;
	
	// Driver information
	FUObjectPtr<FCDAnimated> inputDriver;
	int32 inputDriverIndex;

	// The interpolation values follow the FUDaeInterpolation enum (FUDaeEnum.h)
	UInt32List interpolations;

	// Animation clips that depend on this curve
	FCDAnimationClipList clips;
	FloatList clipOffsets;
	FCDAnimationClip* currentClip;
	float currentOffset;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDAnimationChannel::AddCurve function.
		You should also attach the new curve to an animated
		element using the FCDAnimated::SetCurve function.
		@param document The COLLADA document that owns the animation curve.
		@param parent The animation channel that contains the curve. */
	FCDAnimationCurve(FCDocument* document, FCDAnimationChannel* parent);

	/** Destructor. */
	virtual ~FCDAnimationCurve();

	/** Retrieves the animation channel that contains this animation curve.
		@return The parent animation channel. */
	inline FCDAnimationChannel* GetParent() { return parent; }
	inline const FCDAnimationChannel* GetParent() const { return parent; } /**< See above. */

	/** Retrieves the list of key inputs for the animation curve.
		@return The list of key inputs. */
	inline FloatList& GetKeys() { return keys; }
	inline const FloatList& GetKeys() const { return keys; } /**< See above. */

	/** Retrieves the list of key outputs for the animation curve.
		@return The list of key outputs. */
	inline FloatList& GetKeyValues() { return keyValues; }
	inline const FloatList& GetKeyValues() const { return keyValues; } /**< See above. */

	/** Retrieves the list of interpolation type for the segments of the animation curve.
		There is always one interpolation type for each key in the curve. The interpolation type
		of a segment of the curve is set at the key at which begins the segment.
		@see FUDaeInterpolation
		@return The list of interpolation types. */
	inline UInt32List& GetInterpolations() { return interpolations; }
	inline const UInt32List& GetInterpolations() const { return interpolations; } /**< See above. */

	/** Retrieves the list of key in-tangent values for the animation curve.
		This list has data only for curves that include segments with the bezier interpolation.
		@return The list of in-tangent values. */
	inline FMVector2List& GetInTangents() { return inTangents; }
	inline const FMVector2List& GetInTangents() const { return inTangents; } /**< See above. */

	/** Retrieves the list of key out-tangent values for the animation curve.
		This list has data only for curves that include segments with the bezier interpolation.
		@return The list of out-tangent values. */
	inline FMVector2List& GetOutTangents() { return outTangents; }
	inline const FMVector2List& GetOutTangents() const { return outTangents; } /**< See above. */

	/** Retrieves the list of key TCB values for the animation curve.
		This list has data only for curves that include segments with the TCB interpolation.
		@return The list of TCB values. */
	inline FMVector3List& GetTCBs() { return tcbParameters; }
	inline const FMVector3List& GetTCBs() const { return tcbParameters; } /**< See above. */

	/** Retrieves the list of key ease-in/ease-out values for the TCB animation curve.
		This list has data only for curves that include segments with the TCB interpolation.
		@return The list of ease-in/ease-out values. */
	inline FMVector2List& GetEaseInOuts() { return easeInOuts; }
	inline const FMVector2List& GetEaseInOuts() const { return easeInOuts; } /**< See above. */

	/** Retrieves the type of behavior for the curve if the input value is
		outside the input interval defined by the curve keys and less than any key input value.
		@see FUDaeInfinity
		@return The pre-infinity behavior of the curve. */
	inline FUDaeInfinity::Infinity GetPreInfinity() const { return preInfinity; }

	/** Sets the behavior of the curve if the input value is
		outside the input interval defined by the curve keys and less than any key input value.
		@see FUDaeInfinity
		@param infinity The pre-infinity behavior of the curve. */
	inline void SetPreInfinity(FUDaeInfinity::Infinity infinity) { preInfinity = infinity; SetDirtyFlag(); }

	/** Retrieves the type of behavior for the curve if the input value is
		outside the input interval defined by the curve keys and greater than any key input value.
		@see FUDaeInfinity
		@return The post-infinity behavior of the curve. */
	inline FUDaeInfinity::Infinity GetPostInfinity() const { return postInfinity; }

	/** Sets the behavior of the curve if the input value is
		outside the input interval defined by the curve keys and greater than any key input value.
		@see FUDaeInfinity
		@param infinity The post-infinity behavior of the curve. */
	inline void SetPostInfinity(FUDaeInfinity::Infinity infinity) { postInfinity = infinity; SetDirtyFlag(); }

	/** Retrieves whether this animation curve has a driver.
		@return Whether there is a driver for this curve. */
	bool HasDriver() const;

	/** Retrieves the value pointer that drives this animation curve.
		@param driver A reference to receive the animated input driver. This pointer will
			be set to NULL when there is no input driver.
		@param index A reference to receive the animated input driver element index. */
	void GetDriver(FCDAnimated*& driver, int32& index);
	void GetDriver(const FCDAnimated*& driver, int32& index) const; /**< See above. */

	/** Sets the value pointer that drives the animation curve.
		@param driver The driver animated value. Set this pointer to NULL
			to indicate that time drives the animation curve.
		@param index The driver animated value index. */
	void SetDriver(FCDAnimated* driver, int32 index);

	/** Retrieves the list of animation clips that use this animation curve.
		@return The list of animation clips. */
	inline FCDAnimationClipList& GetClips() { return clips; }
	inline const FCDAnimationClipList& GetClips() const { return clips; } /**< See above. */

	/** Updates the keys to match the timing of an animation clip that has been
		registered using RegisterAnimationClip, or returned from GetClips.
		@param clip The clip to update the keys to. */
	void SetCurrentAnimationClip(FCDAnimationClip* clip);

	/** Gets the offset for an animation clip. When the offset is added to the
		keys, it causes the animation curve to be repositioned so that the 
		animation clip starts at the beginning. 
		@param index The index of the animation clip to get offset for.
		@return The offset value. */
	inline const float GetClipOffset(size_t index) const { return clipOffsets.at(index); }

	/** Readies this curve for evaluation.
		This will create the tangents and the tangent weights, if necessary. */
	void Ready();

	/** Clones the animation curve. The animation clips can be cloned as well,
		but this may lead to an infinite recursion because cloning the clips
		will also clone its curves.
		@param includeClips True if want to also clone the animation clips. 
		@return The cloned animation curve. */
	FCDAnimationCurve* Clone(FCDAnimationCurve* clone = NULL, bool includeClips = true, FCDAnimationClip* dependentAnimationClip = NULL) const;

	/** Applies a conversion function to the key output values of the animation curve.
		@param valueConversion The conversion function to use on the key outputs.
		@param tangentConversion The conversion function to use on the key tangents. */
	void ConvertValues(FCDConversionFunction valueConversion, FCDConversionFunction tangentConversion);
	void ConvertValues(FCDConversionFunctor* valueConversion, FCDConversionFunctor* tangentConversion); /**< See above. */

	/** Applies a conversion function to the key input values of the animation curve.
		@param timeConversion The conversion function to use on the key inputs.
		@param tangentWeightConversion The conversion function to use on the key tangent weights. */
	void ConvertInputs(FCDConversionFunction timeConversion, FCDConversionFunction tangentWeightConversion);
	void ConvertInputs(FCDConversionFunctor* timeConversion, FCDConversionFunctor* tangentWeightConversion); /**< See above. */

	/** Evaluates the animation curve.
		@param input An input value.
		@return The sampled value of the curve at the given input value. */
	float Evaluate(float input) const;

	/** [INTERNAL] Adds an animation clip to the list of animation clips that use this curve.
		@param clip An animation clip. */
	void RegisterAnimationClip(FCDAnimationClip* clip);

	/** [INTERNAL] Writes out the data sources necessary to import the animation curve
		to a given XML tree node.
		@param parentNode The XML tree node in which to create the data sources.
		@param baseId A COLLADA Id prefix to use when generating the source ids. */
	void WriteSourceToXML(xmlNode* parentNode, const fm::string& baseId) const;

	/** [INTERNAL] Writes out the sampler that puts together the data sources
		and generates a sampling function.
		@param parentNode The XML tree node in which to create the sampler.
		@param baseId The COLLADA id prefix used when generating the source ids.
			This prefix is also used to generate the sampler COLLADA id.
		@return The created XML tree node. */
	xmlNode* WriteSamplerToXML(xmlNode* parentNode, const fm::string& baseId) const;

	/** [INTERNAL] Writes out the animation channel that attaches the sampling function
		to the animatable value.
		@param parentNode The XML tree node in which to create the sampler.
		@param baseId The COLLADA Id prefix used when generating the source ids
			and the sampler id.
		@param targetPointer The target pointer prefix for the targeted animated element.
		@return The created XML tree node. */
	xmlNode* WriteChannelToXML(xmlNode* parentNode, const fm::string& baseId, const char* targetPointer) const;

	/** [INTERNAL] Retrieves the target element suffix for the curve.
		This will be -1 if the animated element does not belong to an
		animated element list.
		@return The target element suffix. */
	inline int32 GetTargetElement() const { return targetElement; }

	/** [INTERNAL] Retrieves the target qualifier for the curve.
		This will be the empty string if that the curve affects
		a one-dimensional animated element.
		@return The target qualifier. */
	inline const fm::string& GetTargetQualifier() const { return targetQualifier; }

	/** [INTERNAL] Sets the target element suffix for the curve.
		@param e The target element suffix. Set to value to -1
			if the animated element does not belong to an animated element list. */
	inline void SetTargetElement(int32 e) { targetElement = e; SetDirtyFlag(); }

	/** [INTERNAL] Sets the target qualifier for the curve.
		@param q The target qualifier. You may sets this string to the empty string
			only if that the curve affects a one-dimensional animated element. */
	inline void SetTargetQualifier(const fm::string& q) { targetQualifier = q; SetDirtyFlag(); }

	/** [INTERNAL] Updates the offset for a given animation clip.
		@param offset The new offset. 
		@param clip The animation clip to associate with the offset. */
	void SetClipOffset(float offset, const FCDAnimationClip* clip);
};

/** A simple conversion functor. */
class FCDConversionFunctor
{
public:
	FCDConversionFunctor() {} /**< Constructor. */
	virtual ~FCDConversionFunctor() {} /**< Destructor. */
	virtual float operator() (float v) = 0; /**< Main functor to override. @param v The value to convert. @return The converted value. */
};

/** A sample conversion functor: it scales the value by a given amount. */
class FCDConversionScaleFunctor : public FCDConversionFunctor
{
private:
	float scaleFactor;

public:
	FCDConversionScaleFunctor(float factor) { scaleFactor = factor; } /**< Constructor. @param factor The scale factor. */
	void SetScaleFactor(float factor) { scaleFactor = factor; } /**< Accessor. @param the new scale factor. */
	virtual ~FCDConversionScaleFunctor() {} /**< Destructor. */
	virtual void SetScale(float v) { scaleFactor = v; }
	virtual float operator() (float v) { return v * scaleFactor; } /**< Scales the given value. @param v The value to scale. @return The scaled value. */
	virtual FMVector3 operator() (FMVector3 v) { return v * scaleFactor; } /**< Scales the given FMVector3. @param v The value to scale. @return The scaled value. */
	virtual FMVector4 operator() (FMVector4 v) { return v * scaleFactor; } /**< Scales the given FMVector4. @param v The value to scale. @return The scaled value. */
};

/** A sample conversion functor: it offsets the value by a given amount. */
class FCDConversionOffsetFunctor : public FCDConversionFunctor
{
private:
	float offset;

public:
	FCDConversionOffsetFunctor(float _offset) { offset = _offset; } /**< Constructor. @param _offset The value offset. */
	virtual ~FCDConversionOffsetFunctor() {} /**< Destructor. */
	virtual float operator() (float v) { return v + offset; } /**< Offsets the given value. @param v The value to offset. @return The offseted value. */
};

#endif // _FCD_ANIMATION_CURVE_H_
