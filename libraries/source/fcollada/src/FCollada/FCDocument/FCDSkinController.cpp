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
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDGeometrySpline.h"
#include "FCDocument/FCDSkinController.h"
#include "FCDocument/FCDEntityReference.h"

//
// FCDSkinController
//

ImplementObjectType(FCDSkinController);

FCDSkinController::FCDSkinController(FCDocument* document, FCDController* _parent)
:	FCDObject(document)
,	parent(_parent)
,	InitializeParameter(bindShapeTransform, FMMatrix44::Identity)
{
	// Always create this.
	target = new FCDEntityReference(document, parent);
}

FCDSkinController::~FCDSkinController()
{
	SAFE_RELEASE(target);
}

FUUri FCDSkinController::GetTargetUri() const 
{ 
	return target->GetUri(); 
}

void FCDSkinController::SetTargetUri(const FUUri& uri) 
{ 
	target->SetUri(uri); 
}

FCDEntity* FCDSkinController::GetTarget()
{ 
	return target->GetEntity(); 
}

const FCDEntity* FCDSkinController::GetTarget() const 
{ 
	return target->GetEntity();
}

void FCDSkinController::SetTarget(FCDEntity* _target)
{
	target->SetEntity(NULL);
	SetNewChildFlag();

	// Retrieve the actual base entity, as you can chain controllers.
	FCDEntity* baseEntity = _target;
	if (baseEntity != NULL && baseEntity->GetType() == FCDEntity::CONTROLLER)
	{
		baseEntity = ((FCDController*) baseEntity)->GetBaseGeometry();
	}

	if (baseEntity == NULL || baseEntity->GetType() != FCDEntity::GEOMETRY)
	{
		// The new target is no good!
		return;
	}

	target->SetEntity(_target);
	FCDGeometry* geometry = (FCDGeometry*) baseEntity;

	// Retrieve the new vertex count
	size_t vertexCount = 0;
	if (geometry->IsMesh())
	{
		FCDGeometryMesh* mesh = geometry->GetMesh();
		FCDGeometrySource* positionSource = mesh->GetPositionSource();
		if (positionSource != NULL)
		{
			vertexCount = positionSource->GetValueCount();
		}
	}
	else if (geometry->IsSpline())
	{
		FCDGeometrySpline* spline = geometry->GetSpline();
		vertexCount = spline->GetTotalCVCount();
	}

	// Modify the list of influences to match the new target's vertex count.
	if (influences.size() != 0)
	{
		// This is only really acceptable with equivalent meshes.  Ensure we
		// are still compatable.
		FUAssert(vertexCount == influences.size(), SetInfluenceCount(vertexCount));
	}
	else
	{
		SetInfluenceCount(vertexCount);
	}
}

void FCDSkinController::SetInfluenceCount(size_t count)
{
	// None of the list structures are allocated directly: resize() will work fine.
	influences.resize(count);
	SetDirtyFlag();
}

void FCDSkinController::SetJointCount(size_t count)
{
	joints.resize(count);
	SetDirtyFlag();
}

FCDSkinControllerJoint* FCDSkinController::AddJoint(const fm::string jSubId, const FMMatrix44& bindPose)
{
	SetJointCount(GetJointCount() + 1);
	FCDSkinControllerJoint* joint = &joints.back();
	joint->SetId(jSubId);
	joint->SetBindPoseInverse(bindPose);
	SetDirtyFlag();
	return joint;
}


// Reduce the number of joints influencing each vertex to a maximum count
void FCDSkinController::ReduceInfluences(uint32 maxInfluenceCount, float minimumWeight)
{
	// Pre-cache an empty weight list to the reduced count
	fm::vector<FCDJointWeightPair> reducedWeights;
	reducedWeights.reserve(maxInfluenceCount + 1);

	for (FCDSkinControllerVertex* itM = influences.begin(); itM != influences.end(); ++itM)
	{
		FCDSkinControllerVertex& influence = (*itM);
		size_t oldInfluenceCount = influence.GetPairCount();

		// Reduce the weights, keeping only the more important ones using a sorting algorithm.
		// Also, calculate the current total of the weights, to re-normalize the reduced weights
		float oldTotal = 0.0f;
		reducedWeights.clear();
		for (size_t i = 0; i < oldInfluenceCount; ++i)
		{
			FCDJointWeightPair* pair = influence.GetPair(i);
			if (pair->weight >= minimumWeight)
			{
				FCDJointWeightPair* itRW = reducedWeights.begin();
				for (; itRW != reducedWeights.end() && (*itRW).weight > pair->weight; ++itRW) {}
				if (itRW != reducedWeights.end() || reducedWeights.size() <= maxInfluenceCount)
				{
					reducedWeights.insert(itRW, *pair);
					if (reducedWeights.size() > maxInfluenceCount) reducedWeights.pop_back();
				}
			}
			oldTotal += pair->weight;
		}

		size_t newInfluenceCount = reducedWeights.size();
		if (oldInfluenceCount > newInfluenceCount)
		{
			// Replace the old weights and re-normalize to their old total
			influence.SetPairCount(newInfluenceCount);
			for (size_t i = 0; i < newInfluenceCount; ++i) (*(influence.GetPair(i))) = reducedWeights[i];

			float newTotal = 0.0f;
			for (size_t i = 0; i < newInfluenceCount; ++i) newTotal += influence.GetPair(i)->weight;
			float renormalizingFactor = oldTotal / newTotal;
			for (size_t i = 0; i < newInfluenceCount; ++i) influence.GetPair(i)->weight *= renormalizingFactor;
		}
	}

	SetDirtyFlag();
}

//
// FCDSkinControllerVertex
//


void FCDSkinControllerVertex::SetPairCount(size_t count)
{
	pairs.resize(count);
}

void FCDSkinControllerVertex::AddPair(int32 jointIndex, float weight)
{
	pairs.push_back(FCDJointWeightPair(jointIndex, weight));
}

//
// FCDSkinControllerJoint
//

void FCDSkinControllerJoint::SetId(const fm::string& _id)
{
	// Do not inline, since the line below does memory allocation.
	id = _id;
}
