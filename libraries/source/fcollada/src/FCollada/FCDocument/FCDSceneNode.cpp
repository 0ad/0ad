/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDCamera.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDLight.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDPhysicsRigidBodyInstance.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDTransform.h"
#include "FUtils/FUFileManager.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUUniqueStringMap.h"

//
// FCDSceneNode
//

ImplementObjectType(FCDSceneNode);
ImplementParameterObjectNoCtr(FCDSceneNode, FCDSceneNode, parents);
ImplementParameterObject(FCDSceneNode, FCDSceneNode, children, new FCDSceneNode(parent->GetDocument()));
ImplementParameterObjectNoCtr(FCDSceneNode, FCDTransform, transforms); /** Needs custom construction in the UI */
ImplementParameterObjectNoCtr(FCDSceneNode, FCDEntityInstance, instances); /** Needs custom construction in the UI */

FCDSceneNode::FCDSceneNode(FCDocument* document)
:	FCDEntity(document, "VisualSceneNode")
,	InitializeParameterNoArg(parents)
,	InitializeParameterNoArg(children)
,	InitializeParameterNoArg(transforms)
,	InitializeParameterNoArg(instances)
,	InitializeParameterAnimatable(visibility, 1.0f)
,	targetCount(0)
,	InitializeParameterNoArg(daeSubId)
{
	SetTransformsDirtyFlag();
	ResetJointFlag();
}

FCDSceneNode::~FCDSceneNode()
{
	parents.clear();

	// Delete the children, be watchful for the instantiated nodes
	while (!children.empty())
	{
		FCDSceneNode* child = children.front();
		child->parents.erase(this);
		
		if (child->parents.empty()) { SAFE_RELEASE(child); }
		else
		{
			// Check for external references in the parents
			bool hasLocalReferences = false;
			size_t parentCount = parents.size();
			for (size_t p = 0; p < parentCount; ++p)
			{
				FCDSceneNode* parent = parents[p];
				if (parent == this) children.erase(parent);
				else
				{
					hasLocalReferences |= parent->GetDocument() == GetDocument();
				}
			}

			if (!hasLocalReferences) SAFE_RELEASE(child);
		}
	}
}

// Add this scene node to the list of children scene node
bool FCDSceneNode::AddChildNode(FCDSceneNode* sceneNode)
{
	if (this == sceneNode || sceneNode == NULL)
	{
		return false;
	}

	// Verify that we don't already contain this child node.
	if (children.contains(sceneNode)) return false;

	// Verify that this node is not one of the parents in the full hierarchically.
	fm::pvector<FCDSceneNode> queue;
	size_t parentCount = parents.size();
	for (size_t i = 0; i < parentCount; ++i) queue.push_back(parents.at(i));
	while (!queue.empty())
	{
		FCDSceneNode* parent = queue.back();
		queue.pop_back();
		if (parent == sceneNode) return false;
		queue.insert(queue.end(), parent->parents.begin(), parent->parents.end());
	}

	children.push_back(sceneNode);
	sceneNode->parents.push_back(this);
	SetNewChildFlag();
	return true;
}

FCDSceneNode* FCDSceneNode::AddChildNode()
{
	FCDSceneNode* node = new FCDSceneNode(GetDocument());
	AddChildNode(node);
	return node;
}

void FCDSceneNode::RemoveChildNode(FCDSceneNode* sceneNode)
{
	sceneNode->parents.erase(this);
	children.erase(sceneNode);
}

// Instantiates an entity
FCDEntityInstance* FCDSceneNode::AddInstance(FCDEntity* entity)
{
	if (entity == NULL) return NULL;
	FCDEntityInstance* instance =  AddInstance(entity->GetType());
	instance->SetEntity(entity);
	return instance;
}

FCDEntityInstance* FCDSceneNode::AddInstance(FCDEntity::Type type)
{
	FCDEntityInstance* instance = FCDEntityInstanceFactory::CreateInstance(GetDocument(), this, type);
	
	instances.push_back(instance);
	SetNewChildFlag();
	return instance;
}

// Adds a transform to the stack, at a given position.
FCDTransform* FCDSceneNode::AddTransform(FCDTransform::Type type, size_t index)
{
	FCDTransform* transform = FCDTFactory::CreateTransform(GetDocument(), this, type);
	if (transform != NULL)
	{
		if (index > transforms.size()) transforms.push_back(transform);
		else transforms.insert(index, transform);
	}
	SetNewChildFlag();
	SetTransformsDirtyFlag();
	return transform;
}

// Traverse the scene graph, searching for a node with the given COLLADA id
const FCDEntity* FCDSceneNode::FindDaeId(const fm::string& daeId) const
{
	if (GetDaeId() == daeId) return this;
	
	for (const FCDSceneNode** it = children.begin(); it != children.end(); ++it)
	{
		const FCDEntity* found = (*it)->FindDaeId(daeId);
		if (found != NULL) return found;
	}
	return NULL;
}

void FCDSceneNode::SetSubId(const fm::string& subId)
{
	daeSubId = "";
	if (subId.empty()) return;

	// We must ensure that our sub id is unique in the our scope
	// First, build a list of subIds above us (in the graph)
	FCDSceneNode* curNode = this;

	// Flatten parents up-tree to find all parents subIds
	fm::pvector<FCDSceneNode> parentTree;
	StringList parentSubIds;
	size_t curParent = 0;
	do
	{
		for (size_t i = 0; i < curNode->GetParentCount(); i++)
		{
			FCDSceneNode* parentNode = curNode->GetParent(i);
			parentTree.push_back(parentNode);
			fm::string parentSubId = parentNode->GetSubId();
			if (!parentSubId.empty()) parentSubIds.push_back(parentSubId);
		}
		// Continue iteration.
		if (parentTree.size() > curParent)
		{
			curNode = parentTree[curParent];
			curParent++;
		}
		else curNode = NULL;
	}
	while (curNode != NULL);

	// Now, test for uniqueness, This is enforced for both descendants and ancestors
	fm::string newSubId = FCDObjectWithId::CleanSubId(subId);
	int32 idMod = 0;
	while (FindSubId(newSubId) != NULL || parentSubIds.find(newSubId) != parentSubIds.end())
	{
		newSubId = subId + "_" + FUStringConversion::ToString(idMod++);

		// Dont keep doing this forever.
		if (idMod > 512) break;
	}
	daeSubId = newSubId;
}

// Traverse the scene graph, searching for a node with the given COLLADA sub id
const FCDEntity* FCDSceneNode::FindSubId(const fm::string& subId) const
{
	if (GetSubId() == subId) return this;
	
	for (const FCDSceneNode** it = children.begin(); it != children.end(); ++it)
	{
		const FCDEntity* found = (*it)->FindSubId(subId);
		if (found != NULL) return found;
	}
	return NULL;
}

// Retrieve the list of hierarchical asset information structures that affect this scene node.
void FCDSceneNode::GetHierarchicalAssets(FCDAssetConstList& assets) const
{
	for (const FCDSceneNode* node = this; node != NULL; node = node->GetParent(0))
	{
		// Retrieve the asset information structure for this node.
		const FCDAsset* asset = node->GetAsset();
		if (asset != NULL) assets.push_back(asset);
	}
	assets.push_back(GetDocument()->GetAsset());
}

// Calculate the transform matrix for a given scene node
FMMatrix44 FCDSceneNode::ToMatrix() const
{
	FMMatrix44 localTransform = FMMatrix44::Identity;
	for (const FCDTransform** it = transforms.begin(); it != transforms.end(); ++it)
	{
		localTransform = localTransform * (*it)->ToMatrix();
	}
	return localTransform;
}

FMMatrix44 FCDSceneNode::CalculateWorldTransform() const
{
	const FCDSceneNode* parent = GetParent();
	if (parent != NULL)
	{
		//FMMatrix44 tm1 = parent->CalculateWorldTransform();
		//FMMatrix44 tm2 = CalculateLocalTransform();
		//return tm1 * tm2;
		return parent->CalculateWorldTransform() * CalculateLocalTransform();
	}
	else
	{
		return CalculateLocalTransform();
	}
}

void FCDSceneNode::CleanSubId()
{
	FUSUniqueStringMap myStringMap;

	size_t instanceCount = instances.size();
	for (size_t i = 0; i < instanceCount; ++i)
	{
		instances[i]->CleanSubId(&myStringMap);
	}

	size_t childCount = children.size();
	for (size_t c = 0; c < childCount; ++c)
	{
		children[c]->CleanSubId();
	}
}

FCDEntity* FCDSceneNode::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDSceneNode* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDSceneNode(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDSceneNode::GetClassType())) clone = (FCDSceneNode*) _clone;
	
	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		// Copy over the simple information.
		clone->SetJointFlag(GetJointFlag());
		clone->visibility = visibility;

		// Don't copy the parents list but do clone all the children, transforms and instances
		for (const FCDTransform** it = transforms.begin(); it != transforms.end(); ++it)
		{
			FCDTransform* transform = clone->AddTransform((*it)->GetType());
			(*it)->Clone(transform);
		}

		if (cloneChildren)
		{
			for (const FCDSceneNode** it = children.begin(); it != children.end(); ++it)
			{
				FCDSceneNode* child = clone->AddChildNode();
				(*it)->Clone(child, cloneChildren);
			}
		}

		for (const FCDEntityInstance** it = instances.begin(); it != instances.end(); ++it)
		{
			FCDEntityInstance* instance = clone->AddInstance((*it)->GetEntityType());
			(*it)->Clone(instance);
		}
	}

	return _clone;
}
