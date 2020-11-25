/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsRigidConstraint.h"
#include "FCDocument/FCDocument.h"
#include "FUtils/FUStringConversion.h"
#include "FUtils/FUUniqueStringMap.h"
#include "FUtils/FUUri.h"

ImplementObjectType(FCDPhysicsModel);

FCDPhysicsModel::FCDPhysicsModel(FCDocument* document)
:	FCDEntity(document, "PhysicsModel")
{
}

FCDPhysicsModel::~FCDPhysicsModel()
{
}

FCDPhysicsModelInstance* FCDPhysicsModel::AddPhysicsModelInstance(FCDPhysicsModel* model)
{
	FCDPhysicsModelInstance* instance = instances.Add(GetDocument());	
	instance->SetEntity(model);
	SetNewChildFlag(); 
	return instance;
}

FCDPhysicsRigidBody* FCDPhysicsModel::AddRigidBody()
{
	FCDPhysicsRigidBody* rigidBody = rigidBodies.Add(GetDocument());
	SetNewChildFlag(); 
	return rigidBody;
}

FCDPhysicsRigidConstraint* FCDPhysicsModel::AddRigidConstraint()
{
	FCDPhysicsRigidConstraint* constraint = rigidConstraints.Add(GetDocument(), this);
	SetNewChildFlag(); 
	return constraint;
}

FCDEntity* FCDPhysicsModel::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPhysicsModel* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsModel(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPhysicsModel::GetClassType())) clone = (FCDPhysicsModel*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		// Clone the rigid bodies
		for (FCDPhysicsRigidBodyContainer::const_iterator it = rigidBodies.begin(); it != rigidBodies.end(); ++it)
		{
			FCDPhysicsRigidBody* clonedRigidBody = clone->AddRigidBody();
			(*it)->Clone(clonedRigidBody, cloneChildren);
		}

		// Clone the rigid constraints
		for (FCDPhysicsRigidConstraintContainer::const_iterator it = rigidConstraints.begin(); it != rigidConstraints.end(); ++it)
		{
			FCDPhysicsRigidConstraint* clonedConstraint = clone->AddRigidConstraint();
			(*it)->Clone(clonedConstraint, cloneChildren);
		}

		// Clone the model instances
		for (FCDPhysicsModelInstanceContainer::const_iterator it = instances.begin(); it != instances.end(); ++it)
		{
			FCDPhysicsModelInstance* clonedInstance = clone->AddPhysicsModelInstance();
			(*it)->Clone(clonedInstance);
		}
	}
	return _clone;
}

const FCDPhysicsRigidBody* FCDPhysicsModel::FindRigidBodyFromSid(const fm::string& sid) const
{
	for (FCDPhysicsRigidBodyContainer::const_iterator it = rigidBodies.begin(); it!= rigidBodies.end(); ++it)
	{
		if ((*it)->GetSubId() == sid) return (*it);
	}
	return NULL;
}

const FCDPhysicsRigidConstraint* FCDPhysicsModel::FindRigidConstraintFromSid(const fm::string& sid) const
{
	for (FCDPhysicsRigidConstraintContainer::const_iterator it = rigidConstraints.begin(); it!= rigidConstraints.end(); ++it)
	{
		if ((*it)->GetSubId() == sid) return (*it);
	}
	return NULL;
}

bool FCDPhysicsModel::AttachModelInstances()
{
	bool status = true;
	while (!modelInstancesMap.empty())
	{
		ModelInstanceNameNodeMap::iterator modelNameNode = modelInstancesMap.begin();

		FUUri url = modelNameNode->second;
		if (!url.IsFile()) 
		{ 
			FCDEntity* entity = GetDocument()->FindPhysicsModel(TO_STRING(url.GetFragment()));
			if (entity != NULL) 
			{
				FCDPhysicsModel* model = (FCDPhysicsModel*) entity;

				//
				// [sli 2007-06-11] Need partial loading to be implemented for the plug-in.
				//
				//FCDPhysicsModelInstance* instance = AddPhysicsModelInstance(model);
				//status &= (instance->LoadFromXML(modelNameNode->first));

				// check for cyclic referencing
				fm::pvector<FCDPhysicsModel> modelQueue;
				modelQueue.push_back(model);
				while (!modelQueue.empty())
				{
					FCDPhysicsModel* currentModel = modelQueue.back();
					modelQueue.pop_back();

					if (currentModel == this)
					{
						// we have cyclic referencing!
						FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_PHYSICS_MODEL_CYCLE_DETECTED, 
								modelNameNode->first->line);
						status &= false;
						break;
					}

					FCDPhysicsModelInstanceContainer& modelInstances = currentModel->GetInstances();
					for (FCDPhysicsModelInstanceContainer::iterator modelInstancesIt = modelInstances.begin();
							modelInstancesIt != modelInstances.end(); modelInstancesIt++)
					{
						modelQueue.push_back((FCDPhysicsModel*)(*modelInstancesIt)->GetEntity());
					}
				}
			}
			else
			{
				FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_CORRUPTED_INSTANCE, modelNameNode->first->line);
			}
		}

		modelInstancesMap.erase(modelNameNode);
	}

	return status;
}

void FCDPhysicsModel::CleanSubId()
{
	FUSUniqueStringMap myStringMap;

	for (FCDPhysicsModelInstanceContainer::iterator it = instances.begin(); it != instances.end(); ++it)
	{
		(*it)->CleanSubId(&myStringMap);
	}
}
