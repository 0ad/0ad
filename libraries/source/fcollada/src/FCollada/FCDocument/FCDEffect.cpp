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
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEffectStandard.h"
#include "FCDocument/FCDEffectProfileFX.h"
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterFactory.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDImage.h"

//
// FCDEffect
//

ImplementObjectType(FCDEffect);
ImplementParameterObjectNoCtr(FCDEffect, FCDEffectProfile, profiles);
ImplementParameterObjectNoCtr(FCDEffect, FCDEffectParameter, parameters);

FCDEffect::FCDEffect(FCDocument* document)
:	FCDEntity(document, "Effect")
,	InitializeParameterNoArg(profiles)
,	InitializeParameterNoArg(parameters)
{
}

FCDEffect::~FCDEffect()
{
}

FCDEffectParameter* FCDEffect::AddEffectParameter(uint32 type)
{
	FCDEffectParameter* parameter = FCDEffectParameterFactory::Create(GetDocument(), type);
	parameters.push_back(parameter);
	SetNewChildFlag();
	return parameter;
}

// Search for a profile of the given type
const FCDEffectProfile* FCDEffect::FindProfile(FUDaeProfileType::Type type) const
{
	for (const FCDEffectProfile** itR = profiles.begin(); itR != profiles.end(); ++itR)
	{
		if ((*itR)->GetType() == type) return (*itR);
	}
	return NULL;
}

// Search for a profile of a given type and platform
const FCDEffectProfile* FCDEffect::FindProfileByTypeAndPlatform(FUDaeProfileType::Type type, const fm::string& platform) const
{
	for (const FCDEffectProfile** itR = profiles.begin(); itR != profiles.end(); ++itR)
	{
		if ((*itR)->GetType() == type) 
		{
			if (((FCDEffectProfileFX*)(*itR))->GetPlatform() == TO_FSTRING(platform)) return (*itR);
		}
	}
	return NULL;
}

// Create a new effect profile.
FCDEffectProfile* FCDEffect::AddProfile(FUDaeProfileType::Type type)
{
	FCDEffectProfile* profile = NULL;

	// Create the correct profile for this type.
	if (type == FUDaeProfileType::COMMON) profile = new FCDEffectStandard(GetDocument(), this);
	else
	{
		profile = new FCDEffectProfileFX(GetDocument(), this);
		((FCDEffectProfileFX*) profile)->SetType(type);
	}

	profiles.push_back(profile);
	SetNewChildFlag(); 
	return profile;
}

// Returns a copy of the effect, with all the animations/textures attached
FCDEntity* FCDEffect::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDEffect* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDEffect(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDEffect::GetClassType())) clone = (FCDEffect*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		// Clone the effect profiles
		for (const FCDEffectProfile** itR = profiles.begin(); itR != profiles.end(); ++itR)
		{
			FCDEffectProfile* clonedProfile = clone->AddProfile((*itR)->GetType());
			(*itR)->Clone(clonedProfile);
		}

		// Clone the effect parameters
		size_t parameterCount = parameters.size();
		for (size_t p = 0; p < parameterCount; ++p)
		{
			FCDEffectParameter* parameter = clone->AddEffectParameter(parameters[p]->GetType());
			parameters[p]->Clone(parameter);
		}
	}
	return _clone;
}


