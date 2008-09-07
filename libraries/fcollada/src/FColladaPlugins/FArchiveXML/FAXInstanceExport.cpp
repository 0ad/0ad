/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDEntityReference.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDEmitterInstance.h"
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDMaterialInstance.h"
#include "FCDocument/FCDPhysicsForceFieldInstance.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDPhysicsRigidBodyInstance.h"
#include "FCDocument/FCDPhysicsRigidConstraintInstance.h"
#include "FCDocument/FCDSkinController.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsRigidConstraint.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsRigidBodyParameters.h"

//
// Helpers
//

static const char* GetInstanceClassTypeFCDEntityInstance(FCDEntity::Type type)
{
	const char* instanceEntityName;
	switch (type)
	{
	case FCDEntity::ANIMATION: instanceEntityName = DAE_INSTANCE_ANIMATION_ELEMENT; break;
	case FCDEntity::CAMERA: instanceEntityName = DAE_INSTANCE_CAMERA_ELEMENT; break;
	case FCDEntity::CONTROLLER: instanceEntityName = DAE_INSTANCE_CONTROLLER_ELEMENT; break;
	case FCDEntity::EMITTER: instanceEntityName = DAE_INSTANCE_EMITTER_ELEMENT; break;
	case FCDEntity::EFFECT: instanceEntityName = DAE_INSTANCE_EFFECT_ELEMENT; break;
	case FCDEntity::FORCE_FIELD: instanceEntityName = DAE_INSTANCE_FORCE_FIELD_ELEMENT; break;
	case FCDEntity::GEOMETRY: instanceEntityName = DAE_INSTANCE_GEOMETRY_ELEMENT; break;
	case FCDEntity::LIGHT: instanceEntityName = DAE_INSTANCE_LIGHT_ELEMENT; break;
	case FCDEntity::MATERIAL: instanceEntityName = DAE_INSTANCE_MATERIAL_ELEMENT; break;
	case FCDEntity::PHYSICS_MATERIAL: instanceEntityName = DAE_INSTANCE_PHYSICS_MATERIAL_ELEMENT; break;
    case FCDEntity::PHYSICS_MODEL: instanceEntityName = DAE_INSTANCE_PHYSICS_MODEL_ELEMENT; break;
	case FCDEntity::PHYSICS_RIGID_BODY: instanceEntityName = DAE_INSTANCE_RIGID_BODY_ELEMENT; break;
	case FCDEntity::PHYSICS_RIGID_CONSTRAINT: instanceEntityName = DAE_INSTANCE_RIGID_CONSTRAINT_ELEMENT; break;
	case FCDEntity::SCENE_NODE: instanceEntityName = DAE_INSTANCE_NODE_ELEMENT; break;

	case FCDEntity::ANIMATION_CLIP:
	case FCDEntity::ENTITY:
	case FCDEntity::IMAGE:
	default: FUFail(instanceEntityName = DAEERR_UNKNOWN_ELEMENT);
	}
	return instanceEntityName;
}

//
// Instance Export
//

xmlNode* FArchiveXML::WriteEntityInstance(FCDObject* object, xmlNode* parentNode)
{
	FCDEntityInstance* entityInstance = (FCDEntityInstance*)object;

	const char* instanceEntityName = GetInstanceClassTypeFCDEntityInstance(entityInstance->GetEntityType());
	xmlNode* instanceNode = AddChild(parentNode, instanceEntityName);

	if (!entityInstance->GetWantedSubId().empty())
	{
		AddAttribute(instanceNode, DAE_SID_ATTRIBUTE, entityInstance->GetWantedSubId());
	}
	if (!entityInstance->GetName().empty())
	{
		AddAttribute(instanceNode, DAE_NAME_ATTRIBUTE, entityInstance->GetName());
	}

	// Only write out our entity if we have one.
	const FUUri& uri = entityInstance->GetEntityUri();
	fstring uriString = entityInstance->GetDocument()->GetFileManager()->CleanUri(uri);
	AddAttribute(instanceNode, DAE_URL_ATTRIBUTE, uriString);

	return instanceNode;
}

xmlNode* FArchiveXML::WriteEmitterInstance(FCDObject* object, xmlNode* parentNode)
{
	FCDEmitterInstance* emitterInstance = (FCDEmitterInstance*)object;

	xmlNode* instanceNode = FArchiveXML::WriteEntityInstance(emitterInstance, parentNode);


	FArchiveXML::WriteEntityInstanceExtra(emitterInstance, instanceNode);
	return instanceNode;
}

xmlNode* FArchiveXML::WriteSpriteInstance(FCDEntityInstance* object, xmlNode* parentNode)
{
	xmlNode* theRealExport = WriteGeometryInstance(object, parentNode);
	// Rename from geometry_instance to sprite
	RenameNode(theRealExport, DAE_INSTANCE_SPRITE_ELEMENT);
	// The sprite doesnt have a URL
	RemoveAttribute(theRealExport, DAE_URL_ATTRIBUTE);
	return theRealExport;
}

xmlNode* FArchiveXML::WriteGeometryInstance(FCDObject* object, xmlNode* parentNode)
{
	FCDGeometryInstance* geometryInstance = (FCDGeometryInstance*)object;

	xmlNode* instanceNode = FArchiveXML::WriteEntityInstance(geometryInstance, parentNode);
	if (!(geometryInstance->GetMaterialInstanceCount() == 0))
	{
		xmlNode* bindMaterialNode = AddChild(instanceNode, DAE_BINDMATERIAL_ELEMENT);
		size_t parameterCount = geometryInstance->GetEffectParameterCount();
		for (size_t p = 0; p < parameterCount; ++p)
		{
			FArchiveXML::LetWriteObject(geometryInstance->GetEffectParameter(p), bindMaterialNode);
		}
		xmlNode* techniqueCommonNode = AddChild(bindMaterialNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		for (size_t i = 0; i < geometryInstance->GetMaterialInstanceCount(); ++i)
		{
			FArchiveXML::LetWriteObject(geometryInstance->GetMaterialInstance(i), techniqueCommonNode);
		}
	}
	FArchiveXML::WriteEntityInstanceExtra(geometryInstance, instanceNode);
	return instanceNode;
}

xmlNode* FArchiveXML::WriteControllerInstance(FCDObject* object, xmlNode* parentNode)
{
	FCDControllerInstance* controllerInstance = (FCDControllerInstance*)object;

	// Export the geometry instantiation information.
	xmlNode* instanceNode = FArchiveXML::WriteGeometryInstance(controllerInstance, parentNode);
	xmlNode* insertBeforeNode = (instanceNode != NULL) ? instanceNode->children : NULL;

	// Retrieve the parent joints and export the <skeleton> elements.
	FUUriList& skeletonRoots = controllerInstance->GetSkeletonRoots();
	for (FUUriList::iterator itS = skeletonRoots.begin(); itS != skeletonRoots.end(); ++itS)
	{
		// TODO: External references (again)
		fm::string fragment = TO_STRING((*itS).GetFragment());
		FUSStringBuilder builder; builder.set('#'); builder.append(fragment);
		xmlNode* skeletonNode = InsertChild(instanceNode, insertBeforeNode, DAE_SKELETON_ELEMENT);
		AddContent(skeletonNode, builder);
	}

	FArchiveXML::WriteEntityInstanceExtra(controllerInstance, instanceNode);
	return instanceNode;
}

xmlNode* FArchiveXML::WriteMaterialInstance(FCDObject* object, xmlNode* parentNode)
{
	FCDMaterialInstance* materialInstance = (FCDMaterialInstance*)object;

	xmlNode* instanceNode = FArchiveXML::WriteEntityInstance(materialInstance, parentNode);

	// no url in instance_material
	RemoveAttribute(instanceNode, DAE_URL_ATTRIBUTE);

	AddAttribute(instanceNode, DAE_SYMBOL_ATTRIBUTE, materialInstance->GetSemantic());

	// Only write out our entity if we have one.
	const FUUri& uri = materialInstance->GetEntityReference()->GetUri();
	fstring uriString = materialInstance->GetDocument()->GetFileManager()->CleanUri(uri);
	AddAttribute(instanceNode, DAE_TARGET_ATTRIBUTE, uriString);

	// Write out the bindings.
	for (size_t i = 0; i < materialInstance->GetBindingCount(); ++i)
	{
		const FCDMaterialInstanceBind& bind = *materialInstance->GetBinding(i);
		xmlNode* bindNode = AddChild(instanceNode, DAE_BIND_ELEMENT);
		AddAttribute(bindNode, DAE_SEMANTIC_ATTRIBUTE, bind.semantic);
		AddAttribute(bindNode, DAE_TARGET_ATTRIBUTE, bind.target);
	}
	
	// Write out the vertex input bindings.
	for (size_t i = 0; i < materialInstance->GetVertexInputBindingCount(); ++i)
	{
		const FCDMaterialInstanceBindVertexInput* bind = materialInstance->GetVertexInputBinding(i);
		xmlNode* bindNode = AddChild(instanceNode, DAE_BIND_VERTEX_INPUT_ELEMENT);
		AddAttribute(bindNode, DAE_SEMANTIC_ATTRIBUTE, bind->semantic);
		AddAttribute(bindNode, DAE_INPUT_SEMANTIC_ATTRIBUTE, FUDaeGeometryInput::ToString(bind->GetInputSemantic()));
		AddAttribute(bindNode, DAE_INPUT_SET_ATTRIBUTE, bind->inputSet);
	}


	FArchiveXML::WriteEntityInstanceExtra(materialInstance, instanceNode);
	return instanceNode;
}

xmlNode* FArchiveXML::WritePhysicsForceFieldInstance(FCDObject* object, xmlNode* parentNode)
{
	FCDPhysicsForceFieldInstance* physicsForceFieldInstance = (FCDPhysicsForceFieldInstance*)object;

	xmlNode* instanceNode = FArchiveXML::WriteEntityInstance(physicsForceFieldInstance, parentNode);

	// nothing interesting in force field instance to write

	FArchiveXML::WriteEntityInstanceExtra(physicsForceFieldInstance, instanceNode);
	return instanceNode;
}

xmlNode* FArchiveXML::WritePhysicsModelInstance(FCDObject* object, xmlNode* parentNode)
{
	FCDPhysicsModelInstance* physicsModelInstance = (FCDPhysicsModelInstance*)object;

	xmlNode* instanceNode = FArchiveXML::WriteEntityInstance(physicsModelInstance, parentNode);

	// The sub-instances must be ordered correctly: force fields first, then rigid bodies; rigid constraints are last.
	for (size_t i = 0; i < physicsModelInstance->GetInstanceCount(); ++i)
	{
		if (physicsModelInstance->GetInstance(i)->GetEntityType() == FCDEntity::FORCE_FIELD)
		{
			FArchiveXML::LetWriteObject(physicsModelInstance->GetInstance(i), instanceNode);
		}
	}
	for (size_t i = 0; i < physicsModelInstance->GetInstanceCount(); ++i)
	{
		if (physicsModelInstance->GetInstance(i)->GetEntityType() == FCDEntity::PHYSICS_RIGID_BODY)
		{
			FArchiveXML::LetWriteObject(physicsModelInstance->GetInstance(i), instanceNode);
		}
	}
	for (size_t i = 0; i < physicsModelInstance->GetInstanceCount(); ++i)
	{
		if (physicsModelInstance->GetInstance(i)->GetEntityType() == FCDEntity::PHYSICS_RIGID_CONSTRAINT)
		{
			FArchiveXML::LetWriteObject(physicsModelInstance->GetInstance(i), instanceNode);
		}
	}

	FArchiveXML::WriteEntityInstanceExtra(physicsModelInstance, instanceNode);
	return instanceNode;
}

xmlNode* FArchiveXML::WritePhysicsRigidBodyInstance(FCDObject* object, xmlNode* parentNode)
{
	FCDPhysicsRigidBodyInstance* physicsRigidBodyInstance = (FCDPhysicsRigidBodyInstance*)object;

	xmlNode* instanceNode = FArchiveXML::WriteEntityInstance(physicsRigidBodyInstance, parentNode);

	AddAttribute(instanceNode, DAE_TARGET_ATTRIBUTE, fm::string("#") + physicsRigidBodyInstance->GetTargetNode()->GetDaeId());
	AddAttribute(instanceNode, DAE_BODY_ATTRIBUTE, physicsRigidBodyInstance->GetEntity()->GetDaeId());

	//inconsistency in the spec
	RemoveAttribute(instanceNode, DAE_URL_ATTRIBUTE);
	
	xmlNode* techniqueNode = AddChild(instanceNode, DAE_TECHNIQUE_COMMON_ELEMENT);

	// almost same as FCDPhysicsRigidBody
	FArchiveXML::AddPhysicsParameter(techniqueNode, DAE_ANGULAR_VELOCITY_ELEMENT, physicsRigidBodyInstance->GetAngularVelocity());
	FArchiveXML::AddPhysicsParameter(techniqueNode, DAE_VELOCITY_ELEMENT, physicsRigidBodyInstance->GetVelocity());
	FArchiveXML::WritePhysicsRigidBodyParameters(physicsRigidBodyInstance->GetParameters(), techniqueNode);

	FArchiveXML::WriteEntityInstanceExtra(physicsRigidBodyInstance, instanceNode);
	return instanceNode;
}

xmlNode* FArchiveXML::WritePhysicsRigidConstraintInstance(FCDObject* object, xmlNode* parentNode)
{
	FCDPhysicsRigidConstraintInstance* physicsRigidConstraintInstance = (FCDPhysicsRigidConstraintInstance*)object;

	xmlNode* instanceNode = FArchiveXML::WriteEntityInstance(physicsRigidConstraintInstance, parentNode);

	if (physicsRigidConstraintInstance->GetEntity() != NULL && physicsRigidConstraintInstance->GetEntity()->GetObjectType() == FCDPhysicsRigidConstraint::GetClassType())
	{
		FCDPhysicsRigidConstraint* constraint = (FCDPhysicsRigidConstraint*) physicsRigidConstraintInstance->GetEntity();
		AddAttribute(instanceNode, DAE_CONSTRAINT_ATTRIBUTE, constraint->GetSubId());
	}

	//inconsistency in the spec
	RemoveAttribute(instanceNode, DAE_URL_ATTRIBUTE);

	FArchiveXML::WriteEntityInstanceExtra(physicsRigidConstraintInstance, instanceNode);
	return instanceNode;
}
