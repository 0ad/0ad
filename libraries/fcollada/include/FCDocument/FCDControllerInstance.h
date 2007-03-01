/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDControllerInstance.h
	This file contains the FCDControllerInstance class.
*/

#ifndef _FCD_CONTROLLER_INSTANCE_H_
#define _FCD_CONTROLLER_INSTANCE_H_

#ifndef _FCD_GEOMETRY_ENTITY_H_
#include "FCDocument/FCDGeometryInstance.h"
#endif // _FCD_GEOMETRY_ENTITY_H_

#ifndef _FU_URI_H_
#include "FUtils/FUUri.h"
#endif // _FU_URI_H_

class FCDSceneNode;
typedef fm::pvector<FCDSceneNode> FCDSceneNodeList; /**< A dynamically-sized array of visual scene nodes. */
typedef FUObjectList<FCDSceneNode> FCDSceneNodeTrackList; /**< A dynamically-sized array of tracked visual scene nodes. */

/**
	A COLLADA controller instance.

	When a COLLADA controller is instantiated, all its target(s)
	are instantiated in order to use them for the rendering or
	the logic. As such, all the information necessary to
	instantiate a geometry is also necessary to instantiate a
	controller.

	Each COLLADA skin controller should instantiate its own skeleton,
	for this reason, the skeleton root(s) are defined at instantiation.

	A controller instance should define the skeleton root joint.  Previously
	a FCDSkinController directly linked to its joints.  We now read the skeletonRoot
	here, and call which means that the FCDSkinController should never try and know about
	its own nodes.  It should all be linked through here.
*/
class FCDControllerInstance : public FCDGeometryInstance
{
private:
    DeclareObjectType(FCDGeometryInstance);

	FUUriList skeletonRoots;
	FCDSceneNodeTrackList joints;

public:
	/** Constructor: do not use directly.
		Instead, use the FCDSceneNode::AddInstance function.
		@param document The COLLADA document that owns the controller instance.
		@param parent The parent visual scene node.
		@param entityType The type of the entity instantiate.
			Unless the class is overwritten, FCDEntity::CONTROLLER should be given. */
	FCDControllerInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity::Type entityType = FCDEntity::CONTROLLER);

	/** Destructor. */
	virtual ~FCDControllerInstance();

	/** Retrieves the entity instance class type.
		This is used to determine the up-class for the entity instance object.
		@deprecated Instead use: FCDEntityInstance::HasType(FCDController::GetClassType())
		@return The class type: CONTROLLER. */
	virtual Type GetType() const { return CONTROLLER; }

	/** Clones the controller instance.
		@param clone The controller instance to become the clone.
			If this pointer is NULL, a new controller instance will be created
			and you will need to release the returned pointer.
		@return The clone. */
	virtual FCDEntityInstance* Clone(FCDEntityInstance* clone = NULL) const;

	// Load Controller properties
	virtual bool LoadFromXML(xmlNode* instanceNode);

	/** Retrieves a list of all the root joint ids for the controller.
		@return List of parent Ids */
	const FUUriList& GetSkeletonRoots() const { return skeletonRoots; }

	/** Set a skeleton root.
		@param The Dae ID of the node to use as skeleton root
		@return true if the root contains at least one child specified by
			the SubId's on the SkinController */
	bool SetSkeletonRoot(fstring id);

	/** Calculate our skeleton roots, based on the node list we have */
	void CalculateRootIds();

	/** Link the controller with the nodes it will be applied to */
	virtual bool LinkImport();

	/** Link the controller with the nodes it will be applied to */
	virtual bool LinkExport();

	/** [INTERNAL] Writes out the controller instance to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the node.
		@return The created XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;

	/** Retrieves the number of joints used by this controller.
		Joints only make sense when used with skin controllers.
		Defining the skeleton root affects the actual joints, but not the joint sids.
		@return The number of joints used by this controller. */
	size_t GetJointCount() const { return joints.size(); }

	/** Reset the joint lists. */
	void ResetJoints() { joints.clear(); skeletonRoots.clear(); }

	/** Retrieves a specific joint.
		@param index The index of the joint.
		@return The joint. This pointer will be NULL, if the index is out-of-bounds. */
	inline FCDSceneNode* GetJoint(size_t index) { FUAssert(index < GetJointCount(), return NULL); return joints.at(index); }
	inline const FCDSceneNode* GetJoint(size_t index) const { FUAssert(index < GetJointCount(), return NULL); return joints.at(index); } /**< See above. */

	/** Adds an existing joint to the list of controller joints.
		@param j A joint-typed scene node. */
	bool AddJoint(FCDSceneNode* j);

	/** Find a given joint in this skin instance.
		@param joint The joint.
		@return true if the node is present, false if not. */
	bool FindJoint(FCDSceneNode* joint);
	bool FindJoint(const FCDSceneNode* joint) const; /**< See above. */

	/** Find the index of the given joint in this skin instance
		@param joint The joint.
		@return The joints index, else -1 if not present. */
	int FindJointIndex(FCDSceneNode* joint);
private:
	void AppendJoint(FCDSceneNode* j);

	const FCDSkinController* FindSkin(const FCDEntity* entity) const;
	inline FCDSkinController* FindSkin(FCDEntity* entity) { return const_cast<FCDSkinController*>(const_cast<const FCDControllerInstance*>(this)->FindSkin(entity)); }
};

#endif // _FCD_CONTROLLER_INSTANCE_H_
