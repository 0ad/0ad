/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDEntity.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDSkinController.h"
#include "FCDocument/FCDMorphController.h"
#include "FCDocument/FCDControllerInstance.h"

xmlNode* FArchiveXML::WriteController(FCDObject* object, xmlNode* parentNode)
{
	FCDController* controller = (FCDController*)object;

	xmlNode* controllerNode = FArchiveXML::WriteToEntityXMLFCDEntity(controller, parentNode, DAE_CONTROLLER_ELEMENT);
	if (controller->GetSkinController() != NULL) FArchiveXML::LetWriteObject(controller->GetSkinController(), controllerNode);
	else if (controller->GetMorphController() != NULL) FArchiveXML::LetWriteObject(controller->GetMorphController(), controllerNode);
	FArchiveXML::WriteEntityExtra(controller, controllerNode);
	return controllerNode;
}

xmlNode* FArchiveXML::WriteSkinController(FCDObject* object, xmlNode* parentNode)
{
	FCDSkinController* skinController = (FCDSkinController*)object;

	// Create the <skin> element
	xmlNode* skinNode = AddChild(parentNode, DAE_CONTROLLER_SKIN_ELEMENT);

	FUUri targetUri = skinController->GetTargetUri();
	fstring targetPath = skinController->GetDocument()->GetFileManager()->CleanUri(targetUri);
	AddAttribute(skinNode, DAE_SOURCE_ATTRIBUTE, targetPath);

	// Create the <bind_shape_matrix> element
	fm::string bindShapeMatrixString = FUStringConversion::ToString((FMMatrix44&) skinController->GetBindShapeTransform());
	AddChild(skinNode, DAE_BINDSHAPEMX_SKIN_PARAMETER, bindShapeMatrixString);

	// Prepare the joint source information
	size_t jointCount = skinController->GetJointCount();
	StringList jointSubIds; jointSubIds.reserve(jointCount);
	FMMatrix44List jointBindPoses; jointBindPoses.reserve(jointCount);
	for (size_t i = 0; i < jointCount; ++i)
	{
		FCDSkinControllerJoint* joint = skinController->GetJoint(i);
		jointSubIds.push_back(joint->GetId());
		jointBindPoses.push_back(joint->GetBindPoseInverse());
	}
	
	// Create the joint source.
	FUSStringBuilder jointSourceId(skinController->GetParent()->GetDaeId()); jointSourceId += "-joints";
	AddSourceString(skinNode, jointSourceId.ToCharPtr(), jointSubIds, DAE_JOINT_SKIN_INPUT);
	
	// Create the joint bind matrix source
	FUSStringBuilder jointBindSourceId(skinController->GetParent()->GetDaeId()); jointBindSourceId += "-bind_poses";
	AddSourceMatrix(skinNode, jointBindSourceId.ToCharPtr(), jointBindPoses);

	// Create the weight source
	FloatList weights;
	weights.push_back(1.0f);
	size_t influenceCount = skinController->GetInfluenceCount();
	for (size_t i = 0; i < influenceCount; ++i)
	{
		const FCDSkinControllerVertex* vertex = skinController->GetVertexInfluence(i);
		for (size_t i = 0; i < vertex->GetPairCount(); ++i)
		{
			float w = vertex->GetPair(i)->weight;
			if (!IsEquivalent(w, 1.0f)) weights.push_back(w);
		}
	}
	FUSStringBuilder weightSourceId(skinController->GetParent()->GetDaeId()); weightSourceId += "-weights";
	AddSourceFloat(skinNode, weightSourceId.ToCharPtr(), weights, DAE_WEIGHT_SKIN_INPUT);

	// Create the <joints> element
	xmlNode* jointsNode = AddChild(skinNode, DAE_JOINTS_ELEMENT);
	AddInput(jointsNode, jointSourceId.ToCharPtr(), DAE_JOINT_SKIN_INPUT);
	AddInput(jointsNode, jointBindSourceId.ToCharPtr(), DAE_BINDMATRIX_SKIN_INPUT);

	// Create the <vertex_weights> element
	xmlNode* matchesNode = AddChild(skinNode, DAE_WEIGHTS_ELEMENT);
	AddInput(matchesNode, jointSourceId.ToCharPtr(), DAE_JOINT_SKIN_INPUT, 0);
	AddInput(matchesNode, weightSourceId.ToCharPtr(), DAE_WEIGHT_SKIN_INPUT, 1);
	AddAttribute(matchesNode, DAE_COUNT_ATTRIBUTE, skinController->GetInfluenceCount());

	// Generate the vertex count and match value strings and export the <v> and <vcount> elements
	FUSStringBuilder vertexCounts; vertexCounts.reserve(1024);
	FUSStringBuilder vertexMatches; vertexMatches.reserve(1024);
	uint32 weightOffset = 1;
	for (size_t j = 0; j < influenceCount; ++j)
	{
		const FCDSkinControllerVertex* vertex = skinController->GetVertexInfluence(j);
		vertexCounts.append((uint32) vertex->GetPairCount()); vertexCounts.append(' ');
		for (size_t i = 0; i < vertex->GetPairCount(); ++i)
		{
			const FCDJointWeightPair* pair = vertex->GetPair(i);
			vertexMatches.append(pair->jointIndex); vertexMatches.append(' ');
			if (!IsEquivalent(pair->weight, 1.0f)) vertexMatches.append(weightOffset++);
			else vertexMatches.append('0');
			vertexMatches.append(' ');
		}
	}
	if (!vertexMatches.empty()) vertexMatches.pop_back();
	AddChild(matchesNode, DAE_VERTEXCOUNT_ELEMENT, vertexCounts);
	AddChild(matchesNode, DAE_VERTEX_ELEMENT, vertexMatches);
	return skinNode;
}

xmlNode* FArchiveXML::WriteMorphController(FCDObject* object, xmlNode* parentNode)
{
	FCDMorphController* morphController = (FCDMorphController*)object;

	size_t targetCount = morphController->GetTargetCount();

	// Create the <morph> node and set its attributes
	xmlNode* morphNode = AddChild(parentNode, DAE_CONTROLLER_MORPH_ELEMENT);
	AddAttribute(morphNode, DAE_METHOD_ATTRIBUTE, FUDaeMorphMethod::ToString(morphController->GetMethod()));
	if (morphController->GetBaseTarget() != NULL)
	{
		AddAttribute(morphNode, DAE_SOURCE_ATTRIBUTE, fm::string("#") + morphController->GetBaseTarget()->GetDaeId());
	}

	// Gather up the morph target ids and the morphing weights
	StringList targetIds; targetIds.reserve(targetCount);
	FloatList weights; weights.reserve(targetCount);
	for (size_t i = 0 ; i < morphController->GetTargetCount(); ++i)
	{
		const FCDMorphTarget* t = morphController->GetTarget(i);
		targetIds.push_back(t->GetGeometry() != NULL ? t->GetGeometry()->GetDaeId() : DAEERR_UNKNOWN_IDREF);
		weights.push_back(t->GetWeight());
	}

	// Export the target id source
	FUSStringBuilder targetSourceId(morphController->GetParent()->GetDaeId()); targetSourceId.append("-targets");
	AddSourceIDRef(morphNode, targetSourceId.ToCharPtr(), targetIds, DAE_TARGET_MORPH_INPUT);

	// Export the weight source
	FUSStringBuilder weightSourceId(morphController->GetParent()->GetDaeId()); weightSourceId.append("-morph_weights");
	xmlNode* weightSourceNode = AddSourceFloat(morphNode, weightSourceId.ToCharPtr(), weights, DAE_WEIGHT_MORPH_INPUT);

	// Export the <targets> elements
	xmlNode* targetsNode = AddChild(morphNode, DAE_TARGETS_ELEMENT);
	AddInput(targetsNode, targetSourceId.ToCharPtr(), DAE_TARGET_MORPH_INPUT);
	AddInput(targetsNode, weightSourceId.ToCharPtr(), DAE_WEIGHT_MORPH_INPUT);

	// Record the morphing weight animations
	for (int32 i = 0; i < (int32) targetCount; ++i)
	{
		const FCDMorphTarget* t = morphController->GetTarget(i);
		FArchiveXML::WriteAnimatedValue(&t->GetWeight(), weightSourceNode, "morphing_weights", i);
	}

	return morphNode;
}
