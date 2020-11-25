/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterFactory.h"
#include "FCDocument/FCDEffectStandard.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDTexture.h"
#include "FCDocument/FCDVersion.h"
#include "FUtils/FUDaeEnum.h"
#include "FUtils/FUStringConversion.h"

//
// FCDEffectStandard
//

ImplementObjectType(FCDEffectStandard);
ImplementParameterObject(FCDEffectStandard, FCDTexture, emissionTextures, new FCDTexture(parent->GetDocument(), parent));
ImplementParameterObject(FCDEffectStandard, FCDEffectParameterColor4, emissionColor, new FCDEffectParameterColor4(parent->GetDocument()));
ImplementParameterObject(FCDEffectStandard, FCDEffectParameterFloat, emissionFactor, new FCDEffectParameterFloat(parent->GetDocument()));
ImplementParameterObject(FCDEffectStandard, FCDTexture, reflectivityTextures, new FCDTexture(parent->GetDocument(), parent));
ImplementParameterObject(FCDEffectStandard, FCDEffectParameterColor4, reflectivityColor, new FCDEffectParameterColor4(parent->GetDocument()));
ImplementParameterObject(FCDEffectStandard, FCDEffectParameterFloat, reflectivityFactor, new FCDEffectParameterFloat(parent->GetDocument()));
ImplementParameterObject(FCDEffectStandard, FCDTexture, refractionTextures, new FCDTexture(parent->GetDocument(), parent));
ImplementParameterObject(FCDEffectStandard, FCDEffectParameterFloat, indexOfRefraction, new FCDEffectParameterFloat(parent->GetDocument()));
ImplementParameterObject(FCDEffectStandard, FCDTexture, translucencyTextures, new FCDTexture(parent->GetDocument(), parent));
ImplementParameterObject(FCDEffectStandard, FCDEffectParameterColor4, translucencyColor, new FCDEffectParameterColor4(parent->GetDocument()));
ImplementParameterObject(FCDEffectStandard, FCDEffectParameterFloat, translucencyFactor, new FCDEffectParameterFloat(parent->GetDocument()));
ImplementParameterObject(FCDEffectStandard, FCDTexture, diffuseTextures, new FCDTexture(parent->GetDocument(), parent));
ImplementParameterObject(FCDEffectStandard, FCDEffectParameterColor4, diffuseColor, new FCDEffectParameterColor4(parent->GetDocument()));
ImplementParameterObject(FCDEffectStandard, FCDTexture, ambientTextures, new FCDTexture(parent->GetDocument(), parent));
ImplementParameterObject(FCDEffectStandard, FCDEffectParameterColor4, ambientColor, new FCDEffectParameterColor4(parent->GetDocument()));
ImplementParameterObject(FCDEffectStandard, FCDTexture, specularTextures, new FCDTexture(parent->GetDocument()));
ImplementParameterObject(FCDEffectStandard, FCDEffectParameterColor4, specularColor, new FCDEffectParameterColor4(parent->GetDocument()));
ImplementParameterObject(FCDEffectStandard, FCDTexture, specularFactorTextures, new FCDTexture(parent->GetDocument(), parent));
ImplementParameterObject(FCDEffectStandard, FCDEffectParameterFloat, specularFactor, new FCDEffectParameterFloat(parent->GetDocument()));
ImplementParameterObject(FCDEffectStandard, FCDTexture, shininessTextures, new FCDTexture(parent->GetDocument(), parent));
ImplementParameterObject(FCDEffectStandard, FCDEffectParameterFloat, shininess, new FCDEffectParameterFloat(parent->GetDocument()));
ImplementParameterObject(FCDEffectStandard, FCDTexture, bumpTextures, new FCDTexture(parent->GetDocument(), parent));
ImplementParameterObject(FCDEffectStandard, FCDTexture, displacementTextures, new FCDTexture(parent->GetDocument(), parent));
ImplementParameterObject(FCDEffectStandard, FCDTexture, filterTextures, new FCDTexture(parent->GetDocument()));

const fm::string FCDEffectStandard::EmissionColorSemantic("EMISSION");
const fm::string FCDEffectStandard::EmissionFactorSemantic("EMISSIONFACTOR");
const fm::string FCDEffectStandard::ReflectivityColorSemantic("REFLECTIVITY");
const fm::string FCDEffectStandard::ReflectivityFactorSemantic("REFLECTIVITYFACTOR");
const fm::string FCDEffectStandard::IndexOfRefractionSemantic("INDEXOFREFRACTION");
const fm::string FCDEffectStandard::TranslucencyColorSemantic("TRANSLUCENCY");
const fm::string FCDEffectStandard::TranslucencyFactorSemantic("TRANSLUCENCYFACTOR");
const fm::string FCDEffectStandard::DiffuseColorSemantic("DIFFUSE");
const fm::string FCDEffectStandard::AmbientColorSemantic("AMBIENT");
const fm::string FCDEffectStandard::SpecularColorSemantic("SPECULAR");
const fm::string FCDEffectStandard::SpecularFactorSemantic("SPECULARFACTOR");
const fm::string FCDEffectStandard::ShininessSemantic("SHININESS");

FCDEffectStandard::FCDEffectStandard(FCDocument* document, FCDEffect* _parent)
:	FCDEffectProfile(document, _parent)
,	InitializeParameter(type, CONSTANT)
,	InitializeParameterNoArg(emissionTextures)
,	InitializeParameterNoArg(emissionColor)
,	InitializeParameterNoArg(emissionFactor)
,	InitializeParameter(isEmissionFactor, false)
,	InitializeParameterNoArg(reflectivityTextures)
,	InitializeParameterNoArg(reflectivityColor)
,	InitializeParameterNoArg(reflectivityFactor)
,	InitializeParameter(isReflective, false)
,	InitializeParameterNoArg(refractionTextures)
,	InitializeParameterNoArg(indexOfRefraction)
,	InitializeParameter(isRefractive, false)
,	InitializeParameterNoArg(translucencyTextures)
,	InitializeParameterNoArg(translucencyColor)
,	InitializeParameterNoArg(translucencyFactor)
,	InitializeParameter(transparencyMode, A_ONE)
,	InitializeParameterNoArg(diffuseTextures)
,	InitializeParameterNoArg(diffuseColor)
,	InitializeParameterNoArg(ambientTextures)
,	InitializeParameterNoArg(ambientColor)
,	InitializeParameterNoArg(specularTextures)
,	InitializeParameterNoArg(specularColor)
,	InitializeParameterNoArg(specularFactorTextures)
,	InitializeParameterNoArg(specularFactor)
,	InitializeParameterNoArg(shininessTextures)
,	InitializeParameterNoArg(shininess)
,	InitializeParameterNoArg(bumpTextures)
,	InitializeParameterNoArg(displacementTextures)
,	InitializeParameterNoArg(filterTextures)
{
	emissionColor = new FCDEffectParameterColor4(GetDocument());
	emissionColor->SetValue(FMVector4::AlphaOne);
	emissionColor->SetConstant();
	emissionFactor = new FCDEffectParameterFloat(GetDocument());
	emissionFactor->SetValue(1.0f);
	emissionFactor->SetConstant();
	reflectivityColor = new FCDEffectParameterColor4(GetDocument());
	reflectivityColor->SetValue(FMVector4::One);
	reflectivityColor->SetConstant();
	reflectivityFactor = new FCDEffectParameterFloat(GetDocument());
	reflectivityFactor->SetValue(1.0f);
	reflectivityFactor->SetConstant();
	indexOfRefraction = new FCDEffectParameterFloat(GetDocument());
	indexOfRefraction->SetValue(1.0f);
	indexOfRefraction->SetConstant();
	translucencyColor = new FCDEffectParameterColor4(GetDocument());
	translucencyColor->SetValue(FMVector4::AlphaOne);
	translucencyColor->SetConstant();
	translucencyFactor = new FCDEffectParameterFloat(GetDocument());
	translucencyFactor->SetValue(1.0f);
	translucencyFactor->SetConstant();
	diffuseColor = new FCDEffectParameterColor4(GetDocument());
	diffuseColor->SetValue(FMVector4::AlphaOne);
	diffuseColor->SetConstant();
	ambientColor = new FCDEffectParameterColor4(GetDocument());
	ambientColor->SetValue(FMVector4::AlphaOne);
	ambientColor->SetConstant();
	specularColor = new FCDEffectParameterColor4(GetDocument());
	specularColor->SetValue(FMVector4::AlphaOne);
	specularColor->SetConstant();
	specularFactor = new FCDEffectParameterFloat(GetDocument());
	specularFactor->SetValue(1.0f);
	specularFactor->SetConstant();
	shininess = new FCDEffectParameterFloat(GetDocument());
	shininess->SetValue(20.0f);
	shininess->SetConstant();

	// Note: the 1.4.1 spec calls for A_ONE as default, which breaks backwards compatibility
	// with 1.3 by having a transparency factor of 1 for opaque instead of 0 in 1.3.
	// If we're lower than 1.4.1, we default to RGB_ZERO and a transparency (or translucency)
	// factor of 0.0.
	if (GetDocument()->GetVersion() < FCDVersion(1,4,1)) transparencyMode = RGB_ZERO;
}

FCDEffectStandard::~FCDEffectStandard()
{
}

// Retrieve one of the buckets
const FCDTexture** FCDEffectStandard::GetTextureBucket(uint32 bucket) const
{
	switch (bucket)
	{
	case FUDaeTextureChannel::AMBIENT: return ambientTextures.begin();
	case FUDaeTextureChannel::BUMP: return bumpTextures.begin();
	case FUDaeTextureChannel::DIFFUSE: return diffuseTextures.begin();
	case FUDaeTextureChannel::DISPLACEMENT: return displacementTextures.begin();
	case FUDaeTextureChannel::EMISSION: return emissionTextures.begin();
	case FUDaeTextureChannel::FILTER: return filterTextures.begin();
	case FUDaeTextureChannel::REFLECTION: return reflectivityTextures.begin();
	case FUDaeTextureChannel::REFRACTION: return refractionTextures.begin();
	case FUDaeTextureChannel::SHININESS: return shininessTextures.begin();
	case FUDaeTextureChannel::SPECULAR: return specularTextures.begin();
	case FUDaeTextureChannel::SPECULAR_LEVEL: return specularFactorTextures.begin();
	case FUDaeTextureChannel::TRANSPARENT: return translucencyTextures.begin();
	
	case FUDaeTextureChannel::UNKNOWN:
	default:
		FUFail(return filterTextures.begin()); // because I think this one will almost always be empty.
	}
}

size_t FCDEffectStandard::GetTextureCount(uint32 bucket) const
{
	switch (bucket)
	{
	case FUDaeTextureChannel::AMBIENT: return ambientTextures.size();
	case FUDaeTextureChannel::BUMP: return bumpTextures.size();
	case FUDaeTextureChannel::DIFFUSE: return diffuseTextures.size();
	case FUDaeTextureChannel::DISPLACEMENT: return displacementTextures.size();
	case FUDaeTextureChannel::EMISSION: return emissionTextures.size();
	case FUDaeTextureChannel::FILTER: return filterTextures.size();
	case FUDaeTextureChannel::REFLECTION: return reflectivityTextures.size();
	case FUDaeTextureChannel::REFRACTION: return refractionTextures.size();
	case FUDaeTextureChannel::SHININESS: return shininessTextures.size();
	case FUDaeTextureChannel::SPECULAR: return specularTextures.size();
	case FUDaeTextureChannel::SPECULAR_LEVEL: return specularFactorTextures.size();
	case FUDaeTextureChannel::TRANSPARENT: return translucencyTextures.size();
		 
	case FUDaeTextureChannel::UNKNOWN:
	default:
		FUFail(return 0);
	}
}

// Adds a texture to a specific channel.
FCDTexture* FCDEffectStandard::AddTexture(uint32 bucket)
{
	FCDTexture* texture = new FCDTexture(GetDocument(), this);
	switch (bucket)
	{
	case FUDaeTextureChannel::AMBIENT: ambientTextures.push_back(texture); break;
	case FUDaeTextureChannel::BUMP: bumpTextures.push_back(texture); break;
	case FUDaeTextureChannel::DIFFUSE: diffuseTextures.push_back(texture); break;
	case FUDaeTextureChannel::DISPLACEMENT: displacementTextures.push_back(texture); break;
	case FUDaeTextureChannel::EMISSION: emissionTextures.push_back(texture); break;
	case FUDaeTextureChannel::FILTER: filterTextures.push_back(texture); break;
	case FUDaeTextureChannel::REFLECTION: reflectivityTextures.push_back(texture); break;
	case FUDaeTextureChannel::REFRACTION: refractionTextures.push_back(texture); break;
	case FUDaeTextureChannel::SHININESS: shininessTextures.push_back(texture); break;
	case FUDaeTextureChannel::SPECULAR: specularTextures.push_back(texture); break;
	case FUDaeTextureChannel::SPECULAR_LEVEL: specularFactorTextures.push_back(texture); break;
	case FUDaeTextureChannel::TRANSPARENT: translucencyTextures.push_back(texture); break;
	
	case FUDaeTextureChannel::UNKNOWN:
	default:
		FUFail(texture->Release(); return NULL);
	}
	SetNewChildFlag();
	return texture;
}

// Calculate the opacity for this material
float FCDEffectStandard::GetOpacity() const
{
	if (transparencyMode == RGB_ZERO)
		return 1.0f - (translucencyColor->GetValue()->x + translucencyColor->GetValue()->x + translucencyColor->GetValue()->x) / 3.0f * translucencyFactor->GetValue();
	else
		return translucencyColor->GetValue()->w * translucencyFactor->GetValue();
}

// Calculate the overall reflectivity for this material
float FCDEffectStandard::GetReflectivity() const
{
	return (reflectivityColor->GetValue()->x + reflectivityColor->GetValue()->x + reflectivityColor->GetValue()->x) / 3.0f * reflectivityFactor->GetValue();
}

// Clone the standard effect
FCDEffectProfile* FCDEffectStandard::Clone(FCDEffectProfile* _clone) const
{
	FCDEffectStandard* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDEffectStandard(const_cast<FCDocument*>(GetDocument()), const_cast<FCDEffect*>(GetParent()));
	else if (_clone->GetObjectType() == FCDEffectStandard::GetClassType()) clone = (FCDEffectStandard*) _clone;

	if (_clone != NULL) FCDEffectProfile::Clone(_clone);
	if (clone != NULL)
	{
		clone->type = type;
		for (uint32 i = 0; i < FUDaeTextureChannel::COUNT; ++i)
		{
			size_t count = GetTextureCount(i);
			for (size_t t = 0; t < count; ++t)
			{
				const FCDTexture* texture = GetTexture(i, t);
				texture->Clone(clone->AddTexture(i));
			}
		}
		clone->transparencyMode = transparencyMode;

#define CLONE_ANIMATED(value) clone->value->SetValue(value->GetValue()); if (value->GetValue().IsAnimated()) value->GetValue().GetAnimated()->Clone(clone->value->GetValue().GetAnimated());

		CLONE_ANIMATED(emissionColor); CLONE_ANIMATED(emissionFactor); clone->isEmissionFactor = isEmissionFactor;
		CLONE_ANIMATED(translucencyColor); CLONE_ANIMATED(translucencyFactor);
		CLONE_ANIMATED(diffuseColor); CLONE_ANIMATED(ambientColor);
		CLONE_ANIMATED(specularColor); CLONE_ANIMATED(specularFactor); CLONE_ANIMATED(shininess);
		CLONE_ANIMATED(reflectivityColor); CLONE_ANIMATED(reflectivityFactor); CLONE_ANIMATED(indexOfRefraction);

#undef CLONE_ANIMATED
	}

	return _clone;
}

void FCDEffectStandard::AddExtraAttribute(const char* profile, const char* key, const fchar* value)
{
	FUAssert(GetParent() != NULL, return);
	FCDETechnique* extraTech = GetParent()->GetExtra()->GetDefaultType()->FindTechnique(profile);
	if (extraTech == NULL) extraTech = GetParent()->GetExtra()->GetDefaultType()->AddTechnique(profile);
	FCDENode *enode= extraTech->AddParameter(key, value);
	enode->SetName(key);
	enode->SetContent(value);
	SetNewChildFlag();
}

const fchar* FCDEffectStandard::GetExtraAttribute(const char* profile, const char* key) const
{
	FUAssert(GetParent() != NULL, return NULL);
	const FCDETechnique * extraTech = GetParent()->GetExtra()->GetDefaultType()->FindTechnique(profile);
	if (extraTech == NULL) return NULL;
	const FCDENode * enode = extraTech->FindParameter(key);
	if (enode == NULL) return NULL;
	return enode->GetContent();
}

FCDEffectParameter* FCDEffectStandard::GetParam(const fm::string& semantic, bool* isFloat)
{
	if (semantic == FCDEffectStandard::AmbientColorSemantic)
	{
		*isFloat = false;
		return ambientColor;
	}
	else if (semantic == FCDEffectStandard::DiffuseColorSemantic)
	{
		*isFloat = false;
		return diffuseColor;
	}
	else if (semantic == FCDEffectStandard::EmissionColorSemantic)
	{
		*isFloat = false;
		return emissionColor;
	}
	else if (semantic == FCDEffectStandard::EmissionFactorSemantic)
	{
		*isFloat = true;
		return emissionFactor;
	}
	else if (semantic == FCDEffectStandard::IndexOfRefractionSemantic)
	{
		*isFloat = true;
		return indexOfRefraction;
	}
	else if (semantic == FCDEffectStandard::ReflectivityColorSemantic)
	{
		*isFloat = false;
		return reflectivityColor;
	}
	else if (semantic == FCDEffectStandard::ReflectivityFactorSemantic)
	{
		*isFloat = true;
		return reflectivityFactor;
	}
	else if (semantic == FCDEffectStandard::ShininessSemantic)
	{
		*isFloat = true;
		return shininess;
	}
	else if (semantic == FCDEffectStandard::SpecularColorSemantic)
	{
		*isFloat = false;
		return specularColor;
	}
	else if (semantic == FCDEffectStandard::SpecularFactorSemantic)
	{
		*isFloat = true;
		return specularFactor;
	}
	else if (semantic == FCDEffectStandard::TranslucencyColorSemantic)
	{
		*isFloat = false;
		return translucencyColor;
	}
	else if (semantic == FCDEffectStandard::TranslucencyFactorSemantic)
	{
		*isFloat = true;
		return translucencyFactor;
	}
	else 
	{
		*isFloat = true;
		return NULL;
	}
}

