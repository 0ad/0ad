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
#include "FCDocument/FCDEntityReference.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDImage.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDTexture.h"

bool FArchiveXML::LoadMaterial(FCDObject* object, xmlNode* materialNode)
{ 
	if (!FArchiveXML::LoadEntity(object, materialNode)) return false;

	bool status = true;
	FCDMaterial* material = (FCDMaterial*)object;
	while (material->GetEffectParameterCount() > 0)
	{
		material->GetEffectParameter(material->GetEffectParameterCount() - 1)->Release();
	}

	if (!IsEquivalent(materialNode->name, DAE_MATERIAL_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_MAT_LIB_ELEMENT, materialNode->line);
		return status;
	}

	// Read in the effect pointer node
	xmlNode* effectNode = FindChildByType(materialNode, DAE_INSTANCE_EFFECT_ELEMENT);
	if (effectNode == NULL)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::ERROR_MISSING_ELEMENT, materialNode->line);
	}

	FUUri url = ReadNodeUrl(effectNode);
	material->GetEffectReference()->SetUri(url);

	// Read in the parameter modifications
	for (xmlNode* child = effectNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;
		
		if (IsEquivalent(child->name, DAE_FXCMN_SETPARAM_ELEMENT))
		{
			FCDEffectParameter* parameter = material->AddEffectParameter(FArchiveXML::GetEffectParameterType(child));
			status &= FArchiveXML::LoadSwitch(parameter, &parameter->GetObjectType(), child);
		}
		else if (IsEquivalent(child->name, DAE_FXCMN_HINT_ELEMENT))
		{
			FCDMaterialTechniqueHint& hint = *(material->GetTechniqueHints().insert(material->GetTechniqueHints().end(), FCDMaterialTechniqueHint()));
			hint.platform = TO_FSTRING(ReadNodeProperty(child, DAE_PLATFORM_ATTRIBUTE));
			hint.technique = ReadNodeProperty(child, DAE_REF_ATTRIBUTE);
		}
	}

	if (material->GetEffectReference()->IsLocal() && material->GetEffectReference()->GetEntity() == NULL)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_EFFECT_MISSING, materialNode->line);
		return status;
	}
	
	material->SetDirtyFlag(); 
	return status;
}			

bool FArchiveXML::LoadEffectCode(FCDObject* object, xmlNode* codeNode)
{ 
	FCDEffectCode* effectCode = (FCDEffectCode*)object;

	bool status = true;
	if (IsEquivalent(codeNode->name, DAE_FXCMN_INCLUDE_ELEMENT)) effectCode->SetType(FCDEffectCode::INCLUDE);
	else if (IsEquivalent(codeNode->name, DAE_FXCMN_CODE_ELEMENT)) effectCode->SetType(FCDEffectCode::CODE);
	else
	{
		//return status.Fail(FS("Unknown effect code type."), codeNode->line);
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_UNKNOWN_EFFECT_CODE, codeNode->line); 
		return status;
	}

	// Read in the code identifier and the actual code or filename
	effectCode->SetSubId(ReadNodeProperty(codeNode, DAE_SID_ATTRIBUTE));
	if (effectCode->GetType() == FCDEffectCode::INCLUDE && effectCode->GetSubId().empty())
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_SID_MISSING, codeNode->line);
	}
	if (effectCode->GetType() == FCDEffectCode::INCLUDE) 
	{
		effectCode->SetFilename(ReadNodeUrl(codeNode).GetAbsolutePath());
		effectCode->SetFilename(effectCode->GetDocument()->GetFileManager()->CleanUri(FUUri(effectCode->GetFilename())));
	}
	else
	{
		effectCode->SetCode(TO_FSTRING(ReadNodeContentFull(codeNode))); 
	}

	effectCode->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadEffectParameter(FCDObject* object, xmlNode* parameterNode)
{ 
	FCDEffectParameter* effectParameter = (FCDEffectParameter*)object;

	bool status = true;

	// Retrieves the annotations
	xmlNodeList annotationNodes;
	FindChildrenByType(parameterNode, DAE_ANNOTATE_ELEMENT, annotationNodes);
	for (xmlNodeList::iterator itN = annotationNodes.begin(); itN != annotationNodes.end(); ++itN)
	{
		xmlNode* annotateNode = (*itN);
		FCDEffectParameterAnnotation* annotation = effectParameter->AddAnnotation();
		annotation->name = TO_FSTRING(ReadNodeProperty(annotateNode, DAE_NAME_ATTRIBUTE));

		for (xmlNode* valueNode = annotateNode->children; valueNode != NULL; valueNode = valueNode->next)
		{
			if (valueNode->type != XML_ELEMENT_NODE) continue;
			if (IsEquivalent(valueNode->name, DAE_FXCMN_STRING_ELEMENT)) { annotation->type = FCDEffectParameter::STRING; annotation->value = TO_FSTRING(ReadNodeContentFull(valueNode)); }
			else if (IsEquivalent(valueNode->name, DAE_FXCMN_BOOL_ELEMENT)) { annotation->type = FCDEffectParameter::BOOLEAN; annotation->value = TO_FSTRING(ReadNodeContentDirect(valueNode)); }
			else if (IsEquivalent(valueNode->name, DAE_FXCMN_INT_ELEMENT)) { annotation->type = FCDEffectParameter::INTEGER; annotation->value = TO_FSTRING(ReadNodeContentDirect(valueNode)); }
			else if (IsEquivalent(valueNode->name, DAE_FXCMN_FLOAT_ELEMENT)) { annotation->type = FCDEffectParameter::FLOAT; annotation->value = TO_FSTRING(ReadNodeContentDirect(valueNode)); }
			else {FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_ANNO_TYPE, valueNode->line);}
			break;
		}
	}

	// This parameter is a generator if this is a <newparam> element. Otherwise, it modifies
	// an existing parameter (<bind>, <bind_semantic> or <setparam>.
	if (IsEquivalent(parameterNode->name, DAE_FXCMN_NEWPARAM_ELEMENT)) effectParameter->SetGenerator();
	else if (IsEquivalent(parameterNode->name, DAE_PARAMETER_ELEMENT)) effectParameter->SetAnimator();
	else if (IsEquivalent(parameterNode->name, DAE_FXCMN_SETPARAM_ELEMENT)) effectParameter->SetModifier();

	if (effectParameter->IsGenerator())
	{
		effectParameter->SetReference(ReadNodeProperty(parameterNode, DAE_SID_ATTRIBUTE));
		if (effectParameter->GetReference().empty())
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_GEN_REF_ATTRIBUTE_MISSING, parameterNode->line);
			return status;
		}
	}
	else if (effectParameter->IsModifier())
	{
		effectParameter->SetReference(ReadNodeProperty(parameterNode, DAE_REF_ATTRIBUTE));
		if (effectParameter->GetReference().empty())
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_MOD_REF_ATTRIBUTE_MISSING, parameterNode->line);
			return status;
		}
	}
	else if (effectParameter->IsAnimator())
	{
		effectParameter->SetReference(ReadNodeProperty(parameterNode, DAE_SID_ATTRIBUTE));
		if (effectParameter->GetReference().empty())
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_GEN_REF_ATTRIBUTE_MISSING, parameterNode->line);
		}
		effectParameter->SetSemantic(ReadNodeProperty(parameterNode, DAE_SEMANTIC_ATTRIBUTE));
		if (effectParameter->GetSemantic().empty())
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_GEN_REF_ATTRIBUTE_MISSING, parameterNode->line);
		}
	}
	if (!effectParameter->IsAnimator())
	{
		xmlNode* valueNode = FindChildByType(parameterNode, DAE_FXCMN_SEMANTIC_ELEMENT);
		if (valueNode != NULL)
		{
			effectParameter->SetSemantic(ReadNodeContentFull(valueNode));
		}
	}
	effectParameter->SetDirtyFlag();
	return status;
}		

bool FArchiveXML::LoadEffectParameterBool(FCDObject* object, xmlNode* parameterNode)
{ 
	if (!FArchiveXML::LoadEffectParameter(object, parameterNode)) return false;

	bool status = true;
	FCDEffectParameterBool* effectParameterBool = (FCDEffectParameterBool*)object;
	xmlNode* valueNode = FindChildByType(parameterNode, DAE_FXCMN_BOOL_ELEMENT);
	const char* valueString = ReadNodeContentDirect(valueNode);
	if (valueString == NULL || *valueString == 0)
	{
		//return status.Fail(FS("Bad value for boolean parameter in effect: ") + TO_FSTRING(GetReference()), parameterNode->line);
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_BAD_BOOLEAN_VALUE, parameterNode->line);
	}
	effectParameterBool->SetValue(FUStringConversion::ToBoolean(valueString));
	effectParameterBool->SetDirtyFlag();
	return status;
} 

bool FArchiveXML::LoadEffectParameterFloat(FCDObject* object, xmlNode* parameterNode)
{ 
	if (!FArchiveXML::LoadEffectParameter(object, parameterNode)) return false;

	bool status = true;
	FCDEffectParameterFloat* effectParameterFloat = (FCDEffectParameterFloat*)object;
	if (!effectParameterFloat->IsAnimator())
	{
		xmlNode* valueNode = FindChildByType(parameterNode, DAE_FXCMN_FLOAT_ELEMENT);
		if (valueNode == NULL)
		{
			valueNode = FindChildByType(parameterNode, DAE_FXCMN_HALF_ELEMENT);
			effectParameterFloat->SetFloatType(FCDEffectParameterFloat::HALF);
		}
		else effectParameterFloat->SetFloatType(FCDEffectParameterFloat::FLOAT);
			
		const char* valueString = ReadNodeContentDirect(valueNode);
		if (valueString == NULL || *valueString == 0)
		{
			//return status.Fail(FS("Bad float value for float parameter: ") + TO_FSTRING(GetReference()), parameterNode->line);
			FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_BAD_FLOAT_PARAM, parameterNode->line);
		}
		effectParameterFloat->SetValue(FUStringConversion::ToFloat(valueString));
	}
	FArchiveXML::LoadAnimatable(&effectParameterFloat->GetValue(), parameterNode);
	
	effectParameterFloat->SetDirtyFlag();
	return status;
} 

bool FArchiveXML::LoadEffectParameterFloat2(FCDObject* object, xmlNode* parameterNode)
{ 
	if (!FArchiveXML::LoadEffectParameter(object, parameterNode)) return false;

	bool status = true;
	FCDEffectParameterFloat2* effectParameterFloat2 = (FCDEffectParameterFloat2*)object;
	xmlNode* valueNode = FindChildByType(parameterNode, DAE_FXCMN_FLOAT2_ELEMENT);
	if (valueNode == NULL)
	{
		valueNode = FindChildByType(parameterNode, DAE_FXCMN_HALF2_ELEMENT);
		effectParameterFloat2->SetFloatType(FCDEffectParameterFloat2::HALF);
	}
	else effectParameterFloat2->SetFloatType(FCDEffectParameterFloat2::FLOAT);
		
	const char* valueString = ReadNodeContentDirect(valueNode);
	if (valueString == NULL || *valueString == 0)
	{
		//return status.Fail(FS("Bad value for float2 parameter: ") + TO_FSTRING(GetReference()), parameterNode->line);
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_BAD_FLOAT_PARAM2, parameterNode->line);
	}
	effectParameterFloat2->SetValue(FUStringConversion::ToVector2(&valueString));
	effectParameterFloat2->SetDirtyFlag();
	return status;
} 

bool FArchiveXML::LoadEffectParameterFloat3(FCDObject* object, xmlNode* parameterNode)
{
	if (!FArchiveXML::LoadEffectParameter(object, parameterNode)) return false;

	bool status = true;
	FCDEffectParameterFloat3* effectParameterFloat3 = (FCDEffectParameterFloat3*)object;
	if (!effectParameterFloat3->IsAnimator())
	{
		xmlNode* valueNode = FindChildByType(parameterNode, DAE_FXCMN_FLOAT3_ELEMENT);
		if (valueNode == NULL)
		{
			valueNode = FindChildByType(parameterNode, DAE_FXCMN_HALF3_ELEMENT);
			effectParameterFloat3->SetFloatType(FCDEffectParameterFloat3::HALF);
		}
		else effectParameterFloat3->SetFloatType(FCDEffectParameterFloat3::FLOAT);
			
		const char* valueString = ReadNodeContentDirect(valueNode);
		if (valueString == NULL || *valueString == 0)
		{
			//return status.Fail(FS("Bad value for float3 parameter: ") + TO_FSTRING(GetReference()), parameterNode->line);
			FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_BAD_FLOAT_PARAM3, parameterNode->line);
		}
		effectParameterFloat3->SetValue(FUStringConversion::ToVector3(valueString));
	}
	FArchiveXML::LoadAnimatable(&effectParameterFloat3->GetValue(), parameterNode);
	
	effectParameterFloat3->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadEffectParameterInt(FCDObject* object, xmlNode* parameterNode)
{
	if (!FArchiveXML::LoadEffectParameter(object, parameterNode)) return false;

	bool status = true;
	FCDEffectParameterInt* effectParameterInt = (FCDEffectParameterInt*)object;
	xmlNode* valueNode = FindChildByType(parameterNode, DAE_FXCMN_INT_ELEMENT);
	const char* valueString = ReadNodeContentDirect(valueNode);
	if (valueString == NULL || *valueString == 0)
	{
		//return status.Fail(FS("Bad value for float parameter in integer parameter: ") + TO_FSTRING(GetReference()), parameterNode->line);
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_BAD_FLOAT_VALUE, parameterNode->line);
	}
	effectParameterInt->SetValue(FUStringConversion::ToInt32(valueString));
	effectParameterInt->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadEffectParameterMatrix(FCDObject* object, xmlNode* parameterNode)
{
	if (!FArchiveXML::LoadEffectParameter(object, parameterNode)) return false;

	bool status = true;
	FCDEffectParameterMatrix* effectParameterMatrix = (FCDEffectParameterMatrix*)object;
	xmlNode* valueNode = FindChildByType(parameterNode, DAE_FXCMN_FLOAT4X4_ELEMENT);
	if (valueNode == NULL)
	{
		valueNode = FindChildByType(parameterNode, DAE_FXCMN_HALF4X4_ELEMENT);
		effectParameterMatrix->SetFloatType(FCDEffectParameterMatrix::HALF);
	}
	else effectParameterMatrix->SetFloatType(FCDEffectParameterMatrix::FLOAT);
		
	const char* valueString = ReadNodeContentDirect(valueNode);
	if (valueString == NULL || *valueString == 0)
	{
		//return status.Fail(FS("Bad value for matrix parameter: ") + TO_FSTRING(GetReference()), parameterNode->line);
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_BAD_MATRIX, parameterNode->line);
	}
	FUStringConversion::ToMatrix(valueString, effectParameterMatrix->GetValue());
	effectParameterMatrix->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadEffectParameterString(FCDObject* object, xmlNode* parameterNode)
{
	if (!FArchiveXML::LoadEffectParameter(object, parameterNode)) return false;

	bool status = true;
	FCDEffectParameterString* effectParameterString = (FCDEffectParameterString*)object;
	xmlNode* valueNode = FindChildByType(parameterNode, DAE_FXCMN_STRING_ELEMENT);
	effectParameterString->SetValue(ReadNodeContentFull(valueNode));
	effectParameterString->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadEffectParameterVector(FCDObject* object, xmlNode* parameterNode)
{
	if (!FArchiveXML::LoadEffectParameter(object, parameterNode)) return false;

	bool status = true;
	FCDEffectParameterVector* effectParameterVector = (FCDEffectParameterVector*)object;
	if (!effectParameterVector->IsAnimator())
	{
		xmlNode* valueNode = FindChildByType(parameterNode, DAE_FXCMN_FLOAT4_ELEMENT);
		if (valueNode == NULL)
		{
			valueNode = FindChildByType(parameterNode, DAE_FXCMN_HALF4_ELEMENT);
			effectParameterVector->SetFloatType(FCDEffectParameterVector::HALF);
		}
		else effectParameterVector->SetFloatType(FCDEffectParameterVector::FLOAT);
			
		const char* valueString = ReadNodeContentDirect(valueNode);
		if (valueString == NULL || *valueString == 0)
		{
			//return status.Fail(FS("Bad value for float4 parameter: ") + TO_FSTRING(GetReference()), parameterNode->line);
			FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_BAD_FLOAT_PARAM4, parameterNode->line);
		}

		FMVector4 value;
		value.x = FUStringConversion::ToFloat(&valueString);
		value.y = FUStringConversion::ToFloat(&valueString);
		value.z = FUStringConversion::ToFloat(&valueString);
		value.w = FUStringConversion::ToFloat(&valueString);
		effectParameterVector->SetValue(value);
	}
	FArchiveXML::LoadAnimatable(&effectParameterVector->GetValue(), parameterNode);

	effectParameterVector->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadEffectParameterSampler(FCDObject* object, xmlNode* parameterNode)
{
	if (!FArchiveXML::LoadEffectParameter(object, parameterNode)) return false;

	bool status = true;
	FCDEffectParameterSampler* effectParameterSampler = (FCDEffectParameterSampler*)object;
	FCDEffectParameterSamplerData& data = FArchiveXML::documentLinkDataMap[effectParameterSampler->GetDocument()].effectParameterSamplerDataMap[effectParameterSampler];

	// Find the sampler node
	xmlNode* samplerNode = NULL;
	for (xmlNode* child = parameterNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_FXCMN_SAMPLER1D_ELEMENT)) { effectParameterSampler->SetSamplerType(FCDEffectParameterSampler::SAMPLER1D); samplerNode = child; break; }
		else if (IsEquivalent(child->name, DAE_FXCMN_SAMPLER2D_ELEMENT)) { effectParameterSampler->SetSamplerType(FCDEffectParameterSampler::SAMPLER2D); samplerNode = child; break; }
		else if (IsEquivalent(child->name, DAE_FXCMN_SAMPLER3D_ELEMENT)) { effectParameterSampler->SetSamplerType(FCDEffectParameterSampler::SAMPLER3D); samplerNode = child; break; }
		else if (IsEquivalent(child->name, DAE_FXCMN_SAMPLERCUBE_ELEMENT)) { effectParameterSampler->SetSamplerType(FCDEffectParameterSampler::SAMPLERCUBE); samplerNode = child; break; }
	}

	if (samplerNode == NULL)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_SAMPLER_NODE_MISSING, parameterNode->line);
		return status;
	}

	xmlNode* wrapNode = FindChildByType(samplerNode, DAE_WRAP_S_ELEMENT);
	if (wrapNode) effectParameterSampler->SetWrapS(FUDaeTextureWrapMode::FromString(ReadNodeContentDirect(wrapNode)));
	wrapNode = FindChildByType(samplerNode, DAE_WRAP_T_ELEMENT);
	if (wrapNode) effectParameterSampler->SetWrapT(FUDaeTextureWrapMode::FromString(ReadNodeContentDirect(wrapNode)));
	wrapNode = FindChildByType(samplerNode, DAE_WRAP_P_ELEMENT);
	if (wrapNode) effectParameterSampler->SetWrapP(FUDaeTextureWrapMode::FromString(ReadNodeContentDirect(wrapNode)));

	xmlNode* filterNode = FindChildByType(samplerNode, DAE_MIN_FILTER_ELEMENT);
	if (filterNode) effectParameterSampler->SetMinFilter(FUDaeTextureFilterFunction::FromString(ReadNodeContentDirect(filterNode)));
	filterNode = FindChildByType(samplerNode, DAE_MAG_FILTER_ELEMENT);
	if (filterNode) effectParameterSampler->SetMagFilter(FUDaeTextureFilterFunction::FromString(ReadNodeContentDirect(filterNode)));
	filterNode = FindChildByType(samplerNode, DAE_MIP_FILTER_ELEMENT);
	if (filterNode) effectParameterSampler->SetMipFilter(FUDaeTextureFilterFunction::FromString(ReadNodeContentDirect(filterNode)));

	// Parse the source node
	xmlNode* sourceNode = FindChildByType(samplerNode, DAE_SOURCE_ELEMENT);
	data.surfaceSid = ReadNodeContentDirect(sourceNode);
	if (data.surfaceSid.empty())
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_EMPTY_SURFACE_SOURCE, parameterNode->line);
		return status;
	}
	else
	{
		data.surfaceSid = FCDObjectWithId::CleanSubId(data.surfaceSid);
	}
	
	return status;
}

bool FArchiveXML::LoadEffectParameterSurface(FCDObject* object, xmlNode* parameterNode)
{
	if (!FArchiveXML::LoadEffectParameter(object, parameterNode)) return false;

	bool status = true;
	FCDEffectParameterSurface* effectParameterSurface = (FCDEffectParameterSurface*)object;
	xmlNode* surfaceNode = FindChildByType(parameterNode, DAE_FXCMN_SURFACE_ELEMENT);
    
    // Process the type attribute, as a string, since its usefulness is marginal.
    fm::string typeAttr = ReadNodeProperty(surfaceNode, DAE_TYPE_ATTRIBUTE);
    if (!typeAttr.empty()) effectParameterSurface->SetSurfaceType(typeAttr);

	bool initialized = false;
	xmlNode* valueNode = NULL;
	//The surface can now contain many init_from elements (1.4.1)
	xmlNodeList valueNodes;
	FindChildrenByType(surfaceNode, DAE_INITFROM_ELEMENT, valueNodes);
	for (xmlNodeList::iterator it = valueNodes.begin(); it != valueNodes.end(); ++it)
	{
		initialized = true;
		if (!effectParameterSurface->GetInitMethod())
			effectParameterSurface->SetInitMethod(new FCDEffectParameterSurfaceInitFrom());

		FCDEffectParameterSurfaceInitFrom* ptrInit = (FCDEffectParameterSurfaceInitFrom*)effectParameterSurface->GetInitMethod();
		//StringList names;
		FUStringConversion::ToStringList(ReadNodeContentDirect(*it), effectParameterSurface->GetNames());
		
		if (effectParameterSurface->GetNames().size() == 0 || effectParameterSurface->GetNames()[0].empty())
		{
			effectParameterSurface->GetNames().clear();
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_EMPTY_INIT_FROM, surfaceNode->line);
		}

		if (effectParameterSurface->GetNames().size() == 1) //might be 1.4.1, so allow the attributes mip, slice, face
		{
			if (HasNodeProperty(*it, DAE_MIP_ATTRIBUTE))
			{
				fm::string mip = ReadNodeProperty(*it, DAE_MIP_ATTRIBUTE);
				ptrInit->mip.push_back(mip);
			}
			if (HasNodeProperty(*it, DAE_SLICE_ATTRIBUTE))
			{
				fm::string slice = ReadNodeProperty(*it, DAE_SLICE_ATTRIBUTE);
				ptrInit->slice.push_back(slice);
			}
			if (HasNodeProperty(*it, DAE_FACE_ATTRIBUTE))
			{
				fm::string face = ReadNodeProperty(*it, DAE_FACE_ATTRIBUTE);
				ptrInit->face.push_back(face);
			}
		}
	}

	//Check if it's initialized AS NULL
	if (!initialized)
	{
		valueNode = FindChildByType(surfaceNode, DAE_INITASNULL_ELEMENT);
		if (valueNode)
		{
			initialized = true;
			effectParameterSurface->SetInitMethod(FCDEffectParameterSurfaceInitFactory::Create(FCDEffectParameterSurfaceInitFactory::AS_NULL));
		}
	}
	//Check if it's initialized AS TARGET
	if (!initialized)
	{
		valueNode = FindChildByType(surfaceNode, DAE_INITASTARGET_ELEMENT);
		if (valueNode)
		{
			initialized = true;
			effectParameterSurface->SetInitMethod(FCDEffectParameterSurfaceInitFactory::Create(FCDEffectParameterSurfaceInitFactory::AS_TARGET));
		}
	}
	//Check if it's initialized AS CUBE
	if (!initialized)
	{
		valueNode = FindChildByType(surfaceNode, DAE_INITCUBE_ELEMENT);
		if (valueNode)
		{
			initialized = true;
			effectParameterSurface->SetInitMethod(FCDEffectParameterSurfaceInitFactory::Create(FCDEffectParameterSurfaceInitFactory::CUBE));
			FCDEffectParameterSurfaceInitCube* ptrInit = (FCDEffectParameterSurfaceInitCube*) effectParameterSurface->GetInitMethod();

			//Check if it's an ALL reference
			xmlNode* refNode = FindChildByType(valueNode, DAE_ALL_ELEMENT);
			if (refNode)
			{
				ptrInit->cubeType = FCDEffectParameterSurfaceInitCube::ALL;
				fm::string name = ReadNodeProperty(refNode, DAE_REF_ATTRIBUTE);
				if (name.empty())
				{
					FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_EMPTY_IMAGE_NAME, surfaceNode->line);
				}
				else effectParameterSurface->GetNames().push_back(name);
			}

			//Check if it's a PRIMARY reference
			if (!refNode)
			{
				refNode = FindChildByType(valueNode, DAE_PRIMARY_ELEMENT);
				if (refNode)
				{
					ptrInit->cubeType = FCDEffectParameterSurfaceInitCube::PRIMARY;
					fm::string name = ReadNodeProperty(refNode, DAE_REF_ATTRIBUTE);
					if (name.empty())
					{
						FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_EMPTY_IMAGE_NAME, surfaceNode->line);
					}
					else effectParameterSurface->GetNames().push_back(name);
	
					xmlNode* orderNode = FindChildByType(refNode, DAE_ORDER_ELEMENT);
					if (orderNode)
					{
						//FIXME: complete when the spec has more info
					}
				}
			}

			//Check if it's a FACE reference
			if (!refNode)
			{
				xmlNodeList faceNodes;
				FindChildrenByType(valueNode, DAE_FACE_ELEMENT, faceNodes);
				if (faceNodes.size()==6)
				{
					ptrInit->cubeType = FCDEffectParameterSurfaceInitCube::FACE;
					for (uint8 ii=0; ii<faceNodes.size(); ii++)
					{
						fm::string name = ReadNodeProperty(faceNodes[ii], DAE_REF_ATTRIBUTE);
						if (name.empty())
						{
							FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_EMPTY_IMAGE_NAME, surfaceNode->line);
						}
						else effectParameterSurface->GetNames().push_back(name);
					}
				}
			}
		}
	}

	//Check if it's initialized AS VOLUME
	if (!initialized)
	{
		valueNode = FindChildByType(surfaceNode, DAE_INITVOLUME_ELEMENT);
		if (valueNode)
		{
			initialized = true;
			effectParameterSurface->SetInitMethod(FCDEffectParameterSurfaceInitFactory::Create(FCDEffectParameterSurfaceInitFactory::VOLUME));
			FCDEffectParameterSurfaceInitVolume* ptrInit = (FCDEffectParameterSurfaceInitVolume*) effectParameterSurface->GetInitMethod();

			//Check if it's an ALL reference
			xmlNode* refNode = FindChildByType(valueNode, DAE_ALL_ELEMENT);
			if (refNode)
			{
				ptrInit->volumeType = FCDEffectParameterSurfaceInitVolume::ALL;
				fm::string name = ReadNodeProperty(refNode, DAE_REF_ATTRIBUTE);
				if (name.empty())
				{
					FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_EMPTY_IMAGE_NAME, surfaceNode->line);
				}
				else effectParameterSurface->GetNames().push_back(name);
			}

			//Check if it's a PRIMARY reference
			if (!refNode)
			{
				refNode = FindChildByType(valueNode, DAE_PRIMARY_ELEMENT);
				if (refNode)
				{
					ptrInit->volumeType = FCDEffectParameterSurfaceInitVolume::PRIMARY;
					fm::string name = ReadNodeProperty(refNode, DAE_REF_ATTRIBUTE);
					if (name.empty())
					{
						FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_EMPTY_IMAGE_NAME, surfaceNode->line);
					}
					else effectParameterSurface->GetNames().push_back(name);
				}
			}
		}
	}

	//Check if it's initialized as PLANAR
	if (!initialized)
	{
		valueNode = FindChildByType(surfaceNode, DAE_INITPLANAR_ELEMENT);
		if (valueNode)
		{
			initialized = true;
			effectParameterSurface->SetInitMethod(FCDEffectParameterSurfaceInitFactory::Create(FCDEffectParameterSurfaceInitFactory::PLANAR));

			//Check if it's an ALL reference
			xmlNode* refNode = FindChildByType(valueNode, DAE_ALL_ELEMENT);
			if (refNode)
			{
				fm::string name = ReadNodeProperty(refNode, DAE_REF_ATTRIBUTE);
				if (name.empty())
				{
					FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_EMPTY_IMAGE_NAME, surfaceNode->line);
				}
				else effectParameterSurface->GetNames().push_back(name);
			}
		}
	}
	
	// It is acceptable for a surface not to have an initialization option
	//but we should flag a warning
	if (!initialized)
	{
		WARNING_OUT("Warning: surface %s not initialized", effectParameterSurface->GetReference().c_str());
	}
	
	xmlNode* sizeNode = FindChildByType(surfaceNode, DAE_SIZE_ELEMENT);
	effectParameterSurface->SetSize(FUStringConversion::ToVector3(ReadNodeContentDirect(sizeNode)));
	xmlNode* viewportRatioNode = FindChildByType(surfaceNode, DAE_VIEWPORT_RATIO);
	effectParameterSurface->SetViewportRatio(FUStringConversion::ToFloat(ReadNodeContentDirect(viewportRatioNode)));
	xmlNode* mipLevelsNode = FindChildByType(surfaceNode, DAE_MIP_LEVELS);
	effectParameterSurface->SetMipLevelCount((uint16) FUStringConversion::ToInt32(ReadNodeContentDirect(mipLevelsNode)));
	xmlNode* mipmapGenerateNode = FindChildByType(surfaceNode, DAE_MIPMAP_GENERATE);
	effectParameterSurface->SetGenerateMipMaps(FUStringConversion::ToBoolean(ReadNodeContentDirect(mipmapGenerateNode)));

	xmlNode* formatNode = FindChildByType(surfaceNode, DAE_FORMAT_ELEMENT);
	if (formatNode)
		effectParameterSurface->SetFormat(FUStringConversion::ToString(ReadNodeContentDirect(formatNode)));
	
	xmlNode* formatHintNode = FindChildByType(surfaceNode, DAE_FORMAT_HINT_ELEMENT);
	if (formatHintNode)
	{
		FCDFormatHint* formatHint = effectParameterSurface->AddFormatHint();
		
		xmlNode* hintChild = FindChildByType(formatHintNode, DAE_CHANNELS_ELEMENT);
		if (!hintChild)
		{
			WARNING_OUT("Warning: surface %s misses channel information in its format hint.", effectParameterSurface->GetReference().c_str());
			formatHint->channels = FCDFormatHint::CHANNEL_UNKNOWN;
		}
		else
		{
			fm::string contents = FUStringConversion::ToString(ReadNodeContentDirect(hintChild));
			if (IsEquivalent(contents, DAE_FORMAT_HINT_RGB_VALUE)) formatHint->channels = FCDFormatHint::CHANNEL_RGB;
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_RGBA_VALUE)) formatHint->channels = FCDFormatHint::CHANNEL_RGBA;
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_L_VALUE)) formatHint->channels = FCDFormatHint::CHANNEL_L;
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_LA_VALUE)) formatHint->channels = FCDFormatHint::CHANNEL_LA;
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_D_VALUE)) formatHint->channels = FCDFormatHint::CHANNEL_D;
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_XYZ_VALUE)) formatHint->channels = FCDFormatHint::CHANNEL_XYZ;
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_XYZW_VALUE)) formatHint->channels = FCDFormatHint::CHANNEL_XYZW;
			else
			{
				WARNING_OUT("Warning: surface %s contains an invalid channel description %s in its format hint.", effectParameterSurface->GetReference().c_str(), contents.c_str());
				formatHint->channels = FCDFormatHint::CHANNEL_UNKNOWN;
			}
		}
		
		hintChild = FindChildByType(formatHintNode, DAE_RANGE_ELEMENT);
		if (!hintChild)
		{
			WARNING_OUT("Warning: surface %s misses range information in its format hint.", effectParameterSurface->GetReference().c_str());
			formatHint->range = FCDFormatHint::RANGE_UNKNOWN;
		}
		else
		{
			fm::string contents = FUStringConversion::ToString(ReadNodeContentDirect(hintChild));
			if (IsEquivalent(contents, DAE_FORMAT_HINT_SNORM_VALUE)) formatHint->range = FCDFormatHint::RANGE_SNORM;
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_UNORM_VALUE)) formatHint->range = FCDFormatHint::RANGE_UNORM;
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_SINT_VALUE)) formatHint->range = FCDFormatHint::RANGE_SINT;
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_UINT_VALUE)) formatHint->range = FCDFormatHint::RANGE_UINT;
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_FLOAT_VALUE)) formatHint->range = FCDFormatHint::RANGE_FLOAT;
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_LOW_VALUE)) formatHint->range = FCDFormatHint::RANGE_LOW;
			else
			{
				WARNING_OUT("Warning: surface %s contains an invalid range description %s in its format hint.", effectParameterSurface->GetReference().c_str(), contents.c_str());
				formatHint->range= FCDFormatHint::RANGE_UNKNOWN;
			}
		}

		hintChild = FindChildByType(formatHintNode, DAE_PRECISION_ELEMENT);
		if (hintChild)
		{
			fm::string contents = FUStringConversion::ToString(ReadNodeContentDirect(hintChild));
			if (IsEquivalent(contents, DAE_FORMAT_HINT_LOW_VALUE)) formatHint->precision = FCDFormatHint::PRECISION_LOW;
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_MID_VALUE)) formatHint->precision = FCDFormatHint::PRECISION_MID;
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_HIGH_VALUE)) formatHint->precision = FCDFormatHint::PRECISION_HIGH;
			else
			{
				WARNING_OUT("Warning: surface %s contains an invalid precision description %s in its format hint.", effectParameterSurface->GetReference().c_str(), contents.c_str());
				formatHint->precision = FCDFormatHint::PRECISION_UNKNOWN;
			}
		}

		xmlNodeList optionNodes;
		FindChildrenByType(formatHintNode, DAE_OPTION_ELEMENT, optionNodes);
		for (xmlNodeList::iterator it = optionNodes.begin(); it != optionNodes.end(); ++it)
		{
			fm::string contents = FUStringConversion::ToString(ReadNodeContentDirect(*it));
			if (IsEquivalent(contents, DAE_FORMAT_HINT_SRGB_GAMMA_VALUE)) formatHint->options.push_back(FCDFormatHint::OPT_SRGB_GAMMA);
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_NORMALIZED3_VALUE)) formatHint->options.push_back(FCDFormatHint::OPT_NORMALIZED3);
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_NORMALIZED4_VALUE)) formatHint->options.push_back(FCDFormatHint::OPT_NORMALIZED4);
			else if (IsEquivalent(contents, DAE_FORMAT_HINT_COMPRESSABLE_VALUE)) formatHint->options.push_back(FCDFormatHint::OPT_COMPRESSABLE);
			else 
			{
				WARNING_OUT("Warning: surface %s contains an invalid option description %s in its format hint.", effectParameterSurface->GetReference().c_str(), contents.c_str());
			}
		}
	}

	effectParameterSurface->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadEffectPass(FCDObject* object, xmlNode* passNode)			
{
	FCDEffectPass* effectPass = (FCDEffectPass*)object;

	bool status = true;
	if (!IsEquivalent(passNode->name, DAE_PASS_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_PASS_ELEMENT, passNode->line);
		return status;
	}
	effectPass->SetPassName(TO_FSTRING(ReadNodeProperty(passNode, DAE_SID_ATTRIBUTE)));

	// Iterate over the pass nodes, looking for render states and <shader> elements, in any order.
	for (xmlNode* child = passNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;
		
		// Check for a render state type.
		FUDaePassState::State stateType = FUDaePassState::FromString((const char*) child->name);
		if (stateType != FUDaePassState::INVALID)
		{
			FCDEffectPassState* state = effectPass->AddRenderState(stateType);
			status &= FArchiveXML::LoadEffectPassState(state, child);
		}

		// Look for the <shader> element.
		else if (IsEquivalent(child->name, DAE_SHADER_ELEMENT))
		{
			FCDEffectPassShader* shader = effectPass->AddShader();
			status &= FArchiveXML::LoadEffectPassShader(shader, child);
		}
	}

	effectPass->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadEffectPassShader(FCDObject* object, xmlNode* shaderNode)	
{
	FCDEffectPassShader* effectPassShader = (FCDEffectPassShader*)object;

	bool status = true;
	if (!IsEquivalent(shaderNode->name, DAE_SHADER_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_PASS_SHADER_ELEMENT, shaderNode->line);
		return status;
	}

	// Read in the shader's name and stage
	xmlNode* nameNode = FindChildByType(shaderNode, DAE_FXCMN_NAME_ELEMENT);
	effectPassShader->SetName(ReadNodeContentFull(nameNode));
	fm::string codeSource = ReadNodeProperty(nameNode, DAE_SOURCE_ATTRIBUTE);
	if (effectPassShader->GetName().empty())
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNAMED_EFFECT_PASS_SHADER, shaderNode->line);
		return status;
	}
    
	fm::string stage = ReadNodeStage(shaderNode);
	bool isFragment = (stage == DAE_FXCMN_FRAGMENT_SHADER) || (stage == DAE_FXGLSL_FRAGMENT_SHADER);
    bool isVertex = (stage == DAE_FXCMN_VERTEX_SHADER) || (stage == DAE_FXGLSL_VERTEX_SHADER);
	if (isFragment) effectPassShader->AffectsFragments();
	else if (isVertex) effectPassShader->AffectsVertices();
	else
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_EPS_STAGE, shaderNode->line);
		return status;
	}

	// Look-up the code filename for this shader, if available
	effectPassShader->SetCode(effectPassShader->GetParent()->GetParent()->FindCode(codeSource));
	if (effectPassShader->GetCode() == NULL) effectPassShader->SetCode(effectPassShader->GetParent()->GetParent()->GetParent()->FindCode(codeSource));

	// Read in the compiler-related elements
	xmlNode* compilerTargetNode = FindChildByType(shaderNode, DAE_FXCMN_COMPILERTARGET_ELEMENT);
	effectPassShader->SetCompilerTarget(TO_FSTRING(ReadNodeContentFull(compilerTargetNode)));
	xmlNode* compilerOptionsNode = FindChildByType(shaderNode, DAE_FXCMN_COMPILEROPTIONS_ELEMENT);
	effectPassShader->SetCompilerOptions(TO_FSTRING(ReadNodeContentFull(compilerOptionsNode)));

	// Read in the bind parameters
	xmlNodeList bindNodes;
	FindChildrenByType(shaderNode, DAE_FXCMN_BIND_ELEMENT, bindNodes);
	for (xmlNodeList::iterator itB = bindNodes.begin(); itB != bindNodes.end(); ++itB)
	{
		xmlNode* paramNode = FindChildByType(*itB, DAE_PARAMETER_ELEMENT);

		FCDEffectPassBind* bind = effectPassShader->AddBinding();
		bind->symbol = ReadNodeProperty((*itB), DAE_SYMBOL_ATTRIBUTE);
		bind->reference = ReadNodeProperty(paramNode, DAE_REF_ATTRIBUTE);
	}

	effectPassShader->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadEffectPassState(FCDObject* object, xmlNode* stateNode)		
{
	FCDEffectPassState* effectPassState = (FCDEffectPassState*)object;

	bool status = true;

#define NODE_TYPE(offset, node, valueType, convFn) \
	if (node != NULL && HasNodeProperty(node, DAE_VALUE_ATTRIBUTE)) { \
		*((valueType*)(effectPassState->GetData() + offset)) = (valueType) FUStringConversion::convFn(ReadNodeProperty(node, DAE_VALUE_ATTRIBUTE)); } 
#define NODE_INDEX(offset, node) \
	if (node != NULL && HasNodeProperty(node, DAE_INDEX_ATTRIBUTE)) { \
		*((uint8*)(effectPassState->GetData() + offset)) = (uint8) FUStringConversion::ToUInt32(ReadNodeProperty(node, DAE_INDEX_ATTRIBUTE)); } 
#define NODE_ENUM(offset, node, nameSpace) \
	if (node != NULL && HasNodeProperty(node, DAE_VALUE_ATTRIBUTE)) { \
		*((uint32*)(effectPassState->GetData() + offset)) = (uint32) nameSpace::FromString(ReadNodeProperty(node, DAE_VALUE_ATTRIBUTE)); } 

#define CHILD_NODE_TYPE(offset, elementName, valueType, convFn) { \
	xmlNode* node = FindChildByType(stateNode, elementName); \
	NODE_TYPE(offset, node, valueType, convFn); }
#define CHILD_NODE_ENUM(offset, elementName, nameSpace) { \
	xmlNode* node = FindChildByType(stateNode, elementName); \
	NODE_ENUM(offset, node, nameSpace); }

	switch (effectPassState->GetType())
	{
	case FUDaePassState::ALPHA_FUNC:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FUNC_ELEMENT, FUDaePassStateFunction);
		CHILD_NODE_TYPE(4, DAE_FXSTD_STATE_VALUE_ELEMENT, float, ToFloat);
		break;

	case FUDaePassState::BLEND_FUNC:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_SRC_ELEMENT, FUDaePassStateBlendType);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_DEST_ELEMENT, FUDaePassStateBlendType);
		break;

	case FUDaePassState::BLEND_FUNC_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_SRCRGB_ELEMENT, FUDaePassStateBlendType);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_DESTRGB_ELEMENT, FUDaePassStateBlendType);
		CHILD_NODE_ENUM(8, DAE_FXSTD_STATE_SRCALPHA_ELEMENT, FUDaePassStateBlendType);
		CHILD_NODE_ENUM(12, DAE_FXSTD_STATE_DESTALPHA_ELEMENT, FUDaePassStateBlendType);
		break;

	case FUDaePassState::BLEND_EQUATION:
		NODE_ENUM(0, stateNode, FUDaePassStateBlendEquation);
		break;

	case FUDaePassState::BLEND_EQUATION_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_RGB_ELEMENT, FUDaePassStateBlendEquation);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_ALPHA_ELEMENT, FUDaePassStateBlendEquation);
		break;

	case FUDaePassState::COLOR_MATERIAL:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_MODE_ELEMENT, FUDaePassStateMaterialType);
		break;

	case FUDaePassState::CULL_FACE:
		NODE_ENUM(0, stateNode, FUDaePassStateFaceType);
		break;

	case FUDaePassState::DEPTH_FUNC:
		NODE_ENUM(0, stateNode, FUDaePassStateFunction);
		break;

	case FUDaePassState::FOG_MODE:
		NODE_ENUM(0, stateNode, FUDaePassStateFogType);
		break;

	case FUDaePassState::FOG_COORD_SRC:
		NODE_ENUM(0, stateNode, FUDaePassStateFogCoordinateType);
		break;

	case FUDaePassState::FRONT_FACE:
		NODE_ENUM(0, stateNode, FUDaePassStateFrontFaceType);
		break;

	case FUDaePassState::LIGHT_MODEL_COLOR_CONTROL:
		NODE_ENUM(0, stateNode, FUDaePassStateLightModelColorControlType);
		break;

	case FUDaePassState::LOGIC_OP:
		NODE_ENUM(0, stateNode, FUDaePassStateLogicOperation);
		break;

	case FUDaePassState::POLYGON_MODE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_MODE_ELEMENT, FUDaePassStatePolygonMode);
		break;

	case FUDaePassState::SHADE_MODEL:
		NODE_ENUM(0, stateNode, FUDaePassStateShadeModel);
		break;

	case FUDaePassState::STENCIL_FUNC:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FUNC_ELEMENT, FUDaePassStateFunction);
		CHILD_NODE_TYPE(4, DAE_FXSTD_STATE_REF_ELEMENT, uint8, ToUInt32);
		CHILD_NODE_TYPE(5, DAE_FXSTD_STATE_MASK_ELEMENT, uint8, ToUInt32);
		break;

	case FUDaePassState::STENCIL_OP:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FAIL_ELEMENT, FUDaePassStateStencilOperation);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_ZFAIL_ELEMENT, FUDaePassStateStencilOperation);
		CHILD_NODE_ENUM(8, DAE_FXSTD_STATE_ZPASS_ELEMENT, FUDaePassStateStencilOperation);
		break;

	case FUDaePassState::STENCIL_FUNC_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FRONT_ELEMENT, FUDaePassStateFunction);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_BACK_ELEMENT, FUDaePassStateFunction);
		CHILD_NODE_TYPE(8, DAE_FXSTD_STATE_REF_ELEMENT, uint8, ToUInt32);
		CHILD_NODE_TYPE(9, DAE_FXSTD_STATE_MASK_ELEMENT, uint8, ToUInt32);
		break;

	case FUDaePassState::STENCIL_OP_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_FAIL_ELEMENT, FUDaePassStateStencilOperation);
		CHILD_NODE_ENUM(8, DAE_FXSTD_STATE_ZFAIL_ELEMENT, FUDaePassStateStencilOperation);
		CHILD_NODE_ENUM(12, DAE_FXSTD_STATE_ZPASS_ELEMENT, FUDaePassStateStencilOperation);
		break;

	case FUDaePassState::STENCIL_MASK_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType);
		CHILD_NODE_TYPE(4, DAE_FXSTD_STATE_MASK_ELEMENT, uint8, ToUInt32);
		break;

	case FUDaePassState::LIGHT_AMBIENT:
	case FUDaePassState::LIGHT_DIFFUSE:
	case FUDaePassState::LIGHT_SPECULAR:
	case FUDaePassState::LIGHT_POSITION:
	case FUDaePassState::TEXTURE_ENV_COLOR:
	case FUDaePassState::CLIP_PLANE:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, FMVector4, ToVector4);
		break;

	case FUDaePassState::LIGHT_CONSTANT_ATTENUATION:
	case FUDaePassState::LIGHT_LINEAR_ATTENUATION:
	case FUDaePassState::LIGHT_QUADRATIC_ATTENUATION:
	case FUDaePassState::LIGHT_SPOT_CUTOFF:
	case FUDaePassState::LIGHT_SPOT_EXPONENT:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, float, ToFloat);
		break;

	case FUDaePassState::LIGHT_SPOT_DIRECTION:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, FMVector3, ToVector3);
		break;

	case FUDaePassState::TEXTURE1D:
	case FUDaePassState::TEXTURE2D:
	case FUDaePassState::TEXTURE3D:
	case FUDaePassState::TEXTURECUBE:
	case FUDaePassState::TEXTURERECT:
	case FUDaePassState::TEXTUREDEPTH:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, uint32, ToUInt32);
		break;

	case FUDaePassState::LIGHT_ENABLE:
	case FUDaePassState::TEXTURE1D_ENABLE:
	case FUDaePassState::TEXTURE2D_ENABLE:
	case FUDaePassState::TEXTURE3D_ENABLE:
	case FUDaePassState::TEXTURECUBE_ENABLE:
	case FUDaePassState::TEXTURERECT_ENABLE:
	case FUDaePassState::TEXTUREDEPTH_ENABLE:
	case FUDaePassState::CLIP_PLANE_ENABLE:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, bool, ToBoolean);
		break;

	case FUDaePassState::TEXTURE_ENV_MODE: {
		NODE_INDEX(0, stateNode);
		fm::string value = ReadNodeProperty(stateNode, DAE_VALUE_ATTRIBUTE);
		memcpy(effectPassState->GetData() + 1, value.c_str(), min(value.size(), (size_t) 255));
		effectPassState->GetData()[255] = 0;
		break; }

	case FUDaePassState::BLEND_COLOR:
	case FUDaePassState::CLEAR_COLOR:
	case FUDaePassState::FOG_COLOR:
	case FUDaePassState::LIGHT_MODEL_AMBIENT:
	case FUDaePassState::MATERIAL_AMBIENT:
	case FUDaePassState::MATERIAL_DIFFUSE:
	case FUDaePassState::MATERIAL_EMISSION:
	case FUDaePassState::MATERIAL_SPECULAR:
	case FUDaePassState::SCISSOR:
		NODE_TYPE(0, stateNode, FMVector4, ToVector4);
		break;

	case FUDaePassState::POINT_DISTANCE_ATTENUATION:
		NODE_TYPE(0, stateNode, FMVector3, ToVector3);
		break;

	case FUDaePassState::DEPTH_BOUNDS:
	case FUDaePassState::DEPTH_RANGE:
	case FUDaePassState::POLYGON_OFFSET:
		NODE_TYPE(0, stateNode, FMVector2, ToVector2);
		break;

	case FUDaePassState::CLEAR_STENCIL:
	case FUDaePassState::STENCIL_MASK:
		NODE_TYPE(0, stateNode, uint32, ToUInt32);
		break;

	case FUDaePassState::CLEAR_DEPTH:
	case FUDaePassState::FOG_DENSITY:
	case FUDaePassState::FOG_START:
	case FUDaePassState::FOG_END:
	case FUDaePassState::LINE_WIDTH:
	case FUDaePassState::MATERIAL_SHININESS:
	case FUDaePassState::POINT_FADE_THRESHOLD_SIZE:
	case FUDaePassState::POINT_SIZE:
	case FUDaePassState::POINT_SIZE_MIN:
	case FUDaePassState::POINT_SIZE_MAX:
		NODE_TYPE(0, stateNode, float, ToFloat);
		break;

	case FUDaePassState::COLOR_MASK: {
		fm::string value = ReadNodeProperty(stateNode, DAE_VALUE_ATTRIBUTE);
		BooleanList values;
		FUStringConversion::ToBooleanList(value, values);
		if (values.size() >= 4)
		{
			*(bool*)(effectPassState->GetData() + 0) = values[0];
			*(bool*)(effectPassState->GetData() + 1) = values[1];
			*(bool*)(effectPassState->GetData() + 2) = values[2];
			*(bool*)(effectPassState->GetData() + 3) = values[3];
		}
		break; }

	case FUDaePassState::LINE_STIPPLE: {
		fm::string value = ReadNodeProperty(stateNode, DAE_VALUE_ATTRIBUTE);
		UInt32List values;
		FUStringConversion::ToUInt32List(value, values);
		if (values.size() >= 2)
		{
			*(uint16*)(effectPassState->GetData() + 0) = (uint16) values[0];
			*(uint16*)(effectPassState->GetData() + 2) = (uint16) values[1];
		}
		break; }

	case FUDaePassState::MODEL_VIEW_MATRIX:
	case FUDaePassState::PROJECTION_MATRIX:
		NODE_TYPE(0, stateNode, FMMatrix44, ToMatrix);
		break;

	case FUDaePassState::LIGHTING_ENABLE:
	case FUDaePassState::ALPHA_TEST_ENABLE:
	case FUDaePassState::AUTO_NORMAL_ENABLE:
	case FUDaePassState::BLEND_ENABLE:
	case FUDaePassState::COLOR_LOGIC_OP_ENABLE:
	case FUDaePassState::COLOR_MATERIAL_ENABLE:
	case FUDaePassState::CULL_FACE_ENABLE:
	case FUDaePassState::DEPTH_BOUNDS_ENABLE:
	case FUDaePassState::DEPTH_CLAMP_ENABLE:
	case FUDaePassState::DEPTH_TEST_ENABLE:
	case FUDaePassState::DITHER_ENABLE:
	case FUDaePassState::FOG_ENABLE:
	case FUDaePassState::LIGHT_MODEL_LOCAL_VIEWER_ENABLE:
	case FUDaePassState::LIGHT_MODEL_TWO_SIDE_ENABLE:
	case FUDaePassState::LINE_SMOOTH_ENABLE:
	case FUDaePassState::LINE_STIPPLE_ENABLE:
	case FUDaePassState::LOGIC_OP_ENABLE:
	case FUDaePassState::MULTISAMPLE_ENABLE:
	case FUDaePassState::NORMALIZE_ENABLE:
	case FUDaePassState::POINT_SMOOTH_ENABLE:
	case FUDaePassState::POLYGON_OFFSET_FILL_ENABLE:
	case FUDaePassState::POLYGON_OFFSET_LINE_ENABLE:
	case FUDaePassState::POLYGON_OFFSET_POINT_ENABLE:
	case FUDaePassState::POLYGON_SMOOTH_ENABLE:
	case FUDaePassState::POLYGON_STIPPLE_ENABLE:
	case FUDaePassState::RESCALE_NORMAL_ENABLE:
	case FUDaePassState::SAMPLE_ALPHA_TO_COVERAGE_ENABLE:
	case FUDaePassState::SAMPLE_ALPHA_TO_ONE_ENABLE:
	case FUDaePassState::SAMPLE_COVERAGE_ENABLE:
	case FUDaePassState::SCISSOR_TEST_ENABLE:
	case FUDaePassState::STENCIL_TEST_ENABLE:
	case FUDaePassState::DEPTH_MASK:
		NODE_TYPE(0, stateNode, bool, ToBoolean);
		break;

	default:
		status = false;
		break;
	}

#undef NODE_TYPE
#undef NODE_INDEX
#undef NODE_ENUM
#undef CHILD_NODE_TYPE
#undef CHILD_NODE_ENUM

	return status;
}

bool FArchiveXML::LoadEffectProfile(FCDObject* object, xmlNode* profileNode)		
{
	FCDEffectProfile* effectProfile = (FCDEffectProfile*)object;

	bool status = true;

	// Verify that we are given a valid XML input node.
	const char* profileName = FUDaeProfileType::ToString(effectProfile->GetType());
	if (!IsEquivalent(profileNode->name, profileName))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_PROFILE_INPUT_NODE, profileNode->line);
		return status;
	}

	// Parse in the child elements: parameters and techniques
	for (xmlNode* child = profileNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_FXCMN_NEWPARAM_ELEMENT))
		{
			FCDEffectParameter* parameter = effectProfile->AddEffectParameter(FArchiveXML::GetEffectParameterType(child));
			status &= FArchiveXML::LoadSwitch(parameter, &parameter->GetObjectType(), child);
		}
		else if (IsEquivalent(child->name, DAE_IMAGE_ELEMENT))
		{
			// You can create images in the profile: tell the image library about it.
			FCDImage* image = effectProfile->GetDocument()->GetImageLibrary()->AddEntity();
			status &= (FArchiveXML::LoadImage(image, child));
		}
		else if (IsEquivalent(child->name, DAE_EXTRA_ELEMENT))
		{
			FArchiveXML::LoadExtra(effectProfile->GetExtra(), child);
		}
	}

	effectProfile->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadEffectProfileFX(FCDObject* object, xmlNode* profileNode)		
{
	if (!FArchiveXML::LoadEffectProfile(object, profileNode)) return false;

	bool status = true;
	FCDEffectProfileFX* effectProfileFX = (FCDEffectProfileFX*)object;

	// Read in the target platform for this effect profile
	effectProfileFX->SetPlatform(TO_FSTRING(ReadNodeProperty(profileNode, DAE_PLATFORM_ATTRIBUTE)));

	// Parse in the child technique/code/include elements.
	for (xmlNode* child = profileNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_TECHNIQUE_ELEMENT))
		{
			FCDEffectTechnique* technique = effectProfileFX->AddTechnique();
			status &= (FArchiveXML::LoadEffectTechnique(technique, child));
		}
		else if (IsEquivalent(child->name, DAE_FXCMN_CODE_ELEMENT) || IsEquivalent(child->name, DAE_FXCMN_INCLUDE_ELEMENT))
		{
			FCDEffectCode* code = effectProfileFX->AddCode();
			status &= (FArchiveXML::LoadEffectCode(code, child));
		}
	}
	
	effectProfileFX->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadEffectStandard(FCDObject* object, xmlNode* baseNode)		
{
	if (!FArchiveXML::LoadEffectProfile(object, baseNode)) return false;

	bool status = true;
	FCDEffectStandard* effectStandard = (FCDEffectStandard*)object;

	// Find the node with the Max/Maya/FC-specific parameters
	xmlNode* maxParameterNode = NULL;
	xmlNode* mayaParameterNode = NULL;
	xmlNode* fcParameterNode = NULL;

	// Bump the base node up the first <technique> element
	xmlNode* techniqueNode = FindChildByType(baseNode, DAE_TECHNIQUE_ELEMENT);
	if (techniqueNode == NULL)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_TECHNIQUE_MISSING, baseNode->line);
		return status;
	}
	baseNode = techniqueNode;

	//Look for <newparam>'s at this level also, and add them to the profile's parameters list
	for (xmlNode* child = baseNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_FXCMN_NEWPARAM_ELEMENT))
		{
			FCDEffectParameter* parameter = effectStandard->AddEffectParameter(FArchiveXML::GetEffectParameterType(child));
			status &= FArchiveXML::LoadSwitch(parameter, &parameter->GetObjectType(), child);
		}
	}


	// Look for an <extra><technique> node for Max-specific parameter
	xmlNode* extraNode = FindChildByType(baseNode, DAE_EXTRA_ELEMENT);
	maxParameterNode = FindTechnique(extraNode, DAEMAX_MAX_PROFILE);
	mayaParameterNode = FindTechnique(extraNode, DAEMAYA_MAYA_PROFILE);
	fcParameterNode = FindTechnique(extraNode, DAE_FCOLLADA_PROFILE);

	// Parse the material's program node and figure out the correct shader type
	// Either <phong>, <lambert> or <constant> are expected
	xmlNode* commonParameterNode = NULL;
	for (commonParameterNode = baseNode->children; commonParameterNode != NULL; commonParameterNode = commonParameterNode->next)
	{
		if (commonParameterNode->type != XML_ELEMENT_NODE) continue;
		if (IsEquivalent(commonParameterNode->name, DAE_FXSTD_CONSTANT_ELEMENT)) { effectStandard->SetLightingType(FCDEffectStandard::CONSTANT); break; }
		else if (IsEquivalent(commonParameterNode->name, DAE_FXSTD_LAMBERT_ELEMENT)) { effectStandard->SetLightingType(FCDEffectStandard::LAMBERT); break; }
		else if (IsEquivalent(commonParameterNode->name, DAE_FXSTD_PHONG_ELEMENT)) { effectStandard->SetLightingType(FCDEffectStandard::PHONG); break; }
		else if (IsEquivalent(commonParameterNode->name, DAE_FXSTD_BLINN_ELEMENT)) { effectStandard->SetLightingType(FCDEffectStandard::BLINN); break; }
	}
	if (commonParameterNode == NULL)
	{
		//return status.Fail(FS("Unable to find the program node for standard effect: ") + TO_FSTRING(GetDaeId()), baseNode->line);
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_PROG_NODE_MISSING, baseNode->line);
	}

	bool hasTranslucency = false, hasReflectivity = false;
	bool hasRefractive = false;

	// Read in the parameters for the common program types and apply them to the shader
	StringList parameterNames;
	xmlNodeList parameterNodes;
	FindParameters(commonParameterNode, parameterNames, parameterNodes);
	FindParameters(maxParameterNode, parameterNames, parameterNodes);
	FindParameters(mayaParameterNode, parameterNames, parameterNodes);
	FindParameters(fcParameterNode, parameterNames, parameterNodes);
	size_t parameterCount = parameterNodes.size();
	for (size_t i = 0; i < parameterCount; ++i)
	{
		xmlNode* parameterNode = parameterNodes[i];
		const fm::string& parameterName = parameterNames[i];
		const char* parameterContent = ReadNodeContentDirect(parameterNode);
		if (parameterName == DAE_EMISSION_MATERIAL_PARAMETER)
		{
			status &= (FArchiveXML::ParseColorTextureParameter(effectStandard, parameterNode, effectStandard->GetEmissionColorParam(), FUDaeTextureChannel::EMISSION));
		}
		else if (parameterName == DAE_DIFFUSE_MATERIAL_PARAMETER)
		{
			status &= (FArchiveXML::ParseColorTextureParameter(effectStandard, parameterNode, effectStandard->GetDiffuseColorParam(), FUDaeTextureChannel::DIFFUSE));
		}
		else if (parameterName == DAE_AMBIENT_MATERIAL_PARAMETER)
		{
			status &= (FArchiveXML::ParseColorTextureParameter(effectStandard, parameterNode, effectStandard->GetAmbientColorParam(), FUDaeTextureChannel::AMBIENT));
		}
		else if (parameterName == DAE_TRANSPARENT_MATERIAL_PARAMETER)
		{
			fm::string opaque = ReadNodeProperty(parameterNode, DAE_OPAQUE_MATERIAL_ATTRIBUTE);
			if (IsEquivalentI(opaque, DAE_RGB_ZERO_ELEMENT))
				effectStandard->SetTransparencyMode(FCDEffectStandard::RGB_ZERO);
			else if (IsEquivalentI(opaque, DAE_A_ONE_ELEMENT))
				effectStandard->SetTransparencyMode(FCDEffectStandard::A_ONE);

			status &= (FArchiveXML::ParseColorTextureParameter(effectStandard, parameterNode, effectStandard->GetTranslucencyColorParam(), FUDaeTextureChannel::TRANSPARENT));
			hasTranslucency = true;
		}
		else if (parameterName == DAE_TRANSPARENCY_MATERIAL_PARAMETER)
		{
			status &= (FArchiveXML::ParseFloatTextureParameter(effectStandard, parameterNode, effectStandard->GetTranslucencyFactorParam(), FUDaeTextureChannel::UNKNOWN));
			hasTranslucency = true;
		}
		else if (parameterName == DAE_SPECULAR_MATERIAL_PARAMETER)
		{
			status &= (FArchiveXML::ParseColorTextureParameter(effectStandard, parameterNode, effectStandard->GetSpecularColorParam(), FUDaeTextureChannel::SPECULAR));
		}
		else if (parameterName == DAE_SHININESS_MATERIAL_PARAMETER)
		{
			status &= (FArchiveXML::ParseFloatTextureParameter(effectStandard, parameterNode, effectStandard->GetShininessParam(), FUDaeTextureChannel::SHININESS));
		}
		else if (parameterName == DAE_REFLECTIVE_MATERIAL_PARAMETER)
		{
			status &= (FArchiveXML::ParseColorTextureParameter(effectStandard, parameterNode, effectStandard->GetReflectivityColorParam(), FUDaeTextureChannel::REFLECTION));
			hasReflectivity = true;
		}
		else if (parameterName == DAE_REFLECTIVITY_MATERIAL_PARAMETER)
		{
			status &= (FArchiveXML::ParseFloatTextureParameter(effectStandard, parameterNode, effectStandard->GetReflectivityFactorParam(), FUDaeTextureChannel::UNKNOWN));
			hasReflectivity = true;
		}
		else if (parameterName == DAE_INDEXOFREFRACTION_MATERIAL_PARAMETER)
		{
			status &= (FArchiveXML::ParseFloatTextureParameter(effectStandard, parameterNode, effectStandard->GetIndexOfRefractionParam(), FUDaeTextureChannel::REFRACTION));
			hasRefractive = true;
		}
		else if (parameterName == DAE_BUMP_MATERIAL_PARAMETER)
		{
			status &= (FArchiveXML::ParseSimpleTextureParameter(effectStandard, parameterNode, FUDaeTextureChannel::BUMP));
		}
		else if (parameterName == DAEMAX_SPECLEVEL_MATERIAL_PARAMETER)
		{
			status &= (FArchiveXML::ParseFloatTextureParameter(effectStandard, parameterNode, effectStandard->GetSpecularFactorParam(), FUDaeTextureChannel::SPECULAR_LEVEL));
		}
		else if (parameterName == DAEMAX_EMISSIONLEVEL_MATERIAL_PARAMETER)
		{
			status &= (FArchiveXML::ParseFloatTextureParameter(effectStandard, parameterNode, effectStandard->GetEmissionFactorParam(), FUDaeTextureChannel::EMISSION));
			effectStandard->SetIsEmissionFactor(true);
		}
		else if (parameterName == DAEMAX_FACETED_MATERIAL_PARAMETER)
		{
			effectStandard->AddExtraAttribute(DAEMAX_MAX_PROFILE, DAEMAX_FACETED_MATERIAL_PARAMETER, TO_FSTRING(parameterContent).c_str());
		}
		else if (parameterName == DAESHD_DOUBLESIDED_PARAMETER)
		{
			effectStandard->AddExtraAttribute(DAEMAX_MAX_PROFILE, DAESHD_DOUBLESIDED_PARAMETER, TO_FSTRING(parameterContent).c_str());
		}
		else if (parameterName == DAEMAX_WIREFRAME_MATERIAL_PARAMETER)
		{
			effectStandard->AddExtraAttribute(DAEMAX_MAX_PROFILE, DAEMAX_WIREFRAME_MATERIAL_PARAMETER, TO_FSTRING(parameterContent).c_str());
		}
		else if (parameterName == DAEMAX_FACEMAP_MATERIAL_PARAMETER)
		{
			effectStandard->AddExtraAttribute(DAEMAX_MAX_PROFILE, DAEMAX_FACEMAP_MATERIAL_PARAMETER, TO_FSTRING(parameterContent).c_str());
		}
		else if (parameterName == DAEMAX_DISPLACEMENT_MATERIAL_PARAMETER)
		{
			status &= (FArchiveXML::ParseSimpleTextureParameter(effectStandard, parameterNode, FUDaeTextureChannel::DISPLACEMENT));
		}
		else if (parameterName == DAEMAX_FILTERCOLOR_MATERIAL_PARAMETER)
		{
			status &= (FArchiveXML::ParseSimpleTextureParameter(effectStandard, parameterNode, FUDaeTextureChannel::FILTER));
		}
		else
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_MAT_PARAM_NAME, parameterNode->line);
		}
	}

	// Although the default COLLADA materials gives, wrongly, a transparent material,
	// when neither the TRANSPARENT or TRANSPARENCY parameters are set, assume an opaque material.
	// Similarly for reflectivity
	if (!hasTranslucency)
	{
		effectStandard->SetTranslucencyColor((effectStandard->GetTransparencyMode() == FCDEffectStandard::RGB_ZERO) ? FMVector4::Zero : FMVector4::One);
		effectStandard->SetTranslucencyFactor((effectStandard->GetTransparencyMode() == FCDEffectStandard::RGB_ZERO) ? 0.0f : 1.0f);
	}
	if (!hasReflectivity)
	{
		effectStandard->SetReflectivityColor(FMVector4::Zero);
		effectStandard->SetReflectivityFactor(0.0f);
	}
	effectStandard->SetReflective(hasReflectivity);
	effectStandard->SetRefractive(hasRefractive);
	effectStandard->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadEffectTechnique(FCDObject* object, xmlNode* techniqueNode)		
{
	FCDEffectTechnique* effectTechnique = (FCDEffectTechnique*)object;

	bool status = true;
	if (!IsEquivalent(techniqueNode->name, DAE_TECHNIQUE_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_TECHNIQUE_ELEMENT, techniqueNode->line);
		return status;
	}
	
	fm::string techniqueName = ReadNodeProperty(techniqueNode, DAE_SID_ATTRIBUTE);
	effectTechnique->SetName(TO_FSTRING(techniqueName));
	
	// Clear any old parameters.
	while (effectTechnique->GetEffectParameterCount() > 0)
	{
		effectTechnique->GetEffectParameter(effectTechnique->GetEffectParameterCount() - 1)->Release();
	}

	// Look for the pass and parameter elements
	for (xmlNode* child = techniqueNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_PASS_ELEMENT))
		{
			FCDEffectPass* pass = effectTechnique->AddPass();
			status &= (FArchiveXML::LoadEffectPass(pass, child));
		}
		else if (IsEquivalent(child->name, DAE_FXCMN_NEWPARAM_ELEMENT) || IsEquivalent(child->name, DAE_FXCMN_SETPARAM_ELEMENT))
		{
			FCDEffectParameter* parameter = effectTechnique->AddEffectParameter(FArchiveXML::GetEffectParameterType(child));
			status &= FArchiveXML::LoadSwitch(parameter, &parameter->GetObjectType(), child);
		}
		else if (IsEquivalent(child->name, DAE_FXCMN_CODE_ELEMENT) || IsEquivalent(child->name, DAE_FXCMN_INCLUDE_ELEMENT))
		{
			FCDEffectCode* code = effectTechnique->AddCode();
			status &= (FArchiveXML::LoadEffectCode(code, child));
		}
		else if (IsEquivalent(child->name, DAE_IMAGE_ELEMENT))
		{
			FCDImage* image = effectTechnique->GetDocument()->GetImageLibrary()->AddEntity();
			status &= (FArchiveXML::LoadImage(image, child));
		}
	}
	
	effectTechnique->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadEffect(FCDObject* object, xmlNode* effectNode)
{
	if (!FArchiveXML::LoadEntity(object, effectNode)) return false;

	bool status = true;
	FCDEffect* effect = (FCDEffect*)object;

	// Clear any old parameters.
	while (effect->GetEffectParameterCount() > 0)
	{
		effect->GetEffectParameter(effect->GetEffectParameterCount() - 1)->Release();
	}

	// Accept solely <effect> elements at this point.
	if (!IsEquivalent(effectNode->name, DAE_EFFECT_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_EFFECT_ELEMENT, effectNode->line);
	}

	for (xmlNode* child = effectNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_IMAGE_ELEMENT))
		{
			FCDImage* image = effect->GetDocument()->GetImageLibrary()->AddEntity();
			status &= (FArchiveXML::LoadImage(image, child));
		}
		else if (IsEquivalent(child->name, DAE_FXCMN_SETPARAM_ELEMENT) || IsEquivalent(child->name, DAE_FXCMN_NEWPARAM_ELEMENT))
		{
			FCDEffectParameter* parameter = effect->AddEffectParameter(FArchiveXML::GetEffectParameterType(child));
			status &= FArchiveXML::LoadSwitch(parameter, &parameter->GetObjectType(), child);
		}
		else if (IsEquivalent(child->name, DAE_EXTRA_ELEMENT)) {} // processed by FCDEntity.
		else
		{
			// Check for a valid profile element.
			FUDaeProfileType::Type type = FUDaeProfileType::FromString((const char*) child->name);
			if (type != FUDaeProfileType::UNKNOWN)
			{
				FCDEffectProfile* profile = effect->AddProfile(type);
				status &= (FArchiveXML::LoadSwitch(profile, &profile->GetObjectType(), child));
			}
			else
			{
				FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNSUPPORTED_PROFILE, child->line);
			}
		}
	}

	effect->SetDirtyFlag(); 
	return status;
}				

bool FArchiveXML::LoadImage(FCDObject* object, xmlNode* imageNode)
{
	if (!FArchiveXML::LoadEntity(object, imageNode)) return false;

	bool status = true;
	FCDImage* image = (FCDImage*)object;
	if (!IsEquivalent(imageNode->name, DAE_IMAGE_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_IMAGE_LIB_ELEMENT, imageNode->line);
		return status;
	}

	if (HasNodeProperty(imageNode, DAE_WIDTH_ELEMENT))
		image->SetWidth(FUStringConversion::ToUInt32(ReadNodeProperty(imageNode, DAE_WIDTH_ELEMENT)));
	if (HasNodeProperty(imageNode, DAE_HEIGHT_ELEMENT))
		image->SetHeight(FUStringConversion::ToUInt32(ReadNodeProperty(imageNode, DAE_HEIGHT_ELEMENT)));
	if (HasNodeProperty(imageNode, DAE_DEPTH_ELEMENT))
		image->SetDepth(FUStringConversion::ToUInt32(ReadNodeProperty(imageNode, DAE_DEPTH_ELEMENT)));

	// Read in the image's filename, within the <init_from> element: binary images are not supported.
	xmlNode* filenameSourceNode = FindChildByType(imageNode, DAE_INITFROM_ELEMENT);
	image->SetFilename(TO_FSTRING(ReadNodeContentFull(filenameSourceNode)));

	// Convert the filename to something the OS can use
	fstring fileName = FUUri(image->GetFilename()).GetAbsoluteUri();
	image->SetFilename(fileName);
	if (image->GetFilename().empty())
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::ERROR_INVALID_IMAGE_FILENAME, imageNode->line);
	}

	image->SetDirtyFlag();
	return status;
}				

bool FArchiveXML::LoadTexture(FCDObject* object, xmlNode* textureNode)
{
	FCDTexture* texture = (FCDTexture*)object;
	FCDTextureData& data = FArchiveXML::documentLinkDataMap[texture->GetDocument()].textureDataMap[texture];

	bool status = true;

	// Verify that this is a sampler node
	if (!IsEquivalent(textureNode->name, DAE_FXSTD_TEXTURE_ELEMENT))
	{
		//return status.Fail(FS("Unknown texture sampler element."), textureNode->line);
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_UNKNOWN_TEXTURE_SAMPLER, textureNode->line);
	}
	
	// Read in the 'texture' attribute: points to an image(early 1.4.0) or a sampler(late 1.4.0)
	// Will be resolved at link-time.
	data.samplerSid = ReadNodeProperty(textureNode, DAE_FXSTD_TEXTURE_ATTRIBUTE);
	if (!data.samplerSid.empty()) data.samplerSid = FCDObjectWithId::CleanSubId(data.samplerSid);

	// Read in the 'texcoord' attribute: a texture coordinate set identifier
	fm::string semantic = ReadNodeProperty(textureNode, DAE_FXSTD_TEXTURESET_ATTRIBUTE);
	if (!semantic.empty())
	{
		texture->GetSet()->SetSemantic(semantic);

		//[GLaforte 06-01-2006] Also attempt to convert the value to a signed integer
		// since that was done quite a bit in COLLADA 1.4 preview exporters.
		texture->GetSet()->SetValue(FUStringConversion::ToInt32(semantic));
	}

	// Parse in the extra trees
	xmlNodeList extraNodes;
	FindChildrenByType(textureNode, DAE_EXTRA_ELEMENT, extraNodes);
	for (xmlNodeList::iterator itX = extraNodes.begin(); itX != extraNodes.end(); ++itX)
	{
		status &= (FArchiveXML::LoadExtra(texture->GetExtra(), *itX));
	}
	texture->SetDirtyFlag(); 
	return status;
}

// Parse in the different standard effect parameters, bucketing the textures
bool FArchiveXML::ParseColorTextureParameter(FCDEffectStandard* effectStandard, xmlNode* parameterNode, FCDEffectParameterColor4* value, uint32 bucketIndex)
{
	bool status = true;

	// Look for <texture> elements, they pre-empt everything else
	if (bucketIndex != FUDaeTextureChannel::UNKNOWN)
	{
		size_t originalSize = effectStandard->GetTextureCount(bucketIndex);
		FArchiveXML::ParseSimpleTextureParameter(effectStandard, parameterNode, bucketIndex);
		if (originalSize < effectStandard->GetTextureCount(bucketIndex)) 
		{ 
			value->SetValue(FMVector4::One); 
			return status; 
		}
	}

	// Try to find a <param> element
	xmlNode* colorNode = NULL;
	xmlNode* paramNode = FindChildByType(parameterNode, DAE_PARAMETER_ELEMENT);
	if (paramNode != NULL)
	{
		fm::string name = ReadNodeProperty(paramNode, DAE_REF_ATTRIBUTE);

		//If there's no reference attribute, read in the content.
		if (name.empty()) 
		{
			colorNode = paramNode->children;
			if (!colorNode) 
			{
				FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_MISSING_INPUT, paramNode->line);
			}
			else 
			{
				name = ReadNodeContentFull(colorNode);
				if (name.empty()) 
				{
					FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_MISSING_INPUT, colorNode->line);
				}
				AddAttribute(colorNode, DAE_SID_ATTRIBUTE, name);
			}
		}
		else
		{
			colorNode = paramNode;
			AddAttribute(colorNode, DAE_SID_ATTRIBUTE, name);
		}
		value->SetReference(name);
		value->SetReferencer();
		//The parameter isn't linked here.
	}
	else
	{
		// Look for a <color> element
		colorNode = FindChildByType(parameterNode, DAE_FXSTD_COLOR_ELEMENT);
		const char* content = ReadNodeContentDirect(colorNode);

		// Parse the color value and allow for an animation of it
		value->SetValue(FUStringConversion::ToVector4(content));
	}
	FArchiveXML::LoadAnimatable(&value->GetValue(), colorNode);
	return status;
}

bool FArchiveXML::ParseFloatTextureParameter(FCDEffectStandard* effectStandard, xmlNode* parameterNode, FCDEffectParameterFloat* value, uint32 bucketIndex)
{
	bool status = true;

	// Look for <texture> elements, they pre-empt everything else
	if (bucketIndex != FUDaeTextureChannel::UNKNOWN)
	{
		size_t originalSize = effectStandard->GetTextureCount(bucketIndex);
		FArchiveXML::ParseSimpleTextureParameter(effectStandard, parameterNode, bucketIndex);
		if (originalSize < effectStandard->GetTextureCount(bucketIndex)) 
		{ 
			value->SetValue(1.0f);
			return status;
		}
	}

	// Next, look for a <float> element
	xmlNode* floatNode = NULL;
	xmlNode* paramNode = FindChildByType(parameterNode, DAE_PARAMETER_ELEMENT);
	if (paramNode)
	{
		fm::string name = ReadNodeProperty(paramNode, DAE_REF_ATTRIBUTE);

		//If there's no reference attribute, read in the content.
		if (name.empty()) 
		{
			floatNode = paramNode->children;
			if (!floatNode) 
			{
				FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_MISSING_INPUT, paramNode->line);
			}
			else 
			{
				name = ReadNodeContentFull(floatNode);
				if (name.empty()) 
				{
					FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_MISSING_INPUT, floatNode->line);
				}
				AddAttribute(floatNode, DAE_SID_ATTRIBUTE, name);
			}
		}
		else 
		{
			floatNode = paramNode;
			AddAttribute(floatNode, DAE_SID_ATTRIBUTE, name);
		}
		value->SetReference(name);
		value->SetReferencer();
	}
	else 
	{
		// Next, look for a <float> element
		floatNode = FindChildByType(parameterNode, DAE_FXSTD_FLOAT_ELEMENT);
		const char* content = ReadNodeContentDirect(floatNode);

		// Parse the value and register it for an animation.
		value->SetValue(FUStringConversion::ToFloat(content));
	}
	FArchiveXML::LoadAnimatable(&value->GetValue(), floatNode);
	return status;
}

bool FArchiveXML::ParseSimpleTextureParameter(FCDEffectStandard* effectStandard, xmlNode* parameterNode, uint32 bucketIndex)
{
	FUAssert(bucketIndex != FUDaeTextureChannel::UNKNOWN, return false);
	bool status = true; 

	// Parse in all the <texture> elements as standard effect samplers
	xmlNodeList samplerNodes;
	FindChildrenByType(parameterNode, DAE_FXSTD_TEXTURE_ELEMENT, samplerNodes);
	if (!samplerNodes.empty())
	{
		for (xmlNodeList::iterator itS = samplerNodes.begin(); itS != samplerNodes.end(); ++itS)
		{
			// Parse in the texture element and bucket them
			FCDTexture* texture = effectStandard->AddTexture(bucketIndex);
			status &= (FArchiveXML::LoadTexture(texture, *itS));
			if (!status) { SAFE_RELEASE(texture); }
		}
	}
	return status;
}

uint32 FArchiveXML::GetEffectParameterType(xmlNode* parameterNode)
{
	// If the parent is bind_material, the format is slightly different, there is no children node.
	if (parameterNode->children == NULL)
	{
		xmlNode* parent = parameterNode->parent;
		if (IsEquivalent(parent->name, DAE_BINDMATERIAL_ELEMENT))
		{
			// Find out of the type= ... what type of parameter to create.
			fm::string type = ReadNodeProperty(parameterNode, DAE_TYPE_ATTRIBUTE);
			if (IsEquivalent(type, DAE_FXCMN_BOOL_ELEMENT)) return FCDEffectParameter::BOOLEAN;
			else if (IsEquivalent(type, DAE_FXCMN_FLOAT_ELEMENT)) return FCDEffectParameter::FLOAT;
			else if (IsEquivalent(type, DAE_FXCMN_FLOAT2_ELEMENT)) return FCDEffectParameter::FLOAT2;
			else if (IsEquivalent(type, DAE_FXCMN_FLOAT3_ELEMENT)) return FCDEffectParameter::FLOAT3;
			else if (IsEquivalent(type, DAE_FXCMN_FLOAT4_ELEMENT)) return FCDEffectParameter::VECTOR;
			else if (IsEquivalent(type, DAE_FXCMN_FLOAT4X4_ELEMENT)) return FCDEffectParameter::MATRIX;
			else if (IsEquivalent(type, DAE_FXCMN_HALF_ELEMENT)) return FCDEffectParameter::FLOAT;
			else if (IsEquivalent(type, DAE_FXCMN_HALF2_ELEMENT)) return FCDEffectParameter::FLOAT2;
			else if (IsEquivalent(type, DAE_FXCMN_HALF3_ELEMENT)) return FCDEffectParameter::FLOAT3;
			else if (IsEquivalent(type, DAE_FXCMN_HALF4_ELEMENT)) return FCDEffectParameter::VECTOR;
			else if (IsEquivalent(type, DAE_FXCMN_HALF4X4_ELEMENT)) return FCDEffectParameter::MATRIX;
			else if (IsEquivalent(type, DAE_FXCMN_INT_ELEMENT)) return FCDEffectParameter::INTEGER;
			else if (IsEquivalent(type, DAE_FXCMN_SAMPLER1D_ELEMENT)) return FCDEffectParameter::SAMPLER;
			else if (IsEquivalent(type, DAE_FXCMN_SAMPLER2D_ELEMENT)) return FCDEffectParameter::SAMPLER;
			else if (IsEquivalent(type, DAE_FXCMN_SAMPLER3D_ELEMENT)) return FCDEffectParameter::SAMPLER;
			else if (IsEquivalent(type, DAE_FXCMN_SAMPLERCUBE_ELEMENT)) return FCDEffectParameter::SAMPLER;
			else if (IsEquivalent(type, DAE_FXCMN_SURFACE_ELEMENT)) return FCDEffectParameter::SURFACE;
			else if (IsEquivalent(type, DAE_FXCMN_STRING_ELEMENT)) return FCDEffectParameter::STRING;
		}
	}
	else
	{
		for (xmlNode* child = parameterNode->children; child != NULL; child = child->next)
		{
			if (child->type != XML_ELEMENT_NODE) continue;

			if (IsEquivalent(child->name, DAE_FXCMN_BOOL_ELEMENT)) return FCDEffectParameter::BOOLEAN;
			else if (IsEquivalent(child->name, DAE_FXCMN_FLOAT_ELEMENT)) return FCDEffectParameter::FLOAT;
			else if (IsEquivalent(child->name, DAE_FXCMN_FLOAT2_ELEMENT)) return FCDEffectParameter::FLOAT2;
			else if (IsEquivalent(child->name, DAE_FXCMN_FLOAT3_ELEMENT)) return FCDEffectParameter::FLOAT3;
			else if (IsEquivalent(child->name, DAE_FXCMN_FLOAT4_ELEMENT)) return FCDEffectParameter::VECTOR;
			else if (IsEquivalent(child->name, DAE_FXCMN_FLOAT4X4_ELEMENT)) return FCDEffectParameter::MATRIX;
			else if (IsEquivalent(child->name, DAE_FXCMN_HALF_ELEMENT)) return FCDEffectParameter::FLOAT;
			else if (IsEquivalent(child->name, DAE_FXCMN_HALF2_ELEMENT)) return FCDEffectParameter::FLOAT2;
			else if (IsEquivalent(child->name, DAE_FXCMN_HALF3_ELEMENT)) return FCDEffectParameter::FLOAT3;
			else if (IsEquivalent(child->name, DAE_FXCMN_HALF4_ELEMENT)) return FCDEffectParameter::VECTOR;
			else if (IsEquivalent(child->name, DAE_FXCMN_HALF4X4_ELEMENT)) return FCDEffectParameter::MATRIX;
			else if (IsEquivalent(child->name, DAE_FXCMN_INT_ELEMENT)) return FCDEffectParameter::INTEGER;
			else if (IsEquivalent(child->name, DAE_FXCMN_SAMPLER1D_ELEMENT)) return FCDEffectParameter::SAMPLER;
			else if (IsEquivalent(child->name, DAE_FXCMN_SAMPLER2D_ELEMENT)) return FCDEffectParameter::SAMPLER;
			else if (IsEquivalent(child->name, DAE_FXCMN_SAMPLER3D_ELEMENT)) return FCDEffectParameter::SAMPLER;
			else if (IsEquivalent(child->name, DAE_FXCMN_SAMPLERCUBE_ELEMENT)) return FCDEffectParameter::SAMPLER;
			else if (IsEquivalent(child->name, DAE_FXCMN_SURFACE_ELEMENT)) return FCDEffectParameter::SURFACE;
			else if (IsEquivalent(child->name, DAE_FXCMN_STRING_ELEMENT)) return FCDEffectParameter::STRING;
		}
	}

	FUError::Error(FUError::WARNING_LEVEL, FUError::ERROR_UNKNOWN_CHILD, parameterNode->line);
	return FCDEffectParameter::FLOAT;
}
