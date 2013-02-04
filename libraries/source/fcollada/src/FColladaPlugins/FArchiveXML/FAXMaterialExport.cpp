/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDocument.h"
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

xmlNode* FArchiveXML::WriteMaterial(FCDObject* object, xmlNode* parentNode)
{
	FCDMaterial* material = (FCDMaterial*)object;

	xmlNode* materialNode = FArchiveXML::WriteToEntityXMLFCDEntity(material, parentNode, DAE_MATERIAL_ELEMENT);

	// The <instance_effect> element is required in COLLADA 1.4
	xmlNode* instanceEffectNode = AddChild(materialNode, DAE_INSTANCE_EFFECT_ELEMENT);
	if (material->GetEffect() != NULL)
	{
		const FUUri& uri = material->GetEffectReference()->GetUri();
		fstring uriString = material->GetDocument()->GetFileManager()->CleanUri(uri);
		AddAttribute(instanceEffectNode, DAE_URL_ATTRIBUTE, uriString);

		// Write out the technique hints
		for (FCDMaterialTechniqueHintList::iterator itH = material->GetTechniqueHints().begin(); itH != material->GetTechniqueHints().end(); ++itH)
		{
			xmlNode* hintNode = AddChild(instanceEffectNode, DAE_FXCMN_HINT_ELEMENT);
			AddAttribute(hintNode, DAE_PLATFORM_ATTRIBUTE, (*itH).platform);
			AddAttribute(hintNode, DAE_REF_ATTRIBUTE, (*itH).technique);
		}

		// Write out the parameters
		size_t parameterCount = material->GetEffectParameterCount();
		for (size_t p = 0; p < parameterCount; ++p)
		{
			FArchiveXML::LetWriteObject(material->GetEffectParameter(p), instanceEffectNode);
		}
	}
	else
	{
		AddAttribute(instanceEffectNode, DAE_URL_ATTRIBUTE, fm::string("#"));
	}

	FArchiveXML::WriteEntityExtra(material, materialNode);
	return materialNode;
}

xmlNode* FArchiveXML::WriteEffectCode(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectCode* effectCode = (FCDEffectCode*)object;

	// Place the new element at the correct position in the XML.
	// This is necessary for FX profiles.
	xmlNode* includeAtNode = NULL;
	for (xmlNode* n = parentNode->children; n != NULL; n = n->next)
	{
		if (n->type != XML_ELEMENT_NODE) continue;
		else if (IsEquivalent(n->name, DAE_ASSET_ELEMENT)) continue;
		else if (IsEquivalent(n->name, DAE_FXCMN_CODE_ELEMENT)) continue;
		else if (IsEquivalent(n->name, DAE_FXCMN_INCLUDE_ELEMENT)) continue;
		else { includeAtNode = n; break; }
	}

	// In COLLADA 1.4, the 'sid' and 'url' attributes are required.
	// In the case of the sub-id, save it for later use.
	xmlNode* codeNode;
	fm::string& _sid = const_cast<fm::string&>(effectCode->GetSubId());
	switch (effectCode->GetType())
	{
	case FCDEffectCode::CODE:
		if (includeAtNode == NULL) codeNode = AddChild(parentNode, DAE_FXCMN_CODE_ELEMENT);
		else codeNode = InsertChild(parentNode, includeAtNode, DAE_FXCMN_CODE_ELEMENT);
		AddContent(codeNode, effectCode->GetCode());
		if (_sid.empty()) _sid = "code";
		AddNodeSid(codeNode, _sid);
		break;

	case FCDEffectCode::INCLUDE: {
		if (includeAtNode == NULL) codeNode = AddChild(parentNode, DAE_FXCMN_INCLUDE_ELEMENT);
		else codeNode = InsertChild(parentNode, includeAtNode, DAE_FXCMN_INCLUDE_ELEMENT);
		if (_sid.empty()) _sid = "include";
		AddNodeSid(codeNode, _sid);
		fstring fileURL = effectCode->GetDocument()->GetFileManager()->CleanUri(FUUri(effectCode->GetFilename()));
		FUXmlWriter::ConvertFilename(fileURL);
		AddAttribute(codeNode, DAE_URL_ATTRIBUTE, fileURL);
		break; }

	default:
		codeNode = NULL;
	}
	return codeNode;
}

xmlNode* FArchiveXML::WriteEffectParameter(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectParameter* effectParameter = (FCDEffectParameter*)object;

	xmlNode* parameterNode;
	if (effectParameter->IsGenerator())
	{
		parameterNode = AddChild(parentNode, DAE_FXCMN_NEWPARAM_ELEMENT);
		AddAttribute(parameterNode, DAE_SID_ATTRIBUTE, effectParameter->GetReference());
	}
	else if (effectParameter->IsModifier())
	{
		parameterNode = AddChild(parentNode, DAE_FXCMN_SETPARAM_ELEMENT);
		AddAttribute(parameterNode, DAE_REF_ATTRIBUTE, effectParameter->GetReference());
	}
	else
	{
		parameterNode = AddChild(parentNode, DAE_PARAMETER_ELEMENT);
		if (!effectParameter->GetReference().empty() && !effectParameter->IsReferencer()) 
		{
			AddAttribute(parameterNode, DAE_SID_ATTRIBUTE, effectParameter->GetReference());
		}
	}

	// Write out the annotations
	for (size_t i = 0; i < effectParameter->GetAnnotationCount(); ++i)
	{
		FCDEffectParameterAnnotation* annotation = effectParameter->GetAnnotation(i);
		xmlNode* annotateNode = AddChild(parameterNode, DAE_ANNOTATE_ELEMENT);
		AddAttribute(annotateNode, DAE_NAME_ATTRIBUTE, *annotation->name);
		switch ((uint32) annotation->type)
		{
		case FCDEffectParameter::BOOLEAN: AddChild(annotateNode, DAE_FXCMN_BOOL_ELEMENT, *annotation->value); break;
		case FCDEffectParameter::STRING: AddChild(annotateNode, DAE_FXCMN_STRING_ELEMENT, *annotation->value); break;
		case FCDEffectParameter::INTEGER: AddChild(annotateNode, DAE_FXCMN_INT_ELEMENT, *annotation->value); break;
		case FCDEffectParameter::FLOAT: AddChild(annotateNode, DAE_FXCMN_FLOAT_ELEMENT, *annotation->value); break;
		default: break;
		}
	}

	if (!effectParameter->IsAnimator())
	{
		// Write out the semantic
		if (effectParameter->IsGenerator() && !effectParameter->GetSemantic().empty())
		{
			AddChild(parameterNode, DAE_FXCMN_SEMANTIC_ELEMENT, effectParameter->GetSemantic());
		}
	}
	return parameterNode;
}

xmlNode* FArchiveXML::WriteEffectParameterBool(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectParameterBool* effectParameterBool = (FCDEffectParameterBool*)object;

	xmlNode* parameterNode = FArchiveXML::WriteEffectParameter(effectParameterBool, parentNode);
	AddChild(parameterNode, DAE_FXCMN_BOOL_ELEMENT, effectParameterBool->GetValue());
	return parameterNode;
}

xmlNode* FArchiveXML::WriteEffectParameterFloat(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectParameterFloat* effectParameterFloat = (FCDEffectParameterFloat*)object;

	if (effectParameterFloat->IsReferencer())
	{
		xmlNode* parameterNode = FArchiveXML::WriteEffectParameter(effectParameterFloat, parentNode);
		AddAttribute(parameterNode, DAE_FXSTD_STATE_REF_ELEMENT, effectParameterFloat->GetReference());
		return parameterNode;
	}
	else if (effectParameterFloat->IsAnimator())
	{
		xmlNode* parameterNode = FArchiveXML::WriteEffectParameter(effectParameterFloat, parentNode);
		AddAttribute(parameterNode, DAE_SEMANTIC_ATTRIBUTE, effectParameterFloat->GetSemantic());
		AddAttribute(parameterNode, DAE_TYPE_ATTRIBUTE, DAEFC_FLOAT_ATTRIBUTE_TYPE);
		return parameterNode;
	}
	else
	{
		xmlNode* parameterNode = FArchiveXML::WriteEffectParameter(effectParameterFloat, parentNode);
		AddChild(parameterNode, (effectParameterFloat->GetFloatType() == FCDEffectParameterFloat::FLOAT) ? DAE_FXCMN_FLOAT_ELEMENT : DAE_FXCMN_HALF_ELEMENT, effectParameterFloat->GetValue());
		const char* wantedSubId = effectParameterFloat->GetReference().c_str();
		if (*wantedSubId == 0) wantedSubId = effectParameterFloat->GetSemantic().c_str();
		if (*wantedSubId == 0) wantedSubId = "flt";
		FArchiveXML::WriteAnimatedValue(&effectParameterFloat->GetValue(), parameterNode, wantedSubId);
		return parameterNode;
	}
}

xmlNode* FArchiveXML::WriteEffectParameterFloat2(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectParameterFloat2* effectParameterFloat2 = (FCDEffectParameterFloat2*)object;

	xmlNode* parameterNode = FArchiveXML::WriteEffectParameter(effectParameterFloat2, parentNode);
	FUSStringBuilder builder; builder.set(effectParameterFloat2->GetValue()->x); builder.append(' '); builder.append(effectParameterFloat2->GetValue()->y);
	AddChild(parameterNode, (effectParameterFloat2->GetFloatType() == FCDEffectParameterFloat2::FLOAT) ? DAE_FXCMN_FLOAT2_ELEMENT : DAE_FXCMN_HALF2_ELEMENT, builder);
	return parameterNode;
}

xmlNode* FArchiveXML::WriteEffectParameterFloat3(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectParameterFloat3* effectParameterFloat3 = (FCDEffectParameterFloat3*) object;

	xmlNode* parameterNode = FArchiveXML::WriteEffectParameter(effectParameterFloat3, parentNode);
	fm::string s = FUStringConversion::ToString((FMVector3&) effectParameterFloat3->GetValue());
	AddChild(parameterNode, (effectParameterFloat3->GetFloatType() == FCDEffectParameterFloat3::FLOAT) ? DAE_FXCMN_FLOAT3_ELEMENT : DAE_FXCMN_HALF3_ELEMENT, s);
	const char* wantedSubId = effectParameterFloat3->GetReference().c_str();
	if (*wantedSubId == 0) wantedSubId = effectParameterFloat3->GetSemantic().c_str();
	if (*wantedSubId == 0) wantedSubId = "flt3";
	FArchiveXML::WriteAnimatedValue(&effectParameterFloat3->GetValue(), parameterNode, wantedSubId);
	return parameterNode;
}

xmlNode* FArchiveXML::WriteEffectParameterInt(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectParameterInt* effectParameterInt = (FCDEffectParameterInt*)object;

	xmlNode* parameterNode = FArchiveXML::WriteEffectParameter(effectParameterInt, parentNode);
	AddChild(parameterNode, DAE_FXCMN_INT_ELEMENT, effectParameterInt->GetValue());
	return parameterNode;
}

xmlNode* FArchiveXML::WriteEffectParameterMatrix(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectParameterMatrix* effectParameterMatrix = (FCDEffectParameterMatrix*)object;

	xmlNode* parameterNode = FArchiveXML::WriteEffectParameter(effectParameterMatrix, parentNode);
	fm::string s = FUStringConversion::ToString((FMMatrix44&) effectParameterMatrix->GetValue()); 
	AddChild(parameterNode, (effectParameterMatrix->GetFloatType() == FCDEffectParameterMatrix::FLOAT) ? DAE_FXCMN_FLOAT4X4_ELEMENT : DAE_FXCMN_HALF4X4_ELEMENT, s);
	return parameterNode;
}

xmlNode* FArchiveXML::WriteEffectParameterSampler(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectParameterSampler* effectParameterSampler = (FCDEffectParameterSampler*)object;

	xmlNode* parameterNode = FArchiveXML::WriteEffectParameter(effectParameterSampler, parentNode);
	const char* samplerName;
	switch (effectParameterSampler->GetSamplerType())
	{
	case FCDEffectParameterSampler::SAMPLER1D: samplerName = DAE_FXCMN_SAMPLER1D_ELEMENT; break;
	case FCDEffectParameterSampler::SAMPLER2D: samplerName = DAE_FXCMN_SAMPLER2D_ELEMENT; break;
	case FCDEffectParameterSampler::SAMPLER3D: samplerName = DAE_FXCMN_SAMPLER3D_ELEMENT; break;
	case FCDEffectParameterSampler::SAMPLERCUBE: samplerName = DAE_FXCMN_SAMPLERCUBE_ELEMENT; break;
	default: samplerName = DAEERR_UNKNOWN_ELEMENT; break;
	}
	xmlNode* samplerNode = AddChild(parameterNode, samplerName);
	AddChild(samplerNode, DAE_SOURCE_ELEMENT, effectParameterSampler->GetSurface() != NULL ? effectParameterSampler->GetSurface()->GetReference() : "");

	switch (effectParameterSampler->GetSamplerType())
	{
	case FCDEffectParameterSampler::SAMPLER1D:
		{
			AddChild(samplerNode, DAE_WRAP_S_ELEMENT, FUDaeTextureWrapMode::ToString(effectParameterSampler->GetWrapS()));
		} break;
	case FCDEffectParameterSampler::SAMPLER2D:
		{
			AddChild(samplerNode, DAE_WRAP_S_ELEMENT, FUDaeTextureWrapMode::ToString(effectParameterSampler->GetWrapS()));
			AddChild(samplerNode, DAE_WRAP_T_ELEMENT, FUDaeTextureWrapMode::ToString(effectParameterSampler->GetWrapT()));
		} break;
	case FCDEffectParameterSampler::SAMPLER3D:
	case FCDEffectParameterSampler::SAMPLERCUBE:
		{
			AddChild(samplerNode, DAE_WRAP_S_ELEMENT, FUDaeTextureWrapMode::ToString(effectParameterSampler->GetWrapS()));
			AddChild(samplerNode, DAE_WRAP_T_ELEMENT, FUDaeTextureWrapMode::ToString(effectParameterSampler->GetWrapT()));
			AddChild(samplerNode, DAE_WRAP_P_ELEMENT, FUDaeTextureWrapMode::ToString(effectParameterSampler->GetWrapP()));
		} break;
	}

	AddChild(samplerNode, DAE_MIN_FILTER_ELEMENT, FUDaeTextureFilterFunction::ToString(effectParameterSampler->GetMinFilter()));
	AddChild(samplerNode, DAE_MAG_FILTER_ELEMENT, FUDaeTextureFilterFunction::ToString(effectParameterSampler->GetMagFilter()));
	AddChild(samplerNode, DAE_MIP_FILTER_ELEMENT, FUDaeTextureFilterFunction::ToString(effectParameterSampler->GetMipFilter()));

	return parameterNode;
}

xmlNode* FArchiveXML::WriteEffectParameterString(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectParameterString* effectParameterString = (FCDEffectParameterString*)object;

	xmlNode* parameterNode = FArchiveXML::WriteEffectParameter(effectParameterString, parentNode);
	AddChild(parameterNode, DAE_FXCMN_STRING_ELEMENT, effectParameterString->GetValue());
	return parameterNode;
}

xmlNode* FArchiveXML::WriteEffectParameterSurface(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectParameterSurface* effectParameterSurface = (FCDEffectParameterSurface*)object;

	xmlNode* parameterNode = FArchiveXML::WriteEffectParameter(effectParameterSurface, parentNode);
	xmlNode* surfaceNode = AddChild(parameterNode, DAE_FXCMN_SURFACE_ELEMENT);
	AddAttribute(surfaceNode, DAE_TYPE_ATTRIBUTE, effectParameterSurface->GetSurfaceType());
	
	if (effectParameterSurface->GetInitMethod() != NULL)
	{
		switch (effectParameterSurface->GetInitMethod()->GetInitType())
		{
			case FCDEffectParameterSurfaceInitFactory::FROM:
			{
				//Since 1.4.1, there are two possibilities here.
				//Possibility 1
				//<init_from> image1 image2...imageN </init_from>

				//Possibility 2
				//<init_from mip=... face=... slice=...> image1 </init_from>
				//<init_from mip=... face=... slice=...> image2 </init_from>

				FCDEffectParameterSurfaceInitFrom* in = (FCDEffectParameterSurfaceInitFrom*)effectParameterSurface->GetInitMethod();
				size_t size = effectParameterSurface->GetImageCount();
				size_t sizeMips = in->mip.size();
				size_t sizeSlices = in->slice.size();
				size_t sizeFaces = in->face.size();

				if (size == in->face.size() || size == in->mip.size() || size == in->slice.size())
				{
					//This is possibility 2
					for (uint32 i=0; i<size; i++)
					{
						xmlNode* childNode = AddChild(surfaceNode, DAE_INITFROM_ELEMENT);
						if (i<sizeMips) AddAttribute(childNode, DAE_MIP_ATTRIBUTE, in->mip[i]);
						if (i<sizeSlices) AddAttribute(childNode, DAE_SLICE_ATTRIBUTE, in->slice[i]);
						if (i<sizeFaces) AddAttribute(childNode, DAE_FACE_ATTRIBUTE, in->face[i]);
						AddContent(childNode, effectParameterSurface->GetImage(i)->GetDaeId());
					}
				}
				else
				{
					//This is possibility 1
					FUSStringBuilder builder;
					for (size_t i = 0; i < size; ++i) 
					{ 
						builder.append(effectParameterSurface->GetImage(i)->GetDaeId());
						builder.append(' ');
					}
					builder.pop_back();
					xmlNode* childNode = AddChild(surfaceNode, DAE_INITFROM_ELEMENT);
					AddContent(childNode, builder.ToCharPtr());
				}
				break;
			}
			case FCDEffectParameterSurfaceInitFactory::AS_NULL:
			{
				AddChild(surfaceNode, DAE_INITASNULL_ELEMENT);
				break;
			}
			case FCDEffectParameterSurfaceInitFactory::AS_TARGET:
			{
				AddChild(surfaceNode, DAE_INITASTARGET_ELEMENT);
				break;
			}
			case FCDEffectParameterSurfaceInitFactory::VOLUME:
			{
				FCDEffectParameterSurfaceInitVolume* in = (FCDEffectParameterSurfaceInitVolume*)effectParameterSurface->GetInitMethod();
				xmlNode* childNode = AddChild(surfaceNode, DAE_INITVOLUME_ELEMENT);
				if (in->volumeType == FCDEffectParameterSurfaceInitVolume::ALL)
				{
					xmlNode* typeNode = AddChild(childNode, DAE_ALL_ELEMENT);
					AddAttribute(typeNode, DAE_REF_ATTRIBUTE, effectParameterSurface->GetImage(0)->GetDaeId());
				}
				else if (in->volumeType == FCDEffectParameterSurfaceInitVolume::PRIMARY)
				{
					xmlNode* typeNode = AddChild(childNode, DAE_PRIMARY_ELEMENT);
					AddAttribute(typeNode, DAE_REF_ATTRIBUTE, effectParameterSurface->GetImage(0)->GetDaeId());
				}
				break;
			}
			case FCDEffectParameterSurfaceInitFactory::CUBE:
			{
				FCDEffectParameterSurfaceInitCube* in = (FCDEffectParameterSurfaceInitCube*)effectParameterSurface->GetInitMethod();
				xmlNode* childNode = AddChild(surfaceNode, DAE_INITCUBE_ELEMENT);
				if (in->cubeType == FCDEffectParameterSurfaceInitCube::ALL)
				{
					xmlNode* typeNode = AddChild(childNode, DAE_ALL_ELEMENT);
					AddAttribute(typeNode, DAE_REF_ATTRIBUTE, effectParameterSurface->GetImage(0)->GetDaeId());
				}
				else if (in->cubeType == FCDEffectParameterSurfaceInitCube::PRIMARY)
				{
					xmlNode* typeNode = AddChild(childNode, DAE_PRIMARY_ELEMENT);
					AddChild(typeNode, DAE_ORDER_ELEMENT); //FIXME: complete when the spec gets more info.
					AddAttribute(typeNode, DAE_REF_ATTRIBUTE, effectParameterSurface->GetImage(0)->GetDaeId());
				}
				if (in->cubeType == FCDEffectParameterSurfaceInitCube::FACE)
				{
					for (size_t i = 0; i < effectParameterSurface->GetImageCount(); i++)
					{
						xmlNode* faceNode = AddChild(childNode, DAE_FACE_ELEMENT);
						AddAttribute(faceNode, DAE_REF_ATTRIBUTE, effectParameterSurface->GetImage(i)->GetDaeId());
					}
				}
				break;
			}
			case FCDEffectParameterSurfaceInitFactory::PLANAR:
			{
				xmlNode* childNode = AddChild(surfaceNode, DAE_INITPLANAR_ELEMENT);
				xmlNode* typeNode = AddChild(childNode, DAE_ALL_ELEMENT);
				AddAttribute(typeNode, DAE_REF_ATTRIBUTE, effectParameterSurface->GetImage(0)->GetDaeId());
				break;
			}
			default:
				break;
		}

	}
	if (!effectParameterSurface->GetFormat().empty())
	{
		xmlNode* childNode = AddChild(surfaceNode, DAE_FORMAT_ELEMENT);
		AddContent(childNode, effectParameterSurface->GetFormat().c_str());
	}
	if (effectParameterSurface->GetFormatHint())
	{
		xmlNode* formatHintNode = AddChild(surfaceNode, DAE_FORMAT_HINT_ELEMENT);
		
		xmlNode* channelsNode = AddChild(formatHintNode, DAE_CHANNELS_ELEMENT);
		FCDFormatHint* formatHint = effectParameterSurface->GetFormatHint();
		if (formatHint->channels == FCDFormatHint::CHANNEL_RGB) AddContent(channelsNode, DAE_FORMAT_HINT_RGB_VALUE);
		else if (formatHint->channels == FCDFormatHint::CHANNEL_RGBA) AddContent(channelsNode, DAE_FORMAT_HINT_RGBA_VALUE);
		else if (formatHint->channels == FCDFormatHint::CHANNEL_L) AddContent(channelsNode, DAE_FORMAT_HINT_L_VALUE);
		else if (formatHint->channels == FCDFormatHint::CHANNEL_LA) AddContent(channelsNode, DAE_FORMAT_HINT_LA_VALUE);
		else if (formatHint->channels == FCDFormatHint::CHANNEL_D) AddContent(channelsNode, DAE_FORMAT_HINT_D_VALUE);
		else if (formatHint->channels == FCDFormatHint::CHANNEL_XYZ) AddContent(channelsNode, DAE_FORMAT_HINT_XYZ_VALUE);
		else if (formatHint->channels == FCDFormatHint::CHANNEL_XYZW) AddContent(channelsNode, DAE_FORMAT_HINT_XYZW_VALUE);
		else AddContent(channelsNode, DAEERR_UNKNOWN_ELEMENT);

		xmlNode* rangeNode = AddChild(formatHintNode, DAE_RANGE_ELEMENT);
		if (formatHint->range == FCDFormatHint::RANGE_SNORM) AddContent(rangeNode, DAE_FORMAT_HINT_SNORM_VALUE);
		else if (formatHint->range == FCDFormatHint::RANGE_UNORM) AddContent(rangeNode, DAE_FORMAT_HINT_UNORM_VALUE);
		else if (formatHint->range == FCDFormatHint::RANGE_SINT) AddContent(rangeNode, DAE_FORMAT_HINT_SINT_VALUE);
		else if (formatHint->range == FCDFormatHint::RANGE_UINT) AddContent(rangeNode, DAE_FORMAT_HINT_UINT_VALUE);
		else if (formatHint->range == FCDFormatHint::RANGE_FLOAT) AddContent(rangeNode, DAE_FORMAT_HINT_FLOAT_VALUE);
		else if (formatHint->range == FCDFormatHint::RANGE_LOW) AddContent(rangeNode, DAE_FORMAT_HINT_LOW_VALUE);
		else AddContent(rangeNode, DAEERR_UNKNOWN_ELEMENT);

		xmlNode* precisionNode = AddChild(formatHintNode, DAE_PRECISION_ELEMENT);
		if (formatHint->precision == FCDFormatHint::PRECISION_LOW) AddContent(precisionNode, DAE_FORMAT_HINT_LOW_VALUE);
		else if (formatHint->precision == FCDFormatHint::PRECISION_MID) AddContent(precisionNode, DAE_FORMAT_HINT_MID_VALUE);
		else if (formatHint->precision == FCDFormatHint::PRECISION_HIGH) AddContent(precisionNode, DAE_FORMAT_HINT_HIGH_VALUE);
		else AddContent(precisionNode, DAEERR_UNKNOWN_ELEMENT);

		for (fm::vector<FCDFormatHint::optionValue>::iterator it = formatHint->options.begin(); it != formatHint->options.end(); ++it)
		{
			xmlNode* optionNode = AddChild(formatHintNode, DAE_OPTION_ELEMENT);
			if (*it == FCDFormatHint::OPT_SRGB_GAMMA) AddContent(optionNode, DAE_FORMAT_HINT_SRGB_GAMMA_VALUE);
			else if (*it == FCDFormatHint::OPT_NORMALIZED3) AddContent(optionNode, DAE_FORMAT_HINT_NORMALIZED3_VALUE);
			else if (*it == FCDFormatHint::OPT_NORMALIZED4) AddContent(optionNode, DAE_FORMAT_HINT_NORMALIZED4_VALUE);
			else if (*it == FCDFormatHint::OPT_COMPRESSABLE) AddContent(optionNode, DAE_FORMAT_HINT_COMPRESSABLE_VALUE);
		}
	}
	if (effectParameterSurface->GetSize().x != 0.f || effectParameterSurface->GetSize().y != 0.f || effectParameterSurface->GetSize().z != 0.f)
	{
		xmlNode* sizeNode = AddChild(surfaceNode, DAE_SIZE_ELEMENT);
		AddContent(sizeNode, FUStringConversion::ToString(effectParameterSurface->GetSize()));
	}
	else if (effectParameterSurface->GetViewportRatio() != 1.f) //viewport_ratio can only be there if size is not specified
	{
		xmlNode* viewportRatioNode = AddChild(surfaceNode, DAE_VIEWPORT_RATIO);
		AddContent(viewportRatioNode, FUStringConversion::ToString(effectParameterSurface->GetViewportRatio()));
	}
	
	if (effectParameterSurface->GetMipLevelCount() != 0)
	{
		xmlNode* mipmapLevelsNode = AddChild(surfaceNode, DAE_MIP_LEVELS);
		AddContent(mipmapLevelsNode, FUStringConversion::ToString(effectParameterSurface->GetMipLevelCount()));
	}
	
	return parameterNode;
}

xmlNode* FArchiveXML::WriteEffectParameterVector(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectParameterVector* effectParameterVector = (FCDEffectParameterVector*)object;

	if (effectParameterVector->IsReferencer())
	{
		xmlNode* parameterNode = FArchiveXML::WriteEffectParameter(effectParameterVector, parentNode);
		AddAttribute(parameterNode, DAE_FXSTD_STATE_REF_ELEMENT, effectParameterVector->GetReference());
		return parameterNode; 
	}
	else if (effectParameterVector->IsAnimator())
	{
		xmlNode* parameterNode = FArchiveXML::WriteEffectParameter(effectParameterVector, parentNode);
		AddAttribute(parameterNode, DAE_FXSTD_STATE_REF_ELEMENT, effectParameterVector->GetReference());
		AddAttribute(parameterNode, DAE_TYPE_ATTRIBUTE, DAEFC_FLOAT4_ATTRIBUTE_TYPE);
		return parameterNode;
	}
	else
	{
		xmlNode* parameterNode = FArchiveXML::WriteEffectParameter(effectParameterVector, parentNode);
		FUSStringBuilder builder;
		FUStringConversion::ToString(builder, effectParameterVector->GetValue()); 
		AddChild(parameterNode, (effectParameterVector->GetFloatType() == FCDEffectParameterVector::FLOAT) ? DAE_FXCMN_FLOAT4_ELEMENT : DAE_FXCMN_HALF4_ELEMENT, builder);
		const char* wantedSubId = effectParameterVector->GetReference().c_str();
		if (*wantedSubId == 0) wantedSubId = effectParameterVector->GetSemantic().c_str();
		if (*wantedSubId == 0) wantedSubId = "flt4";
		FArchiveXML::WriteAnimatedValue(&effectParameterVector->GetValue(), parameterNode, wantedSubId);
		return parameterNode;
	}
}

xmlNode* FArchiveXML::WriteEffectPass(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectPass* effectPass = (FCDEffectPass*)object;

	// Write out the <pass> element, with the shader's name
	xmlNode* passNode = AddChild(parentNode, DAE_PASS_ELEMENT);
	if (!effectPass->GetPassName().empty())
	{
		fstring& _name = const_cast<fstring&>(effectPass->GetPassName());
		AddNodeSid(passNode, _name);
	}

	// Write out the render states
	for (size_t i = 0 ; i < effectPass->GetRenderStateCount(); ++i)
	{
		FArchiveXML::LetWriteObject(effectPass->GetRenderState(i), passNode);
	}

	// Write out the shaders
	for (size_t i = 0 ; i < effectPass->GetShaderCount(); ++i)
	{
		FArchiveXML::LetWriteObject(effectPass->GetShader(i), passNode);
	}

	return passNode;
}

xmlNode* FArchiveXML::WriteEffectPassShader(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectPassShader* effectPassShader = (FCDEffectPassShader*)object;

	xmlNode* shaderNode = AddChild(parentNode, DAE_SHADER_ELEMENT);

	// Write out the compiler information and the shader's name/stage
	if (!effectPassShader->GetCompilerTarget().empty()) AddChild(shaderNode, DAE_FXCMN_COMPILERTARGET_ELEMENT, effectPassShader->GetCompilerTarget());
	if (!effectPassShader->GetCompilerOptions().empty()) AddChild(shaderNode, DAE_FXCMN_COMPILEROPTIONS_ELEMENT, effectPassShader->GetCompilerOptions());
	AddAttribute(shaderNode, DAE_STAGE_ATTRIBUTE, effectPassShader->IsFragmentShader() ? DAE_FXCMN_FRAGMENT_SHADER : DAE_FXCMN_VERTEX_SHADER);
	if (!effectPassShader->GetName().empty())
	{
		xmlNode* nameNode = AddChild(shaderNode, DAE_FXCMN_NAME_ELEMENT, effectPassShader->GetName());
		if (effectPassShader->GetCode() != NULL) AddAttribute(nameNode, DAE_SOURCE_ATTRIBUTE, effectPassShader->GetCode()->GetSubId());
	}

	// Write out the bindings
	for (size_t i = 0; i < effectPassShader->GetBindingCount(); ++i)
	{
		const FCDEffectPassBind* b = effectPassShader->GetBinding(i);
		if (!b->reference->empty() && !b->symbol->empty())
		{
			xmlNode* bindNode = AddChild(shaderNode, DAE_BIND_ELEMENT);
			AddAttribute(bindNode, DAE_SYMBOL_ATTRIBUTE, b->symbol);
			xmlNode* paramNode = AddChild(bindNode, DAE_PARAMETER_ELEMENT);
			AddAttribute(paramNode, DAE_REF_ATTRIBUTE, b->reference);
		}
	}
	return shaderNode;
}

xmlNode* FArchiveXML::WriteEffectPassState(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectPassState* effectPassState = (FCDEffectPassState*)object;
	FUSStringBuilder builder;

	xmlNode* stateNode = AddChild(parentNode, FUDaePassState::ToString(effectPassState->GetType()));

#define NODE_TYPE(offset, node, valueType, castType) \
	AddAttribute(node, DAE_VALUE_ATTRIBUTE, FUStringConversion::ToString((castType) *((valueType*)(data + offset))));
#define NODE_INDEX(offset, node) \
	AddAttribute(node, DAE_INDEX_ATTRIBUTE, FUStringConversion::ToString((uint32) *((uint8*)(data + offset))));
#define NODE_ENUM(offset, node, nameSpace, enumName) \
	AddAttribute(node, DAE_VALUE_ATTRIBUTE, nameSpace::ToString((nameSpace::enumName) *((uint32*)(data + offset))));

#define CHILD_NODE_TYPE(offset, elementName, valueType, castType) { \
	xmlNode* node = AddChild(stateNode, elementName); \
	NODE_TYPE(offset, node, valueType, castType); }
#define CHILD_NODE_ENUM(offset, elementName, nameSpace, enumName) { \
	xmlNode* node = AddChild(stateNode, elementName); \
	NODE_ENUM(offset, node, nameSpace, enumName); }

	uint8* data = effectPassState->GetData();
	switch (effectPassState->GetType())
	{
	case FUDaePassState::ALPHA_FUNC:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FUNC_ELEMENT, FUDaePassStateFunction, Function);
		CHILD_NODE_TYPE(4, DAE_FXSTD_STATE_VALUE_ELEMENT, float, float);
		break;

	case FUDaePassState::BLEND_FUNC:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_SRC_ELEMENT, FUDaePassStateBlendType, Type);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_DEST_ELEMENT, FUDaePassStateBlendType, Type);
		break;

	case FUDaePassState::BLEND_FUNC_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_SRCRGB_ELEMENT, FUDaePassStateBlendType, Type);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_DESTRGB_ELEMENT, FUDaePassStateBlendType, Type);
		CHILD_NODE_ENUM(8, DAE_FXSTD_STATE_SRCALPHA_ELEMENT, FUDaePassStateBlendType, Type);
		CHILD_NODE_ENUM(12, DAE_FXSTD_STATE_DESTALPHA_ELEMENT, FUDaePassStateBlendType, Type);
		break;

	case FUDaePassState::BLEND_EQUATION:
		NODE_ENUM(0, stateNode, FUDaePassStateBlendEquation, Equation);
		break;

	case FUDaePassState::BLEND_EQUATION_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_RGB_ELEMENT, FUDaePassStateBlendEquation, Equation);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_ALPHA_ELEMENT, FUDaePassStateBlendEquation, Equation);
		break;

	case FUDaePassState::COLOR_MATERIAL:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType, Type);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_MODE_ELEMENT, FUDaePassStateMaterialType, Type);
		break;

	case FUDaePassState::CULL_FACE:
		NODE_ENUM(0, stateNode, FUDaePassStateFaceType, Type);
		break;

	case FUDaePassState::DEPTH_FUNC:
		NODE_ENUM(0, stateNode, FUDaePassStateFunction, Function);
		break;

	case FUDaePassState::FOG_MODE:
		NODE_ENUM(0, stateNode, FUDaePassStateFogType, Type);
		break;

	case FUDaePassState::FOG_COORD_SRC:
		NODE_ENUM(0, stateNode, FUDaePassStateFogCoordinateType, Type);
		break;

	case FUDaePassState::FRONT_FACE:
		NODE_ENUM(0, stateNode, FUDaePassStateFrontFaceType, Type);
		break;

	case FUDaePassState::LIGHT_MODEL_COLOR_CONTROL:
		NODE_ENUM(0, stateNode, FUDaePassStateLightModelColorControlType, Type);
		break;

	case FUDaePassState::LOGIC_OP:
		NODE_ENUM(0, stateNode, FUDaePassStateLogicOperation, Operation);
		break;

	case FUDaePassState::POLYGON_MODE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType, Type);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_MODE_ELEMENT, FUDaePassStatePolygonMode, Mode);
		break;

	case FUDaePassState::SHADE_MODEL:
		NODE_ENUM(0, stateNode, FUDaePassStateShadeModel, Model);
		break;

	case FUDaePassState::STENCIL_FUNC:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FUNC_ELEMENT, FUDaePassStateFunction, Function);
		CHILD_NODE_TYPE(4, DAE_FXSTD_STATE_REF_ELEMENT, uint8, uint32);
		CHILD_NODE_TYPE(5, DAE_FXSTD_STATE_MASK_ELEMENT, uint8, uint32);
		break;

	case FUDaePassState::STENCIL_OP:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FAIL_ELEMENT, FUDaePassStateStencilOperation, Operation);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_ZFAIL_ELEMENT, FUDaePassStateStencilOperation, Operation);
		CHILD_NODE_ENUM(8, DAE_FXSTD_STATE_ZPASS_ELEMENT, FUDaePassStateStencilOperation, Operation);
		break;

	case FUDaePassState::STENCIL_FUNC_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FRONT_ELEMENT, FUDaePassStateFunction, Function);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_BACK_ELEMENT, FUDaePassStateFunction, Function);
		CHILD_NODE_TYPE(8, DAE_FXSTD_STATE_REF_ELEMENT, uint8, uint32);
		CHILD_NODE_TYPE(9, DAE_FXSTD_STATE_MASK_ELEMENT, uint8, uint32);
		break;

	case FUDaePassState::STENCIL_OP_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType, Type);
		CHILD_NODE_ENUM(4, DAE_FXSTD_STATE_FAIL_ELEMENT, FUDaePassStateStencilOperation, Operation);
		CHILD_NODE_ENUM(8, DAE_FXSTD_STATE_ZFAIL_ELEMENT, FUDaePassStateStencilOperation, Operation);
		CHILD_NODE_ENUM(12, DAE_FXSTD_STATE_ZPASS_ELEMENT, FUDaePassStateStencilOperation, Operation);
		break;

	case FUDaePassState::STENCIL_MASK_SEPARATE:
		CHILD_NODE_ENUM(0, DAE_FXSTD_STATE_FACE_ELEMENT, FUDaePassStateFaceType, Type);
		CHILD_NODE_TYPE(4, DAE_FXSTD_STATE_MASK_ELEMENT, uint8, uint32);
		break;

	case FUDaePassState::LIGHT_AMBIENT:
	case FUDaePassState::LIGHT_DIFFUSE:
	case FUDaePassState::LIGHT_SPECULAR:
	case FUDaePassState::LIGHT_POSITION:
	case FUDaePassState::TEXTURE_ENV_COLOR:
	case FUDaePassState::CLIP_PLANE:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, FMVector4, FMVector4);
		break;

	case FUDaePassState::LIGHT_CONSTANT_ATTENUATION:
	case FUDaePassState::LIGHT_LINEAR_ATTENUATION:
	case FUDaePassState::LIGHT_QUADRATIC_ATTENUATION:
	case FUDaePassState::LIGHT_SPOT_CUTOFF:
	case FUDaePassState::LIGHT_SPOT_EXPONENT:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, float, float);
		break;

	case FUDaePassState::LIGHT_SPOT_DIRECTION:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, FMVector3, FMVector3);
		break;

	case FUDaePassState::TEXTURE1D:
	case FUDaePassState::TEXTURE2D:
	case FUDaePassState::TEXTURE3D:
	case FUDaePassState::TEXTURECUBE:
	case FUDaePassState::TEXTURERECT:
	case FUDaePassState::TEXTUREDEPTH:
		NODE_INDEX(0, stateNode);
		NODE_TYPE(1, stateNode, uint32, uint32);
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
		NODE_TYPE(1, stateNode, bool, bool);
		break;

	case FUDaePassState::TEXTURE_ENV_MODE: {
		NODE_INDEX(0, stateNode);
		fm::string overrideAvoidance((const char*) (data + 1), 254);
		AddAttribute(stateNode, DAE_VALUE_ATTRIBUTE, overrideAvoidance);
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
		NODE_TYPE(0, stateNode, FMVector4, FMVector4);
		break;

	case FUDaePassState::POINT_DISTANCE_ATTENUATION:
		NODE_TYPE(0, stateNode, FMVector3, FMVector3);
		break;

	case FUDaePassState::DEPTH_BOUNDS:
	case FUDaePassState::DEPTH_RANGE:
	case FUDaePassState::POLYGON_OFFSET:
		NODE_TYPE(0, stateNode, FMVector2, FMVector2);
		break;

	case FUDaePassState::CLEAR_STENCIL:
	case FUDaePassState::STENCIL_MASK:
		NODE_TYPE(0, stateNode, uint32, uint32);
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
		NODE_TYPE(0, stateNode, float, float);
		break;

	case FUDaePassState::COLOR_MASK:
		builder.set(*(bool*)(data + 0)); builder.append(' ');
		builder.append(*(bool*)(data + 1)); builder.append(' ');
		builder.append(*(bool*)(data + 2)); builder.append(' ');
		builder.append(*(bool*)(data + 3));
		AddAttribute(stateNode, DAE_VALUE_ATTRIBUTE, builder);
		break;

	case FUDaePassState::LINE_STIPPLE:
		builder.set((uint32) *(uint16*)(data + 0)); builder.append(' ');
		builder.append((uint32) *(uint16*)(data + 2));
		AddAttribute(stateNode, DAE_VALUE_ATTRIBUTE, builder);
		break;

	case FUDaePassState::MODEL_VIEW_MATRIX:
	case FUDaePassState::PROJECTION_MATRIX:
		NODE_TYPE(0, stateNode, FMMatrix44, FMMatrix44);
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
		NODE_TYPE(0, stateNode, bool, bool);
		break;

	case FUDaePassState::COUNT:
	case FUDaePassState::INVALID:
	default:
		break;
	}

#undef NODE_TYPE
#undef NODE_INDEX
#undef NODE_ENUM
#undef CHILD_NODE_TYPE
#undef CHILD_NODE_ENUM

	return stateNode;
}

xmlNode* FArchiveXML::WriteEffectProfile(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectProfile* effectProfile = (FCDEffectProfile*)object;

	xmlNode* profileNode = FUDaeWriter::AddChild(parentNode, FUDaeProfileType::ToString(effectProfile->GetType()));

	// Write out the parameters
	size_t parameterCount = effectProfile->GetEffectParameterCount();
	for (size_t p = 0; p < parameterCount; ++p)
	{
		FArchiveXML::LetWriteObject(effectProfile->GetEffectParameter(p), profileNode);
	}

	return profileNode;
}

xmlNode* FArchiveXML::WriteEffectProfileFX(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectProfileFX* effectProfileFX = (FCDEffectProfileFX*)object;

	// Call the parent to create the profile node and to export the parameters.
	xmlNode* profileNode = FArchiveXML::WriteEffectProfile(effectProfileFX, parentNode);

	// Write out the profile properties/base elements
	if (!effectProfileFX->GetPlatform().empty()) AddAttribute(profileNode, DAE_PLATFORM_ATTRIBUTE, effectProfileFX->GetPlatform());

	// Write out the code/includes
	// These will include themselves before the parameters
	for (size_t i = 0; i < effectProfileFX->GetCodeCount(); ++i)
	{
		FArchiveXML::LetWriteObject(effectProfileFX->GetCode(i), profileNode);
	}

	// Write out the techniques
	for (size_t i = 0; i < effectProfileFX->GetTechniqueCount(); ++i)
	{
		FArchiveXML::LetWriteObject(effectProfileFX->GetTechnique(i), profileNode);
	}

	FArchiveXML::LetWriteObject(effectProfileFX->GetExtra(), profileNode);
	return profileNode;
}

xmlNode* FArchiveXML::WriteEffectStandard(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectStandard* effectStandard = (FCDEffectStandard*)object;

	// Call the parent to create the profile node and to export the parameters.
	xmlNode* profileNode = FArchiveXML::WriteEffectProfile(effectStandard, parentNode);
	xmlNode* techniqueCommonNode = AddChild(profileNode, DAE_TECHNIQUE_ELEMENT);
	AddNodeSid(techniqueCommonNode, "common");

	const char* materialName;
	switch (effectStandard->GetLightingType())
	{
	case FCDEffectStandard::CONSTANT: materialName = DAE_FXSTD_CONSTANT_ELEMENT; break;
	case FCDEffectStandard::LAMBERT: materialName = DAE_FXSTD_LAMBERT_ELEMENT; break;
	case FCDEffectStandard::PHONG: materialName = DAE_FXSTD_PHONG_ELEMENT; break;
	case FCDEffectStandard::BLINN: materialName = DAE_FXSTD_BLINN_ELEMENT; break;
	case FCDEffectStandard::UNKNOWN:
	default: materialName = DAEERR_UNKNOWN_ELEMENT; break;
	}
	xmlNode* materialNode = AddChild(techniqueCommonNode, materialName);
	xmlNode* techniqueNode = AddExtraTechniqueChild(techniqueCommonNode, DAE_FCOLLADA_PROFILE);

	// Export the color/float parameters
	if (!effectStandard->IsEmissionFactor())
	{
		FArchiveXML::WriteColorTextureParameter(effectStandard, materialNode, DAE_EMISSION_MATERIAL_PARAMETER, effectStandard->GetEmissionColorParam(), FUDaeTextureChannel::EMISSION);
	}
	if (effectStandard->GetLightingType() != FCDEffectStandard::CONSTANT)
	{
		FArchiveXML::WriteColorTextureParameter(effectStandard, materialNode, DAE_AMBIENT_MATERIAL_PARAMETER, effectStandard->GetAmbientColorParam(), FUDaeTextureChannel::AMBIENT);
		FArchiveXML::WriteColorTextureParameter(effectStandard, materialNode, DAE_DIFFUSE_MATERIAL_PARAMETER, effectStandard->GetDiffuseColorParam(), FUDaeTextureChannel::DIFFUSE);
		if (effectStandard->GetLightingType() != FCDEffectStandard::LAMBERT)
		{
			FArchiveXML::WriteColorTextureParameter(effectStandard, materialNode, DAE_SPECULAR_MATERIAL_PARAMETER, effectStandard->GetSpecularColorParam(), FUDaeTextureChannel::SPECULAR);
			FArchiveXML::WriteFloatTextureParameter(effectStandard, materialNode, DAE_SHININESS_MATERIAL_PARAMETER, effectStandard->GetShininessParam(), FUDaeTextureChannel::UNKNOWN);
			if (effectStandard->GetTextureCount(FUDaeTextureChannel::SHININESS) > 0)
			{
				FArchiveXML::WriteFloatTextureParameter(effectStandard, techniqueNode, DAE_SHININESS_MATERIAL_PARAMETER, effectStandard->GetShininessParam(), FUDaeTextureChannel::SHININESS);
			}
			if (effectStandard->GetSpecularFactor() != 1.0f)
			{
				FArchiveXML::WriteFloatTextureParameter(effectStandard, techniqueNode, DAEMAX_SPECLEVEL_MATERIAL_PARAMETER, effectStandard->GetSpecularFactorParam(), FUDaeTextureChannel::SPECULAR_LEVEL);
			}
		}
	}

	// Export the reflectivity parameters, which belong to the common material.
	if (effectStandard->IsReflective())
	{
		FArchiveXML::WriteColorTextureParameter(effectStandard, materialNode, DAE_REFLECTIVE_MATERIAL_PARAMETER, effectStandard->GetReflectivityColorParam(), FUDaeTextureChannel::REFLECTION);
		FArchiveXML::WriteFloatTextureParameter(effectStandard, materialNode, DAE_REFLECTIVITY_MATERIAL_PARAMETER, effectStandard->GetReflectivityFactorParam(), FUDaeTextureChannel::UNKNOWN);
	}
	
	// Translucency includes both transparent and opacity textures
	xmlNode* transparentNode = FArchiveXML::WriteColorTextureParameter(effectStandard, materialNode, DAE_TRANSPARENT_MATERIAL_PARAMETER, effectStandard->GetTranslucencyColorParam(), FUDaeTextureChannel::TRANSPARENT);
	AddAttribute(transparentNode, DAE_OPAQUE_MATERIAL_ATTRIBUTE, effectStandard->GetTransparencyMode() == FCDEffectStandard::RGB_ZERO ? DAE_RGB_ZERO_ELEMENT : DAE_A_ONE_ELEMENT);
	FArchiveXML::WriteFloatTextureParameter(effectStandard, materialNode, DAE_TRANSPARENCY_MATERIAL_PARAMETER, effectStandard->GetTranslucencyFactorParam(), FUDaeTextureChannel::UNKNOWN);

	// The index of refraction parameter belongs to the common material.
	if (effectStandard->IsRefractive())
	{
		FArchiveXML::WriteFloatTextureParameter(effectStandard, materialNode, DAE_INDEXOFREFRACTION_MATERIAL_PARAMETER, effectStandard->GetIndexOfRefractionParam(), FUDaeTextureChannel::UNKNOWN);
	}

	// Non-COLLADA parameters
	if (effectStandard->GetTextureCount(FUDaeTextureChannel::BUMP) > 0)
	{
		FArchiveXML::WriteFloatTextureParameter(effectStandard, techniqueNode, DAE_BUMP_MATERIAL_PARAMETER, NULL, FUDaeTextureChannel::BUMP);
	}
	if (effectStandard->IsEmissionFactor())
	{
		FArchiveXML::WriteFloatTextureParameter(effectStandard, techniqueNode, DAEMAX_EMISSIONLEVEL_MATERIAL_PARAMETER, effectStandard->GetEmissionFactorParam(), FUDaeTextureChannel::UNKNOWN);
	}
	if (effectStandard->GetTextureCount(FUDaeTextureChannel::DISPLACEMENT) > 0)
	{
		FArchiveXML::WriteFloatTextureParameter(effectStandard, techniqueNode, DAEMAX_DISPLACEMENT_MATERIAL_PARAMETER, NULL, FUDaeTextureChannel::DISPLACEMENT);
	}
	if (effectStandard->GetTextureCount(FUDaeTextureChannel::FILTER) > 0)
	{
		FArchiveXML::WriteColorTextureParameter(effectStandard, techniqueNode, DAEMAX_FILTERCOLOR_MATERIAL_PARAMETER, NULL, FUDaeTextureChannel::FILTER);
	}
	if (effectStandard->GetTextureCount(FUDaeTextureChannel::REFRACTION) > 0)
	{
		FArchiveXML::WriteColorTextureParameter(effectStandard, techniqueNode, DAEMAX_INDEXOFREFRACTION_MATERIAL_PARAMETER, NULL, FUDaeTextureChannel::REFRACTION);
	}

	FArchiveXML::LetWriteObject(effectStandard->GetExtra(), profileNode);
	return profileNode;
}

xmlNode* FArchiveXML::WriteEffectTechnique(FCDObject* object, xmlNode* parentNode)
{
	FCDEffectTechnique* effectTechnique = (FCDEffectTechnique*)object;

	xmlNode* techniqueNode = AddChild(parentNode, DAE_TECHNIQUE_ELEMENT);
	fstring& _name = const_cast<fstring&>(effectTechnique->GetName());
	if (_name.empty()) _name = FC("common");
	AddNodeSid(techniqueNode, _name);

	// Write out the code/includes
	for (size_t i = 0; i < effectTechnique->GetCodeCount(); ++i)
	{
		FArchiveXML::LetWriteObject(effectTechnique->GetCode(i), techniqueNode);
	}

	// Write out the effect parameters at this level
	size_t parameterCount = effectTechnique->GetEffectParameterCount();
	for (size_t p = 0; p < parameterCount; ++p)
	{
		FArchiveXML::LetWriteObject(effectTechnique->GetEffectParameter(p), techniqueNode);
	}

	// Write out the passes.
	// In COLLADA 1.4: there should always be at least one pass.
	if (effectTechnique->GetPassCount() > 0)
	{
		size_t passCount = effectTechnique->GetPassCount();
		for (size_t p = 0; p < passCount; ++p)
		{
			FArchiveXML::LetWriteObject(effectTechnique->GetPass(p), techniqueNode);
		}
	}
	else
	{
		UNUSED(xmlNode* dummyPassNode =) AddChild(techniqueNode, DAE_PASS_ELEMENT);
	}

	return techniqueNode;
}

xmlNode* FArchiveXML::WriteEffect(FCDObject* object, xmlNode* parentNode)
{
	FCDEffect* effect = (FCDEffect*)object;

	xmlNode* effectNode = FArchiveXML::WriteToEntityXMLFCDEntity(effect, parentNode, DAE_EFFECT_ELEMENT);

	// Write out the parameters
	size_t parameterCount = effect->GetEffectParameterCount();
	for (size_t p = 0; p < parameterCount; ++p)
	{
		FArchiveXML::LetWriteObject(effect->GetEffectParameter(p), effectNode);
	}

	// Write out the profiles
	size_t profileCount = effect->GetProfileCount();
	for (size_t p = 0; p < profileCount; ++p)
	{
		FArchiveXML::LetWriteObject(effect->GetProfile(p), effectNode);
	}

	FArchiveXML::WriteEntityExtra(effect, effectNode);
	return effectNode;
}

xmlNode* FArchiveXML::WriteTexture(FCDObject* object, xmlNode* parentNode)
{
	FCDTexture* texture = (FCDTexture*)object;

	// Create the <texture> element
	xmlNode* textureNode = AddChild(parentNode, DAE_TEXTURE_ELEMENT);
	AddAttribute(textureNode, DAE_FXSTD_TEXTURE_ATTRIBUTE, (texture->GetSampler() != NULL) ? texture->GetSampler()->GetReference() : "");
	AddAttribute(textureNode, DAE_FXSTD_TEXTURESET_ATTRIBUTE, (texture->GetSet() != NULL) ? texture->GetSet()->GetSemantic() : "");
	FArchiveXML::LetWriteObject(texture->GetExtra(), textureNode);
	return textureNode;
}

xmlNode* FArchiveXML::WriteImage(FCDObject* object, xmlNode* parentNode)
{
	FCDImage* image = (FCDImage*)object;

	xmlNode* imageNode = FArchiveXML::WriteToEntityXMLFCDEntity(image, parentNode, DAE_IMAGE_ELEMENT);
	if (!image->GetFilename().empty())
	{
		fstring url = image->GetDocument()->GetFileManager()->CleanUri(FUUri(image->GetFilename()));
		FUXmlWriter::ConvertFilename(url);
		AddChild(imageNode, DAE_INITFROM_ELEMENT, url);
	}

	if (image->GetWidth() > 0) AddAttribute(imageNode, DAE_WIDTH_ELEMENT, image->GetWidth());
	if (image->GetHeight() > 0) AddAttribute(imageNode, DAE_HEIGHT_ELEMENT, image->GetHeight());
	if (image->GetDepth() > 0) AddAttribute(imageNode, DAE_DEPTH_ELEMENT, image->GetDepth());

	FArchiveXML::WriteEntityExtra(image, imageNode);
	return imageNode;
}


xmlNode* FArchiveXML::WriteColorTextureParameter(FCDEffectStandard* effectStandard, xmlNode* parentNode, const char* parameterNodeName, const FCDEffectParameterColor4* value, uint32 bucketIndex)
{
	xmlNode* parameterNode = AddChild(parentNode, parameterNodeName);
	if (FArchiveXML::WriteTextureParameter(effectStandard, parameterNode, bucketIndex) == NULL)
	{
		if (value->IsConstant())
		{
			fm::string colorValue = FUStringConversion::ToString((const FMVector4&) value->GetValue());
			xmlNode* valueNode = AddChild(parameterNode, DAE_FXSTD_COLOR_ELEMENT, colorValue);
			FArchiveXML::WriteAnimatedValue(&value->GetValue(), valueNode, parameterNodeName);
		}
		else if (value->IsReferencer())
		{
			xmlNode* child = FArchiveXML::LetWriteObject((FCDObject*)value, parameterNode);
			FArchiveXML::WriteAnimatedValue(&value->GetValue(), child, parameterNodeName);
		}
	}
	return parameterNode;
}


xmlNode* FArchiveXML::WriteFloatTextureParameter(FCDEffectStandard* effectStandard, xmlNode* parentNode, const char* parameterNodeName, const FCDEffectParameterFloat* value, uint32 bucketIndex)
{
	xmlNode* parameterNode = AddChild(parentNode, parameterNodeName);
	if (FArchiveXML::WriteTextureParameter(effectStandard, parameterNode, bucketIndex) == NULL)
	{
		if (value->IsConstant())
		{
			xmlNode* valueNode = AddChild(parameterNode, DAE_FXSTD_FLOAT_ELEMENT, value->GetValue());
			FArchiveXML::WriteAnimatedValue(&value->GetValue(), valueNode, parameterNodeName);
		}
		else if (value->IsReferencer())
		{
			xmlNode* child = FArchiveXML::LetWriteObject((FCDObject*)value, parameterNode);
			FArchiveXML::WriteAnimatedValue(&value->GetValue(), child, parameterNodeName);
		}	
	}
	return parameterNode;
}

xmlNode* FArchiveXML::WriteTextureParameter(FCDEffectStandard* effectStandard, xmlNode* parentNode, uint32 bucketIndex)
{
	xmlNode* textureNode = NULL;
	if (bucketIndex != FUDaeTextureChannel::UNKNOWN)
	{
		size_t textureCount = effectStandard->GetTextureCount(bucketIndex);
		for (size_t t = 0; t < textureCount; ++t)
		{
			xmlNode* newTextureNode = FArchiveXML::LetWriteObject(effectStandard->GetTexture(bucketIndex, t), parentNode);
			if (newTextureNode != NULL && textureNode == NULL) textureNode = newTextureNode;
		}
	}
	return textureNode;
}
