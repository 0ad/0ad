/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDTargetedEntity.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDTransform.h"
#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDEmitterInstance.h"
#include "FCDocument/FCDEntityInstance.h"

bool FArchiveXML::LoadEntity(FCDObject* object, xmlNode* entityNode)
{ 
	FCDEntity* entity = (FCDEntity*)object;

	bool status = true;

	fm::string fileId = FUDaeParser::ReadNodeId(entityNode);
	if (!fileId.empty()) entity->SetDaeId(fileId);
	else entity->RemoveDaeId();

	entity->SetName(TO_FSTRING(FUDaeParser::ReadNodeName(entityNode)));
	if (entity->GetName().empty()) entity->SetName(TO_FSTRING(fileId));

	// Read in the asset information.
	xmlNode* assetNode = FindChildByType(entityNode, DAE_ASSET_ELEMENT);
	if (assetNode != NULL) FArchiveXML::LoadAsset(entity->GetAsset(), assetNode);

	// Read in the extra nodes
	xmlNodeList extraNodes;
	FindChildrenByType(entityNode, DAE_EXTRA_ELEMENT, extraNodes);
	for (xmlNodeList::iterator it = extraNodes.begin(); it != extraNodes.end(); ++it)
	{
		xmlNode* extraNode = (*it);
		FArchiveXML::LoadExtra(entity->GetExtra(), extraNode);

		// Look for an extra node at this level and a valid technique
		FCDETechnique* mayaTechnique = entity->GetExtra()->GetDefaultType()->FindTechnique(DAEMAYA_MAYA_PROFILE);
		FCDETechnique* maxTechnique = entity->GetExtra()->GetDefaultType()->FindTechnique(DAEMAX_MAX_PROFILE);
		FCDETechnique* fcTechnique = entity->GetExtra()->GetDefaultType()->FindTechnique(DAE_FCOLLADA_PROFILE);

		// Read in all the extra parameters
		StringList parameterNames;
		FCDENodeList parameterNodes;
		if (mayaTechnique != NULL) mayaTechnique->FindParameters(parameterNodes, parameterNames);
		if (maxTechnique != NULL) maxTechnique->FindParameters(parameterNodes, parameterNames);
		if (fcTechnique != NULL) fcTechnique->FindParameters(parameterNodes, parameterNames);

		// Look for the note and user-properties, which is the only parameter currently supported at this level
		size_t parameterCount = parameterNodes.size();
		for (size_t i = 0; i < parameterCount; ++i)
		{
			FCDENode* parameterNode = parameterNodes[i];
			const fm::string& parameterName = parameterNames[i];

			if (parameterName == DAEMAX_USERPROPERTIES_NODE_PARAMETER || parameterName == DAEMAYA_NOTE_PARAMETER)
			{
				entity->SetNote(parameterNode->GetContent());
				SAFE_RELEASE(parameterNode);
			}
		}
	}

	entity->SetDirtyFlag();
	return status;	
}

bool FArchiveXML::LoadTargetedEntity(FCDObject* object, xmlNode* entityNode)
{ 
	if (!FArchiveXML::LoadEntity(object, entityNode)) return false;

	bool status = true;
	FCDTargetedEntity* targetedEntity = (FCDTargetedEntity*)object;
	FCDTargetedEntityData& data = FArchiveXML::documentLinkDataMap[targetedEntity->GetDocument()].targetedEntityDataMap[targetedEntity];

	// Look for and extract the target information from the extra tree nodes.
	// For backward-compatibility: we want to process the <technique> straight into the extra tree..
	FCDExtra* extra = targetedEntity->GetExtra();
	FArchiveXML::LoadExtra(extra, entityNode);

	// Extract out the target information. 
	FCDENode* targetNode = extra->GetDefaultType()->FindRootNode(DAEFC_TARGET_PARAMETER);
	if (targetNode != NULL)
	{
		data.targetId = TO_STRING(targetNode->GetContent());
		SAFE_RELEASE(targetNode);
	}

	return status;
}	

bool FArchiveXML::LoadSceneNode(FCDObject* object, xmlNode* node)
{ 
	if (!FArchiveXML::LoadEntity(object, node)) return false;

	bool status = true;
	FCDSceneNode* sceneNode = (FCDSceneNode*)object;
	if (!IsEquivalent(node->name, DAE_VSCENE_ELEMENT) && !IsEquivalent(node->name, DAE_NODE_ELEMENT))
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_UNKNOWN_ELEMENT, node->line);
	}

	// Read a subid if we gots one
	fm::string nodeSubId = ReadNodeProperty(node, DAE_SID_ATTRIBUTE);
	sceneNode->SetSubId(nodeSubId);

	// Read in the <node> element's type
	fm::string nodeType = ReadNodeProperty(node, DAE_TYPE_ATTRIBUTE);
	if (nodeType == DAE_JOINT_NODE_TYPE) sceneNode->SetJointFlag(true);
	else if (nodeType.length() == 0 || nodeType == DAE_NODE_NODE_TYPE) {} // No special consideration
	else
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOW_NODE_ELEMENT_TYPE, node->line);
	}

	// The scene node has ordered elements, so process them directly and in order.
	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_NODE_ELEMENT))
		{
			// Load the child scene node
			FCDSceneNode* node = sceneNode->AddChildNode();
			status = FArchiveXML::LoadSceneNode(node, child);
			if (!status) break;
		}
		// Although this case can be handled by FCDEntityInstanceFactory,
		// we can do some special case handling here.
		else if (IsEquivalent(child->name, DAE_INSTANCE_NODE_ELEMENT))
		{
			FUUri url = ReadNodeUrl(child);
			if (!url.IsFile())
			{
				// cannot find the node
				FCDSceneNode* node = sceneNode->GetDocument()->FindSceneNode(TO_STRING(url.GetFragment()));
				if (node != NULL)
				{
					
					if (!sceneNode->AddChildNode(node))
					{
						FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_CYCLE_DETECTED, child->line);
					}
				}
				else
				{
					FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_NODE_INST, child->line);
				}
			}
			else
			{
				FCDEntityInstance* reference = sceneNode->AddInstance(FCDEntity::SCENE_NODE);
				FArchiveXML::LoadEntityInstance(reference, child);
			}
		}
		else if (IsEquivalent(child->name, DAE_EXTRA_ELEMENT)) {} // Handled by FCDEntity.
		else if (IsEquivalent(child->name, DAE_ASSET_ELEMENT)) {} // Handled by FCDEntity.
		else
		{
			uint32 transformType = FArchiveXML::GetTransformType(child);
			if (transformType != (uint32) ~0)
			{
				FCDTransform* transform = sceneNode->AddTransform((FCDTransform::Type) transformType);
				fm::string childSubId = ReadNodeProperty(child, DAE_SID_ATTRIBUTE);
				transform->SetSubId(childSubId);
				status &= (FArchiveXML::LoadSwitch(transform, &transform->GetObjectType(), child));
			}
			else
			{
				uint32 instanceType = FArchiveXML::GetEntityInstanceType(child);
				if (instanceType != (uint32) ~0)
				{
					FCDEntityInstance* instance = sceneNode->AddInstance((FCDEntity::Type) instanceType);
					status &= (FArchiveXML::LoadSwitch(instance, &instance->GetObjectType(), child));
				}
				else
				{
					FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_TRANSFORM, child->line);
				}
			}
		}
	}

	status &= FArchiveXML::LoadFromExtraSceneNode(sceneNode);
	sceneNode->SetTransformsDirtyFlag();
	sceneNode->SetDirtyFlag();
	return status;
}		

bool FArchiveXML::LoadFromExtraSceneNode(FCDSceneNode* sceneNode)
{
	bool status = true;

	FCDENodeList parameterNodes;
	StringList parameterNames;

	// Retrieve the extra information from the base entity class
	FCDExtra* extra = sceneNode->GetExtra();

	// List all the parameters
	size_t techniqueCount = extra->GetDefaultType()->GetTechniqueCount();
	for (size_t i = 0; i < techniqueCount; ++i)
	{
		FCDETechnique* technique = extra->GetDefaultType()->GetTechnique(i);
		technique->FindParameters(parameterNodes, parameterNames);
	}

	// Process the known parameters
	size_t parameterCount = parameterNodes.size();
	for (size_t i = 0; i < parameterCount; ++i)
	{
		FCDENode* parameterNode = parameterNodes[i];
		const fm::string& parameterName = parameterNames[i];
		FCDEAttribute* parameterType = parameterNode->FindAttribute(DAE_TYPE_ATTRIBUTE);
		if (parameterName == DAEMAYA_STARTTIME_PARAMETER)
		{
			sceneNode->GetDocument()->SetStartTime(FUStringConversion::ToFloat(parameterNode->GetContent()));
		}
		else if (parameterName == DAEMAYA_ENDTIME_PARAMETER)
		{
			sceneNode->GetDocument()->SetEndTime(FUStringConversion::ToFloat(parameterNode->GetContent()));
		}
		else if (parameterName == DAEFC_VISIBILITY_PARAMETER)
		{
			sceneNode->SetVisibility(FUStringConversion::ToBoolean(parameterNode->GetContent()));
			if (parameterNode->GetAnimated()->HasCurve())
			{
				parameterNode->GetAnimated()->Clone(sceneNode->GetVisibility().GetAnimated());
			}
		}
		else if (parameterName == DAEMAYA_LAYER_PARAMETER || (parameterType != NULL && FUStringConversion::ToString(parameterType->GetValue()) == DAEMAYA_LAYER_PARAMETER))
		{
			FCDEAttribute* nameAttribute = parameterNode->FindAttribute(DAE_NAME_ATTRIBUTE);
			if (nameAttribute == NULL) continue;

			// Create a new layer object list
			FCDLayerList& layers = sceneNode->GetDocument()->GetLayers();
			FCDLayer* layer = new FCDLayer(); layers.push_back(layer);

			// Parse in the layer
			layer->name = FUStringConversion::ToString(nameAttribute->GetValue());
			FUStringConversion::ToStringList(parameterNode->GetContent(), layer->objects);
		}
		else continue;

		SAFE_RELEASE(parameterNode);
	}

	// Read in the extra instances from the typed extra.
	FCDEType* instancesExtra = extra->FindType(DAEFC_INSTANCES_TYPE);
	if (instancesExtra != NULL)
	{
		FCDETechnique* fcolladaTechnique = instancesExtra->FindTechnique(DAE_FCOLLADA_PROFILE);
		if (fcolladaTechnique != NULL)
		{
			FCDENodeList nodesToRelease;
			size_t childNodeCount = fcolladaTechnique->GetChildNodeCount();
			for (size_t c = 0; c < childNodeCount; ++c)
			{
				FCDENode* node = fcolladaTechnique->GetChildNode(c);
				xmlNode* baseNode = FUXmlWriter::CreateNode("_temp_");
				xmlNode* instanceNode = FArchiveXML::LetWriteObject(node, baseNode);

				uint32 instanceType = FArchiveXML::GetEntityInstanceType(instanceNode);
				if (instanceType == (uint32) ~0)
				{
					status = false;
				}
				else
				{
					FCDEntityInstance* instance = sceneNode->AddInstance((FCDEntity::Type) instanceType);
					status &= (FArchiveXML::LoadSwitch(instance, &instance->GetObjectType(), instanceNode));
					nodesToRelease.push_back(node);
				}

				xmlFreeNodeList(baseNode);
			}
			CLEAR_POINTER_VECTOR(nodesToRelease);
		}
	}
 
	sceneNode->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadTransform(FCDObject* UNUSED(object), xmlNode* UNUSED(node))
{ 
	//
	// Should never be called
	//
	FUBreak;
	return false;
}			

bool FArchiveXML::LoadTransformLookAt(FCDObject* object, xmlNode* lookAtNode)
{ 
	FCDTLookAt* tLookAt = (FCDTLookAt*)object;

	const char* content = FUDaeParser::ReadNodeContentDirect(lookAtNode);
	FloatList factors;
	factors.reserve(9);
	FUStringConversion::ToFloatList(content, factors);
	if (factors.size() != 9) return false;

	tLookAt->GetPosition().Set(factors[0], factors[1], factors[2]);
	tLookAt->GetTarget().Set(factors[3], factors[4], factors[5]);
	tLookAt->GetUp().Set(factors[6], factors[7], factors[8]);

	// Register the animated values
	FArchiveXML::LoadAnimatable(&tLookAt->GetLookAt(), lookAtNode);

	tLookAt->SetDirtyFlag();
	return true;
}

bool FArchiveXML::LoadTransformMatrix(FCDObject* object, xmlNode* node)
{ 
	FCDTMatrix* tMatrix = (FCDTMatrix*)object;

	const char* content = FUDaeParser::ReadNodeContentDirect(node);
	FUStringConversion::ToMatrix(&content, tMatrix->GetTransform());

	// Register the matrix in the animation system for transform animations
	FArchiveXML::LoadAnimatable(&tMatrix->GetTransform(), node);

	tMatrix->SetDirtyFlag();
	return true;
}

bool FArchiveXML::LoadTransformRotation(FCDObject* object, xmlNode* node)
{ 
	FCDTRotation* tRotation = (FCDTRotation*)object;

	const char* content = FUDaeParser::ReadNodeContentDirect(node);
	FloatList factors;
	factors.reserve(4);
	FUStringConversion::ToFloatList(content, factors);
	if (factors.size() != 4) return false;

	tRotation->SetAxis(factors[0], factors[1], factors[2]);
	tRotation->SetAngle(factors[3]);
	FArchiveXML::LoadAnimatable(&tRotation->GetAngleAxis(), node);

	tRotation->SetDirtyFlag();
	return true;
}

bool FArchiveXML::LoadTransformScale(FCDObject* object, xmlNode* node)
{ 
	FCDTScale* tScale = (FCDTScale*)object;

	const char* content = FUDaeParser::ReadNodeContentDirect(node);
	FloatList factors;
	factors.reserve(3);
	FUStringConversion::ToFloatList(content, factors);
	if (factors.size() != 3) return false;

	tScale->SetScale(factors[0], factors[1], factors[2]);

	// Register the animated values
	FArchiveXML::LoadAnimatable(&tScale->GetScale(), node);

	tScale->SetDirtyFlag();
	return true;
}

bool FArchiveXML::LoadTransformSkew(FCDObject* object, xmlNode* skewNode)
{
	FCDTSkew* tSkew = (FCDTSkew*)object;

	const char* content = FUDaeParser::ReadNodeContentDirect(skewNode);
	FloatList factors;
	factors.reserve(7);
	FUStringConversion::ToFloatList(content, factors);
	if (factors.size() != 7) return false;

	tSkew->SetAngle(factors[0]);
	tSkew->SetRotateAxis(FMVector3(factors[1], factors[2], factors[3]));
	tSkew->SetAroundAxis(FMVector3(factors[4], factors[5], factors[6]));

	// Check and pre-process the axises
	if (IsEquivalent(tSkew->GetRotateAxis(), FMVector3::Origin) || IsEquivalent(tSkew->GetAroundAxis(), FMVector3::Origin)) return false;
	tSkew->SetRotateAxis(tSkew->GetRotateAxis().Normalize());
	tSkew->SetAroundAxis(tSkew->GetAroundAxis().Normalize());

	// Register the animated values
	FArchiveXML::LoadAnimatable(&tSkew->GetSkew(), skewNode);

	tSkew->SetDirtyFlag();
	return true;
}

bool FArchiveXML::LoadTransformTranslation(FCDObject* object, xmlNode* node)
{
	FCDTTranslation* tTranslation = (FCDTTranslation*)object;

	const char* content = FUDaeParser::ReadNodeContentDirect(node);
	FloatList factors;
	factors.reserve(3);
	FUStringConversion::ToFloatList(content, factors);
	if (factors.size() != 3) return false;
	
	tTranslation->SetTranslation(factors[0], factors[1], factors[2]);
	FArchiveXML::LoadAnimatable(&tTranslation->GetTranslation(), node);
	
	tTranslation->SetDirtyFlag();
	return true;
}	

uint32 FArchiveXML::GetTransformType(xmlNode* node)
{
	if (IsEquivalent(node->name, DAE_ROTATE_ELEMENT)) return FCDTransform::ROTATION;
	else if (IsEquivalent(node->name, DAE_TRANSLATE_ELEMENT)) return FCDTransform::TRANSLATION;
	else if (IsEquivalent(node->name, DAE_SCALE_ELEMENT)) return FCDTransform::SCALE;
	else if (IsEquivalent(node->name, DAE_SKEW_ELEMENT)) return FCDTransform::SKEW;
	else if (IsEquivalent(node->name, DAE_MATRIX_ELEMENT)) return FCDTransform::MATRIX;
	else if (IsEquivalent(node->name, DAE_LOOKAT_ELEMENT)) return FCDTransform::LOOKAT;
	return (uint32) ~0;
}


