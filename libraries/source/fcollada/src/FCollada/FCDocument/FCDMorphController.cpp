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
#include "FCDocument/FCDMorphController.h"

//
// FCDMorphController
//

ImplementObjectType(FCDMorphController);
ImplementParameterObjectNoCtr(FCDMorphController, FCDEntity, baseTarget);
ImplementParameterObject(FCDMorphController, FCDMorphTarget, morphTargets, new FCDMorphTarget(parent->GetDocument(), parent));

FCDMorphController::FCDMorphController(FCDocument* document, FCDController* _parent)
:	FCDObject(document), parent(_parent)
,	InitializeParameter(method, FUDaeMorphMethod::NORMALIZED)
,	InitializeParameterNoArg(baseTarget)
,	InitializeParameterNoArg(morphTargets)
{
}

FCDMorphController::~FCDMorphController()
{
	parent = NULL;
}

// Changes the base target of the morpher
void FCDMorphController::SetBaseTarget(FCDEntity* entity)
{
	baseTarget = NULL;

	// Retrieve the actual base entity, as you can chain controllers.
	FCDEntity* baseEntity = entity;
	if (baseEntity != NULL && baseEntity->GetType() == FCDEntity::CONTROLLER)
	{
		baseEntity = ((FCDController*) baseEntity)->GetBaseGeometry();
	}
	if (baseEntity != NULL && baseEntity->GetType() == FCDEntity::GEOMETRY)
	{
		baseTarget = entity;

		// Remove the old morph targets which are not similar, anymore, to the new base entity.
		for (size_t i = 0; i < morphTargets.size();)
		{
			if (IsSimilar(morphTargets[i]->GetGeometry()))
			{
				++i;
			}
			else
			{
				morphTargets[i]->Release();
			}
		}
	}
	else
	{
		// The new base target is not valid.
		morphTargets.clear();
	}

	SetNewChildFlag();
}

// Adds a new morph target.
FCDMorphTarget* FCDMorphController::AddTarget(FCDGeometry* geometry, float weight)
{
	FCDMorphTarget* target = NULL;
	// It is legal to add targets with out a base geometry
	if (baseTarget == NULL || IsSimilar(geometry))
	{
		target = new FCDMorphTarget(GetDocument(), this);
		morphTargets.push_back(target);
		target->SetGeometry(geometry);
		target->SetWeight(weight);
	}
	SetNewChildFlag();
	return target;
}

// Retrieves whether a given entity is similar to the base target.
bool FCDMorphController::IsSimilar(FCDEntity* entity)
{
	bool similar = false;
	if (entity != NULL && baseTarget != NULL)
	{
		size_t vertexCount = 0;
		bool isMesh = false;
		bool isSpline = false;

		// Find the number of vertices in the base target
		FCDEntity* baseEntity = baseTarget;
		if (baseEntity != NULL && baseEntity->GetType() == FCDEntity::CONTROLLER)
		{
			baseEntity = ((FCDController*) baseEntity)->GetBaseGeometry();
		}
		if (baseEntity != NULL && baseEntity->GetType() == FCDEntity::GEOMETRY)
		{
			FCDGeometry* g = (FCDGeometry*) baseEntity;
			if (g->IsMesh())
			{
				isMesh = true;
				FCDGeometryMesh* m = g->GetMesh();
				FCDGeometrySource* positions = m->GetPositionSource();
				if (positions != NULL)
				{
					vertexCount = positions->GetValueCount();
				}
			}

			if (g->IsSpline())
			{
				isSpline = true;
				FCDGeometrySpline* s = g->GetSpline();
				vertexCount = s->GetTotalCVCount();
			}
		}


		// Find the number of vertices in the given entity
		baseEntity = entity;
		if (baseEntity != NULL && baseEntity->GetType() == FCDEntity::CONTROLLER)
		{
			baseEntity = ((FCDController*) baseEntity)->GetBaseGeometry();
		}
		if (baseEntity != NULL && baseEntity->GetType() == FCDEntity::GEOMETRY)
		{
			FCDGeometry* g = (FCDGeometry*) baseEntity;
			if (g->IsMesh() && isMesh)
			{
				FCDGeometryMesh* m = g->GetMesh();
				FCDGeometrySource* positions = m->GetPositionSource();
				if (positions != NULL)
				{
					similar = (vertexCount == positions->GetValueCount());
				}
			}

			if (g->IsSpline() && isSpline)
			{
				FCDGeometrySpline* s = g->GetSpline();
				similar = (vertexCount == s->GetTotalCVCount());
			}
		}
	}

	return similar;
}

//
// FCDMorphTarget
//

ImplementObjectType(FCDMorphTarget);
ImplementParameterObjectNoCtr(FCDMorphTarget, FCDGeometry, geometry);

FCDMorphTarget::FCDMorphTarget(FCDocument* document, FCDMorphController* _parent)
:	FCDObject(document), parent(_parent)
,	InitializeParameterNoArg(geometry)
,	InitializeParameterAnimatable(weight, 0.0f)
{
}

FCDMorphTarget::~FCDMorphTarget()
{
	parent = NULL;
}

void FCDMorphTarget::SetGeometry(FCDGeometry* _geometry)
{
	// Let go of the old geometry
	FCDGeometry* oldGeometry = geometry;
	if (oldGeometry != NULL && oldGeometry->GetTrackerCount() == 1)
	{
		SAFE_RELEASE(geometry);
	}

	// Check if this geometry is similar to the controller base target
	if (GetParent()->GetBaseTarget() == NULL || GetParent()->IsSimilar(_geometry))
	{
		geometry = _geometry;
	}
	SetNewChildFlag();
}

FCDAnimated* FCDMorphTarget::GetAnimatedWeight()
{
	return weight.GetAnimated();
}
const FCDAnimated* FCDMorphTarget::GetAnimatedWeight() const
{
	return weight.GetAnimated();
}

bool FCDMorphTarget::IsAnimated() const
{
	return weight.IsAnimated();
}
