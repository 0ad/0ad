/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDPhysicsRigidBodyParameters.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsRigidBodyInstance.h"
#include "FCDocument/FCDPhysicsMaterial.h"
#include "FCDocument/FCDPhysicsShape.h"

//
// FCDPhysicsRigidBodyParameters
//

ImplementObjectType(FCDPhysicsRigidBodyParameters);
ImplementParameterObject(FCDPhysicsRigidBodyParameters, FCDPhysicsShape, physicsShape, new FCDPhysicsShape(parent->GetDocument()));

FCDPhysicsRigidBodyParameters::FCDPhysicsRigidBodyParameters(FCDocument* document, FCDPhysicsRigidBody* _owner)
:	FCDObject(document)
,	ownsPhysicsMaterial(false)
,	parent(_owner)
,	InitializeParameterNoArg(physicsShape)
,	InitializeParameterAnimatable(dynamic, true)
,	InitializeParameterAnimatable(mass, 1.0f)
,	InitializeParameter(density, 0.0f)
,	InitializeParameterAnimatable(inertia, FMVector3::Zero)
,	InitializeParameterAnimatable(massFrameTranslate, FMVector3::Zero)
,	InitializeParameterAnimatable(massFrameOrientation, FMAngleAxis(FMVector3::XAxis, 0.0f))
,	entityOwner(_owner)
,	InitializeParameter(isDensityMoreAccurate, false)
,	InitializeParameter(isInertiaAccurate, false)
{
}

FCDPhysicsRigidBodyParameters::FCDPhysicsRigidBodyParameters(FCDocument* document, FCDPhysicsRigidBodyInstance* _owner)
:	FCDObject(document)
,	ownsPhysicsMaterial(false)
,	parent(_owner)
,	InitializeParameterNoArg(physicsShape)
,	InitializeParameterAnimatable(dynamic, true)
,	InitializeParameterAnimatable(mass, 1.0f)
,	InitializeParameter(density, 0.0f)
,	InitializeParameterAnimatable(inertia, FMVector3::Zero)
,	InitializeParameterAnimatable(massFrameTranslate, FMVector3::Zero)
,	InitializeParameterAnimatable(massFrameOrientation, FMAngleAxis(FMVector3::XAxis, 0.0f))
,	instanceOwner(_owner)
,	InitializeParameter(isDensityMoreAccurate, false)
,	InitializeParameter(isInertiaAccurate, false)
{
}

FCDPhysicsRigidBodyParameters::~FCDPhysicsRigidBodyParameters()
{
	if (physicsMaterial && ownsPhysicsMaterial) SAFE_RELEASE(physicsMaterial);
	SAFE_RELEASE(instanceMaterialRef);
	if (ownsPhysicsMaterial)
	{
		SAFE_RELEASE(physicsMaterial);
	}
	else
	{
		physicsMaterial = NULL;
	}
}

void FCDPhysicsRigidBodyParameters::CopyFrom(const FCDPhysicsRigidBodyParameters& original)
{
	// copy everything except parent since this is set already in constructor

	dynamic = original.dynamic;
	mass = original.mass;
	inertia = original.inertia;
	massFrameTranslate = original.massFrameTranslate;
	massFrameOrientation = original.massFrameOrientation;

	for (const FCDPhysicsShape** it = original.physicsShape.begin(); it != original.physicsShape.end(); ++it)
	{
		FCDPhysicsShape* clonedShape = AddPhysicsShape();
		(*it)->Clone(clonedShape);
	}

	if (original.physicsMaterial != NULL)
	{
		if (parent->IsLocal(original.parent))
		{
			SetPhysicsMaterial(const_cast<FCDPhysicsMaterial*> (original.physicsMaterial.operator->()));
		}
		else
		{
			FCDPhysicsMaterial* clonedMaterial = AddOwnPhysicsMaterial();
			original.physicsMaterial->Clone(clonedMaterial);
		}
	}

	// Clone the material instance
	if (original.instanceMaterialRef != NULL)
	{
		instanceMaterialRef = original.instanceMaterialRef->Clone();
	}
}

FCDPhysicsShape* FCDPhysicsRigidBodyParameters::AddPhysicsShape()
{
	FCDPhysicsShape* shape = new FCDPhysicsShape(GetDocument());
	physicsShape.push_back(shape);
	parent->SetNewChildFlag();
	return shape;
}

void FCDPhysicsRigidBodyParameters::SetPhysicsMaterial(FCDPhysicsMaterial* _physicsMaterial)
{
	if (physicsMaterial && ownsPhysicsMaterial)
	{
		SAFE_RELEASE(physicsMaterial);
	}

	physicsMaterial = _physicsMaterial;
	ownsPhysicsMaterial = false;
	parent->SetNewChildFlag();
}

void FCDPhysicsRigidBodyParameters::SetDynamic(bool _dynamic)
{ 
	dynamic = _dynamic;
	parent->SetDirtyFlag();
}

FCDPhysicsMaterial* FCDPhysicsRigidBodyParameters::AddOwnPhysicsMaterial()
{
	if (physicsMaterial && ownsPhysicsMaterial)
	{
		SAFE_RELEASE(physicsMaterial);
	}

	physicsMaterial = new FCDPhysicsMaterial(parent->GetDocument());
	ownsPhysicsMaterial = true;
	parent->SetNewChildFlag();
	return physicsMaterial;
}
