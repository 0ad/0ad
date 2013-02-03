/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationClip.h"
#include "FCDocument/FCDAnimationKey.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimationMultiCurve.h"

#define DONT_DEFINE_THIS

#ifdef DONT_DEFINE_THIS
#include <FCDocument/FCDExtra.h>
#endif

bool FArchiveXML::LoadAnimationChannel(FCDObject* object, xmlNode* channelNode)
{ 
	FCDAnimationChannel* animationChannel = (FCDAnimationChannel*)object;
	FCDAnimationChannelData& data = FArchiveXML::documentLinkDataMap[animationChannel->GetDocument()].animationChannelData[animationChannel];

	bool status = true;

	// Read the channel-specific ID
	fm::string daeId = ReadNodeId(channelNode);
	fm::string samplerId = ReadNodeSource(channelNode);
	ReadNodeTargetProperty(channelNode, data.targetPointer, data.targetQualifier);

#ifdef DONT_DEFINE_THIS
	FCDAnimation* anim = animationChannel->GetParent();
	FCDExtra* extra = anim->GetExtra();
	extra->SetTransientFlag(); // Dont save this, its wasted whatever it is.
	FCDEType* type = extra->AddType("AnimTargets");
	FCDETechnique* teq = type->AddTechnique("TEMP");
	teq->AddChildNode("pointer")->SetContent(TO_FSTRING(data.targetPointer));
	teq->AddChildNode("pointer")->SetContent(TO_FSTRING(data.targetQualifier));
#endif
	


	xmlNode* samplerNode = FArchiveXML::FindChildByIdFCDAnimation(animationChannel->GetParent(), samplerId);
	if (samplerNode == NULL || !IsEquivalent(samplerNode->name, DAE_SAMPLER_ELEMENT))
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_MISSING_ELEMENT, channelNode->line);
		return false;
	}

	// Find and process the sources
	xmlNode* inputSource = NULL, * outputSource = NULL, * inTangentSource = NULL, * outTangentSource = NULL, * tcbSource = NULL, * easeSource = NULL, * interpolationSource = NULL;
	xmlNodeList samplerInputNodes;
	fm::string inputDriver;
	FindChildrenByType(samplerNode, DAE_INPUT_ELEMENT, samplerInputNodes);
	for (size_t i = 0; i < samplerInputNodes.size(); ++i) // Don't use iterator here because we are possibly appending source nodes in the loop
	{
		xmlNode* inputNode = samplerInputNodes[i];
		fm::string sourceId = ReadNodeSource(inputNode);
		xmlNode* sourceNode = FArchiveXML::FindChildByIdFCDAnimation(animationChannel->GetParent(), sourceId);
		fm::string sourceSemantic = ReadNodeSemantic(inputNode);

		if (sourceSemantic == DAE_INPUT_ANIMATION_INPUT) inputSource = sourceNode;
		else if (sourceSemantic == DAE_OUTPUT_ANIMATION_INPUT) outputSource = sourceNode;
		else if (sourceSemantic == DAE_INTANGENT_ANIMATION_INPUT) inTangentSource = sourceNode;
		else if (sourceSemantic == DAE_OUTTANGENT_ANIMATION_INPUT) outTangentSource = sourceNode;
		else if (sourceSemantic == DAEFC_TCB_ANIMATION_INPUT) tcbSource = sourceNode;
		else if (sourceSemantic == DAEFC_EASE_INOUT_ANIMATION_INPUT) easeSource = sourceNode;
		else if (sourceSemantic == DAE_INTERPOLATION_ANIMATION_INPUT) interpolationSource = sourceNode;
		else if (sourceSemantic == DAEMAYA_DRIVER_INPUT) inputDriver = sourceId;
	}
	if (inputSource == NULL || outputSource == NULL)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_MISSING_INPUT, samplerNode->line);
		return false;
	}

	// Calculate the number of curves that in contained by this channel
	xmlNode* outputAccessor = FindTechniqueAccessor(outputSource);
	fm::string accessorStrideString = ReadNodeProperty(outputAccessor, DAE_STRIDE_ATTRIBUTE);
	uint32 curveCount = FUStringConversion::ToUInt32(accessorStrideString);
	if (curveCount == 0) curveCount = 1;

	// Create the animation curves
	for (uint32 i = 0; i < curveCount; ++i)
	{
		animationChannel->AddCurve();
	}

	// Read in the animation curves
	// The input keys and interpolations are shared by all the curves
	FloatList inputs;
    ReadSource(inputSource, inputs);
	size_t keyCount = inputs.size();
	if (keyCount == 0) return true; // Valid although very boring channel.

	UInt32List interpolations; interpolations.reserve(keyCount);
	ReadSourceInterpolation(interpolationSource, interpolations);
	if (interpolations.size() < keyCount)
	{
		// Not enough interpolation types provided, so append BEZIER as many times as needed.
		interpolations.insert(interpolations.end(), keyCount - interpolations.size(), FUDaeInterpolation::FromString(""));
	}

	// Read in the interleaved outputs as floats
	fm::vector<FloatList> tempFloatArrays;
	tempFloatArrays.resize(curveCount);
	fm::pvector<FloatList> outArrays(curveCount);
	for (uint32 i = 0; i < curveCount; ++i) outArrays[i] = &tempFloatArrays[i];
	ReadSourceInterleaved(outputSource, outArrays);
	for (uint32 i = 0; i < curveCount; ++i)
	{
		// Fill in the output array with zeroes, if it was not large enough.
		if (tempFloatArrays[i].size() < keyCount)
		{
			tempFloatArrays[i].insert(tempFloatArrays[i].end(), keyCount - tempFloatArrays[i].size(), 0.0f);
		}

		// Create all the keys, on the curves, according to the interpolation types.
		for (size_t j = 0; j < keyCount; ++j)
		{
			FCDAnimationKey* key = animationChannel->GetCurve(i)->AddKey((FUDaeInterpolation::Interpolation) interpolations[j]);
			key->input = inputs[j];
			key->output = tempFloatArrays[i][j];

			// Set the default values for Bezier/TCB interpolations.
			if (key->interpolation == FUDaeInterpolation::BEZIER)
			{
				FCDAnimationKeyBezier* bkey = (FCDAnimationKeyBezier*) key;
				float previousInput = (j == 0) ? inputs[j] - 1.0f : inputs[j-1];
				float nextInput = (j == keyCount - 1) ? inputs[j] + 1.0f : inputs[j+1];
				bkey->inTangent.x = (previousInput + 2.0f * bkey->input) / 3.0f;
				bkey->outTangent.x = (nextInput + 2.0f * bkey->input) / 3.0f;
				bkey->inTangent.y = bkey->outTangent.y = bkey->output;
			}
			else if (key->interpolation == FUDaeInterpolation::TCB)
			{
				FCDAnimationKeyTCB* tkey = (FCDAnimationKeyTCB*) key;
				tkey->tension = tkey->continuity = tkey->bias = 0.5f;
				tkey->easeIn = tkey->easeOut = 0.0f;
			}
		}
	}
	tempFloatArrays.clear();

	// Read in the interleaved in_tangent source.
	if (inTangentSource != NULL)
	{
		fm::vector<FMVector2List> tempVector2Arrays;
		tempVector2Arrays.resize(curveCount);
		fm::pvector<FMVector2List> arrays(curveCount);
		for (uint32 i = 0; i < curveCount; ++i) arrays[i] = &tempVector2Arrays[i];

		uint32 stride = ReadSourceInterleaved(inTangentSource, arrays);
		if (stride == curveCount)
		{
			// Backward compatibility with 1D tangents.
			// Remove the relativity from the 1D tangents and calculate the second-dimension.
			for (uint32 i = 0; i < curveCount; ++i)
			{
				FMVector2List& inTangents = tempVector2Arrays[i];
				FCDAnimationKey** keys = animationChannel->GetCurve(i)->GetKeys();
				size_t end = min(inTangents.size(), keyCount);
				for (size_t j = 0; j < end; ++j)
				{
					if (keys[j]->interpolation == FUDaeInterpolation::BEZIER)
					{
						FCDAnimationKeyBezier* bkey = (FCDAnimationKeyBezier*) keys[j];
						bkey->inTangent.y = bkey->output - inTangents[j].x;
					}
				}
			}
		}
		else if (stride == curveCount * 2)
		{
			// This is the typical, 2D tangent case.
			for (uint32 i = 0; i < curveCount; ++i)
			{
				FMVector2List& inTangents = tempVector2Arrays[i];
				FCDAnimationKey** keys = animationChannel->GetCurve(i)->GetKeys();
				size_t end = min(inTangents.size(), keyCount);
				for (size_t j = 0; j < end; ++j)
				{
					if (keys[j]->interpolation == FUDaeInterpolation::BEZIER)
					{
						FCDAnimationKeyBezier* bkey = (FCDAnimationKeyBezier*) keys[j];
						bkey->inTangent = inTangents[j];
					}
				}
			}
		}
	}

	// Read in the interleaved out_tangent source.
	if (outTangentSource != NULL)
	{
		fm::vector<FMVector2List> tempVector2Arrays;
		tempVector2Arrays.resize(curveCount);
		fm::pvector<FMVector2List> arrays(curveCount);
		for (uint32 i = 0; i < curveCount; ++i) arrays[i] = &tempVector2Arrays[i];

		uint32 stride = ReadSourceInterleaved(outTangentSource, arrays);
		if (stride == curveCount)
		{
			// Backward compatibility with 1D tangents.
			// Remove the relativity from the 1D tangents and calculate the second-dimension.
			for (uint32 i = 0; i < curveCount; ++i)
			{
				FMVector2List& outTangents = tempVector2Arrays[i];
				FCDAnimationKey** keys = animationChannel->GetCurve(i)->GetKeys();
				size_t end = min(outTangents.size(), keyCount);
				for (size_t j = 0; j < end; ++j)
				{
					if (keys[j]->interpolation == FUDaeInterpolation::BEZIER)
					{
						FCDAnimationKeyBezier* bkey = (FCDAnimationKeyBezier*) keys[j];
						bkey->outTangent.y = bkey->output + outTangents[j].x;
					}
				}
			}
		}
		else if (stride == curveCount * 2)
		{
			// This is the typical, 2D tangent case.
			for (uint32 i = 0; i < curveCount; ++i)
			{
				FMVector2List& outTangents = tempVector2Arrays[i];
				FCDAnimationKey** keys = animationChannel->GetCurve(i)->GetKeys();
				size_t end = min(outTangents.size(), keyCount);
				for (size_t j = 0; j < end; ++j)
				{
					if (keys[j]->interpolation == FUDaeInterpolation::BEZIER)
					{
						FCDAnimationKeyBezier* bkey = (FCDAnimationKeyBezier*) keys[j];
						bkey->outTangent = outTangents[j];
					}
				}
			}
		}
	}

	if (tcbSource != NULL)
	{
		//Process TCB parameters
		fm::vector<FMVector3List> tempVector3Arrays;
		tempVector3Arrays.resize(curveCount);
		fm::pvector<FMVector3List> arrays(curveCount);
		for (uint32 i = 0; i < curveCount; ++i) arrays[i] = &tempVector3Arrays[i];

		ReadSourceInterleaved(tcbSource, arrays);

		for (uint32 i = 0; i < curveCount; ++i)
		{
			FMVector3List& tcbs = tempVector3Arrays[i];
			FCDAnimationKey** keys = animationChannel->GetCurve(i)->GetKeys();
			size_t end = min(tcbs.size(), keyCount);
			for (size_t j = 0; j < end; ++j)
			{
				if (keys[j]->interpolation == FUDaeInterpolation::TCB)
				{
					FCDAnimationKeyTCB* tkey = (FCDAnimationKeyTCB*) keys[j];
					tkey->tension = tcbs[j].x;
					tkey->continuity = tcbs[j].y;
					tkey->bias = tcbs[j].z;
				}
			}
		}
	}

	if (easeSource != NULL)
	{
		//Process Ease-in and ease-out data
		fm::vector<FMVector2List> tempVector2Arrays;
		tempVector2Arrays.resize(curveCount);
		fm::pvector<FMVector2List> arrays(curveCount);
		for (uint32 i = 0; i < curveCount; ++i) arrays[i] = &tempVector2Arrays[i];

		ReadSourceInterleaved(easeSource, arrays);

		for (uint32 i = 0; i < curveCount; ++i)
		{
			FMVector2List& eases = tempVector2Arrays[i];
			FCDAnimationKey** keys = animationChannel->GetCurve(i)->GetKeys();
			size_t end = min(eases.size(), keyCount);
			for (size_t j = 0; j < end; ++j)
			{
				if (keys[j]->interpolation == FUDaeInterpolation::TCB)
				{
					FCDAnimationKeyTCB* tkey = (FCDAnimationKeyTCB*) keys[j];
					tkey->easeIn = eases[j].x;
					tkey->easeOut = eases[j].y;
				}
			}
		}
	}

	// Read in the pre/post-infinity type
	xmlNodeList mayaParameterNodes; StringList mayaParameterNames;
	xmlNode* mayaTechnique = FindTechnique(inputSource, DAEMAYA_MAYA_PROFILE);
	FindParameters(mayaTechnique, mayaParameterNames, mayaParameterNodes);
	size_t parameterCount = mayaParameterNodes.size();
	for (size_t i = 0; i < parameterCount; ++i)
	{
		xmlNode* parameterNode = mayaParameterNodes[i];
		const fm::string& paramName = mayaParameterNames[i];
		const char* content = ReadNodeContentDirect(parameterNode);

		if (paramName == DAEMAYA_PREINFINITY_PARAMETER)
		{
			size_t curveCount = animationChannel->GetCurveCount();
			for (size_t c = 0; c < curveCount; ++c)
			{
				animationChannel->GetCurve(c)->SetPreInfinity(FUDaeInfinity::FromString(content));
			}
		}
		else if (paramName == DAEMAYA_POSTINFINITY_PARAMETER)
		{
			size_t curveCount = animationChannel->GetCurveCount();
			for (size_t c = 0; c < curveCount; ++c)
			{
				animationChannel->GetCurve(c)->SetPostInfinity(FUDaeInfinity::FromString(content));
			}
		}
		else
		{
			// Look for driven-key input target
			if (paramName == DAE_INPUT_ELEMENT)
			{
				fm::string semantic = ReadNodeSemantic(parameterNode);
				if (semantic == DAEMAYA_DRIVER_INPUT)
				{
					inputDriver = ReadNodeSource(parameterNode);
				}
			}
		}
	}

	if (!inputDriver.empty())
	{
		const char* driverTarget = FUDaeParser::SkipPound(inputDriver);
		if (driverTarget != NULL)
		{
			fm::string driverQualifierValue;
			FUStringConversion::SplitTarget(driverTarget, data.driverPointer, driverQualifierValue);
			data.driverQualifier = FUStringConversion::ParseQualifier(driverQualifierValue);
			if (data.driverQualifier < 0) data.driverQualifier = 0;
		}
	}
	animationChannel->SetDirtyFlag();

	return status;
}


xmlNode* FArchiveXML::FindChildByIdFCDAnimation(FCDAnimation* animation, const fm::string& _id)
{
	FCDAnimationDataMap::iterator animationIt = FArchiveXML::documentLinkDataMap[animation->GetDocument()].animationData.find(animation);
	FUAssert(animationIt != FArchiveXML::documentLinkDataMap[animation->GetDocument()].animationData.end(),);
	FCDAnimationData& data = animationIt->second;

	FUCrc32::crc32 id = FUCrc32::CRC32(_id.c_str() + ((_id[0] == '#') ? 1 : 0));
	for (FAXNodeIdPair* it = data.childNodes.begin(); it != data.childNodes.end(); ++it)
	{
		if (it->second == id) return it->first;
	}

	return (animation->GetParent() != NULL) ? FArchiveXML::FindChildByIdFCDAnimation(animation->GetParent(), _id) : NULL;
}

bool FArchiveXML::LoadAnimationCurve(FCDObject* UNUSED(object), xmlNode* UNUSED(node))
{ 
	//
	// FCDAnimationCurve doesn't correspond to any COLLADA element. It's used internally to
	// encapsulate the animation data.
	//
	FUBreak;
	return false;
}

bool FArchiveXML::LoadAnimationMultiCurve(FCDObject* UNUSED(object), xmlNode* UNUSED(node))
{ 
	//
	// FCDAnimationMultiCurve doesn't correspond to any COLLADA element. It's used internally to
	// encapsulate multiple animation data.
	//
	FUBreak;
	return false;
}

bool FArchiveXML::LoadAnimation(FCDObject* object, xmlNode* node)
{ 
	FCDAnimation* animation = (FCDAnimation*)object;
	FCDAnimationData& data = FArchiveXML::documentLinkDataMap[animation->GetDocument()].animationData[animation];

	bool status = FArchiveXML::LoadEntity(animation, node);
	if (!status) return status;
	if (!IsEquivalent(node->name, DAE_ANIMATION_ELEMENT))
	{
		return FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_ANIM_LIB, node->line);
	}

	// Optimization: Grab all the IDs of the child nodes, in CRC format.
	ReadChildrenIds(node, data.childNodes);

	// Parse all the inner <channel> elements
	xmlNodeList channelNodes;
	FindChildrenByType(node, DAE_CHANNEL_ELEMENT, channelNodes);
	for (xmlNodeList::iterator itC = channelNodes.begin(); itC != channelNodes.end(); ++itC)
	{
		// Parse each <channel> element individually
		// They each handle reading the <sampler> and <source> elements
		FCDAnimationChannel* channel = animation->AddChannel();
		status &= (FArchiveXML::LoadAnimationChannel(channel, *itC));
		if (!status) SAFE_RELEASE(channel);
	}

	// Parse all the hierarchical <animation> elements
	xmlNodeList animationNodes;
	FindChildrenByType(node, DAE_ANIMATION_ELEMENT, animationNodes);
	for (xmlNodeList::iterator itA = animationNodes.begin(); itA != animationNodes.end(); ++itA)
	{
		FArchiveXML::LoadAnimation(animation->AddChild(), *itA);
	}
	return status;
}			

bool FArchiveXML::LoadAnimationClip(FCDObject* object, xmlNode* clipNode)
{ 
	FCDAnimationClip* animationClip = (FCDAnimationClip*)object;

	bool status = FArchiveXML::LoadEntity(animationClip, clipNode);
	if (!status) return status;
	if (!IsEquivalent(clipNode->name, DAE_ANIMCLIP_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_ANIM_LIB_ELEMENT, clipNode->line);
		return status;
	}

	// Read in and verify the clip's time/input bounds
	animationClip->SetStart(FUStringConversion::ToFloat(ReadNodeProperty(clipNode, DAE_START_ATTRIBUTE)));
	animationClip->SetEnd(FUStringConversion::ToFloat(ReadNodeProperty(clipNode, DAE_END_ATTRIBUTE)));
	if (animationClip->GetEnd() - animationClip->GetStart() < FLT_TOLERANCE)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_SE_PAIR, clipNode->line);
	}

	// Read in the <input> elements and segment the corresponding animation curves
	xmlNodeList inputNodes;
	FindChildrenByType(clipNode, DAE_INSTANCE_ANIMATION_ELEMENT, inputNodes);
	for (xmlNodeList::iterator itI = inputNodes.begin(); itI != inputNodes.end(); ++itI)
	{
		FCDEntityInstance* animationInstance = animationClip->AddInstanceAnimation();
		if (!LoadSwitch(animationInstance, &animationInstance->GetObjectType(), *itI))
		{
			SAFE_DELETE(animationInstance);
			continue;
		}

		fm::string name = ReadNodeProperty(*itI, DAE_NAME_ATTRIBUTE);
		animationClip->SetAnimationName(name, animationClip->GetAnimationCount() - 1);
	}

	// Check for an empty clip
	if (animationClip->GetClipCurves().empty())
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_EMPTY_ANIM_CLIP, clipNode->line);
	}

	animationClip->SetDirtyFlag();
	return status;
}		

void FArchiveXML::LoadAnimatable(FCDParameterAnimatable* animatable, xmlNode* node)
{
	if (animatable == NULL || node == NULL) return;
	FCDAnimated* animated = animatable->GetAnimated();
	if (!FArchiveXML::LinkAnimated(animated, node)) SAFE_RELEASE(animated);
}

void FArchiveXML::LoadAnimatable(FCDocument* document, FCDParameterListAnimatable* animatable, xmlNode* node)
{
	if (animatable == NULL || node == NULL) return;

	// Look for an animation on this list object
	Int32List animatedIndices;
	FArchiveXML::FindAnimationChannelsArrayIndices(document, node, animatedIndices);
	for (Int32List::iterator it = animatedIndices.begin(); it != animatedIndices.end(); ++it)
	{
		// Check for repeated animated indices
		if (it > animatedIndices.find(*it)) continue;

		FCDAnimated* animated = animatable->GetAnimated(*it);
		if (!FArchiveXML::LinkAnimated(animated, node)) SAFE_RELEASE(animated);
	}
}

bool FArchiveXML::ProcessChannels(FCDAnimated* animated, FCDAnimationChannelList& channels)
{
	bool linked = false;

	StringList& qualifiers = animated->GetQualifiers();
	for (FCDAnimationChannelList::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		FCDAnimationChannel* channel = *it;
		size_t curveCount = channel->GetCurveCount();
		if (curveCount == 0) continue;
		
		// Retrieve the channel's qualifier and check for a requested matrix element
		FCDAnimationChannelDataMap::iterator itChannelData = FArchiveXML::documentLinkDataMap[channel->GetDocument()].animationChannelData.find(channel);
		FUAssert(itChannelData != FArchiveXML::documentLinkDataMap[channel->GetDocument()].animationChannelData.end(),);
		FCDAnimationChannelData& channelData = itChannelData->second;

		const fm::string& qualifier = channelData.targetQualifier;
		if (!qualifier.empty())
		{
			// Qualifed curves can only target a single element?
			FUAssert(curveCount == 1,);
			int32 element = -1;
			// If the animated is part of a list (ie, morph target weight, not a transform etc)
			if (animated->GetArrayElement() != -1)
			{
				// By definition, if an animated has an array element, it can only
				// animate a single value.
				element = FUStringConversion::ParseQualifier(qualifier);
				if (animated->GetArrayElement() != element) continue;
				else 
				{
					linked = animated->AddCurve(0, channel->GetCurve(0));
				}
			}
			else
			{
				// Attempt to match the qualifier with this animated qualifiers
				size_t index;
				for (index = 0; index < qualifiers.size(); ++index)
				{
					if (qualifiers[index] == qualifier) break;
				}


				// Check for bracket notation eg -(X)- instead
				if (index == qualifiers.size()) index = FUStringConversion::ParseQualifier(qualifier);
				if (index < qualifiers.size())
				{
					linked = animated->AddCurve(index, channel->GetCurve(0));
				}
				else 
				{
					// Attempt to match with some of the standard qualifiers instead.
					size_t checkCount = min((size_t) 4u, qualifiers.size());
					for (index = 0; index < checkCount; ++index)
					{
						if (IsEquivalent(qualifier, FCDAnimatedStandardQualifiers::XYZW[index])) break;
						else if (IsEquivalent(qualifier, FCDAnimatedStandardQualifiers::RGBA[index])) break;
					}
					if (index < checkCount)
					{
						linked = animated->AddCurve(index, channel->GetCurve(0));
						WARNING_OUT("Invalid qualfiier for animation channel target: %s - %s. Using non-standard match.", channelData.targetPointer.c_str(), qualifier.c_str());
					}
					else
					{
						const char* temp1 = channelData.targetPointer.c_str();
						const char* temp2 = qualifier.c_str();
						ERROR_OUT("Invalid qualifier for animation channel target: %s - %s", temp1, temp2);
					}
				}
				//else return status.Fail(FS("Invalid qualifier for animation channel target: ") + TO_FSTRING(pointer));

			}
		}
		else
		{
			// An empty qualifier implies that the channel should provide ALL the curves
			size_t nCurves = min(curveCount, qualifiers.size());
			for (size_t i = 0; i < nCurves; ++i)
			{
				animated->AddCurve(i, channel->GetCurve(i));
				linked = true;
			}
		}
	}

	if (linked)
	{
		// Now that the curves are imported: set their target information
		for (size_t i = 0; i < animated->GetCurves().size(); ++i)
		{
			for (size_t j = 0; j < animated->GetCurves()[i].size(); ++j)
			{
				FCDAnimationCurveData& curveData = FArchiveXML::documentLinkDataMap[animated->GetCurves()[i][j]->GetDocument()].animationCurveData[animated->GetCurves()[i][j]];

				curveData.targetElement = animated->GetArrayElement();
				curveData.targetQualifier = qualifiers[i];
			}
		}
	}

	animated->SetDirtyFlag();
	return linked;
}

void FArchiveXML::FindAnimationChannels(FCDocument* fcdocument, const fm::string& pointer, FCDAnimationChannelList& channels)
{
	if (pointer.empty()) return;

	size_t animationCount = (uint32) fcdocument->GetAnimationLibrary()->GetEntityCount();
	for (size_t i = 0; i < animationCount; ++i)
	{
		FCDAnimation* animation = fcdocument->GetAnimationLibrary()->GetEntity(i);
		FArchiveXML::FindAnimationChannels(animation, pointer, channels);
	}
}

void FArchiveXML::FindAnimationChannels(FCDAnimation* animation, const fm::string& pointer, FCDAnimationChannelList& targetChannels)
{
	// Look for channels locally
	for (size_t i = 0; i < animation->GetChannelCount(); ++i)
	{
		FCDAnimationChannelDataMap::iterator itChannelData = FArchiveXML::documentLinkDataMap[animation->GetChannel(i)->GetDocument()].animationChannelData.find(animation->GetChannel(i));
		FUAssert(itChannelData != FArchiveXML::documentLinkDataMap[animation->GetChannel(i)->GetDocument()].animationChannelData.end(),);
		FCDAnimationChannelData& channelData = itChannelData->second;

		if (channelData.targetPointer == pointer)
		{
			targetChannels.push_back(animation->GetChannel(i));
		}
	}

	// Look for channel(s) within the child animations
	for (size_t i = 0; i < animation->GetChildrenCount(); ++i)
	{
		FArchiveXML::FindAnimationChannels(animation->GetChild(i), pointer, targetChannels);
	}
}
