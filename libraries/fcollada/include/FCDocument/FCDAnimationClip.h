/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDAnimationClip.h
	This file contains the FCDAnimationClip class.
*/

#ifndef _FCD_ANIMATION_CLIP_H_
#define _FCD_ANIMATION_CLIP_H_

class FCDocument;
class FCDAnimationCurve;

typedef FUObjectList<FCDAnimationCurve> FCDAnimationCurveTrackList; /**< A dynamically-sized tracking array of animation curves. */

#ifndef _FCD_ENTITY_H_
#include "FCDocument/FCDEntity.h"
#endif // _FCD_ENTITY_H_

/**
	A COLLADA animation clip.

	Animation clips are used to group together animation segments.
	Animation clips are typically used to form complex animation sequences
	where all the curves should only be used simultaneously.

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDAnimationClip : public FCDEntity
{
private:
	DeclareObjectType(FCDEntity);
	FCDAnimationCurveTrackList curves;
	float start, end;

public:
	/** Constructor.
		@param document The COLLADA document that holds this animation clip. */
	FCDAnimationClip(FCDocument* document);

	/** Destructor. */
	virtual ~FCDAnimationClip();

	/** Copies the animation clip entity into a clone.
		The clone may reside in another document.
		@param clone The empty clone. If this pointer is NULL, a new animation clip
			will be created and you will need to release the returned pointer manually.
		@param cloneChildren Whether to recursively clone this entity's children.
		@return The clone. */
	virtual FCDEntity* Clone(FCDEntity* clone = NULL, bool cloneChildren = false, FCDAnimationCurve* dependentAnimationCurve = NULL) const;

	/** Retrieves the entity type for this class. This function is part
		of the FCDEntity class interface.
		@return The entity type: IMAGE. */
	virtual Type GetType() const { return ANIMATION_CLIP; }

	/** Retrieves the list of curves that are used by this animation clip.
		@return The list of curves for the clip. */
	FCDAnimationCurveTrackList& GetClipCurves() { return curves; }
	const FCDAnimationCurveTrackList& GetClipCurves() const { return curves; } /**< See above. */

	/** Inserts an existing curve within this animation clip.
		@param curve An animation curve to be used within this clip. */
	void AddClipCurve(FCDAnimationCurve* curve);

	/** Retrieves the start time marker position for this animation clip.
		When using the animation clip, all the animation curves will need
		to be synchronized in order for the animation to start at the start time.
		@return The start time marker position, in seconds. */
	float GetStart() const { return start; }

	/** Sets the start time marker position for this animation clip.
		@param _start The new start time marker position. */
	void SetStart(float _start) { start = _start; SetDirtyFlag(); } 

	/** Retrieves the end time marker position for this animation clip.
		When using the animation clip, all the animation curves will need
		to be synchronized in order for the animation to complete at the end time.
		@return The end time marker position, in seconds. */
	float GetEnd() const { return end; }

	/** Sets the end time marker position for this animation clip.
		@param _end The end time marker position. */
	void SetEnd(float _end) { end = _end; SetDirtyFlag(); }

	/** [INTERNAL] Reads in the animation clip from a given COLLADA XML tree node.
		@param clipNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the animation clip. */
	virtual bool LoadFromXML(xmlNode* clipNode);

	/** [INTERNAL] Writes out the animation clip to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the animation clip.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_ANIMATION_CLIP_H_

