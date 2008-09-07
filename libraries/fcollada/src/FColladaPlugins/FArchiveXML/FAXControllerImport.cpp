/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDSkinController.h"
#include "FCDocument/FCDMorphController.h"
#include "FCDocument/FCDControllerInstance.h"

bool FArchiveXML::LoadController(FCDObject* object, xmlNode* controllerNode)
{
	FCDController* controller = (FCDController*)object;

	bool status = FArchiveXML::LoadEntity(object, controllerNode);
	if (!status) return status;
	if (!IsEquivalent(controllerNode->name, DAE_CONTROLLER_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_CONTROLLER_LIB_NODE, controllerNode->line);
		return status;
	}

	// Find the <skin> or <morph> element and process it
	xmlNode* skinNode = FindChildByType(controllerNode, DAE_CONTROLLER_SKIN_ELEMENT);
	xmlNode* morphNode = FindChildByType(controllerNode, DAE_CONTROLLER_MORPH_ELEMENT);
	if (skinNode != NULL && morphNode != NULL)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_CONTROLLER_TYPE_CONFLICT, controllerNode->line);
	}
	if (skinNode != NULL)
	{
		// Create and parse in the skin controller
		FCDSkinController* skin = controller->CreateSkinController();
		status &= (FArchiveXML::LoadSkinController(skin, skinNode));
	}
	else if (morphNode != NULL)
	{
		// Create and parse in the morph controller
		FCDMorphController* morph = controller->CreateMorphController();
		status &= (FArchiveXML::LoadMorphController(morph, morphNode));
	}
	else
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_SM_BASE_MISSING, controllerNode->line);
	}
	return status;
}			

bool FArchiveXML::LoadSkinController(FCDObject* object, xmlNode* skinNode)
{
	FCDSkinController* skinController = (FCDSkinController*)object;
	FCDSkinControllerDataMap& skinDataMap = FArchiveXML::documentLinkDataMap[skinController->GetDocument()].skinControllerDataMap;
	if (skinDataMap.find(skinController) == skinDataMap.end())
	{
		FCDSkinControllerData data;
		data.jointAreSids = false;
		skinDataMap.insert(skinController, data);
	}
	FCDSkinControllerData& data = skinDataMap.find(skinController)->second;

	bool status = true;
	if (!IsEquivalent(skinNode->name, DAE_CONTROLLER_SKIN_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_CONTROLLER_LIB_NODE, skinNode->line);
		return status;
	}

	// Read in the <bind_shape_matrix> element
	xmlNode* bindShapeTransformNode = FindChildByType(skinNode, DAE_BINDSHAPEMX_SKIN_PARAMETER);
	if (bindShapeTransformNode == NULL) skinController->SetBindShapeTransform(FMMatrix44::Identity);
	else
	{
		const char* content = ReadNodeContentDirect(bindShapeTransformNode);
		skinController->SetBindShapeTransform(FUStringConversion::ToMatrix(content));
	}

	// Find the target geometry, this is linked post-load
	FUUri targetUri = ReadNodeUrl(skinNode, DAE_SOURCE_ATTRIBUTE);
	skinController->SetTargetUri(targetUri);

	// Retrieve the <joints> element and the <vertex_weights> element
	xmlNode* jointsNode = FindChildByType(skinNode, DAE_JOINTS_ELEMENT);
	xmlNode* combinerNode = FindChildByType(skinNode, DAE_WEIGHTS_ELEMENT);

	// Verify that we have the necessary data structures: bind-shape, <joints> elements, <combiner> element
	if (jointsNode == NULL || combinerNode == NULL)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_MISSING_ELEMENT, skinNode->line);
		return status = false;
	}

	// Gather the inputs for the <joints> element and the <combiner> element
	xmlNode* firstCombinerValueNode = NULL;
	xmlNodeList skinningInputNodes;
	FindChildrenByType(jointsNode, DAE_INPUT_ELEMENT, skinningInputNodes);
	uint32 combinerValueCount = ReadNodeCount(combinerNode);
	for (xmlNode* child = combinerNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;
		if (IsEquivalent(child->name, DAE_INPUT_ELEMENT)) skinningInputNodes.push_back(child);
		else if (IsEquivalent(child->name, DAE_VERTEX_ELEMENT) || IsEquivalent(child->name, DAE_VERTEXCOUNT_ELEMENT))
		{ 
			firstCombinerValueNode = child;
			break;
		}
	}
	if (firstCombinerValueNode == NULL)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_MISSING_ELEMENT, combinerNode->line);
	}

	// Process these inputs
	FloatList weights;
	StringList jointSubIds;
	FMMatrix44List invertedBindPoses;
	int32 jointIdx = 0, weightIdx = 1;
	for (xmlNodeList::iterator it = skinningInputNodes.begin(); it != skinningInputNodes.end(); ++it)
	{
		fm::string semantic = ReadNodeSemantic(*it);
		fm::string sourceId = ReadNodeSource(*it);
		xmlNode* sourceNode = FindChildById(skinNode, sourceId);

		if (semantic == DAE_JOINT_SKIN_INPUT)
		{
			fm::string idx = ReadNodeProperty(*it, DAE_OFFSET_ATTRIBUTE);
			if (!idx.empty()) jointIdx = FUStringConversion::ToInt32(idx);
			if (!jointSubIds.empty()) continue;
			ReadSource(sourceNode, jointSubIds);
			data.jointAreSids = FindChildByType(sourceNode, DAE_NAME_ARRAY_ELEMENT) != NULL;
		}
		else if (semantic == DAE_BINDMATRIX_SKIN_INPUT)
		{
			if (!invertedBindPoses.empty())
			{
				FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_IB_MATRIX_MISSING, (*it)->line);
			}

			// Read in the bind-pose matrices <source> element
			ReadSource(sourceNode, invertedBindPoses);
		}
		else if (semantic == DAE_WEIGHT_SKIN_INPUT)
		{
			fm::string idx = ReadNodeProperty(*it, DAE_OFFSET_ATTRIBUTE);
			if (!idx.empty()) weightIdx = FUStringConversion::ToInt32(idx);

			// Read in the weights <source> element
			ReadSource(sourceNode, weights);
		}
	}

	// Parse the <vcount> and the <v> elements
	UInt32List combinerVertexCounts; combinerVertexCounts.reserve(combinerValueCount);
	Int32List combinerVertexIndices; combinerVertexIndices.reserve(combinerValueCount * 5);

	// The <vcount> and the <v> elements are ordered. Read the <vcount> element first.
	if (!IsEquivalent(firstCombinerValueNode->name, DAE_VERTEXCOUNT_ELEMENT))
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_VCOUNT_MISSING, firstCombinerValueNode->line);
	}
	const char* content = ReadNodeContentDirect(firstCombinerValueNode);
	FUStringConversion::ToUInt32List(content, combinerVertexCounts);

	// Read the <v> element second.
	xmlNode* vNode = firstCombinerValueNode->next;
	while (vNode != NULL && vNode->type != XML_ELEMENT_NODE) vNode = vNode->next;
	if (vNode == NULL || !IsEquivalent(vNode->name, DAE_VERTEX_ELEMENT))
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_V_ELEMENT_MISSING, vNode->line);
	}
	content = ReadNodeContentDirect(vNode);
	FUStringConversion::ToInt32List(content, combinerVertexIndices);
	size_t combinerVertexIndexCount = combinerVertexIndices.size();

	// Validate the inputs
	if (jointSubIds.size() != invertedBindPoses.size())
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_JC_BPMC_NOT_EQUAL, skinNode->line);
	}
	if (combinerVertexCounts.size() != combinerValueCount)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_INVALID_VCOUNT, skinNode->line);
	}

	// Setup the joint-weight-vertex matches
	skinController->SetInfluenceCount(combinerValueCount);
	size_t jointCount = jointSubIds.size(), weightCount = weights.size(), offset = 0;
	for (size_t j = 0; j < combinerValueCount; ++j)
	{
		FCDSkinControllerVertex* vert = skinController->GetVertexInfluence(j);
		uint32 localValueCount = combinerVertexCounts[j];
		vert->SetPairCount(localValueCount);
		for (size_t i = 0; i < localValueCount && offset < combinerVertexIndexCount - 1; ++i)
		{
			FCDJointWeightPair* pair = vert->GetPair(i);
			pair->jointIndex = combinerVertexIndices[offset + jointIdx];
			if (pair->jointIndex < -1 || pair->jointIndex >= (int32) jointCount)
			{
				FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_JOINT_INDEX);
				pair->jointIndex = -1;
			}
			uint32 weightIndex = combinerVertexIndices[offset + weightIdx];
			if (weightIndex >= weightCount)
			{
				FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_WEIGHT_INDEX);
				weightIndex = 0;
			}
			pair->weight = weights[weightIndex];
			offset += 2;
		}
	}

	// Normalize the weights, per-vertex, to 1 (or 0)
	// This step is still being debated as necessary or not, for COLLADA 1.4.
	for (size_t j = 0; j < combinerValueCount; ++j)
	{
		FCDSkinControllerVertex* vertex = skinController->GetVertexInfluence(j);
		float weightSum = 0.0f;
		for (size_t i = 0; i < vertex->GetPairCount(); ++i) weightSum += vertex->GetPair(i)->weight;
		if (IsEquivalent(weightSum, 0.0f) || IsEquivalent(weightSum, 1.0f)) continue;

		float invWeightSum = 1.0f / weightSum;
		for (size_t i = 0; i < vertex->GetPairCount(); ++i) vertex->GetPair(i)->weight *= invWeightSum;
	}

	// Setup the joints.
	skinController->SetJointCount(jointCount);
	for (size_t i = 0; i < jointCount; ++i)
	{
		skinController->GetJoint(i)->SetId(jointSubIds[i]);
		skinController->GetJoint(i)->SetBindPoseInverse(invertedBindPoses[i]);
	}

	skinController->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadMorphController(FCDObject* object, xmlNode* morphNode)
{
	FCDMorphController* morphController = (FCDMorphController*)object;
	FCDMorphControllerData& data = FArchiveXML::documentLinkDataMap[morphController->GetDocument()].morphControllerDataMap[morphController];

	bool status = true;
	if (!IsEquivalent(morphNode->name, DAE_CONTROLLER_MORPH_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_CONTROLLER_LIB_NODE, morphNode->line);
		return status;
	}

	// Parse in the morph method
	fm::string methodValue = ReadNodeProperty(morphNode, DAE_METHOD_ATTRIBUTE);
	morphController->SetMethod(FUDaeMorphMethod::FromString(methodValue));
	if (morphController->GetMethod() == FUDaeMorphMethod::UNKNOWN)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_MC_PROC_METHOD, morphNode->line);
	}

	// Find the base geometry, this is linked after load.
	data.targetId = ReadNodeSource(morphNode);

	// Find the <targets> element and process its inputs
	xmlNode* targetsNode = FindChildByType(morphNode, DAE_TARGETS_ELEMENT);
	if (targetsNode == NULL)
	{
		//return status.Fail(FS("Cannot find necessary <targets> element for morph controller: ") + TO_FSTRING(parent->GetDaeId()), morphNode->line);
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_MISSING_ELEMENT, morphNode->line);

	}
	xmlNodeList inputNodes;
	FindChildrenByType(targetsNode, DAE_INPUT_ELEMENT, inputNodes);

	// Find the TARGET and WEIGHT input necessary sources
	xmlNode* targetSourceNode = NULL,* weightSourceNode = NULL;
	for (xmlNodeList::iterator it = inputNodes.begin(); it != inputNodes.end(); ++it)
	{
		xmlNode* inputNode = (*it);
		fm::string semantic = ReadNodeSemantic(inputNode);
		fm::string sourceId = ReadNodeSource(inputNode);
		if (semantic == DAE_WEIGHT_MORPH_INPUT || semantic == DAE_WEIGHT_MORPH_INPUT_DEPRECATED)
		{
			weightSourceNode = FindChildById(morphNode, sourceId);
		}
		else if (semantic == DAE_TARGET_MORPH_INPUT || semantic == DAE_TARGET_MORPH_INPUT_DEPRECATED)
		{
			targetSourceNode = FindChildById(morphNode, sourceId);
		}
		else
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_MORPH_TARGET_TYPE, inputNode->line);
		}
	}
	if (targetSourceNode == NULL || weightSourceNode == NULL)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_MISSING_INPUT, targetsNode->line);
		return status;
	}

	// Read in the sources
	StringList morphTargetIds;
	ReadSource(targetSourceNode, morphTargetIds);
	FloatList weights;
	ReadSource(weightSourceNode, weights);
	size_t targetCount = morphTargetIds.size();
	if (weights.size() != targetCount)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_SOURCE_SIZE, targetSourceNode->line);
	}

	// Find the target geometries and build the morph targets
	for (int32 i = 0; i < (int32) targetCount; ++i)
	{
		FCDGeometry* targetGeometry = morphController->GetDocument()->FindGeometry(morphTargetIds[i]);
		if (targetGeometry == NULL)
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_TARGET_GEOMETRY_MISSING, morphNode->line);
		}
		FCDMorphTarget* morphTarget = morphController->AddTarget(targetGeometry, weights[i]);

		// Record the morphing weight as animatable
		// COLLADA has a morph weight array, but FCollada considers each weight independently.
		// For this reason, we temporarly assign an array index to the weight animated.
		morphTarget->GetWeight().GetAnimated()->SetArrayElement(i);
		FArchiveXML::LoadAnimatable(&morphTarget->GetWeight(), weightSourceNode);
		if (morphTarget->GetWeight().IsAnimated()) morphTarget->GetWeight().GetAnimated()->SetArrayElement(-1);
	}

	morphController->SetDirtyFlag();
	return status;
}

FCDSkinController* FArchiveXML::FindSkinController(FCDControllerInstance* controllerInstance, FCDEntity* entity)
{
	if (entity != NULL && entity->GetType() == FCDEntity::CONTROLLER)
	{
		FCDController* controller = (FCDController*) entity;
	
		if (controller->IsSkin()) 
		{
			return controller->GetSkinController();
		}
		else return FArchiveXML::FindSkinController(controllerInstance, controller->GetBaseTarget());
	}
	return NULL;
}
