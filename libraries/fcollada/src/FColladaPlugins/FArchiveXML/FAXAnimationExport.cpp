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
#include "FCDocument/FCDAnimationCurveTools.h"

xmlNode* FArchiveXML::WriteAnimationChannel(FCDObject* object, xmlNode* parentNode)
{
	FCDAnimationChannel* animationChannel = (FCDAnimationChannel*)object;

	FCDAnimationChannelData& data = FArchiveXML::documentLinkDataMap[animationChannel->GetDocument()].animationChannelData[animationChannel];
	//FUAssert(!data.targetPointer.empty(), NULL);
	fm::string baseId = FCDObjectWithId::CleanId(animationChannel->GetParent()->GetDaeId() + "_" + data.targetPointer);

	// Check for curve merging
	uint32 realCurveCount = 0;
	const FCDAnimationCurve* masterCurve = NULL;
	FCDAnimationCurveList mergingCurves;
	mergingCurves.resize(data.defaultValues.size());
	bool mergeCurves = true;
	size_t curveCount = animationChannel->GetCurveCount();
	for (size_t i = 0; i < curveCount; ++i)
	{
		const FCDAnimationCurve* curve = animationChannel->GetCurve(i);
		if (curve != NULL)
		{
			// Check that we have a default placement for this curve in the default value listing
			size_t dv;
			for (dv = 0; dv < data.defaultValues.size(); ++dv)
			{
				if (data.defaultValues[dv].curve == curve)
				{
					mergingCurves[dv] = const_cast<FCDAnimationCurve*>(curve);
					break;
				}
			}
			mergeCurves &= dv != data.defaultValues.size();

			// Check that the curves can be merged correctly.
			++realCurveCount;
			if (masterCurve == NULL)
			{
				masterCurve = curve;
			}
			else
			{
				// Check the infinity types, the keys and the interpolations.
				size_t curveKeyCount = curve->GetKeyCount();
				size_t masterKeyCount = masterCurve->GetKeyCount();
				mergeCurves &= masterKeyCount == curveKeyCount;
				if (!mergeCurves) break;

				for (size_t j = 0; j < curveKeyCount && mergeCurves; ++j)
				{
					const FCDAnimationKey* curveKey = curve->GetKey(j);
					const FCDAnimationKey* masterKey = masterCurve->GetKey(j);
					mergeCurves &= IsEquivalent(curveKey->input, masterKey->input);
					mergeCurves &= curveKey->interpolation == masterKey->interpolation;

					// Prevent curve having TCB interpolation from merging
					mergeCurves &= curveKey->interpolation != FUDaeInterpolation::TCB;
					mergeCurves &= masterKey->interpolation != FUDaeInterpolation::TCB;
				}
				if (!mergeCurves) break;

				mergeCurves &= curve->GetPostInfinity() == masterCurve->GetPostInfinity();
				mergeCurves &= curve->GetPreInfinity() == masterCurve->GetPreInfinity();
			}

			// Disallow the merging of any curves with a driver.
			mergeCurves &= !curve->HasDriver();
		}
	}

	if (mergeCurves && realCurveCount > 1)
	{
		// Prepare the list of default values
		FloatList values;
		values.reserve(data.defaultValues.size());
		for (FAXAnimationChannelDefaultValueList::iterator itDV = data.defaultValues.begin(); itDV != data.defaultValues.end(); ++itDV)
		{
			values.push_back((*itDV).defaultValue);
		}

		FUAssert(data.animatedValue != NULL, return parentNode);
		const char** qualifiers = new const char*[values.size()];
		memset(qualifiers, 0, sizeof(char*) * values.size());
		for (size_t i = 0; i < values.size() && i < data.animatedValue->GetValueCount(); ++i)
		{
			qualifiers[i] = data.animatedValue->GetQualifier(i);
		}

		// Merge and export the curves
		FCDAnimationMultiCurve* multiCurve = FCDAnimationCurveTools::MergeCurves(mergingCurves, values);
		FArchiveXML::WriteSourceFCDAnimationMultiCurve(multiCurve, parentNode, qualifiers, baseId);
		FArchiveXML::WriteSamplerFCDAnimationMultiCurve(multiCurve, parentNode, baseId);
		FArchiveXML::WriteChannelFCDAnimationMultiCurve(multiCurve, parentNode, baseId, data.targetPointer);
		SAFE_RELEASE(multiCurve);
	}
	else
	{
		// Interlace the curve's sources, samplers and channels
		// Generate new ids for each of the curve's data sources, to avoid collision in special cases
		size_t curveCount = animationChannel->GetCurveCount();
		StringList ids; ids.resize(curveCount);
		FUSStringBuilder curveId;
		for (size_t c = 0; c < curveCount; ++c)
		{
			FCDAnimationCurve* curCurve = animationChannel->GetCurve(c);
			if (curCurve != NULL)
			{
				FCDAnimationCurveData& curveData = FArchiveXML::documentLinkDataMap[curCurve->GetDocument()].animationCurveData[curCurve];
				//FUAssert(curveDataIt != FArchiveXML::animationCurveData.end(), NULL);

				// Generate a valid id for this curve
				curveId.set(baseId);
				if (curveData.targetElement >= 0)
				{
					curveId.append('_'); curveId.append(curveData.targetElement); curveId.append('_');
				}
				curveId.append(curveData.targetQualifier);
				ids[c] = FCDObjectWithId::CleanId(curveId.ToCharPtr());

				// Write out the curve's sources
				FArchiveXML::WriteSourceFCDAnimationCurve(animationChannel->GetCurve(c), parentNode, ids[c]);
			}
		}
		for (size_t c = 0; c < curveCount; ++c)
		{
			if (animationChannel->GetCurve(c) != NULL) FArchiveXML::WriteSamplerFCDAnimationCurve(animationChannel->GetCurve(c), parentNode, ids[c]);
		}
		for (size_t c = 0; c < curveCount; ++c)
		{
			if (animationChannel->GetCurve(c) != NULL) FArchiveXML::WriteChannelFCDAnimationCurve(animationChannel->GetCurve(c), parentNode, ids[c], data.targetPointer.c_str());
		}
	}

	data.defaultValues.clear();
	return parentNode;
	
}

xmlNode* FArchiveXML::WriteAnimationCurve(FCDObject* UNUSED(object), xmlNode* UNUSED(parentNode))
{
	//
	// Currently not reachable
	//
	FUBreak;
	return NULL;
}

xmlNode* FArchiveXML::WriteAnimationMultiCurve(FCDObject* UNUSED(object), xmlNode* UNUSED(parentNode))
{
	//
	// Currently not reachable
	//
	FUBreak;
	return NULL;
}

xmlNode* FArchiveXML::WriteAnimation(FCDObject* object, xmlNode* parentNode)
{
	FCDAnimation* animation = (FCDAnimation*)object;

	xmlNode* animationNode = FArchiveXML::WriteToEntityXMLFCDEntity(animation, parentNode, DAE_ANIMATION_ELEMENT);

	// Write out the local channels
	size_t channelCount = animation->GetChannelCount();
	for (size_t i = 0; i < channelCount; ++i)
	{
		FArchiveXML::LetWriteObject(animation->GetChannel(i), animationNode);
	}

	// Write out the child animations
	size_t childCount = animation->GetChildrenCount();
	for (size_t i = 0; i < childCount; ++i)
	{
		FArchiveXML::LetWriteObject(animation->GetChild(i), animationNode);
	}

	FArchiveXML::WriteEntityExtra(animation, animationNode);
	return animationNode;
}

xmlNode* FArchiveXML::WriteAnimationClip(FCDObject* object, xmlNode* parentNode)
{
	FCDAnimationClip* animationClip = (FCDAnimationClip*)object;

	// Create the <clip> element and write out its start/end information.
	xmlNode* clipNode = FArchiveXML::WriteToEntityXMLFCDEntity(animationClip, parentNode, DAE_ANIMCLIP_ELEMENT);
	AddAttribute(clipNode, DAE_START_ATTRIBUTE, animationClip->GetStart());
	AddAttribute(clipNode, DAE_END_ATTRIBUTE, animationClip->GetEnd());

	// Build a list of the animations to instantiate
	// from the list of curves for this clip
	typedef fm::pvector<const FCDAnimation> FCDAnimationConstList;
	FCDAnimationConstList animations;
	for (FCDAnimationCurveTrackList::iterator itC = animationClip->GetClipCurves().begin(); itC != animationClip->GetClipCurves().end(); ++itC)
	{
		const FCDAnimationChannel* channel = (*itC)->GetParent();
		if (channel == NULL) continue;
		const FCDAnimation* animation = channel->GetParent();
		if (animations.find(animation) == animations.end())
		{
			animations.push_back(animation);
		}
	}

	// Instantiate all the animations
	for (FCDAnimationConstList::iterator itA = animations.begin(); itA != animations.end(); ++itA)
	{
		xmlNode* instanceNode = AddChild(clipNode, DAE_INSTANCE_ANIMATION_ELEMENT);
		AddAttribute(instanceNode, DAE_URL_ATTRIBUTE, fm::string("#") + (*itA)->GetDaeId());
	}

	FArchiveXML::WriteEntityExtra(animationClip, clipNode);
	return clipNode;
}

// Writes out a value's animations, if any, to the animation library of a COLLADA XML document.
bool FArchiveXML::WriteAnimatedValue(const FCDParameterAnimatable* value, xmlNode* valueNode, const char* wantedSid, int32 arrayElement)
{
	if (value->IsAnimated() && valueNode != NULL)
	{
		// Find the value's animations
		FCDAnimated* animated = const_cast<FCDAnimated*>(value->GetAnimated());
		if (!animated->HasCurve()) return false;
		animated->SetArrayElement(arrayElement);
		FArchiveXML::WriteAnimatedValue(animated, valueNode, wantedSid);
		return true;
	}
	return false;
}

void FArchiveXML::WriteAnimatedValue(const FCDAnimated* _animated, xmlNode* valueNode, const char* wantedSid)
{
	FCDAnimated* animated = const_cast<FCDAnimated*>(_animated);
	int32 arrayElement = animated->GetArrayElement();

	FCDAnimatedData& animatedData = FArchiveXML::documentLinkDataMap[animated->GetDocument()].animatedData[animated];

	// Set a sid unto the XML tree node, in order to support animations
	if (!HasNodeProperty(valueNode, DAE_SID_ATTRIBUTE) && !HasNodeProperty(valueNode, DAE_ID_ATTRIBUTE))
	{
		AddNodeSid(valueNode, wantedSid);
	}

	// Calculate the XML tree node's target for the animation channel
	CalculateNodeTargetPointer(valueNode, animatedData.pointer);

	if (!animatedData.pointer.empty())
	{
		FCDAnimationChannelList channels;

		// Enforce the target pointer on all the curves
		for (size_t i = 0; i < animated->GetValueCount(); ++i)
		{
			FCDAnimationCurveTrackList& curves = animated->GetCurves()[i];
			for (FCDAnimationCurveTrackList::iterator itC = curves.begin(); itC != curves.end(); ++itC)
			{
				FCDAnimationCurveData& curveData = FArchiveXML::documentLinkDataMap[(*itC)->GetDocument()].animationCurveData[*itC];

				(*itC)->SetTargetElement(arrayElement);
				(*itC)->SetTargetQualifier(animated->GetQualifier(i));
				curveData.targetElement = arrayElement;
				curveData.targetQualifier = animated->GetQualifier(i);

				FCDAnimationChannel* channel = (*itC)->GetParent();
				FCDAnimationChannelData& channelData = FArchiveXML::documentLinkDataMap[channel->GetDocument()].animationChannelData[channel];

				FUAssert(channel != NULL, continue);

				channelData.targetPointer = animatedData.pointer;
				channelData.animatedValue = animated;

				if (!channels.contains(channel)) channels.push_back(channel);
			}
		}

		// Enforce the default values on the channels. This is used for curve merging.
		for (FCDAnimationChannelList::iterator itC = channels.begin(); itC != channels.end(); ++itC)
		{
			FCDAnimationChannel* channel = (*itC);
			FCDAnimationChannelData& channelData = FArchiveXML::documentLinkDataMap[channel->GetDocument()].animationChannelData[channel];

			for (size_t i = 0; i < animated->GetValueCount(); ++i)
			{
				FCDAnimationCurveTrackList& curves = animated->GetCurves()[i];

				// Find the curve, if any, that comes from this channel.
				FCDAnimationCurve* curve = NULL;
				for (size_t j = 0; j < curves.size() && curve == NULL; ++j)
				{
					if (curves[j]->GetParent() == channel)
					{
						curve = curves[j];
						break;
					}
				}

				float defaultValue = !animated->GetRelativeAnimationFlag() ? *animated->GetValue(i) : 0.0f;
				channelData.defaultValues.push_back(FAXAnimationChannelDefaultValue(curve, defaultValue));
			}
		}
	}
}

void FArchiveXML::WriteSourceFCDAnimationCurve(FCDAnimationCurve* animationCurve, xmlNode* parentNode, const fm::string& baseId)
{
	FCDAnimationCurveDataMap::iterator it = FArchiveXML::documentLinkDataMap[animationCurve->GetDocument()].animationCurveData.find(animationCurve);
	FUAssert(it != FArchiveXML::documentLinkDataMap[animationCurve->GetDocument()].animationCurveData.end(),);
	FCDAnimationCurveData& data = it->second;

	const char* parameter = data.targetQualifier.c_str();
	if (*parameter == '.') ++parameter;
	
	// Retrieve the source information data
	bool hasTangents = false, hasTCB = false;
	for (size_t i  = 0; i < animationCurve->GetKeyCount(); ++i)
	{
		hasTangents |= animationCurve->GetKey(i)->interpolation == FUDaeInterpolation::BEZIER;
		hasTCB |= animationCurve->GetKey(i)->interpolation == FUDaeInterpolation::TCB;
	}

	// Prebuffer the source information lists
	size_t keyCount = animationCurve->GetKeyCount();
	FloatList inputs; inputs.reserve(keyCount);
	FloatList outputs; outputs.reserve(keyCount);
	UInt32List interpolations; interpolations.reserve(keyCount);
	FMVector2List inTangents; if (hasTangents) inTangents.reserve(keyCount);
	FMVector2List outTangents; if (hasTangents) outTangents.reserve(keyCount);
	FMVector3List tcbs; if (hasTCB) tcbs.reserve(keyCount);
	FMVector2List eases; if (hasTCB) eases.reserve(keyCount);

	// Retrieve the source information data
	for (size_t i  = 0; i < animationCurve->GetKeyCount(); ++i)
	{
		FCDAnimationKey* key = animationCurve->GetKey(i);
		inputs.push_back(key->input);
		outputs.push_back(key->output);
		if (hasTangents)
		{
			if (key->interpolation == FUDaeInterpolation::BEZIER)
			{
				FCDAnimationKeyBezier* bkey = (FCDAnimationKeyBezier*)key;
				if (inTangents.size() < interpolations.size())
				{
					// Grow to the correct index
					inTangents.resize(interpolations.size(), FMVector2::Zero);
					outTangents.resize(interpolations.size(), FMVector2::Zero);
				}
				inTangents.push_back(bkey->inTangent);
				outTangents.push_back(bkey->outTangent);
			}
			else
			{
				// Export flat tangents
				inTangents.push_back(FMVector2(key->input - 0.0001f, key->output));
				outTangents.push_back(FMVector2(key->input + 0.0001f, key->output));
			}
		}
		if (hasTCB)
		{
			if (key->interpolation == FUDaeInterpolation::TCB)
			{
				FCDAnimationKeyTCB* tkey = (FCDAnimationKeyTCB*)key;
				if (tcbs.size() < interpolations.size())
				{
					// Grow to the correct index
					tcbs.resize(interpolations.size(), FMVector3::Zero);
					eases.resize(interpolations.size(), FMVector2::Zero);
				}
				tcbs.push_back(FMVector3(tkey->tension, tkey->continuity, tkey->bias));
				eases.push_back(FMVector2(tkey->easeIn, tkey->easeOut));
			}
			else
			{
				// Export flat tangents
				tcbs.push_back(FMVector3(0.5f, 0.5f, 0.5f));
				eases.push_back(FMVector2::Zero);
			}
		}

		// Append the interpolation type last, because we use this list's size to correctly
		// index TCB and Bezier information when exporting mixed-interpolation curves.
		interpolations.push_back(key->interpolation);
	}

	// Export the data arrays.
	xmlNode* sourceNode = AddSourceFloat(parentNode, baseId + "-input", inputs, "TIME");
	AddSourceFloat(parentNode, baseId + "-output", outputs, parameter);
	AddSourceInterpolation(parentNode, baseId + "-interpolations", *(FUDaeInterpolationList*)(size_t)&interpolations);
	if (!inTangents.empty() && !outTangents.empty())
	{
		AddSourceTangent(parentNode, baseId + "-intangents", inTangents);
		AddSourceTangent(parentNode, baseId + "-outtangents", outTangents);
	}
	if (!tcbs.empty() && !eases.empty())
	{
		AddSourceFloat(parentNode, baseId + "-tcbs", tcbs);
		AddSourceFloat(parentNode, baseId + "-eases", eases);
	}

	// Export the infinity parameters
	xmlNode* mayaTechnique = AddTechniqueChild(sourceNode, DAEMAYA_MAYA_PROFILE);
	fm::string infinityType = FUDaeInfinity::ToString(animationCurve->GetPreInfinity());
	AddChild(mayaTechnique, DAEMAYA_PREINFINITY_PARAMETER, infinityType);
	infinityType = FUDaeInfinity::ToString(animationCurve->GetPostInfinity());
	AddChild(mayaTechnique, DAEMAYA_POSTINFINITY_PARAMETER, infinityType);
}

xmlNode* FArchiveXML::WriteSamplerFCDAnimationCurve(FCDAnimationCurve* animationCurve, xmlNode* parentNode, const fm::string& baseId)
{
	xmlNode* samplerNode = AddChild(parentNode, DAE_SAMPLER_ELEMENT);
	AddAttribute(samplerNode, DAE_ID_ATTRIBUTE, baseId + "-sampler");

	// Retrieve the source information data
	bool hasTangents = false, hasTCB = false;
	for (size_t i = 0; i < animationCurve->GetKeyCount(); ++i)
	{
		hasTangents |= animationCurve->GetKey(i)->interpolation == FUDaeInterpolation::BEZIER;
		hasTCB |= animationCurve->GetKey(i)->interpolation == FUDaeInterpolation::TCB;
	}

	// Add the sampler inputs
	AddInput(samplerNode, baseId + "-input", DAE_INPUT_ANIMATION_INPUT);
	AddInput(samplerNode, baseId + "-output", DAE_OUTPUT_ANIMATION_INPUT);
	AddInput(samplerNode, baseId + "-interpolations", DAE_INTERPOLATION_ANIMATION_INPUT);
	if (hasTangents)
	{
		AddInput(samplerNode, baseId + "-intangents", DAE_INTANGENT_ANIMATION_INPUT);
		AddInput(samplerNode, baseId + "-outtangents", DAE_OUTTANGENT_ANIMATION_INPUT);
	}
	if (hasTCB)
	{
		AddInput(samplerNode, baseId + "-tcbs", DAEFC_TCB_ANIMATION_INPUT);
		AddInput(samplerNode, baseId + "-eases", DAEFC_EASE_INOUT_ANIMATION_INPUT);
	}

	// Add the driver input
	if (animationCurve->HasDriver())
	{
		FCDAnimatedDataMap::iterator it = FArchiveXML::documentLinkDataMap[animationCurve->GetDriverPtr()->GetDocument()].animatedData.find(animationCurve->GetDriverPtr());
		FUAssert(it != FArchiveXML::documentLinkDataMap[animationCurve->GetDriverPtr()->GetDocument()].animatedData.end(),);
		FCDAnimatedData& data = it->second;

		FUSStringBuilder builder(data.pointer);
		int32 driverElement = animationCurve->GetDriverIndex();
		if (driverElement >= 0)
		{
			builder.append('('); builder.append(driverElement); builder.append(')');
		}
		if (animationCurve->GetDriverIndex() == 0)
		{
			builder.append('('); builder.append(animationCurve->GetDriverIndex()); builder.append(')');
		}
		AddInput(samplerNode, builder.ToCharPtr(), DAEMAYA_DRIVER_INPUT);
	}

	return samplerNode;	
}

xmlNode* FArchiveXML::WriteChannelFCDAnimationCurve(FCDAnimationCurve* animationCurve, xmlNode* parentNode, const fm::string& baseId, const char* targetPointer)
{
	xmlNode* channelNode = AddChild(parentNode, DAE_CHANNEL_ELEMENT);
	AddAttribute(channelNode, DAE_SOURCE_ATTRIBUTE, fm::string("#") + baseId + "-sampler");

	FCDAnimationCurveDataMap::iterator it = FArchiveXML::documentLinkDataMap[animationCurve->GetDocument()].animationCurveData.find(animationCurve);
	FUAssert(it != FArchiveXML::documentLinkDataMap[animationCurve->GetDocument()].animationCurveData.end(),);
	FCDAnimationCurveData& data = it->second;	

	// Generate and export the channel target
	FUSStringBuilder builder(targetPointer);
	if (data.targetElement >= 0)
	{
		builder.append('('); builder.append(data.targetElement); builder.append(')');
	}
	builder.append(data.targetQualifier);
	AddAttribute(channelNode, DAE_TARGET_ATTRIBUTE, builder);
	return channelNode;
}


// Write out the specific animation elements to the COLLADA XML tree node
void FArchiveXML::WriteSourceFCDAnimationMultiCurve(FCDAnimationMultiCurve* animationMultiCurve, xmlNode* parentNode, const char** qualifiers, const fm::string& baseId)
{
	if (animationMultiCurve->GetDimension() == 0) return;

	// Generate the list of the parameters
	typedef const char* pchar;
	pchar* parameters = new pchar[animationMultiCurve->GetDimension()];
	for (size_t i = 0; i < animationMultiCurve->GetDimension(); ++i)
	{
		parameters[i] = qualifiers[i];
		if (*(parameters[i]) == '.') ++(parameters[i]);
	}
	if (animationMultiCurve->GetDimension() == 16)
	{
		// Hack to avoid parameters on matrices.
		SAFE_DELETE_ARRAY(parameters);
	}

	// Retrieve the source information data
	bool hasTangents = false, hasTCB = false;
	for (size_t i = 0; i < animationMultiCurve->GetKeyCount(); ++i)
	{
		hasTangents |= animationMultiCurve->GetKey(i)->interpolation == FUDaeInterpolation::BEZIER;
		hasTCB |= animationMultiCurve->GetKey(i)->interpolation == FUDaeInterpolation::TCB;
	}

	// Prebuffer the source information lists
	size_t keyCount = animationMultiCurve->GetKeyCount();
	FloatList inputs; inputs.reserve(keyCount);
	FloatList outputs; outputs.reserve(keyCount * animationMultiCurve->GetDimension());
	UInt32List interpolations; interpolations.reserve(keyCount);
	FloatList inTangents; if (hasTangents) inTangents.reserve(keyCount * animationMultiCurve->GetDimension() * 2);
	FloatList outTangents; if (hasTangents) outTangents.reserve(keyCount * animationMultiCurve->GetDimension() * 2);
	FloatList tcbs; if (hasTCB) tcbs.reserve(keyCount * animationMultiCurve->GetDimension() * 3);
	FloatList eases; if (hasTCB) eases.reserve(keyCount * animationMultiCurve->GetDimension() * 2);

	// Retrieve the source information data
	for (size_t i = 0; i < animationMultiCurve->GetKeyCount(); ++i)
	{
		FCDAnimationMKey* key = animationMultiCurve->GetKey(i);
		inputs.push_back(key->input);
		for (uint32 i = 0; i < animationMultiCurve->GetDimension(); ++i)
		{
			outputs.push_back(key->output[i]);
			if (hasTangents)
			{
				if (key->interpolation == FUDaeInterpolation::BEZIER)
				{
					FCDAnimationMKeyBezier* bkey = (FCDAnimationMKeyBezier*)key;
					if (inTangents.size() * animationMultiCurve->GetDimension() * 2 < interpolations.size())
					{
						// Grow to the correct index
						inTangents.resize(interpolations.size() * animationMultiCurve->GetDimension() * 2, 0.0f);
						outTangents.resize(interpolations.size() * animationMultiCurve->GetDimension() * 2, 0.0f);
					}
					inTangents.push_back(bkey->inTangent[i].u);
					inTangents.push_back(bkey->inTangent[i].v);
					outTangents.push_back(bkey->outTangent[i].u);
					outTangents.push_back(bkey->outTangent[i].v);
				}
				else
				{
					// Export flat tangents
					inTangents.push_back(key->input - 0.0001f);
					inTangents.push_back(key->output[i]);
					outTangents.push_back(key->input + 0.0001f);
					outTangents.push_back(key->output[i]);
				}
			}
			if (hasTCB)
			{
				if (key->interpolation == FUDaeInterpolation::TCB)
				{
					FCDAnimationMKeyTCB* tkey = (FCDAnimationMKeyTCB*)key;
					if (tcbs.size() * animationMultiCurve->GetDimension() * 3 < interpolations.size())
					{
						// Grow to the correct index
						tcbs.resize(interpolations.size() * animationMultiCurve->GetDimension() * 3, 0.0f);
						eases.resize(interpolations.size() * animationMultiCurve->GetDimension() * 2, 0.0f);
					}
					tcbs.push_back(tkey->tension[i]);
					tcbs.push_back(tkey->continuity[i]);
					tcbs.push_back(tkey->bias[i]);
					eases.push_back(tkey->easeIn[i]);
					eases.push_back(tkey->easeOut[i]);
				}
				else
				{
					// Export flat tangents
					tcbs.push_back(0.5f);
					tcbs.push_back(0.5f);
					tcbs.push_back(0.5f);
					eases.push_back(0.0f);
					eases.push_back(0.0f);
				}
			}
		}

		// Append the interpolation type last, because we use this list's size to correctly
		// index TCB and Bezier information when exporting mixed-interpolation curves.
		interpolations.push_back(key->interpolation);
	}

	// Export the data arrays
	xmlNode* inputSourceNode = AddSourceFloat(parentNode, baseId + "-input", inputs, "TIME");
	AddSourceFloat(parentNode, baseId + "-output", outputs, animationMultiCurve->GetDimension(), parameters);
	AddSourceInterpolation(parentNode, baseId + "-interpolations", *(FUDaeInterpolationList*) (size_t) &interpolations);
	if (!inTangents.empty())
	{
		AddSourceFloat(parentNode, baseId + "-intangents", inTangents, animationMultiCurve->GetDimension() * 2, FUDaeAccessor::XY);
		AddSourceFloat(parentNode, baseId + "-outtangents", outTangents, animationMultiCurve->GetDimension() * 2, FUDaeAccessor::XY);
	}
	if (!tcbs.empty())
	{
		AddSourceFloat(parentNode, baseId + "-tcbs", tcbs, animationMultiCurve->GetDimension() * 3, FUDaeAccessor::XYZW);
		AddSourceFloat(parentNode, baseId + "-eases", eases, animationMultiCurve->GetDimension() * 2, FUDaeAccessor::XY);
	}

	SAFE_DELETE_ARRAY(parameters);

	// Export the infinity parameters
	xmlNode* mayaTechnique = AddTechniqueChild(inputSourceNode, DAEMAYA_MAYA_PROFILE);
	fm::string infinityType = FUDaeInfinity::ToString(animationMultiCurve->GetPreInfinity());
	AddChild(mayaTechnique, DAEMAYA_PREINFINITY_PARAMETER, infinityType);
	infinityType = FUDaeInfinity::ToString(animationMultiCurve->GetPostInfinity());
	AddChild(mayaTechnique, DAEMAYA_POSTINFINITY_PARAMETER, infinityType);
}

xmlNode* FArchiveXML::WriteSamplerFCDAnimationMultiCurve(FCDAnimationMultiCurve* animationMultiCurve, xmlNode* parentNode, const fm::string& baseId)
{
	xmlNode* samplerNode = AddChild(parentNode, DAE_SAMPLER_ELEMENT);
	AddAttribute(samplerNode, DAE_ID_ATTRIBUTE, baseId + "-sampler");

	// Retrieve the source information data
	bool hasTangents = false, hasTCB = false;
	for (size_t i = 0; i < animationMultiCurve->GetKeyCount(); ++i)
	{
		hasTangents |= animationMultiCurve->GetKey(i)->interpolation == FUDaeInterpolation::BEZIER;
		hasTCB |= animationMultiCurve->GetKey(i)->interpolation == FUDaeInterpolation::TCB;
	}

	// Add the sampler inputs
	AddInput(samplerNode, baseId + "-input", DAE_INPUT_ANIMATION_INPUT);
	AddInput(samplerNode, baseId + "-output", DAE_OUTPUT_ANIMATION_INPUT);
	AddInput(samplerNode, baseId + "-interpolations", DAE_INTERPOLATION_ANIMATION_INPUT);
	if (hasTangents)
	{
		AddInput(samplerNode, baseId + "-intangents", DAE_INTANGENT_ANIMATION_INPUT);
		AddInput(samplerNode, baseId + "-outtangents", DAE_OUTTANGENT_ANIMATION_INPUT);
	}
	if (hasTCB)
	{
		AddInput(samplerNode, baseId + "-tcbs", DAEFC_TCB_ANIMATION_INPUT);
		AddInput(samplerNode, baseId + "-eases", DAEFC_EASE_INOUT_ANIMATION_INPUT);
	}
	return samplerNode;	
}

xmlNode* FArchiveXML::WriteChannelFCDAnimationMultiCurve(FCDAnimationMultiCurve* animationMultiCurve, xmlNode* parentNode, const fm::string& baseId, const fm::string& pointer)
{
	xmlNode* channelNode = AddChild(parentNode, DAE_CHANNEL_ELEMENT);
	AddAttribute(channelNode, DAE_SOURCE_ATTRIBUTE, fm::string("#") + baseId + "-sampler");

	// Generate and export the full target [no qualifiers]
	FUSStringBuilder builder(pointer);
	if (animationMultiCurve->GetTargetElement() >= 0)
	{
		builder.append('('); builder.append(animationMultiCurve->GetTargetElement()); builder.append(')');
	}
	AddAttribute(channelNode, DAE_TARGET_ATTRIBUTE, builder);
	return channelNode;
}
