/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDLight.h"
#include "FCDocument/FCDLightTools.h"

bool FArchiveXML::LoadLight(FCDObject* object, xmlNode* lightNode)
{
	if (!FArchiveXML::LoadTargetedEntity(object, lightNode)) return false;

	bool status = true;
	FCDLight* light = (FCDLight*)object;
	if (!IsEquivalent(lightNode->name, DAE_LIGHT_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_LIGHT_LIB_ELEMENT, lightNode->line);
		return status;
	}

	// Retrieve the <technique_common> element.
	xmlNode* commonTechniqueNode = FindChildByType(lightNode, DAE_TECHNIQUE_COMMON_ELEMENT);

	// Look for the <point>, <directional>, <spot> or <ambient> element under the common-profile technique
	xmlNode* lightParameterNode = NULL;
	for (xmlNode* child = commonTechniqueNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;
		if (IsEquivalent(child->name, DAE_LIGHT_POINT_ELEMENT)) { lightParameterNode = child; light->SetLightType(FCDLight::POINT); break; }
		else if (IsEquivalent(child->name, DAE_LIGHT_SPOT_ELEMENT)) { lightParameterNode = child; light->SetLightType(FCDLight::SPOT); break; }
		else if (IsEquivalent(child->name, DAE_LIGHT_AMBIENT_ELEMENT)) { lightParameterNode = child; light->SetLightType(FCDLight::AMBIENT); break; }
		else if (IsEquivalent(child->name, DAE_LIGHT_DIRECTIONAL_ELEMENT)) { lightParameterNode = child; light->SetLightType(FCDLight::DIRECTIONAL); break; }
		else
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_LT_ELEMENT, child->line);
		}
	}

	// Verify the light's basic structures are found
	if (lightParameterNode == NULL)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_MISSING_ELEMENT, lightNode->line);
	}

	// Retrieve the common light parameters
	StringList parameterNames;
	xmlNodeList parameterNodes;
	FindParameters(lightParameterNode, parameterNames, parameterNodes);

	// Parse the common light parameters
	size_t parameterCount = parameterNodes.size();
	for (size_t i = 0; i < parameterCount; ++i)
	{
		xmlNode* parameterNode = parameterNodes[i];
		const fm::string& parameterName = parameterNames[i];
		const char* content = ReadNodeContentDirect(parameterNode);
		if (parameterName == DAE_COLOR_LIGHT_PARAMETER)
		{
			light->SetColor(FUStringConversion::ToVector3(content));
			FArchiveXML::LoadAnimatable(&light->GetColor(), parameterNode);
		}
		else if (parameterName == DAE_CONST_ATTENUATION_LIGHT_PARAMETER)
		{
			light->SetConstantAttenuationFactor(FUStringConversion::ToFloat(content));
			FArchiveXML::LoadAnimatable(&light->GetConstantAttenuationFactor(), parameterNode);
		}
		else if (parameterName == DAE_LIN_ATTENUATION_LIGHT_PARAMETER)
		{
			light->SetLinearAttenuationFactor(FUStringConversion::ToFloat(content));
			FArchiveXML::LoadAnimatable(&light->GetLinearAttenuationFactor(), parameterNode);
		}
		else if (parameterName == DAE_QUAD_ATTENUATION_LIGHT_PARAMETER)
		{
			light->SetQuadraticAttenuationFactor(FUStringConversion::ToFloat(content));
			FArchiveXML::LoadAnimatable(&light->GetQuadraticAttenuationFactor(), parameterNode);
		}
		else if (parameterName == DAE_FALLOFFEXPONENT_LIGHT_PARAMETER)
		{
			light->SetFallOffExponent(FUStringConversion::ToFloat(content));
			FArchiveXML::LoadAnimatable(&light->GetFallOffExponent(), parameterNode);
		}
		else if (parameterName == DAE_FALLOFFANGLE_LIGHT_PARAMETER)
		{
			light->SetFallOffAngle(FUStringConversion::ToFloat(content));
			FArchiveXML::LoadAnimatable(&light->GetFallOffAngle(), parameterNode);
		}
		else
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_LIGHT_PROG_PARAM, parameterNode->line);
		}
	}

	// Process and remove the known extra parameters
	StringList extraParameterNames;
	FCDENodeList extraParameters;
	FCDExtra* extra = light->GetExtra();
	size_t techniqueCount = extra->GetDefaultType()->GetTechniqueCount();
	for (size_t t = 0; t < techniqueCount; ++t)
	{
		FCDETechnique* technique = extra->GetDefaultType()->GetTechnique(t);
		technique->FindParameters(extraParameters, extraParameterNames);
	}

	FCDENode* penumbraNode = NULL;

	size_t extraParameterCount = extraParameters.size();
	for (size_t p = 0; p < extraParameterCount; ++p)
	{
		FCDENode* extraParameterNode = extraParameters[p];
		const fm::string& parameterName = extraParameterNames[p];
		const fchar* content = extraParameterNode->GetContent();
		FCDParameterAnimatableFloat* fpValue = NULL;

		if (parameterName == DAE_FALLOFFEXPONENT_LIGHT_PARAMETER)
		{
			fpValue = &light->GetFallOffExponent();
		}
		else if (parameterName == DAE_FALLOFFANGLE_LIGHT_PARAMETER)
		{
			fpValue = &light->GetFallOffAngle();
		}
		else if (parameterName == DAE_CONST_ATTENUATION_LIGHT_PARAMETER)
		{
			fpValue = &light->GetConstantAttenuationFactor();
		}
		else if (parameterName == DAE_LIN_ATTENUATION_LIGHT_PARAMETER)
		{
			fpValue = &light->GetLinearAttenuationFactor();
		}
		else if (parameterName == DAE_QUAD_ATTENUATION_LIGHT_PARAMETER)
		{
			fpValue = &light->GetQuadraticAttenuationFactor();
		}
		else if (parameterName == DAEFC_INTENSITY_LIGHT_PARAMETER)
		{
			fpValue = &light->GetIntensity();
		}
		else if (parameterName == DAEMAX_OUTERCONE_LIGHT_PARAMETER)
		{
			fpValue = &light->GetOuterAngle();
		}
		else if (parameterName == DAEMAYA_PENUMBRA_LIGHT_PARAMETER)
		{
			// back-ward compatibility, penumbra is now represented with outerAngle and FallOffAngle
			penumbraNode = extraParameterNode;
			continue; // do not delete the parameter node since we need it for penumbra animations
		}
		else if (parameterName == DAEMAYA_DROPOFF_LIGHT_PARAMETER)
		{
			fpValue = &light->GetDropoff();
		}
		else continue;

		// If we have requested a value, convert and animate it
		if (fpValue != NULL)
		{
			*fpValue = FUStringConversion::ToFloat(content);
			if (extraParameterNode->GetAnimated()->HasCurve())
			{
				extraParameterNode->GetAnimated()->Clone(fpValue->GetAnimated());
			}
		}

		// We have processed this extra node: remove it from the extra tree.
		SAFE_RELEASE(extraParameterNode);
	}

	if (penumbraNode != NULL)
	{
		float penumbraValue = FUStringConversion::ToFloat(penumbraNode->GetContent());
		FCDAnimated* penumbraAnimatedValue = penumbraNode->GetAnimated();
		FCDLightTools::LoadPenumbra(light, penumbraValue, penumbraAnimatedValue, false);
		SAFE_RELEASE(penumbraNode);
	}

	light->SetDirtyFlag();
	return status;
}
