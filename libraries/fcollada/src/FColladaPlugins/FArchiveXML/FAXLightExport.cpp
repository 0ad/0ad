/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDLight.h"

xmlNode* FArchiveXML::WriteLight(FCDObject* object, xmlNode* parentNode)
{
	FCDLight* light = (FCDLight*)object;

	// Create the base light node
	xmlNode* lightNode = FArchiveXML::WriteToEntityXMLFCDEntity(light, parentNode, DAE_LIGHT_ELEMENT);
	xmlNode* baseNode = AddChild(lightNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	const char* baseNodeName;
	switch (light->GetLightType())
	{
	case FCDLight::POINT: baseNodeName = DAE_LIGHT_POINT_ELEMENT; break;
	case FCDLight::SPOT: baseNodeName = DAE_LIGHT_SPOT_ELEMENT; break;
	case FCDLight::AMBIENT: baseNodeName = DAE_LIGHT_AMBIENT_ELEMENT; break;
	case FCDLight::DIRECTIONAL: baseNodeName = DAE_LIGHT_DIRECTIONAL_ELEMENT; break;
	default: baseNodeName = DAEERR_UNKNOWN_INPUT; break;
	}
	baseNode = AddChild(baseNode, baseNodeName);

	// Add the application-specific technique
	// Buffer the extra light parameters so we can remove them after the export.
	FUTrackedPtr<FCDETechnique> techniqueNode = const_cast<FCDExtra*>(light->GetExtra())->GetDefaultType()->AddTechnique(DAE_FCOLLADA_PROFILE);
	FCDENodeList extraParameterNodes;

	// Write out the light parameters
	fm::string colorValue = FUStringConversion::ToString((FMVector3&) light->GetColor());
	xmlNode* colorNode = AddChild(baseNode, DAE_COLOR_LIGHT_PARAMETER, colorValue);
	FArchiveXML::WriteAnimatedValue(&light->GetColor(), colorNode, "color");
	
	if (light->GetLightType() == FCDLight::POINT || light->GetLightType() == FCDLight::SPOT)
	{
		xmlNode* attenuationNode = AddChild(baseNode, DAE_CONST_ATTENUATION_LIGHT_PARAMETER, light->GetConstantAttenuationFactor());
		FArchiveXML::WriteAnimatedValue(&light->GetConstantAttenuationFactor(), attenuationNode, "constant_attenuation");
		attenuationNode = AddChild(baseNode, DAE_LIN_ATTENUATION_LIGHT_PARAMETER, light->GetLinearAttenuationFactor());
		FArchiveXML::WriteAnimatedValue(&light->GetLinearAttenuationFactor(), attenuationNode, "linear_attenuation");
		attenuationNode = AddChild(baseNode, DAE_QUAD_ATTENUATION_LIGHT_PARAMETER, light->GetQuadraticAttenuationFactor());
		FArchiveXML::WriteAnimatedValue(&light->GetQuadraticAttenuationFactor(), attenuationNode, "quadratic_attenuation");
	}
	else if (light->GetLightType() == FCDLight::DIRECTIONAL)
	{
		FCDENode* attenuationNode = techniqueNode->AddParameter(DAE_CONST_ATTENUATION_LIGHT_PARAMETER, light->GetConstantAttenuationFactor());
		attenuationNode->GetAnimated()->Copy(light->GetConstantAttenuationFactor().GetAnimated());
		extraParameterNodes.push_back(attenuationNode);
		attenuationNode = techniqueNode->AddParameter(DAE_LIN_ATTENUATION_LIGHT_PARAMETER, light->GetLinearAttenuationFactor());
		attenuationNode->GetAnimated()->Copy(light->GetLinearAttenuationFactor().GetAnimated());
		extraParameterNodes.push_back(attenuationNode);
		attenuationNode = techniqueNode->AddParameter(DAE_QUAD_ATTENUATION_LIGHT_PARAMETER, light->GetQuadraticAttenuationFactor());
		attenuationNode->GetAnimated()->Copy(light->GetQuadraticAttenuationFactor().GetAnimated());
		extraParameterNodes.push_back(attenuationNode);
	}

	if (light->GetLightType() == FCDLight::SPOT)
	{
		xmlNode* falloffNode = AddChild(baseNode, DAE_FALLOFFANGLE_LIGHT_PARAMETER, light->GetFallOffAngle());
		FArchiveXML::WriteAnimatedValue(&light->GetFallOffAngle(), falloffNode, "falloff_angle");
		falloffNode = AddChild(baseNode, DAE_FALLOFFEXPONENT_LIGHT_PARAMETER, light->GetFallOffExponent());
		FArchiveXML::WriteAnimatedValue(&light->GetFallOffExponent(), falloffNode, "falloff_exponent");
	}
	else if (light->GetLightType() == FCDLight::DIRECTIONAL)
	{
		FCDENode* falloffNode = techniqueNode->AddParameter(DAE_FALLOFFANGLE_LIGHT_PARAMETER, light->GetFallOffAngle());
		falloffNode->GetAnimated()->Copy(light->GetFallOffAngle().GetAnimated());
		extraParameterNodes.push_back(falloffNode);
		falloffNode = techniqueNode->AddParameter(DAE_FALLOFFEXPONENT_LIGHT_PARAMETER, light->GetFallOffExponent());
		falloffNode->GetAnimated()->Copy(light->GetFallOffExponent().GetAnimated());
		extraParameterNodes.push_back(falloffNode);
	}

	FCDENode* intensityNode = techniqueNode->AddParameter(DAEFC_INTENSITY_LIGHT_PARAMETER, light->GetIntensity());
	intensityNode->GetAnimated()->Copy(light->GetIntensity().GetAnimated());
	extraParameterNodes.push_back(intensityNode);
		
	if (light->GetLightType() == FCDLight::DIRECTIONAL || light->GetLightType() == FCDLight::SPOT)
	{
		FCDENode* outerAngleNode = techniqueNode->AddParameter(DAEMAX_OUTERCONE_LIGHT_PARAMETER, light->GetOuterAngle());
		outerAngleNode->GetAnimated()->Copy(light->GetOuterAngle().GetAnimated());
		extraParameterNodes.push_back(outerAngleNode);
	}

	if (light->GetLightType() == FCDLight::SPOT)
	{
		FCDENode* dropoffNode = techniqueNode->AddParameter(DAEMAYA_DROPOFF_LIGHT_PARAMETER, light->GetDropoff());
		dropoffNode->GetAnimated()->Copy(light->GetDropoff().GetAnimated());
		extraParameterNodes.push_back(dropoffNode);
	}

	// Export the <extra> elements and release the temporarily-added parameters/technique
	FArchiveXML::WriteTargetedEntityExtra(light, lightNode);
	CLEAR_POINTER_VECTOR(extraParameterNodes);
	if (techniqueNode != NULL && techniqueNode->GetChildNodeCount() == 0) SAFE_RELEASE(techniqueNode);
	return lightNode;
}
