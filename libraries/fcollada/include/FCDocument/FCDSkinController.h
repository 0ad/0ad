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
	@file FCDSkinController.h
	This file contains the FCDSkinController class.
*/

#ifndef _FCD_SKIN_CONTROLLER_H_
#define _FCD_SKIN_CONTROLLER_H_

#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_

class FCDocument;
class FCDController;
class FCDGeometry;
class FCDSceneNode;

/**
	A weighted joint index used in skinning.
*/
struct FCDJointWeightPair
{
	/** Default constructor: sets both the joint index and the weight to zero. */
	FCDJointWeightPair() { jointIndex = 0; weight = 0.0f; }

	/** Constructor: sets the joint index and the weight to the given values.
		@param _jointIndex The jointIndex.
		@param _weight Its weight. */
	FCDJointWeightPair(uint32 _jointIndex, float _weight) { jointIndex = _jointIndex; weight = _weight; }

	uint32 jointIndex; /**< A joint index. Use this index within the skin's joint list. */
	float weight; /**< A skinning weight. */
};

/** A dynamically-sized array of weighted joint indices.
	The sum of the weights within this list should always add up to one. */
typedef fm::vector<FCDJointWeightPair> FCDJointWeightPairList;

/** A dynamically-sized array of weighted joint index lists.
	Each entry within this list represents one vertex of the skinned geometry. */
typedef fm::vector<FCDJointWeightPairList> FCDWeightedMatches;

/**
	A COLLADA skin controller.

	The skin controller holds the information to skin a geometric object.
	That information includes a target/base entity and its bind-pose matrix,
	a list of joints and their bind pose and the influences for the joints.

	The influences are a list, for each vertex of the target entity, of which
	joints affect the vertex and by how much.

	@ingroup FCDGeometry
*/
class FCOLLADA_EXPORT FCDSkinController : public FCDObject
{
private:
	DeclareObjectType(FCDObject);
	FCDController* parent;

	FUObjectPtr<FCDEntity> target;
	FMMatrix44 bindShapeTransform;


	//fstring jointType;
	StringList jointIds;
	// One or other of the below has to go...
	FMMatrix44List jointBindPoses;
	FCDWeightedMatches weightedMatches;

	fm::string targetId; // Valid during load only

public:
	/** Constructor: do not use directly.
		Instead, use the FCDController::CreateSkinController function.
		@param document The COLLADA document that owns the skin.
		@param parent The COLLADA controller that contains this skin. */
	FCDSkinController(FCDocument* document, FCDController* parent);

	/** Destructor. */
	virtual ~FCDSkinController();
	
	/** Retrieves the parent entity for the morpher.
		@return The parent controller entity. */
	FCDController* GetParent() { return parent; }
	const FCDController* GetParent() const { return parent; } /**< See above. */

	/** Retrieves the target entity.
		This entity may be a geometric entity or another controller.
		@return The target entity. */
	FCDEntity* GetTarget() { return target; }
	const FCDEntity* GetTarget() const { return target; } /**< See above. */

	/** Sets the target entity.
		This function has very important ramifications, as the number
		of vertices may change. The influences list will be modified to
		follow the number of vertices.
		This entity may be a geometric entity or another controller.
		@param _target The target entity. */
	void SetTarget(FCDEntity* _target);

	/** Retrieves the bind-pose transform of the target entity.
		@return The bind-pose transform. */
	FMMatrix44& GetBindShapeTransform() { return bindShapeTransform; }
	const FMMatrix44& GetBindShapeTransform() const { return bindShapeTransform; } /**< See above. */

	/** Sets the bind-pose transform of the target entity.
		@param bindPose The bind-pose transform. */
	void SetBindShapeTransform(const FMMatrix44& bindPose) { bindShapeTransform = bindPose;	SetDirtyFlag(); }

	/** Retrieves the type of ids stored in jointIds.  This can be
		IDREF or Name 
		@return Return the type of ids listed */
	//const fstring& GetJointType() const { return jointType; }

	/** Return Joint poses.  */
	FMMatrix44List& GetBindPoses() { return jointBindPoses; }
	const FMMatrix44List& GetBindPoses() const { return jointBindPoses; } /**< See above. */

	/** Set a Joint pose.  This will not resize the joint array 
		@param i Index to set pose at
		@param p Matrix to store as pose */
	void SetBindPose(size_t i, const FMMatrix44 &p) { FUAssert(i < jointBindPoses.size(), return); jointBindPoses[i] = p; }

	/** Retreives the list of joint ids for this skin 
		@return The list of joint ids */
	StringList& GetJointIds() { return jointIds; }
	const StringList& GetJointIds() const { return jointIds; }

	/** Retrieves the number of joints that influence the skin.
		@return The number of joints. */
	size_t GetJointCount() const { return jointBindPoses.size(); }

	/** Adds a joint and its bind-pose to the list of joint influencing the skin.
		@param joint The joint.
		@param bindPose The joint's bind-pose. */
	void AddJoint(const fm::string jSubId, const FMMatrix44& bindPose);

	/** Removes a joint from the list of joints influencing the skin.
		All the per-vertex influences that use this joint will be removed.
		@param joint The joint. */
	//void RemoveJoint(FCDSceneNode* joint);

	/** Retrieves a list of the per-vertex influences for the skin.
		You should not modify the size of the list. Instead, use the SetTarget function.
		@deprecated Will be replaces by GetVertexInfluences.
		@return The list of per-vertex influences. */
	FCDWeightedMatches& GetWeightedMatches() { return weightedMatches; }
	const FCDWeightedMatches& GetWeightedMatches() const { return weightedMatches; } /**< See above. */

	/** Retrieves a list of the per-vertex influences for the skin.
		You should not modify the size of the list. Instead, use the SetTarget function.
		@return The list of per-vertex influences. */
	FCDWeightedMatches& GetVertexInfluences() { return weightedMatches; }
	const FCDWeightedMatches& GetVertexInfluences() const { return weightedMatches; } /**< See above. */

	/** Retrieves the number of per-vertex influences.
		This value should be equal to the number of vertices/control points
		within the target geometric entity.
		@return The number of per-vertex influences. */
	size_t GetVertexInfluenceCount() const { return weightedMatches.size(); }

	/** Retrieves the per-vertex influences for a given vertex. 
		@param index The vertex index.
		@return The per-vertex influences. */
	FCDJointWeightPairList* GetInfluences(size_t index) { FUAssert(index < GetVertexInfluenceCount(), return NULL); return &weightedMatches.at(index); }
	const FCDJointWeightPairList* GetInfluences(size_t index) const { FUAssert(index < GetVertexInfluenceCount(), return NULL); return &weightedMatches.at(index); } /**< See above. */

	/** Reduces the number of joints influencing each vertex.
		1) All the influences with a weight less than the minimum will be removed.
		2) If a vertex has more influences than the given maximum, they will be sorted and the
			most important influences will be kept.
		If some of the influences for a vertex are removed, the weight will be normalized.
		@param maxInfluenceCount The maximum number of influence to keep for each vertex.
		@param minimumWeight The smallest weight to keep. */
	void ReduceInfluences(uint32 maxInfluenceCount, float minimumWeight=0.0f);

	/** [INTERNAL] Reads in the \<skin\> element from a given COLLADA XML tree node.
		@param skinNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the skin.*/
	bool LoadFromXML(xmlNode* skinNode);

	/** [INTERNAL] Writes out the \<skin\> element to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the skin information.
		@return The created element XML tree node. */
	xmlNode* WriteToXML(xmlNode* parentNode) const;

	/** [INTERNAL] Links a controller to its target after load.  This
		function will be called by an entities instances after load,
		after which it will become invalid and do nothing */
	virtual bool LinkImport();
};

#endif // _FCD_SKIN_CONTROLLER_H_

