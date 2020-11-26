/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsRigidBodyParameters.h"
#include "FCDocument/FCDPhysicsShape.h"

//
// FCDPhysicsRigidBody
//

ImplementObjectType(FCDPhysicsRigidBody);
ImplementParameterObject(FCDPhysicsRigidBody, FCDPhysicsRigidBodyParameters, parameters, new FCDPhysicsRigidBodyParameters(parent->GetDocument(), parent));

FCDPhysicsRigidBody::FCDPhysicsRigidBody(FCDocument* document)
:	FCDEntity(document, "RigidBody")
,	InitializeParameterNoArg(parameters)
{
	parameters = new FCDPhysicsRigidBodyParameters(document, this);
}

FCDPhysicsRigidBody::~FCDPhysicsRigidBody()
{
}

FCDEntity* FCDPhysicsRigidBody::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPhysicsRigidBody* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsRigidBody(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPhysicsRigidBody::GetClassType())) clone = (FCDPhysicsRigidBody*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		clone->GetParameters()->CopyFrom(*parameters);
	}
	return _clone;
}

float FCDPhysicsRigidBody::GetShapeMassFactor() const
{
	float shapesMass = 0.0f;
	size_t shapeCount = parameters->GetPhysicsShapeCount();
	for (size_t s = 0; s < shapeCount; ++s)
	{
		shapesMass += parameters->GetPhysicsShape(s)->GetMass();
	}
	return parameters->GetMass() / shapesMass;
}
