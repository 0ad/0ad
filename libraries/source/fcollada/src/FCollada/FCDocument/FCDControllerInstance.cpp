/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSkinController.h"
#include "FCDocument/FCDocument.h"

//
// FCDControllerInstance
//

ImplementObjectType(FCDControllerInstance);
ImplementParameterObjectNoCtr(FCDControllerInstance, FCDSceneNode, joints);

FCDControllerInstance::FCDControllerInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity::Type entityType)
:	FCDGeometryInstance(document, parent, entityType)
,	InitializeParameterNoArg(joints)
{
}

FCDControllerInstance::~FCDControllerInstance()
{
}

FCDEntityInstance* FCDControllerInstance::Clone(FCDEntityInstance* _clone) const
{
	FCDControllerInstance* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDControllerInstance(const_cast<FCDocument*>(GetDocument()), NULL, GetEntityType());
	else if (_clone->HasType(FCDControllerInstance::GetClassType())) clone = (FCDControllerInstance*) _clone;

	Parent::Clone(_clone);
	
	if (clone != NULL)
	{
		// Clone the URI list.
		clone->skeletonRoots = skeletonRoots;

		// Clone the joint list.
		clone->joints = joints;
	}
	return _clone;
}

// Retrieves a list of all the root joints for the controller.
void FCDControllerInstance::CalculateRootIds()
{
	skeletonRoots.clear();

	for (const FCDSceneNode** itJ = (const FCDSceneNode**) joints.begin(); itJ != joints.end(); ++itJ)
	{
		const FCDSceneNode* joint = (*itJ);
		if (joint == NULL) continue;

		bool addToList = true;
		size_t parentCount = joint->GetParentCount();
		for (size_t p = 0; p < parentCount; ++p)
		{
			const FCDSceneNode* parentJoint = joint->GetParent(p);
			if (FindJoint(parentJoint))
			{
				addToList = false;
				break;
			}
		}

		if (addToList)
		{
			fstring utf16id = TO_FSTRING(joint->GetDaeId());
			FUUri newRoot(FS("#") + utf16id);
			skeletonRoots.push_back(newRoot);
		}
	}
}

bool FCDControllerInstance::AddJoint(FCDSceneNode* j)
{ 
	if (j != NULL) 
	{ 
		j->SetJointFlag(true);
		AppendJoint(j);
		return true;
	}
	return false;
}

// Look for the information on a given joint
bool FCDControllerInstance::FindJoint(const FCDSceneNode* joint) const
{
	return joints.contains(joint);
}


size_t FCDControllerInstance::FindJointIndex(const FCDSceneNode* joint) const
{
	size_t i = 0;
	for (const FCDSceneNode** itr = joints.begin();  itr != joints.end(); ++i, ++itr)
	{
		if (*itr == joint) return i;
	}
	return (size_t) ~0;
}

void FCDControllerInstance::AppendJoint(FCDSceneNode* j) 
{ 
	joints.push_back(j);
}

const FCDSkinController* FCDControllerInstance::FindSkin(const FCDEntity* entity) const
{
	if (entity != NULL && entity->GetType() == FCDEntity::CONTROLLER)
	{
		const FCDController* controller = (const FCDController*) entity;
	
		if (controller->IsSkin()) 
		{
			return controller->GetSkinController();
		}
		else return FindSkin(controller->GetBaseTarget());
	}
	return NULL;
}

void FCDControllerInstance::FindSkeletonNodes(FCDSceneNodeList& skeletonNodes) const
{
	const FCDocument* document = GetDocument();
	size_t numRoots = skeletonRoots.size();
	skeletonNodes.reserve(numRoots);
	for (size_t i = 0; i < numRoots; ++i)
	{
		const FCDSceneNode* aRoot = document->FindSceneNode(TO_STRING(skeletonRoots[i].GetFragment()).c_str());
		if (aRoot == NULL)
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_JOINT, 0);
		}
		else skeletonNodes.push_back(const_cast<FCDSceneNode*>(aRoot));
	}

	// If we have no root, add the visual scene root.
	if (skeletonNodes.empty()) 
	{
		skeletonNodes.push_back(const_cast<FCDSceneNode*>(document->GetVisualSceneInstance()));
	}
}
