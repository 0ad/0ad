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
#include "FCDocument/FCDEntityReference.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDEmitterInstance.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDMaterialInstance.h"
#include "FCDocument/FCDPhysicsForceFieldInstance.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDPhysicsRigidBodyInstance.h"
#include "FCDocument/FCDPhysicsRigidConstraintInstance.h"
#include "FCDocument/FCDSkinController.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsRigidBody.h"

bool FArchiveXML::LoadEntityInstance(FCDObject* object, xmlNode* instanceNode)
{ 
	FCDEntityInstance* entityInstance = (FCDEntityInstance*)object;

	bool status = true;

	FUUri uri = ReadNodeUrl(instanceNode);
	entityInstance->GetEntityReference()->SetUri(uri);
	if (!entityInstance->IsExternalReference() && entityInstance->GetEntity() == NULL)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INST_ENTITY_MISSING, instanceNode->line);
	}

	entityInstance->SetWantedSubId(TO_STRING(ReadNodeSid(instanceNode)));
	entityInstance->SetName(TO_FSTRING(ReadNodeName(instanceNode)));

	// Read in the extra nodes
	xmlNodeList extraNodes;
	FindChildrenByType(instanceNode, DAE_EXTRA_ELEMENT, extraNodes);
	for (xmlNodeList::iterator it = extraNodes.begin(); it != extraNodes.end(); ++it)
	{
		xmlNode* extraNode = (*it);
		FArchiveXML::LoadExtra(entityInstance->GetExtra(), extraNode);
	}

	entityInstance->SetDirtyFlag(); 
	return status;
}

bool FArchiveXML::LoadEmitterInstance(FCDObject* object, xmlNode* instanceNode)
{
	if (!FArchiveXML::LoadEntityInstance(object, instanceNode)) return false;

	FCDEmitterInstance* emitterInstance = (FCDEmitterInstance*)object;

	bool status = true;

	// Check for the expected instantiation node type
	if (!IsEquivalent(instanceNode->name, DAE_INSTANCE_EMITTER_ELEMENT))
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_UNKNOWN_ELEMENT, instanceNode->line);
		return false;
	}


	emitterInstance->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadGeometryInstance(FCDObject* object, xmlNode* instanceNode)
{
	if (!FArchiveXML::LoadEntityInstance(object, instanceNode)) return false;

	bool status = true;
	FCDGeometryInstance* geometryInstance = (FCDGeometryInstance*)object;

	// Look for the <bind_material> element. The others are discarded for now.
	xmlNode* bindMaterialNode = FindChildByType(instanceNode, DAE_BINDMATERIAL_ELEMENT);
	if (bindMaterialNode != NULL)
	{
		for (xmlNode* child = bindMaterialNode->children; child != NULL; child = child->next)
		{
			if (child->type != XML_ELEMENT_NODE) continue;

			if (IsEquivalent(child->name, DAE_PARAMETER_ELEMENT))
			{
				FCDEffectParameter* parameter = geometryInstance->AddEffectParameter(FArchiveXML::GetEffectParameterType(child));
				parameter->SetAnimator();
				status &= FArchiveXML::LoadSwitch(parameter, &parameter->GetObjectType(), child);
			}
		} 

		// Retrieve the list of the <technique_common><instance_material> elements.
		xmlNode* techniqueNode = FindChildByType(bindMaterialNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		xmlNodeList materialNodes;
		FindChildrenByType(techniqueNode, DAE_INSTANCE_MATERIAL_ELEMENT, materialNodes);
		for (xmlNodeList::iterator itM = materialNodes.begin(); itM != materialNodes.end(); ++itM)
		{
			FCDMaterialInstance* material = geometryInstance->AddMaterialInstance();
			status &= (FArchiveXML::LoadMaterialInstance(material, *itM));
		}
	}
	else
	{
		// Blinding attempt to use the material semantic from the polygons as a material id.
		FCDGeometry* geometry = (FCDGeometry*) geometryInstance->GetEntity();
		if (geometry != NULL && geometry->HasType(FCDGeometry::GetClassType()) && geometry->IsMesh())
		{
			FCDGeometryMesh* mesh = geometry->GetMesh();
			size_t polyCount = mesh->GetPolygonsCount();
			for (size_t i = 0; i < polyCount; ++i)
			{
				FCDGeometryPolygons* polys = mesh->GetPolygons(i);
				const fstring& semantic = polys->GetMaterialSemantic();
				fm::string semanticUTF8 = TO_STRING(semantic);
				semanticUTF8 = FCDObjectWithId::CleanId(semanticUTF8.c_str());
				FCDMaterial* material = geometry->GetDocument()->FindMaterial(semanticUTF8);
				if (material != NULL)
				{
					geometryInstance->AddMaterialInstance(material, polys);
				}
			}
		}
	}

	geometryInstance->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadControllerInstance(FCDObject* object, xmlNode* instanceNode)
{
	if (!FArchiveXML::LoadGeometryInstance(object, instanceNode)) return false;

	bool status = true;
	FCDControllerInstance* controllerInstance = (FCDControllerInstance*)object;

	xmlNodeList skeletonList;
	FUDaeParser::FindChildrenByType(instanceNode, DAE_SKELETON_ELEMENT, skeletonList);
	size_t numRoots = skeletonList.size();
	controllerInstance->GetSkeletonRoots().resize(numRoots);

	for (size_t i = 0; i < numRoots; ++i)
	{
		controllerInstance->GetSkeletonRoots()[i] = FUUri(TO_FSTRING(FUDaeParser::ReadNodeContentDirect(skeletonList[i])));
	}

	return status;
}

bool FArchiveXML::LoadMaterialInstance(FCDObject* object, xmlNode* instanceNode)
{
	FCDMaterialInstance* materialInstance = (FCDMaterialInstance*)object;

	// This is not loaded the same as the FCDEntityInstance ones.
	// Load it first, otherwise FCDEntityInstance will ASSERT (with no Uri)
	fm::string uri = ReadNodeProperty(instanceNode, DAE_TARGET_ATTRIBUTE);
	AddAttribute(instanceNode, DAE_URL_ATTRIBUTE, uri);
	if (!FArchiveXML::LoadEntityInstance(object, instanceNode)) return false;

	materialInstance->SetSemantic(TO_FSTRING(ReadNodeProperty(instanceNode, DAE_SYMBOL_ATTRIBUTE)));

	// Read in the ColladaFX bindings
	while (materialInstance->GetBindingCount() != 0) materialInstance->GetBinding(materialInstance->GetBindingCount() - 1)->Release();
	xmlNodeList bindNodes;
	FindChildrenByType(instanceNode, DAE_BIND_ELEMENT, bindNodes);
	for (xmlNodeList::iterator itB = bindNodes.begin(); itB != bindNodes.end(); ++itB)
	{
		fm::string semantic = ReadNodeSemantic(*itB);
		fm::string target = ReadNodeProperty(*itB, DAE_TARGET_ATTRIBUTE);
		materialInstance->AddBinding(semantic, target);
	}

	// Read in the ColladaFX vertex inputs
	xmlNodeList bindVertexNodes;
	while (materialInstance->GetVertexInputBindingCount() != 0) materialInstance->GetVertexInputBinding(materialInstance->GetVertexInputBindingCount() - 1)->Release();
	FindChildrenByType(instanceNode, DAE_BIND_VERTEX_INPUT_ELEMENT, bindVertexNodes);
	for (xmlNodeList::iterator itB = bindVertexNodes.begin(); itB != bindVertexNodes.end(); ++itB)
	{
		fm::string inputSet = ReadNodeProperty(*itB, DAE_INPUT_SET_ATTRIBUTE);
		fm::string inputSemantic = ReadNodeProperty(*itB, DAE_INPUT_SEMANTIC_ATTRIBUTE);
		materialInstance->AddVertexInputBinding(ReadNodeSemantic(*itB).c_str(), FUDaeGeometryInput::FromString(inputSemantic.c_str()), FUStringConversion::ToInt32(inputSet));
	}


	materialInstance->SetDirtyFlag();
	return true;
}

bool FArchiveXML::LoadPhysicsForceFieldInstance(FCDObject* object, xmlNode* instanceNode)
{
	if (!FArchiveXML::LoadEntityInstance(object, instanceNode)) return false;

	bool status = true;
	FCDPhysicsForceFieldInstance* physicsForceFieldInstance = (FCDPhysicsForceFieldInstance*)object;
	if (physicsForceFieldInstance->GetEntity() == NULL && !physicsForceFieldInstance->IsExternalReference())
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_INVALID_URI, 
				instanceNode->line);
	}

	// Check for the expected instantiation node type
	if (!IsEquivalent(instanceNode->name, DAE_INSTANCE_FORCE_FIELD_ELEMENT))
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_UNKNOWN_ELEMENT, 
				instanceNode->line);
		status = false;
	}

	// nothing interesting in force field instance to load

	physicsForceFieldInstance->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadPhysicsModelInstance(FCDObject* object, xmlNode* instanceNode)
{
	if (!FArchiveXML::LoadEntityInstance(object, instanceNode)) return false;

	bool status = true;
	FCDPhysicsModelInstance* physicsModelInstance = (FCDPhysicsModelInstance*)object;

	if (physicsModelInstance->GetEntity() == NULL)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::WARNING_MISSING_URI_TARGET, instanceNode->line);
	}

	// Check for the expected instantiation node type
	if (!IsEquivalent(instanceNode->name, DAE_INSTANCE_PHYSICS_MODEL_ELEMENT))
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_UNKNOWN_ELEMENT, instanceNode->line);
	}

	//this is already done in the FCDSceneNode
//	fm::string physicsModelId = ReadNodeProperty(instanceNode, DAE_TARGET_ATTRIBUTE);
//	entity = GetDocument()->FindPhysicsModel(physicsModelId);
//	if (!entity)	return status.Fail(FS("Couldn't find physics model for instantiation"), instanceNode->line);

	xmlNodeList rigidBodyNodes;
	FindChildrenByType(instanceNode, DAE_INSTANCE_RIGID_BODY_ELEMENT, rigidBodyNodes);
	for (xmlNodeList::iterator itB = rigidBodyNodes.begin(); itB != rigidBodyNodes.end(); ++itB)
	{
		FCDPhysicsRigidBodyInstance* instance = physicsModelInstance->AddRigidBodyInstance(NULL);
		status &= (FArchiveXML::LoadPhysicsRigidBodyInstance(instance, *itB));
	}

	xmlNodeList rigidConstraintNodes;
	FindChildrenByType(instanceNode, DAE_INSTANCE_RIGID_CONSTRAINT_ELEMENT, rigidConstraintNodes);
	for (xmlNodeList::iterator itC = rigidConstraintNodes.begin(); itC != rigidConstraintNodes.end(); ++itC)
	{
		FCDPhysicsRigidConstraintInstance* instance = physicsModelInstance->AddRigidConstraintInstance(NULL);
		status &= (FArchiveXML::LoadPhysicsRigidConstraintInstance(instance, *itC));
	}

	xmlNodeList forceFieldNodes;
	FindChildrenByType(instanceNode, DAE_INSTANCE_FORCE_FIELD_ELEMENT, forceFieldNodes);
	for (xmlNodeList::iterator itN = forceFieldNodes.begin(); itN != forceFieldNodes.end(); ++itN)
	{
		FCDPhysicsForceFieldInstance* instance = physicsModelInstance->AddForceFieldInstance(NULL);
		status &= (FArchiveXML::LoadPhysicsForceFieldInstance(instance, *itN));
	}

	physicsModelInstance->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadPhysicsRigidBodyInstance(FCDObject* object, xmlNode* instanceNode)
{
	if (!FArchiveXML::LoadEntityInstance(object, instanceNode)) return false;

	bool status = true;
	FCDPhysicsRigidBodyInstance* physicsRigidBodyInstance = (FCDPhysicsRigidBodyInstance*)object;

	// Check for the expected instantiation node type
	if (!IsEquivalent(instanceNode->name, DAE_INSTANCE_RIGID_BODY_ELEMENT) || physicsRigidBodyInstance->GetModelParentInstance() == NULL)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_UNKNOWN_ELEMENT, instanceNode->line);
		status = false;
	}

	// Find the target scene node/rigid body
	fm::string targetNodeId = ReadNodeProperty(instanceNode, DAE_TARGET_ATTRIBUTE);
	physicsRigidBodyInstance->SetTargetNode(physicsRigidBodyInstance->GetDocument()->FindSceneNode(SkipPound(targetNodeId)));
	if (!physicsRigidBodyInstance->GetTargetNode())
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::WARNING_MISSING_URI_TARGET, instanceNode->line);
	}

	// Find the instantiated rigid body
	FCDPhysicsRigidBody* body = NULL;
	fm::string physicsRigidBodySid = ReadNodeProperty(instanceNode, DAE_BODY_ATTRIBUTE);
	if (physicsRigidBodyInstance->GetModelParentInstance()->GetEntity() != NULL &&  physicsRigidBodyInstance->GetModelParentInstance()->GetEntity()->GetType() == FCDEntity::PHYSICS_MODEL)
	{
		FCDPhysicsModel* model = (FCDPhysicsModel*) physicsRigidBodyInstance->GetModelParentInstance()->GetEntity();
		body = model->FindRigidBodyFromSid(physicsRigidBodySid);
		if (body == NULL)
		{
			FUError::Error(FUError::ERROR_LEVEL, FUError::WARNING_MISSING_URI_TARGET, instanceNode->line);
			return false;
		}
		physicsRigidBodyInstance->SetRigidBody(body);
	}

	//Read in the same children as rigid_body + velocity and angular_velocity
	xmlNode* techniqueNode = FindChildByType(instanceNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	if (techniqueNode == NULL)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_TECHNIQUE_NODE_MISSING,
				instanceNode->line);
		return false;
	}

	xmlNode* param = 
			FindChildByType(techniqueNode, DAE_ANGULAR_VELOCITY_ELEMENT);
	if (param != NULL)
	{
		physicsRigidBodyInstance->SetAngularVelocity(FUStringConversion::ToVector3(
				ReadNodeContentDirect(param)));
	}
	else
	{
		physicsRigidBodyInstance->SetAngularVelocity(FMVector3::Zero);
	}

	param = FindChildByType(techniqueNode, DAE_VELOCITY_ELEMENT);
	if (param != NULL)
	{
		physicsRigidBodyInstance->SetVelocity(FUStringConversion::ToVector3(ReadNodeContentDirect(param)));
	}
	else
	{
		physicsRigidBodyInstance->SetVelocity(FMVector3::Zero);
	}

	FArchiveXML::LoadPhysicsRigidBodyParameters(physicsRigidBodyInstance->GetParameters(), techniqueNode, body->GetParameters());

	physicsRigidBodyInstance->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadPhysicsRigidConstraintInstance(FCDObject* object, xmlNode* instanceNode)
{
	if (!FArchiveXML::LoadEntityInstance(object, instanceNode)) return false;

	bool status = true;
	FCDPhysicsRigidConstraintInstance* physicsRigidConstraintInstance = (FCDPhysicsRigidConstraintInstance*)object;

	// Check for the expected instantiation node type
	if (!IsEquivalent(instanceNode->name, DAE_INSTANCE_RIGID_CONSTRAINT_ELEMENT)
		|| physicsRigidConstraintInstance->GetModelParentInstance() == NULL 
		|| physicsRigidConstraintInstance->GetModelParentInstance()->GetEntity() == NULL)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_UNKNOWN_ELEMENT, instanceNode->line);
		status = false;
	}

	FCDPhysicsModel* model = (FCDPhysicsModel*) physicsRigidConstraintInstance->GetModelParentInstance()->GetEntity();
	fm::string physicsRigidConstraintSid = ReadNodeProperty(instanceNode, DAE_CONSTRAINT_ATTRIBUTE);
	FCDPhysicsRigidConstraint* rigidConstraint = model->FindRigidConstraintFromSid(physicsRigidConstraintSid);
	if (!rigidConstraint)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_RIGID_CONSTRAINT_MISSING, instanceNode->line);
		return status;
	}
	physicsRigidConstraintInstance->SetRigidConstraint(rigidConstraint);
	physicsRigidConstraintInstance->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LinkControllerInstance(FCDControllerInstance* controllerInstance)
{
	const FCDSkinController* skin =  FArchiveXML::FindSkinController(controllerInstance, controllerInstance->GetEntity());
	if (skin == NULL) return true;
	FCDSkinControllerData& data = FArchiveXML::documentLinkDataMap[skin->GetDocument()].skinControllerDataMap.find(const_cast<FCDSkinController*>(skin))->second;

	// Look for each joint, by COLLADA id, within the scene graph
	size_t jointCount = skin->GetJointCount();
	
	FCDSceneNodeList rootNodes;
	controllerInstance->FindSkeletonNodes(rootNodes);
	size_t numRoots = rootNodes.size();
	
	while (controllerInstance->GetJointCount() != 0) controllerInstance->RemoveJoint(controllerInstance->GetJointCount() - 1);
	for (size_t i = 0; i < jointCount; ++i)
	{
		const fm::string& jid = skin->GetJoint(i)->GetId();
		FCDSceneNode* boneNode = NULL;

		if (data.jointAreSids)
		{
			// Find by subId
			for (size_t i = 0; i < numRoots; i++)
			{
				boneNode = (FCDSceneNode*)rootNodes[i]->FindSubId(jid);
				if (boneNode != NULL) break;
			}
		}
		else
		{
			// Find by DaeId
			for (size_t i = 0; i < numRoots; i++)
			{
				boneNode = (FCDSceneNode*)rootNodes[i]->FindDaeId(jid);
				if (boneNode != NULL) break;
			}
		}

		if (boneNode != NULL)
		{
			controllerInstance->AddJoint(boneNode);
		}
		else
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_JOINT, 0);
		}
	}
	return true;
}

bool FArchiveXML::LinkEmitterInstance(FCDEmitterInstance* emitterInstance)
{
	(void) emitterInstance;
	return true;
}

bool FArchiveXML::ImportEmittedInstanceList(FCDEmitterInstance* emitterInstance, xmlNode* node)
{
	bool status = true;
	(void) emitterInstance; (void) node;
	return status;
}

uint32 FArchiveXML::GetEntityInstanceType(xmlNode* node)
{
	if (IsEquivalent(node->name, DAE_INSTANCE_CAMERA_ELEMENT)) return FCDEntity::CAMERA;
	else if (IsEquivalent(node->name, DAE_INSTANCE_CONTROLLER_ELEMENT)) return FCDEntity::CONTROLLER;
	else if (IsEquivalent(node->name, DAE_INSTANCE_EMITTER_ELEMENT)) return FCDEntity::EMITTER;
	else if (IsEquivalent(node->name, DAE_INSTANCE_FORCE_FIELD_ELEMENT)) return FCDEntity::FORCE_FIELD;
	else if (IsEquivalent(node->name, DAE_INSTANCE_GEOMETRY_ELEMENT)) return FCDEntity::GEOMETRY;
	else if (IsEquivalent(node->name, DAE_INSTANCE_SPRITE_ELEMENT)) return FCDEntity::GEOMETRY; // same class does 2 jobs
	else if (IsEquivalent(node->name, DAE_INSTANCE_LIGHT_ELEMENT)) return FCDEntity::LIGHT;
	else if (IsEquivalent(node->name, DAE_INSTANCE_NODE_ELEMENT)) return FCDEntity::SCENE_NODE; 
	else return (uint32) ~0;
}



