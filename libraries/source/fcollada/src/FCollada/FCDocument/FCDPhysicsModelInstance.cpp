/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDForceField.h"
#include "FCDocument/FCDPhysicsForceFieldInstance.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDPhysicsRigidBodyInstance.h"
#include "FCDocument/FCDPhysicsRigidConstraintInstance.h"
#include "FUtils/FUUniqueStringMap.h"
#include "FUtils/FUUri.h"

//
// FCDPhysicsModelInstance
//

ImplementObjectType(FCDPhysicsModelInstance);
ImplementParameterObjectNoCtr(FCDPhysicsModelInstance, FCDEntityInstance, instances);

FCDPhysicsModelInstance::FCDPhysicsModelInstance(FCDocument* document)
:	FCDEntityInstance(document, NULL, FCDEntity::PHYSICS_MODEL)
,	InitializeParameterNoArg(instances)
{
}

FCDPhysicsModelInstance::~FCDPhysicsModelInstance()
{
}

FCDPhysicsRigidBodyInstance* FCDPhysicsModelInstance::AddRigidBodyInstance(FCDPhysicsRigidBody* rigidBody)
{
	FCDPhysicsRigidBodyInstance* instance = new FCDPhysicsRigidBodyInstance(GetDocument(), this, rigidBody);
	instances.push_back(instance);
	SetNewChildFlag();
	return instance;
}

FCDPhysicsRigidConstraintInstance* FCDPhysicsModelInstance::AddRigidConstraintInstance(FCDPhysicsRigidConstraint* rigidConstraint)
{
	FCDPhysicsRigidConstraintInstance* instance = new FCDPhysicsRigidConstraintInstance(GetDocument(), this, rigidConstraint);
	instances.push_back(instance);
	SetNewChildFlag();
	return instance;
}

FCDPhysicsForceFieldInstance* FCDPhysicsModelInstance::AddForceFieldInstance(FCDForceField* forceField)
{
	FCDEntityInstance* instance = FCDEntityInstanceFactory::CreateInstance(GetDocument(), (FCDSceneNode*) NULL, forceField);
	instances.push_back(instance);
	SetNewChildFlag();
	return (FCDPhysicsForceFieldInstance*)instance;
}

bool FCDPhysicsModelInstance::RemoveInstance(FCDEntityInstance* instance)
{
	SAFE_RELEASE(instance);
	return true;
}

FCDEntityInstance* FCDPhysicsModelInstance::Clone(FCDEntityInstance* _clone) const
{
	FCDPhysicsModelInstance* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsModelInstance(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPhysicsModelInstance::GetClassType())) clone = (FCDPhysicsModelInstance*) _clone;
	
	Parent::Clone(_clone);

	if (clone != NULL)
	{
		for (const FCDEntityInstance** it = instances.begin(); it != instances.end(); ++it)
		{
			FCDEntityInstance* clonedInstance = NULL;
			switch ((*it)->GetEntityType())
			{
			case FCDEntity::PHYSICS_RIGID_BODY: clonedInstance = clone->AddRigidBodyInstance(); break;
			case FCDEntity::PHYSICS_RIGID_CONSTRAINT: clonedInstance = clone->AddRigidConstraintInstance(); break;
			case FCDEntity::FORCE_FIELD: clonedInstance = clone->AddForceFieldInstance(); break;
			default: FUFail(break);
			}
			if (clonedInstance != NULL) (*it)->Clone(clonedInstance);
		}
	}
	return _clone;
}

void FCDPhysicsModelInstance::CleanSubId(FUSUniqueStringMap* parentStringMap)
{
	Parent::CleanSubId(parentStringMap);
	FUSUniqueStringMap myStringMap;

	size_t subInstanceCount = instances.size();
	for (size_t i = 0; i < subInstanceCount; ++i)
	{
		instances[i]->CleanSubId(&myStringMap);
	}
}
