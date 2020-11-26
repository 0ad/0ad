/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsRigidBodyInstance.h"
#include "FCDocument/FCDPhysicsRigidBodyParameters.h"
#include "FCDocument/FCDSceneNode.h"
#include "FUtils/FUEvent.h"
#include "FUtils/FUUri.h"

//
// FCDPhysicsRigidBodyInstance
//

ImplementObjectType(FCDPhysicsRigidBodyInstance);
ImplementParameterObject(FCDPhysicsRigidBodyInstance, FCDPhysicsRigidBodyParameters, parameters, new FCDPhysicsRigidBodyParameters(parent->GetDocument(), parent));
ImplementParameterObjectNoCtr(FCDPhysicsRigidBodyInstance, FCDSceneNode, targetNode);

FCDPhysicsRigidBodyInstance::FCDPhysicsRigidBodyInstance(FCDocument* document, FCDPhysicsModelInstance* _parent, FCDPhysicsRigidBody* body)
:	FCDEntityInstance(document, NULL, FCDEntity::PHYSICS_RIGID_BODY)
,	parent(_parent), onCollisionEvent(NULL)
,	InitializeParameterAnimatable(velocity, FMVector3::Zero)
,	InitializeParameterAnimatable(angularVelocity, FMVector3::Zero)
,	InitializeParameterNoArg(parameters)
,	InitializeParameterNoArg(targetNode)
{
	onCollisionEvent = new CollisionEvent();
	parameters = new FCDPhysicsRigidBodyParameters(document, this);
	if (body != NULL) SetRigidBody(body);
}

FCDPhysicsRigidBodyInstance::~FCDPhysicsRigidBodyInstance()
{
	parent = NULL;
	SAFE_DELETE(onCollisionEvent);
}

FCDEntityInstance* FCDPhysicsRigidBodyInstance::Clone(FCDEntityInstance* _clone) const
{
	FCDPhysicsRigidBodyInstance* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsRigidBodyInstance(const_cast<FCDocument*>(GetDocument()), NULL, NULL);
	else clone = DynamicCast<FCDPhysicsRigidBodyInstance>(_clone);

	Parent::Clone(_clone);
	
	if (clone != NULL)
	{
		clone->angularVelocity = angularVelocity;
		clone->velocity = velocity;
		clone->GetParameters()->CopyFrom(*parameters);

		// Intentionally leave the target scene node as NULL.
	}
	return _clone;
}

void FCDPhysicsRigidBodyInstance::SetRigidBody(FCDPhysicsRigidBody* body)
{
	FUAssert(body != NULL, return);

	SetEntity(body); 

	// copy some of the default values from the body
	FCDPhysicsRigidBodyParameters* bodyParams = body->GetParameters();
	parameters->SetDynamic(bodyParams->IsDynamic());
	parameters->SetMass(bodyParams->GetMass());
	parameters->SetMassFrameTranslate(bodyParams->GetMassFrameTranslate());
	parameters->SetMassFrameRotateAxis(bodyParams->GetMassFrameRotateAxis());
	parameters->SetMassFrameRotateAngle(bodyParams->GetMassFrameRotateAngle());
	parameters->SetInertia(bodyParams->GetInertia());
	parameters->SetDensity(bodyParams->GetDensity());
	parameters->SetDensityMoreAccurate(bodyParams->IsDensityMoreAccurate());
	parameters->SetInertiaAccurate(bodyParams->IsInertiaAccurate());
}
