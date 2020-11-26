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
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDForceField.h"
#include "FCDocument/FCDPhysicsForceFieldInstance.h"
#include "FCDocument/FCDPhysicsScene.h"
#include "FUtils/FUFileManager.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUUniqueStringMap.h"
#include "FCDocument/FCDExtra.h"

ImplementObjectType(FCDPhysicsScene);

FCDPhysicsScene::FCDPhysicsScene(FCDocument* document)
:	FCDEntity(document, "PhysicsSceneNode")
,	gravity(0.0f, -9.8f, 0.0f), timestep(1.0f)
{
}

FCDPhysicsScene::~FCDPhysicsScene()
{
}

FCDEntity* FCDPhysicsScene::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPhysicsScene* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsScene(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPhysicsScene::GetClassType())) clone = (FCDPhysicsScene*) _clone;

	Parent::Clone(_clone, cloneChildren);
	
	if (clone == NULL)
	{
		// Clone the miscellaneous parameters
		clone->gravity = gravity;
		clone->timestep = timestep;

		// Clone the physics model instances
		for (FCDPhysicsModelInstanceContainer::const_iterator it = physicsModelInstances.begin(); it != physicsModelInstances.end(); ++it)
		{
			FCDPhysicsModelInstance* clonedInstance = clone->AddPhysicsModelInstance();
			(*it)->Clone(clonedInstance);
		}

		// Clone the force field instances
		for (FCDForceFieldInstanceContainer::const_iterator it = forceFieldInstances.begin(); it != forceFieldInstances.end(); ++it)
		{
			FCDPhysicsForceFieldInstance* clonedInstance = clone->AddForceFieldInstance();
			(*it)->Clone(clonedInstance);
		}
	}
	return _clone;
}

FCDPhysicsModelInstance* FCDPhysicsScene::AddPhysicsModelInstance(FCDPhysicsModel* model)
{
	FCDPhysicsModelInstance* instance = physicsModelInstances.Add(GetDocument());
	instance->SetEntity(model);
	SetNewChildFlag();
	return instance;
}

FCDPhysicsForceFieldInstance* FCDPhysicsScene::AddForceFieldInstance(FCDForceField* forceField)
{
	FCDPhysicsForceFieldInstance* instance = (FCDPhysicsForceFieldInstance*)
			FCDEntityInstanceFactory::CreateInstance(
					GetDocument(), (FCDSceneNode*) NULL, forceField);
	forceFieldInstances.push_back(instance);
	SetNewChildFlag();
	return instance;
}

void FCDPhysicsScene::CleanSubId()
{
	FUSUniqueStringMap myStringMap;

	for (FCDForceFieldInstanceContainer::iterator itI = forceFieldInstances.begin(); itI != forceFieldInstances.end(); ++itI)
	{
		(*itI)->CleanSubId(&myStringMap);
	}

	for (FCDPhysicsModelInstanceContainer::iterator itI = physicsModelInstances.begin(); itI != physicsModelInstances.end(); ++itI)
	{
		(*itI)->CleanSubId(&myStringMap);
	}
}
