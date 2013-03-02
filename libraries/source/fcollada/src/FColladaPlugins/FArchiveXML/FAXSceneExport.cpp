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
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDTargetedEntity.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDTransform.h"
#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDEmitterInstance.h"
#include "FCDocument/FCDEntityInstance.h"

xmlNode* FArchiveXML::WriteEntity(FCDObject* object, xmlNode* parentNode)
{
	FCDEntity* entity = (FCDEntity*)object;

	return FArchiveXML::WriteToEntityXMLFCDEntity(entity, parentNode, DAEERR_UNKNOWN_ELEMENT);
}

xmlNode* FArchiveXML::WriteTargetedEntity(FCDObject* UNUSED(object), xmlNode* UNUSED(parentNode))
{
	// Currently not reachable
	FUBreak;
	return NULL;
}

xmlNode* FArchiveXML::WriteSceneNode(FCDObject* object, xmlNode* parentNode)
{
	FCDSceneNode* sceneNode = (FCDSceneNode*)object;

	xmlNode* node = NULL;
	bool isVisualScene = false;

	FCDENodeList extraParameters;
	FCDETechnique* extraTechnique = NULL;

	if (sceneNode->GetParentCount() == 0)
	{
		node = FArchiveXML::WriteToEntityXMLFCDEntity(sceneNode, parentNode, DAE_VSCENE_ELEMENT);
		isVisualScene = true;
	}
	else
	{
		node = FArchiveXML::WriteToEntityXMLFCDEntity(sceneNode, parentNode, DAE_NODE_ELEMENT);
		if (sceneNode->GetSubId().length() > 0) AddAttribute(node, DAE_SID_ATTRIBUTE, sceneNode->GetSubId());

		// Set the scene node's type.
		const char* nodeType = sceneNode->GetJointFlag() ? DAE_JOINT_NODE_TYPE : DAE_NODE_NODE_TYPE;
		AddAttribute(node, DAE_TYPE_ATTRIBUTE, nodeType);

		// Write out the visibility of this node, if it is not visible or if it is animated.
		if (sceneNode->GetVisibility().IsAnimated() || !sceneNode->IsVisible())
		{
			extraTechnique = const_cast<FCDExtra*>(sceneNode->GetExtra())->GetDefaultType()->AddTechnique(DAE_FCOLLADA_PROFILE);
			FCDENode* visibilityNode = extraTechnique->AddParameter(DAEFC_VISIBILITY_PARAMETER, sceneNode->GetVisibility() >= 0.5f);
			visibilityNode->GetAnimated()->Copy(sceneNode->GetVisibility().GetAnimated());
			extraParameters.push_back(visibilityNode);
		}
	}

	// Write out the transforms
	size_t transformCount = sceneNode->GetTransformCount();
	for (size_t t = 0; t < transformCount; ++t)
	{
		FCDTransform* transform = sceneNode->GetTransform(t);
		FArchiveXML::LetWriteObject(transform, node);
	}

	// Write out the instances
	// Some of the FCollada instance types are not a part of COLLADA, so buffer them to export in the <extra>.
	FCDENodeList extraInstanceNodes;
	FCDETechnique* extraInstanceTechnique = NULL;
	size_t instanceCount = sceneNode->GetInstanceCount();
	for (size_t i = 0; i < instanceCount; ++i)
	{
		FCDEntityInstance* instance = sceneNode->GetInstance(i);
		if (instance->GetEntityType() == FCDEntity::FORCE_FIELD || instance->GetEntityType() == FCDEntity::EMITTER)
		{
			if (extraInstanceTechnique == NULL)
			{
				FCDExtra* extra = const_cast<FCDExtra*>(sceneNode->GetExtra());
				FCDEType* extraType = extra->AddType(DAEFC_INSTANCES_TYPE);
				extraInstanceTechnique = extraType->AddTechnique(DAE_FCOLLADA_PROFILE);
			}

			xmlNode* base = FUXmlWriter::CreateNode("_temp_");
			xmlNode* instanceNode = FArchiveXML::LetWriteObject(instance, base);
			FCDENode* instanceAsExtra = extraInstanceTechnique->AddChildNode();
			
			bool loadSuccess = FArchiveXML::LoadExtraNode(instanceAsExtra, instanceNode);
			
			xmlFreeNodeList(base);

			if (loadSuccess)
			{
				extraInstanceNodes.push_back(instanceAsExtra);
				continue;
			}
			// If we fail to create the extra tree, by default we fallback to writing to XML
		}
		FArchiveXML::LetWriteObject(instance, node);
	}

	// First, write out the child nodes that we consider instances: there is more than one
	// parent node and we aren't the first one.
	size_t childCount = sceneNode->GetChildrenCount();
	for (size_t c = 0; c < childCount; ++c)
	{
		FCDSceneNode* child = sceneNode->GetChild(c);
		if (child->GetParent() != sceneNode)
		{
			bool alreadyInstantiated = false;
			for (size_t i = 0; i < instanceCount; ++i)
			{
				FCDEntityInstance* instance = sceneNode->GetInstance(i);
				if (!instance->IsExternalReference())
				{
					alreadyInstantiated |= instance->GetEntity() == child;
				}
			}
			if (!alreadyInstantiated)
			{
				FCDEntityInstance* instance = FCDEntityInstanceFactory::CreateInstance(const_cast<FCDocument*>(sceneNode->GetDocument()), NULL, FCDEntity::SCENE_NODE);
				instance->SetEntity(const_cast<FCDSceneNode*>(child));
				FArchiveXML::LetWriteObject(instance, node);
			}
		}
	}

	// Then, hierarchically write out the child nodes.
	for (size_t c = 0; c < childCount; ++c)
	{
		FCDSceneNode* child = sceneNode->GetChild(c);
		if (child->GetParent() == sceneNode)
		{
			FArchiveXML::LetWriteObject(child, node);
		}
	}

	// This only writes extra-related stuff, so execute last.
	if (isVisualScene) FArchiveXML::WriteVisualScene(sceneNode, node);

	// Write out the extra information and release the temporarily added extra parameters
	FArchiveXML::WriteEntityExtra(sceneNode, node);

	if (extraTechnique != NULL)
	{
		CLEAR_POINTER_VECTOR(extraParameters);
		if (extraTechnique->GetChildNodeCount() == 0) SAFE_RELEASE(extraTechnique);
	}
	CLEAR_POINTER_VECTOR(extraInstanceNodes);

	return parentNode;
}

xmlNode* FArchiveXML::WriteTransform(FCDObject* UNUSED(object), xmlNode* UNUSED(parentNode))
{
	//
	// Currently not reachable
	//
	FUBreak;
	return NULL;
}

xmlNode* FArchiveXML::WriteTransformLookAt(FCDObject* object, xmlNode* parentNode)
{
	FCDTLookAt* tLookAt = (FCDTLookAt*)object;

	FUSStringBuilder builder;
	FUStringConversion::ToString(builder, tLookAt->GetPosition()); builder.append(' ');
	FUStringConversion::ToString(builder, tLookAt->GetTarget()); builder.append(' ');
	FUStringConversion::ToString(builder, tLookAt->GetUp());
	xmlNode* transformNode = FUDaeWriter::AddChild(parentNode, DAE_LOOKAT_ELEMENT, builder);
	FArchiveXML::WriteTransformBase(tLookAt, transformNode, "transform");
	return transformNode;
}

xmlNode* FArchiveXML::WriteTransformMatrix(FCDObject* object, xmlNode* parentNode)
{
	FCDTMatrix* tMatrix = (FCDTMatrix*)object;
	fm::string content = FUStringConversion::ToString((FMMatrix44&) tMatrix->GetTransform());
	xmlNode* transformNode = FUDaeWriter::AddChild(parentNode, DAE_MATRIX_ELEMENT, content);
	FArchiveXML::WriteTransformBase(tMatrix, transformNode, "transform");
	return transformNode;
}

xmlNode* FArchiveXML::WriteTransformRotation(FCDObject* object, xmlNode* parent)
{
	FCDTRotation* tRotation = (FCDTRotation*)object;

	FUSStringBuilder builder;
	FUStringConversion::ToString(builder, tRotation->GetAxis()); builder += ' '; builder += tRotation->GetAngle();
	xmlNode* transformNode = FUDaeWriter::AddChild(parent, DAE_ROTATE_ELEMENT);
	FUXmlWriter::AddContentUnprocessed(transformNode, builder);
	FArchiveXML::WriteTransformBase(tRotation, transformNode, "rotation");
	return transformNode;
}

xmlNode* FArchiveXML::WriteTransformScale(FCDObject* object, xmlNode* parentNode)
{
	FCDTScale* tScale = (FCDTScale*)object;

	fm::string content = FUStringConversion::ToString(tScale->GetScale());
	xmlNode* transformNode = FUDaeWriter::AddChild(parentNode, DAE_SCALE_ELEMENT);
	FUXmlWriter::AddContentUnprocessed(transformNode, content);
	FArchiveXML::WriteTransformBase(tScale, transformNode, "scale");
	return transformNode;
}

xmlNode* FArchiveXML::WriteTransformSkew(FCDObject* object, xmlNode* parentNode)
{
	FCDTSkew* tSkew = (FCDTSkew*)object;

	FUSStringBuilder builder;
	builder.set(tSkew->GetAngle()); builder += ' ';
	FUStringConversion::ToString(builder, tSkew->GetRotateAxis()); builder += ' ';
	FUStringConversion::ToString(builder, tSkew->GetAroundAxis());
	xmlNode* transformNode = FUDaeWriter::AddChild(parentNode, DAE_SKEW_ELEMENT);
	FUXmlWriter::AddContentUnprocessed(transformNode, builder);
	FArchiveXML::WriteTransformBase(tSkew, transformNode, "skew");
	return transformNode;
}

xmlNode* FArchiveXML::WriteTransformTranslation(FCDObject* object, xmlNode* parentNode)
{
	FCDTTranslation* tTranslation = (FCDTTranslation*)object;

	fm::string content = FUStringConversion::ToString(tTranslation->GetTranslation());
	xmlNode* transformNode = FUDaeWriter::AddChild(parentNode, DAE_TRANSLATE_ELEMENT);
	FUXmlWriter::AddContentUnprocessed(transformNode, content);
	FArchiveXML::WriteTransformBase(tTranslation, transformNode, "translation");
	return transformNode;
}

xmlNode* FArchiveXML::WriteToEntityXMLFCDEntity(FCDEntity* entity, xmlNode* parentNode, const char* nodeName, bool writeId)
{
	// Create the entity node and write out the id and name attributes
	xmlNode* entityNode = AddChild(parentNode, nodeName);
	if (writeId)
	{
		AddAttribute(entityNode, DAE_ID_ATTRIBUTE, entity->GetDaeId());
	}
	if (!entity->GetName().empty())
	{
		AddAttribute(entityNode, DAE_NAME_ATTRIBUTE, entity->GetName());
	}

	// Write out the asset information.
	if (const_cast<const FCDEntity*>(entity)->GetAsset() != NULL) FArchiveXML::LetWriteObject(entity->GetAsset(), entityNode);

	return entityNode;
}

void FArchiveXML::WriteEntityExtra(FCDEntity* entity, xmlNode* entityNode)
{
	if (entity->GetExtra() != NULL)
	{
		FCDENodeList extraParameters;
		FCDETechnique* extraTechnique = NULL;

		// Add the note to the extra information
		if (entity->HasNote())
		{
			extraTechnique = const_cast<FCDExtra&>(*entity->GetExtra()).GetDefaultType()->AddTechnique(DAE_FCOLLADA_PROFILE);
			FCDENode* noteNode = extraTechnique->AddParameter(DAEMAX_USERPROPERTIES_NODE_PARAMETER, entity->GetNote());
			extraParameters.push_back(noteNode);
		}

		// Write out all the typed and untyped extra information and release the temporarily-added extra parameters.
		FArchiveXML::LetWriteObject(entity->GetExtra(), entityNode);
		if (extraTechnique != NULL)
		{
			CLEAR_POINTER_VECTOR(extraParameters);
			if (extraTechnique->GetChildNodeCount() == 0) SAFE_RELEASE(extraTechnique);
		}
	}
}

void FArchiveXML::WriteEntityInstanceExtra(FCDEntityInstance* entityInstance, xmlNode* instanceNode)
{
	if (entityInstance->GetExtra() != NULL)
	{
		FArchiveXML::LetWriteObject(entityInstance->GetExtra(), instanceNode);
	}
}

void FArchiveXML::WriteTargetedEntityExtra(FCDTargetedEntity* targetedEntity, xmlNode* entityNode)
{
	FCDETechnique* technique = NULL;
	FCDENode* parameter = NULL;

	if (targetedEntity->GetTargetNode() != NULL)
	{
		// Just for the export-time, add to the extra tree, the target information.
        FCDExtra* extra = const_cast<FCDExtra*>(targetedEntity->GetExtra());
		technique = extra->GetDefaultType()->AddTechnique(DAE_FCOLLADA_PROFILE);
		parameter = technique->AddParameter(DAEFC_TARGET_PARAMETER, FS("#") + TO_FSTRING(targetedEntity->GetTargetNode()->GetDaeId()));
	}

	// Export the extra tree to XML
	FArchiveXML::WriteEntityExtra(targetedEntity, entityNode);

	if (targetedEntity->GetTargetNode() != NULL)
	{
		// Delete the created extra tree nodes.
		SAFE_RELEASE(parameter);
		if (technique->GetChildNodeCount() == 0) SAFE_RELEASE(technique);
	}
}

void FArchiveXML::WriteVisualScene(FCDSceneNode* sceneNode, xmlNode* parentNode)
{
	// Only one of the visual scenes should write this.
	if (sceneNode->GetDocument()->GetVisualSceneInstance() == sceneNode)
	{
		// For the main visual scene: export the layer information
		const FCDLayerList& layers = sceneNode->GetDocument()->GetLayers();
		if (!layers.empty())
		{
			xmlNode* techniqueNode = AddExtraTechniqueChild(parentNode, DAEMAYA_MAYA_PROFILE);
			for (FCDLayerList::const_iterator itL = layers.begin(); itL != layers.end(); ++itL)
			{
				xmlNode* layerNode = AddChild(techniqueNode, DAEMAYA_LAYER_PARAMETER);
				if (!(*itL)->name.empty()) AddAttribute(layerNode, DAE_NAME_ATTRIBUTE, (*itL)->name);
				FUSStringBuilder layerObjects;
				for (StringList::const_iterator itO = (*itL)->objects.begin(); itO != (*itL)->objects.end(); ++itO)
				{
					layerObjects.append(*itO);
					layerObjects.append(' ');
				}
				layerObjects.pop_back();
				AddContent(layerNode, layerObjects);
			}
		}

		// Export the start/end time.
		if (sceneNode->GetDocument()->HasStartTime() || sceneNode->GetDocument()->HasEndTime())
		{
			xmlNode* techniqueNode = AddExtraTechniqueChild(parentNode, DAE_FCOLLADA_PROFILE);
			if (sceneNode->GetDocument()->HasStartTime()) AddChild(techniqueNode, DAEMAYA_STARTTIME_PARAMETER, sceneNode->GetDocument()->GetStartTime());
			if (sceneNode->GetDocument()->HasEndTime()) AddChild(techniqueNode, DAEMAYA_ENDTIME_PARAMETER, sceneNode->GetDocument()->GetEndTime());
		}
	}
}

void FArchiveXML::WriteTransformBase(FCDTransform* transform, xmlNode* transformNode, const char* wantedSid)
{
	// Add the sub-id to the node.
	if (!transform->GetSubId()->empty())
	{
		fm::string& _sid = const_cast<FUParameterString&>(transform->GetSubId());
		FUDaeWriter::AddNodeSid(transformNode, _sid);
		wantedSid = _sid.c_str();
	}

	// Process the animation of the transform.
	if (transform->IsAnimated())
	{
		FArchiveXML::WriteAnimatedValue(transform->GetAnimated(), transformNode, wantedSid);
	}
}
