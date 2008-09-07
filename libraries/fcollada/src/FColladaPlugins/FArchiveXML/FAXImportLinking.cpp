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
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDTargetedEntity.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDEmitterInstance.h"
#include "FCDocument/FCDImage.h"
#include "FCDocument/FCDTexture.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEffectCode.h"
#include "FCDocument/FCDEffectParameterSampler.h"
#include "FCDocument/FCDEffectParameterSurface.h"
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectPass.h"
#include "FCDocument/FCDEffectPassShader.h"
#include "FCDocument/FCDEffectPassState.h"
#include "FCDocument/FCDEffectProfile.h"
#include "FCDocument/FCDEffectProfileFX.h"
#include "FCDocument/FCDEffectStandard.h"
#include "FCDocument/FCDEffectTechnique.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDSkinController.h"
#include "FCDocument/FCDMorphController.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"

bool FArchiveXML::LinkDriver(FCDocument* fcdoument, FCDAnimated* animated, const fm::string& animatedTargetPointer)
{
	bool driven = false;
	size_t animationCount = fcdoument->GetAnimationLibrary()->GetEntityCount();
	for (size_t i = 0; i < animationCount; ++i)
	{
		FCDAnimation* animation = fcdoument->GetAnimationLibrary()->GetEntity(i);
		driven |= FArchiveXML::LinkDriver(animation, animated, animatedTargetPointer);
	}
	return driven;
}

bool FArchiveXML::LinkDriver(FCDAnimation* animation, FCDAnimated* animated, const fm::string& animatedTargetPointer)
{
	bool driver = false;
	for (size_t i = 0; i < animation->GetChannelCount(); ++i)
	{
		driver |= FArchiveXML::LinkDriver(animation->GetChannel(i), animated, animatedTargetPointer);
	}
	for (size_t i = 0; i < animation->GetChildrenCount(); ++i)
	{
		driver |= FArchiveXML::LinkDriver(animation->GetChild(i), animated, animatedTargetPointer);
	}
	return driver;
}

bool FArchiveXML::LinkDriver(FCDAnimationChannel* animationChannel, FCDAnimated* animated, const fm::string& animatedTargetPointer)
{
	FCDAnimationChannelDataMap::iterator it = FArchiveXML::documentLinkDataMap[animationChannel->GetDocument()].animationChannelData.find(animationChannel);
	FUAssert(it != FArchiveXML::documentLinkDataMap[animationChannel->GetDocument()].animationChannelData.end(),);
	FCDAnimationChannelData& data = it->second;

	bool driver = !data.driverPointer.empty();
	driver &= animatedTargetPointer == data.driverPointer;
	driver &= data.driverQualifier >= 0 && (uint32) data.driverQualifier < animated->GetValueCount();
	if (driver)
	{
		// Retrieve the value pointer for the driver
		size_t curveCount = animationChannel->GetCurveCount();
		for (size_t c = 0; c < curveCount; ++c)
		{
			animationChannel->GetCurve(c)->SetDriver(animated, data.driverQualifier);
		}
	}
	return driver;
}

bool FArchiveXML::LinkAnimation(FCDAnimation* animation)
{
	bool status = true;

	// Link the child nodes and check the curves for their drivers
	for (size_t i = 0; i < animation->GetChannelCount(); ++i)
	{
		FCDAnimationChannel* channel = animation->GetChannel(i);
		FCDAnimationChannelDataMap::iterator it = FArchiveXML::documentLinkDataMap[channel->GetDocument()].animationChannelData.find(channel);
		FUAssert(it != FArchiveXML::documentLinkDataMap[channel->GetDocument()].animationChannelData.end(), continue);
		FCDAnimationChannelData& data = it->second;

		if (!data.driverPointer.empty() && channel->GetCurveCount() > 0 && channel->GetCurve(0)->HasDriver())
		{
			status &= FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_ANIM_CURVE_DRIVER_MISSING);
		}
	}

	for (size_t i = 0; i < animation->GetChildrenCount(); ++i)
	{
		status &= FArchiveXML::LinkAnimation(animation->GetChild(i));
	}

	return status;
}


bool FArchiveXML::LinkAnimated(FCDAnimated* animated, xmlNode* node)
{
	bool linked;
	if (node != NULL)
	{
		// Write down the expected target string for the given node
		FCDAnimatedData data;
		FUDaeParser::CalculateNodeTargetPointer(node, data.pointer);

		// Check if this animated value is used as a driver
		linked = FArchiveXML::LinkDriver(animated->GetDocument(), animated, data.pointer);

		// Retrieve the list of the channels pointing to this node
		FCDAnimationChannelList channels;
		FArchiveXML::FindAnimationChannels(animated->GetDocument(), data.pointer, channels);
		linked |= FArchiveXML::ProcessChannels(animated, channels);
		if (linked)
		{
			FArchiveXML::documentLinkDataMap[animated->GetDocument()].animatedData.insert(animated, data);
		}
	}
	else linked = true;

	if (linked)
	{
		// Register this animated value with the document
		animated->GetDocument()->RegisterAnimatedValue(animated);
	}

	animated->SetDirtyFlag();
	return linked;
}

bool FArchiveXML::LinkAnimatedCustom(FCDAnimatedCustom* animatedCustom, xmlNode* node)
{
	bool linked = false;

	if (node != NULL)
	{
		// Retrieve the list of the channels pointing to this node
		FCDAnimatedData data;
		FUDaeParser::CalculateNodeTargetPointer(node, data.pointer);
		FCDAnimationChannelList channels;
		FArchiveXML::FindAnimationChannels(animatedCustom->GetDocument(), data.pointer, channels);

		StringList& qualifiers = animatedCustom->GetQualifiers();

		// Extract all the qualifiers needed to hold these channels
		qualifiers.clear();
		for (FCDAnimationChannelList::iterator itC = channels.begin(); itC != channels.end(); ++itC)
		{
			FCDAnimationChannel* channel = *itC;
			size_t chanelCurveCount = channel->GetCurveCount();
			if (chanelCurveCount == 0) continue;
			
			// Retrieve the channel's qualifier
			FCDAnimationChannelData& channelData = FArchiveXML::documentLinkDataMap[channel->GetDocument()].animationChannelData[channel];
			fm::string qualifier = channelData.targetQualifier;
			if (qualifier.empty())
			{
				// Implies one channel holding multiple curves
				qualifiers.resize(chanelCurveCount);
				break;
			}
			else
			{
				qualifiers.push_back(qualifier);
			}
		}

		// Link the curves and check if this animated value is used as a driver
		if (qualifiers.empty()) qualifiers.resize(1);
		animatedCustom->Resize(qualifiers, false);
		
		linked |= FArchiveXML::ProcessChannels(animatedCustom, channels);
		linked |= FArchiveXML::LinkDriver(animatedCustom->GetDocument(), animatedCustom, data.pointer);
		if (linked)
		{
			FArchiveXML::documentLinkDataMap[animatedCustom->GetDocument()].animatedData.insert(animatedCustom, data);
		}
	}
	else linked = true;

	if (linked)
	{
		// Register this animated value with the document
		animatedCustom->GetDocument()->RegisterAnimatedValue(animatedCustom);
	}
	animatedCustom->SetDirtyFlag();
	return linked;
}

bool FArchiveXML::LinkTargetedEntity(FCDTargetedEntity* targetedEntity)
{
	bool status = true;

	FCDTargetedEntityDataMap::iterator it = FArchiveXML::documentLinkDataMap[targetedEntity->GetDocument()].targetedEntityDataMap.find(targetedEntity);
	FUAssert(it != FArchiveXML::documentLinkDataMap[targetedEntity->GetDocument()].targetedEntityDataMap.end(),);
	FCDTargetedEntityData& data = it->second;

	if (data.targetId.empty()) return status;

	// Skip externally-referenced targets, for now.
	FUUri targetUri(TO_FSTRING(data.targetId));
	if (!targetUri.IsFile() && !targetUri.GetFragment().empty())
	{
		FCDSceneNode* target = targetedEntity->GetDocument()->FindSceneNode(TO_STRING(targetUri.GetFragment()));
		if (target == NULL)
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_TARGET_SCENE_NODE_MISSING);
		}
		targetedEntity->SetTargetNode(target);
	}
	else
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNSUPPORTED_EXTERN_REF);
	}

	return status;
}

bool FArchiveXML::LinkSceneNode(FCDSceneNode* sceneNode)
{
	bool status = true;
	size_t i, size = sceneNode->GetInstanceCount();
	for (i = 0; i < size; i++)
	{
		FCDEntityInstance* instance = sceneNode->GetInstance(i);
		if (instance->IsType(FCDControllerInstance::GetClassType()))
		{
			status &= FArchiveXML::LinkControllerInstance((FCDControllerInstance*)instance);
		}
		else if (instance->IsType(FCDEmitterInstance::GetClassType()))
		{
			status &= FArchiveXML::LinkEmitterInstance((FCDEmitterInstance*)instance);
		}
	}

	size = sceneNode->GetChildrenCount();
	for (i = 0; i < size; i++)
	{
		status &= FArchiveXML::LinkSceneNode(sceneNode->GetChild(i));
	}

	return status;
}

void FArchiveXML::LinkMaterial(FCDMaterial* material)
{
	FCDEffectParameterList parameters;
	size_t parameterCount = material->GetEffectParameterCount();
	for (size_t p = 0; p < parameterCount; ++p)
	{
		FCDEffectParameter* parameter = material->GetEffectParameter(p);
		parameters.push_back(parameter);
	}

	for (size_t p = 0; p < parameterCount; ++p)
	{
		FCDEffectParameter* parameter = material->GetEffectParameter(p);
		if (parameter->IsType(FCDEffectParameterSurface::GetClassType()))
		{
			FArchiveXML::LinkEffectParameterSurface((FCDEffectParameterSurface*) parameter);
		}
		else if (parameter->IsType(FCDEffectParameterSampler::GetClassType()))
		{
			FArchiveXML::LinkEffectParameterSampler((FCDEffectParameterSampler*) parameter, parameters);
		}
	}
}

void FArchiveXML::LinkEffect(FCDEffect* effect)
{
	// Link up the parameters
	FCDEffectParameterList parameters;
	size_t parameterCount = effect->GetEffectParameterCount();
	for (size_t p = 0; p < parameterCount; ++p)
	{
		FCDEffectParameter* parameter = effect->GetEffectParameter(p);
		parameters.push_back(parameter);
	}
	for (size_t p = 0; p < parameterCount; ++p)
	{
		FCDEffectParameter* parameter = effect->GetEffectParameter(p);
		if (parameter->IsType(FCDEffectParameterSurface::GetClassType()))
		{
			FArchiveXML::LinkEffectParameterSurface((FCDEffectParameterSurface*) parameter);
		}
		else if (parameter->IsType(FCDEffectParameterSampler::GetClassType()))
		{
			FArchiveXML::LinkEffectParameterSampler((FCDEffectParameterSampler*) parameter, parameters);
		}
	}

	// Link up the profiles and their parameters/textures/images
	size_t profileCount = effect->GetProfileCount();
	for (size_t p = 0; p < profileCount; ++p)
	{
		FCDEffectProfile* profile = effect->GetProfile(p);
		if (profile->IsType(FCDEffectStandard::GetClassType()))
		{
			FArchiveXML::LinkEffectStandard((FCDEffectStandard*) profile);
		}
		else if (profile->IsType(FCDEffectProfileFX::GetClassType()))
		{
			FArchiveXML::LinkEffectProfileFX((FCDEffectProfileFX*) profile);
		}
		else
		{
			FArchiveXML::LinkEffectProfile(profile);
		}
	}
}

void FArchiveXML::LinkEffectParameterSurface(FCDEffectParameterSurface* effectParameterSurface)
{
	for (StringList::iterator itN = effectParameterSurface->GetNames().begin(); itN != effectParameterSurface->GetNames().end(); ++itN)
	{
		FCDImage* image = effectParameterSurface->GetDocument()->FindImage(*itN);
		if (image != NULL)
		{
			effectParameterSurface->AddImage(image);
		}
	}
}

void FArchiveXML::LinkEffectParameterSampler(FCDEffectParameterSampler* effectParameterSampler, FCDEffectParameterList& parameters)
{
	FCDEffectParameterSamplerDataMap::iterator it = FArchiveXML::documentLinkDataMap[effectParameterSampler->GetDocument()].effectParameterSamplerDataMap.find(effectParameterSampler);
	FUAssert(it != FArchiveXML::documentLinkDataMap[effectParameterSampler->GetDocument()].effectParameterSamplerDataMap.end(),);
	FCDEffectParameterSamplerData& data = it->second;

	FCDEffectParameter* surface = NULL;
	size_t count = parameters.size();
	for (size_t i = 0; i < count; ++i)
	{
		if (IsEquivalent(parameters[i]->GetReference(), data.surfaceSid))
		{
			surface = parameters[i];
			break;
		}
	}
	FUAssert(surface == NULL || surface->HasType(FCDEffectParameterSurface::GetClassType()), return);

	effectParameterSampler->SetSurface((FCDEffectParameterSurface*) surface);
	data.surfaceSid.clear();
}

void FArchiveXML::LinkEffectProfile(FCDEffectProfile* effectProfile)
{
	// Make up a list of all the available parameters.
	FCDEffectParameterList parameters;
	size_t parameterCount = effectProfile->GetEffectParameterCount();
	for (size_t p = 0; p < parameterCount; ++p)
	{
		parameters.push_back(effectProfile->GetEffectParameter(p));
	}
	// Also retrieve the parent's parameters.
	size_t parentParameterCount = effectProfile->GetParent()->GetEffectParameterCount();
	for (size_t p = 0; p < parentParameterCount; ++p)
	{
		parameters.push_back(effectProfile->GetParent()->GetEffectParameter(p));
	}

	for (size_t p = 0; p < parameterCount; ++p)
	{
		FCDEffectParameter* parameter = effectProfile->GetEffectParameter(p);
		if (parameter->HasType(FCDEffectParameterSurface::GetClassType()))
		{
			FArchiveXML::LinkEffectParameterSurface((FCDEffectParameterSurface*) parameter);
		}
		else if (parameter->HasType(FCDEffectParameterSampler::GetClassType()))
		{
			FArchiveXML::LinkEffectParameterSampler((FCDEffectParameterSampler*) parameter, parameters);
		}
	}
}

void FArchiveXML::LinkEffectProfileFX(FCDEffectProfileFX* effectProfileFX)
{
	// Call the parent to link the effect parameters
	FArchiveXML::LinkEffectProfile(effectProfileFX);

	size_t techniqueCount = effectProfileFX->GetTechniqueCount();
	for (size_t t = 0; t < techniqueCount; ++t)
	{
		FArchiveXML::LinkEffectTechnique(effectProfileFX->GetTechnique(t));
	}
}

void FArchiveXML::LinkEffectStandard(FCDEffectStandard* effectStandard)
{
	FArchiveXML::LinkEffectProfile(effectStandard);

	// Make up a list of all the parameters available for linkage.
	FCDEffectParameterList parameters;
	size_t parameterCount = effectStandard->GetEffectParameterCount();
	for (size_t p = 0; p < parameterCount; ++p)
	{
		parameters.push_back(effectStandard->GetEffectParameter(p));
	}
	// Also retrieve the parent's parameters.
	size_t parentParameterCount = effectStandard->GetParent()->GetEffectParameterCount();
	for (size_t p = 0; p < parentParameterCount; ++p)
	{
		parameters.push_back(effectStandard->GetParent()->GetEffectParameter(p));
	}

	// Link the textures with the sampler parameters
	for (uint32 i = 0; i < FUDaeTextureChannel::COUNT; ++i)
	{
		size_t bucketSize = effectStandard->GetTextureCount(i);
		for (size_t t = 0; t < bucketSize; ++t)
		{
			FArchiveXML::LinkTexture(effectStandard->GetTexture(i, t), parameters);
		}
	}
}

void FArchiveXML::LinkEffectTechnique(FCDEffectTechnique* effectTechnique)
{
	// Make up a list of all the parameters available for linkage.
	FCDEffectParameterList parameters;
	size_t parameterCount = effectTechnique->GetEffectParameterCount();
	for (size_t p = 0; p < parameterCount; ++p)
	{
		parameters.push_back(effectTechnique->GetEffectParameter(p));
	}
	// Also retrieve the parent's parameters.
	size_t parentParameterCount = effectTechnique->GetParent()->GetEffectParameterCount();
	for (size_t p = 0; p < parentParameterCount; ++p)
	{
		parameters.push_back(effectTechnique->GetParent()->GetEffectParameter(p));
	}
	// Also retrieve the parent's parent's parameters.
	parentParameterCount = effectTechnique->GetParent()->GetParent()->GetEffectParameterCount();
	for (size_t p = 0; p < parentParameterCount; ++p)
	{
		parameters.push_back(effectTechnique->GetParent()->GetParent()->GetEffectParameter(p));
	}

	for (size_t p = 0; p < parameterCount; ++p)
	{
		FCDEffectParameter* parameter = effectTechnique->GetEffectParameter(p);
		if (parameter->HasType(FCDEffectParameterSurface::GetClassType()))
		{
			FArchiveXML::LinkEffectParameterSurface((FCDEffectParameterSurface*) parameter);
		}
		else if (parameter->HasType(FCDEffectParameterSampler::GetClassType()))
		{
			FArchiveXML::LinkEffectParameterSampler((FCDEffectParameterSampler*) parameter, parameters);
		}
	}
}

void FArchiveXML::LinkTexture(FCDTexture* texture, FCDEffectParameterList& parameters)
{
	FCDTextureDataMap::iterator it = FArchiveXML::documentLinkDataMap[texture->GetDocument()].textureDataMap.find(texture);
	FUAssert(it != FArchiveXML::documentLinkDataMap[texture->GetDocument()].textureDataMap.end(),);
	FCDTextureData& data = it->second;

	if (!data.samplerSid.empty())
	{
		// Check for the sampler parameter in the parent profile and the effect.
		if (texture->GetParent() != NULL)
		{
			const fm::string& cleanRef = FCDObjectWithId::CleanSubId(data.samplerSid);
			FCDEffectParameterSampler* sampler = NULL;
			size_t parameterCount = parameters.size();
			for (size_t p = 0; p < parameterCount; ++p)
			{
				if (IsEquivalent(parameters[p]->GetReference(), cleanRef) && parameters[p]->IsType(FCDEffectParameterSampler::GetClassType()))
				{
					sampler = (FCDEffectParameterSampler*) parameters[p];
					break;
				}
			}
			if (sampler != NULL) texture->SetSampler(sampler);
		}

		if (!texture->HasSampler() && !data.samplerSid.empty())
		{
			// Early COLLADA 1.4.0 backward compatibility: Also look for an image with this id.
			if (data.samplerSid[0] == '#') data.samplerSid.erase(0, 1);
            FCDImage* image = texture->GetDocument()->FindImage(data.samplerSid);
			texture->SetImage(image);
			texture->SetDirtyFlag();

			FUAssert(texture->HasSampler(), FUError::Error(FUError::WARNING_LEVEL, FUError::ERROR_INVALID_TEXTURE_SAMPLER));
		}
		data.samplerSid.clear();
	}
}

bool FArchiveXML::LinkController(FCDController* controller)
{
	bool ret = true;
	if (controller->GetBaseTarget() == NULL)
	{
		if (controller->IsSkin())
		{
			ret &= FArchiveXML::LinkSkinController(controller->GetSkinController());
		}
		else if (controller->IsMorph())
		{
			ret &= FArchiveXML::LinkMorphController(controller->GetMorphController());
		}
		else return false;

		// If our target is a controller, link it too.
		FCDEntity* entity = controller->GetBaseTarget();
		if (entity != NULL && entity->GetType() == FCDEntity::CONTROLLER)
		{
			ret &= FArchiveXML::LinkController((FCDController*)entity);
		}
	}
	return ret;
}

bool FArchiveXML::LinkSkinController(FCDSkinController* UNUSED(skinController))
{
	return true;
}


bool FArchiveXML::LinkMorphController(FCDMorphController* morphController)
{
	FCDMorphControllerDataMap::iterator it = FArchiveXML::documentLinkDataMap[morphController->GetDocument()].morphControllerDataMap.find(morphController);
	FUAssert(it != FArchiveXML::documentLinkDataMap[morphController->GetDocument()].morphControllerDataMap.end(),);
	FCDMorphControllerData& data = it->second;

	if (morphController->GetBaseTarget() == NULL)
	{
		fm::string targetId = SkipPound(data.targetId);
		FCDEntity* baseTarget = morphController->GetDocument()->FindGeometry(targetId);
		if (baseTarget == NULL) baseTarget = morphController->GetDocument()->FindController(data.targetId);
		if (baseTarget == NULL)
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_MC_BASE_TARGET_MISSING, 0);
			return false;
		}

		morphController->SetBaseTarget(baseTarget);

		data.targetId.clear();
	}
	return true;
}

bool FArchiveXML::LinkGeometryMesh(FCDGeometryMesh* geometryMesh)
{
	bool status = true;
	if (!geometryMesh->IsConvex() || geometryMesh->GetConvexHullOf().empty())
		return status;

	FCDGeometry* concaveGeom = geometryMesh->GetDocument()->FindGeometry(geometryMesh->GetConvexHullOf());
	if (concaveGeom)
	{
		FCDGeometryMesh* origMesh = concaveGeom->GetMesh();
		if (origMesh != NULL)
		{
			origMesh->Clone(geometryMesh);
			geometryMesh->SetConvexify(geometryMesh != NULL);
			geometryMesh->SetConvex(true); // may have been overwritten by clone
		}
		return status;
	}
	else
	{
		//return status.Fail(FS("Unknown geometry for creation of convex hull of: ") + TO_FSTRING(parent->GetDaeId()));
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_UNKNOWN_GEO_CH);
		return status;
	}
}

