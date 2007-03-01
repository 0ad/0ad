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
	@file FCDAnimationChannel.h
	This file contains the FCDAnimationChannel class.
*/

#ifndef _FCD_ANIMATION_CHANNEL_H_
#define _FCD_ANIMATION_CHANNEL_H_

#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_

class FCDocument;
class FCDAnimated;
class FCDAnimation;
class FCDAnimationCurve;

/** [INTERNAL] An animation channel default value setting.
	This structure is valid only during the export and is used by the animated value
	to prepare the channel for a possible curve merging operation. */
struct FCDAnimationChannelDefaultValue
{
	FCDAnimationCurve* curve; /**< An animation curve contained by this channel. */
	float defaultValue; /**< The default value for an animation value pointer that is not animated but may be merged. */
	fm::string defaultQualifier; /**< The qualifier to use with the default value when the animation value pointer is not animated. */

	/** Default constructor. */
	FCDAnimationChannelDefaultValue() : curve(NULL), defaultValue(0.0f) {}
	/** Simple constructor. @param c A curve. @param f The default value. @param q The default value's qualifier. */
	FCDAnimationChannelDefaultValue(FCDAnimationCurve* c, float f, const char* q) { curve = c; defaultValue = f; defaultQualifier = q; }
};

typedef fm::pvector<FCDAnimationCurve> FCDAnimationCurveList; /**< A dynamically-sized array of animation curves. */
typedef FUObjectContainer<FCDAnimationCurve> FCDAnimationCurveContainer; /**< A dynamically-sized containment array for animation curves. */
typedef fm::vector<FCDAnimationChannelDefaultValue> FCDAnimationChannelDefaultValueList; /**< A dynamically-szied array for animation channel default values. */

/**
	A COLLADA animation channel.
	Each animation channel holds the animation curves for one animatable element,
	such as a single floating-point value, a 3D vector or a matrix.

	@see FCDAnimated
	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDAnimationChannel : public FCDObject
{
private:
	DeclareObjectType(FCDObject);
	FCDAnimation* parent;

	// Channel target
	fm::string targetPointer;
	fm::string targetQualifier;

	// Maya-specific: the driver for this/these curves
	fm::string driverPointer;
	int32 driverQualifier;

	FCDAnimationCurveContainer curves;
	FCDAnimationChannelDefaultValueList defaultValues; // valid during the export only.

public:
	/** Constructor: do not use directly.
		Instead, call the FCDAnimation::AddChannel function.
		@param document The COLLADA document that owns the animation channel.
		@param parent The animation sub-tree that contains the animation channel. */
	FCDAnimationChannel(FCDocument* document, FCDAnimation* parent);

	/** Destructor. */
	virtual ~FCDAnimationChannel();
	
	/** Copies the animation channel into a clone.
		The clone may reside in another document.
		@param clone The empty clone. If this pointer is NULL, a new animation channel
			will be created and you will need to release the returned pointer manually.
		@return The clone. */
	FCDAnimationChannel* Clone(FCDAnimationChannel* _clone = NULL) const;

	/** Retrieves the animation sub-tree that contains the animation channel.
		@return The parent animation sub-tree. */
	FCDAnimation* GetParent() { return parent; }
	const FCDAnimation* GetParent() const { return parent; } /**< See above. */

	/** Retrieves the list of animation curves contained within the channel.
		@return The list of animation curves. */
	FCDAnimationCurveContainer& GetCurves() { return curves; }
	const FCDAnimationCurveContainer& GetCurves() const { return curves; } /**< See above. */

	/** Retrieves the number of animation curves contained within the channel.
		@return The number of animation curves. */
	size_t GetCurveCount() const { return curves.size(); }

	/** Retrieves an animation curve contained within the channel.
		@param index The index of the animation curve.
		@return The animation curve at the given index. This pointer will be NULL
			if the index is out-of-bounds. */
	FCDAnimationCurve* GetCurve(size_t index) { FUAssert(index < GetCurveCount(), return NULL); return curves.at(index); }
	const FCDAnimationCurve* GetCurve(size_t index) const { FUAssert(index < GetCurveCount(), return NULL); return curves.at(index); } /**< See above. */

	/** Adds a new animation curve to this animation channel.
		@return The new animation curve. */
	FCDAnimationCurve* AddCurve();

	/** [INTERNAL] Retrieves the target pointer prefix for this animation channel.
		This function is used during the import of a COLLADA document to match the
		target pointer prefixes with the animated elements.
		@return The target pointer prefix. */
	const fm::string& GetTargetPointer() const { return targetPointer; }

	/** [INTERNAL] Retrieves the target qualifier for this animation channel.
		This function is used during the import of a COLLADA document.
		Where there is a target qualifier, there should be only one curve contained by the channel.
		@return The target qualifier. This value may be the empty string if the channel
		targets all the values targeted by the target pointer prefix. */
	const fm::string& GetTargetQualifier() const { return targetQualifier; }

	/** [INTERNAL] Enforces the tarrget pointer prefix for the animation channel.
		This function is used during the export of a COLLADA document.
		@param p The new target pointer prefix. */
	void SetTargetPointer(const fm::string& p) { targetPointer = p; }

	/** [INTERNAL] Considers the given animated element as the driver for this animation channel.
		@param animated An animated element. 
		@return Whether the animated element is in fact the driver for the animation channel. */
	bool LinkDriver(FCDAnimated* animated);

	/** [INTERNAL] Verifies that if a driver is used by this channel, then it was found during
		the import of the animated elements.
		@return The status of the verification. */
	bool CheckDriver();

	/** [INTERNAL] Reads in the animation channel from a given COLLADA XML tree node.
		@param channelNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the animation channel. */
	bool LoadFromXML(xmlNode* channelNode);

	/** [INTERNAL] Sets the default values for the non-animated elements of the 
		animated value represented by this channel. This information is generated
		by FCollada and is valid only during the export procedure.
		@param _defaultValues The default values for the non-animated elements. */
	void SetDefaultValues(const FCDAnimationChannelDefaultValueList& _defaultValues) { defaultValues = _defaultValues; SetDirtyFlag(); }

	/** [INTERNAL] Writes out the animation channel to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the animation channel.
		@return The created element XML tree node. */
	void WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_ANIMATION_CHANNEL_H_
