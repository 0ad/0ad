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
	@file FCDAnimation.h
	This file contains the FCDAnimation class.
*/

#ifndef _FCD_ANIMATION_H_
#define _FCD_ANIMATION_H_

#ifndef _FU_XML_NODE_ID_PAIR_H_
#include "FUtils/FUXmlNodeIdPair.h"
#endif // _FU_XML_NODE_ID_PAIR_H_
#ifndef _FCD_ENTITY_H_
#include "FCDocument/FCDEntity.h"
#endif // _FCD_ENTITY_H_

class FCDocument;
class FCDAnimated;
class FCDAnimation;
class FCDAnimationChannel;
class FCDAnimationCurve;

typedef fm::pvector<FCDAnimation> FCDAnimationList; /**< A dynamically-sized array of animation entities. */
typedef fm::pvector<FCDAnimationChannel> FCDAnimationChannelList; /**< A dynamically-sized array of animation channels. */
typedef fm::pvector<FCDAnimationCurve> FCDAnimationCurveList; /**< A dynamically-sized array of animation curves. */
typedef FUObjectContainer<FCDAnimation> FCDAnimationContainer; /**< A dynamically-sized containment array for animation entities. */
typedef FUObjectContainer<FCDAnimationChannel> FCDAnimationChannelContainer; /**< A dynamically-sized containment array for animation channels. */

/**
	A COLLADA animation entity.
	An animation entity contains a list of child animation entities,
	in order to form a tree of animation entities.
	It also hold a list of animation channels, which hold the information
	to generate animation curves.

	In other words, the animation entity is a structural class
	used to group animation channels hierarchically.

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDAnimation : public FCDEntity
{
private:
	DeclareObjectType(FCDEntity);

	// Animation hierarchy
	FCDAnimation* parent;
	FCDAnimationContainer children;

	// Animation sources and channels
	FUXmlNodeIdPairList childNodes; // import-only.
	FCDAnimationChannelContainer channels;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDLibrary::AddEntity function
		or the AddChild function, depending on the
		hierarchical level of the animation entity.
		@param document The COLLADA document that owns the animation entity. */
	FCDAnimation(FCDocument* document, FCDAnimation* parent = NULL);

	/** Destructor .*/
	virtual ~FCDAnimation();

	/** Retrieves the entity class type.
		This function is a part of the FCDEntity interface.
		@return The entity class type: ANIMATION. */
	virtual Type GetType() const { return ANIMATION; }

	/** Retrieves the parent of the animation structure.
		@return The animation parent. This pointer will be NULL
			to indicate a root-level animation structure that is
			contained within the animation library. */
	inline FCDAnimation* GetParent() { return parent; }
	inline const FCDAnimation* GetParent() const { return parent; }
    
	/** Copies the animation tree into a clone.
		The clone may reside in another document.
		@param clone The empty clone. If this pointer is NULL, a new animation tree
			will be created and you will need to release the returned pointer manually.
		@param cloneChildren Whether to recursively clone this entity's children.
		@return The clone. */
	virtual FCDEntity* Clone(FCDEntity* clone = NULL, bool cloneChildren = false) const;

	/** Retrieves the entity with the given COLLADA id.
		This function will look through the local sub-tree of animations
		for the given COLLADA id.
		@param daeId A COLLADA id.
		@return The animation entity that matches the COLLADA id. This pointer
			will be NULL if there are no animation entities that matches the COLLADA id. */
	virtual FCDEntity* FindDaeId(const fm::string& daeId);

	/** Retrieves the number of animation entity sub-trees contained
		by this animation entity tree.
		@return The number of animation entity sub-trees. */
	inline size_t GetChildrenCount() const { return children.size(); }

	/** Retrieves an animation entity sub-tree contained by this
		animation entity tree.
		@param index The index of the sub-tree.
		@return The animation entity sub-tree at the given index. This pointer will
			be NULL if the index is out-of-bounds. */
	inline FCDAnimation* GetChild(size_t index) { FUAssert(index < children.size(), return NULL); return children.at(index); }
	inline const FCDAnimation* GetChild(size_t index) const { FUAssert(index < children.size(), return NULL); return children.at(index); } /**< See above. */

	/** Creates a new animation entity sub-tree contained within this animation entity tree.
		@return The new animation sub-tree. */
	FCDAnimation* AddChild();

	/** Releases an animation entity sub-tree contained by this animation entity tree.
		@param animation The animation entity the release. */
	inline void ReleaseChild(FCDAnimation* animation);

	/** Retrieves the asset information structures that affect
		this entity in its hierarchy.
		@param assets A list of asset information structures to fill in. */
	inline void GetHierarchicalAssets(FCDAssetList& assets) { GetHierarchicalAssets(*(FCDAssetConstList*) &assets); }
	virtual void GetHierarchicalAssets(FCDAssetConstList& assets) const; /**< See above. */

	/** Retrieves the animation channels that target the given COLLADA target pointer.
		@param pointer A COLLADA target pointer.
		@param targetChannels A list of animation channels to fill in.
			This list is not cleared. */
	void FindAnimationChannels(const fm::string& pointer, FCDAnimationChannelList& targetChannels);

	/** Retrieves the number of animation channels at this level within the animation tree.
		@return The number of animation channels. */
	size_t GetChannelCount() const { return channels.size(); }

	/** Retrieves an animation channel contained by this animation entity.
		@param index The index of the channel.
		@return The channel at the given index. This pointer will be NULL
			if the index is out-of-bounds. */
	FCDAnimationChannel* GetChannel(size_t index) { FUAssert(index < GetChannelCount(), return NULL); return channels.at(index); }
	const FCDAnimationChannel* GetChannel(size_t index) const { FUAssert(index < GetChannelCount(), return NULL); return channels.at(index); } /**< See above. */

	/** Adds a new animation channel to this animation entity.
		@return The new animation channel. */
	FCDAnimationChannel* AddChannel();

	/** Retrieves all the curves created in the subtree of this animation element.
		@param curves A list of animation curves to fill in.
			This list is not cleared. */
	void GetCurves(FCDAnimationCurveList& curves);

	/** [INTERNAL] Links the animation sub-tree with the other entities within the document.
		This function is used at the end of the import of a document to verify that all the
		necessary drivers were found.
		@return The status of the linkage. */
	bool Link();

	/** [INTERNAL] Reads in the animation entity from a given COLLADA XML tree node.
		@param animationNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the animation. */
	virtual bool LoadFromXML(xmlNode* animationNode);

	/** [INTERNAL] Writes out the \<animation\> element to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the animation tree.
		@return The created element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;

	/** [INTERNAL] Retrieves the child source or sampler.
		This function should only be used by the FCDAnimationChannel class
		during the import of a COLLADA document.
		@param id The COLLADA id of a sampler or a source.
		@return The XML node tree for the sampler or the source. This pointer
			will be NULL if there are no child nodes for the given id. */
	xmlNode* FindChildById(const fm::string& id);

	/** [INTERNAL] Links a possible driver with the animation curves contained
		within the subtree of this animation element.
		This function is used during the import of a COLLADA document.
		@param animated The driver animated value.
		@return Whether any linkage was done. */
	bool LinkDriver(FCDAnimated* animated);
};

#endif // _FCD_ANIMATION_H_
