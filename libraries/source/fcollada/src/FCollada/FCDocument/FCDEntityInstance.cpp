/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument.h"
#include "FCDEntity.h"
#include "FCDEntityInstance.h"
#include "FCDExtra.h"
#include "FCDSceneNode.h"
#include "FCDControllerInstance.h"
#include "FCDEmitterInstance.h"
#include "FCDGeometryInstance.h"
#include "FCDPhysicsForceFieldInstance.h"
#include "FCDEntityReference.h"
#include <FUtils/FUFileManager.h>
#include <FUtils/FUUniqueStringMap.h>

//
// FCDEntityInstance
//

ImplementObjectType(FCDEntityInstance);
ImplementParameterObject(FCDEntityInstance, FCDEntityReference, entityReference, new FCDEntityReference(parent->GetDocument(), parent->GetParent()));
ImplementParameterObject(FCDEntityInstance, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));

FCDEntityInstance::FCDEntityInstance(FCDocument* document, FCDSceneNode* _parent, FCDEntity::Type type)
:	FCDObject(document), parent(_parent)
,	entityType(type)
,	InitializeParameterNoArg(entityReference)
,	InitializeParameterNoArg(wantedSubId)
,	InitializeParameterNoArg(extra)
{
	// Always create this
	entityReference = new FCDEntityReference(document, _parent);
	TrackObject(entityReference);
}

FCDEntityInstance::~FCDEntityInstance()
{

	if (entityReference != NULL)
	{
		UntrackObject(entityReference);
		SAFE_RELEASE(entityReference);
	}
}

FCDEntity* FCDEntityInstance::GetEntity() 
{ 
	return entityReference->GetEntity(); 
}

void FCDEntityInstance::SetEntity(FCDEntity* entity) 
{ 
	entityReference->SetEntity(entity); 
}

const FUUri FCDEntityInstance::GetEntityUri() const 
{ 
	return entityReference->GetUri(); 
}

void FCDEntityInstance::SetEntityUri(const FUUri& uri) 
{ 
	entityReference->SetUri(uri); 
}

void FCDEntityInstance::SetName(const fstring& _name) 
{
	name = FCDEntity::CleanName(_name.c_str());
	SetDirtyFlag();
}

FCDExtra* FCDEntityInstance::GetExtra()
{
	return (extra != NULL) ? extra : (extra = new FCDExtra(GetDocument(), this));
}

bool FCDEntityInstance::IsExternalReference() const
{ 
	return entityReference->GetPlaceHolder() != NULL; 
}

/*
void FCDEntityInstance::LoadExternalEntity(FCDocument* externalDocument, const fm::string& daeId)
{
	if (externalDocument == NULL || entity != NULL) return;

	FCDEntity* instancedEntity = NULL;
	switch (entityType)
	{
	case FCDEntity::ANIMATION: instancedEntity = (FCDEntity*) externalDocument->FindAnimation(daeId); break;
	case FCDEntity::CAMERA: instancedEntity = (FCDEntity*) externalDocument->FindCamera(daeId); break;
	case FCDEntity::EMITTER: instancedEntity = (FCDEntity*) externalDocument->FindEmitter(daeId); break;
	case FCDEntity::LIGHT: instancedEntity = (FCDEntity*) externalDocument->FindLight(daeId); break;
	case FCDEntity::GEOMETRY: instancedEntity = (FCDEntity*) externalDocument->FindGeometry(daeId); break;
	case FCDEntity::CONTROLLER: instancedEntity = (FCDEntity*) externalDocument->FindController(daeId); break;
	case FCDEntity::MATERIAL: instancedEntity = (FCDEntity*) externalDocument->FindMaterial(daeId); break;
	case FCDEntity::EFFECT: instancedEntity = (FCDEntity*) externalDocument->FindEffect(daeId); break;
	case FCDEntity::SCENE_NODE: instancedEntity = (FCDEntity*) externalDocument->FindSceneNode(daeId); break;
	case FCDEntity::FORCE_FIELD: instancedEntity = (FCDEntity*) externalDocument->FindForceField(daeId); break;
	case FCDEntity::PHYSICS_MATERIAL: instancedEntity = (FCDEntity*) externalDocument->FindPhysicsMaterial(daeId); break;
	case FCDEntity::PHYSICS_MODEL: instancedEntity = (FCDEntity*) externalDocument->FindPhysicsModel(daeId); break;
	default: break;
	}

	if (instancedEntity != NULL)
	{
		SetEntity(instancedEntity);
	}
}
*/
bool FCDEntityInstance::HasForParent(FCDSceneNode* node) const
{
	if (node == NULL) return false;
	if (parent == NULL) return false;
	FCDSceneNodeList parentStack;
	parentStack.push_back(parent);
	while (!parentStack.empty())
	{
		FCDSceneNode* p = parentStack.front();
		if (p == node) return true;
		for (size_t i = 0; i < p->GetParentCount(); ++i)
		{
			parentStack.push_back(p->GetParent(i));
		}
		parentStack.pop_front();
	}
	return false;
}

void FCDEntityInstance::CleanSubId(FUSUniqueStringMap* parentStringMap)
{
	if (!wantedSubId->empty() && (parentStringMap != NULL))
	{
		parentStringMap->insert(wantedSubId);
	}
}

FCDEntityInstance* FCDEntityInstance::Clone(FCDEntityInstance* clone) const
{
	if (clone == NULL)
	{
		clone = new FCDEntityInstance(const_cast<FCDocument*>(GetDocument()), const_cast<FCDSceneNode*>(parent), entityType);
	}

	clone->SetEntity(const_cast<FCDEntity*>(entityReference->GetEntity()));
	return clone;
}

void FCDEntityInstance::OnObjectReleased(FUTrackable* object)
{
	FUAssert(object == entityReference, return);
	entityReference = NULL;
	Release();
}


/******************* FCDEntityInstanceFactory implementation ***********************/


FCDEntityInstance* FCDEntityInstanceFactory::CreateInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity::Type type)
{
	switch (type)
	{
	case FCDEntity::CONTROLLER: return new FCDControllerInstance(document, parent, type); break;
	case FCDEntity::EMITTER: return new FCDEmitterInstance(document, parent, type); break;
	case FCDEntity::GEOMETRY: return new FCDGeometryInstance(document, parent, type); break;
	case FCDEntity::FORCE_FIELD: return new FCDPhysicsForceFieldInstance(document, parent, type); break;
	case FCDEntity::PHYSICS_MATERIAL:
	case FCDEntity::CAMERA:
	case FCDEntity::LIGHT:
	case FCDEntity::ANIMATION:
	case FCDEntity::SCENE_NODE: return new FCDEntityInstance(document, parent, type); break;

	default: 
		FUFail(;);
		// Default to always return something.
		return new FCDEntityInstance(document, parent, type);
		break;
	}
}

FCDEntityInstance* FCDEntityInstanceFactory::CreateInstance(FCDocument* document, FCDSceneNode* parent, FCDEntity* entity)
{
	FUAssert(entity != NULL, return NULL);

	FCDEntityInstance* instance = CreateInstance(document, parent, entity->GetType());
	instance->SetEntity(entity);
	return instance;
}
