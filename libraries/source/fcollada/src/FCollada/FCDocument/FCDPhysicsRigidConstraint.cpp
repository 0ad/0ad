/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsRigidConstraint.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDTransform.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDAnimated.h"
#include "FUtils/FUStringConversion.h"

//
// FCDPhysicsRigidConstraint
//

ImplementObjectType(FCDPhysicsRigidConstraint);

FCDPhysicsRigidConstraint::FCDPhysicsRigidConstraint(FCDocument* document, FCDPhysicsModel* _parent)
:	FCDEntity(document, "PhysicsRigidConstraint")
,	parent(_parent)
,	InitializeParameterAnimatable(enabled, 1.0f)
,	InitializeParameterAnimatable(interpenetrate, 0.0f)
,	InitializeParameter(limitsLinearMin, FMVector3::Zero)
,	InitializeParameter(limitsLinearMax, FMVector3::Zero)
,	InitializeParameter(limitsSCTMin, FMVector3::Zero)
,	InitializeParameter(limitsSCTMax, FMVector3::Zero)
,	InitializeParameter(springLinearStiffness, 1.0f)
,	InitializeParameter(springLinearDamping, 0.0f)
,	InitializeParameter(springLinearTargetValue, 0.0f)
,	InitializeParameter(springAngularStiffness, 1.0f)
,	InitializeParameter(springAngularDamping, 0.0f)
,	InitializeParameter(springAngularTargetValue, 0.0f)
{
}

FCDPhysicsRigidConstraint::~FCDPhysicsRigidConstraint()
{
	referenceRigidBody = NULL;
	targetRigidBody = NULL;
	transformsTar.clear();
	transformsRef.clear();
}

FCDTransform* FCDPhysicsRigidConstraint::AddTransformRef(FCDTransform::Type type, size_t index)
{
	FCDTransform* transform = FCDTFactory::CreateTransform(GetDocument(), NULL, type);
	if (transform != NULL)
	{
		if (index > transformsRef.size()) transformsRef.push_back(transform);
		else transformsRef.insert(transformsRef.begin() + index, transform);
	}
	SetNewChildFlag();
	return transform;
}

FCDTransform* FCDPhysicsRigidConstraint::AddTransformTar(FCDTransform::Type type, size_t index)
{
	FCDTransform* transform = FCDTFactory::CreateTransform(GetDocument(), NULL, type);
	if (transform != NULL)
	{
		if (index > transformsTar.size()) transformsTar.push_back(transform);
		else transformsTar.insert(transformsTar.begin() + index, transform);
	}
	SetNewChildFlag();
	return transform;
}

// Create a copy of this physicsRigidConstraint, with the vertices overwritten
FCDEntity* FCDPhysicsRigidConstraint::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPhysicsRigidConstraint* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsRigidConstraint(const_cast<FCDocument*>(GetDocument()), NULL);
	else if (_clone->HasType(FCDPhysicsRigidConstraint::GetClassType())) clone = (FCDPhysicsRigidConstraint*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		clone->enabled = enabled;
		clone->interpenetrate = interpenetrate;
		clone->referenceRigidBody = referenceRigidBody;
		clone->referenceNode = referenceNode;
		clone->targetRigidBody = targetRigidBody;
		clone->targetNode = targetNode;
		clone->limitsLinearMax = limitsLinearMax;
		clone->limitsLinearMin = limitsLinearMin;
		clone->limitsSCTMax = limitsSCTMax;
		clone->limitsSCTMin = limitsSCTMin;
		clone->springAngularDamping = springAngularDamping;
		clone->springAngularStiffness = springAngularStiffness;
		clone->springAngularTargetValue = springAngularTargetValue;
		clone->springLinearDamping = springLinearDamping;
		clone->springLinearStiffness = springLinearStiffness;
		clone->springLinearTargetValue = springLinearTargetValue;

		// Clone the transforms
		for (FCDTransformContainer::const_iterator it = transformsRef.begin(); it != transformsRef.end(); ++it)
		{
			FCDTransform* clonedTransform = clone->AddTransformRef((*it)->GetType());
			(*it)->Clone(clonedTransform);
		}
		for (FCDTransformContainer::const_iterator it = transformsTar.begin(); it != transformsTar.end(); ++it)
		{
			FCDTransform* clonedTransform = clone->AddTransformTar((*it)->GetType());
			(*it)->Clone(clonedTransform);
		}
	}
	return _clone;
}
