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
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterFactory.h"
#include "FCDocument/FCDEffectParameterSurface.h"
#include "FCDocument/FCDEffectParameterSampler.h"
#include "FCDocument/FCDEffectStandard.h"
#include "FCDocument/FCDEffectTools.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDImage.h"
#include "FCDocument/FCDTexture.h"
#include "FUtils/FUStringConversion.h"

//
// FCDTexture
//

ImplementObjectType(FCDTexture);
ImplementParameterObjectNoCtr(FCDTexture, FCDEffectParameterSampler, sampler);
ImplementParameterObject(FCDTexture, FCDEffectParameterInt, set, new FCDEffectParameterInt(parent->GetDocument()));
ImplementParameterObject(FCDTexture, FCDExtra, extra, new FCDExtra(parent->GetDocument(), parent));

FCDTexture::FCDTexture(FCDocument* document, FCDEffectStandard* _parent)
:	FCDObject(document), parent(_parent)
,	InitializeParameterNoArg(sampler)
,	InitializeParameterNoArg(set)
,	InitializeParameterNoArg(extra)
{
	set = new FCDEffectParameterInt(document);
	set->SetValue(-1);
	extra = new FCDExtra(document, this);
}

FCDTexture::~FCDTexture()
{
	parent = NULL;
}

// Retrieves the sampler parameter: creates one if none are attached.
FCDEffectParameterSampler* FCDTexture::GetSampler()
{
	if (parent == NULL && sampler == NULL) return NULL;
	if (sampler == NULL)
	{
		sampler = (FCDEffectParameterSampler*) parent->AddEffectParameter(FCDEffectParameter::SAMPLER);
	}
	return sampler;
}

// Retrieves the image information for this texture.
const FCDImage* FCDTexture::GetImage() const
{
	if (sampler == NULL) return NULL;
	const FCDEffectParameterSurface* surface = sampler->GetSurface();
	if (surface == NULL) return NULL;
	return surface->GetImage();
}

// Set the image information for this texture.
void FCDTexture::SetImage(FCDImage* image)
{
	// TODO: No parameter re-use for now.
	SAFE_RELEASE(sampler);
	if (image != NULL && parent != NULL)
	{
		// Look for a surface with the expected sid.
		fm::string surfaceSid = image->GetDaeId() + "-surface";
		FCDEffectParameter* _surface = FCDEffectTools::FindEffectParameterByReference(parent, surfaceSid);
		FCDEffectParameterSurface* surface = NULL;
		if (_surface == NULL)
		{
			// Create the surface parameter
			surface = (FCDEffectParameterSurface*) parent->AddEffectParameter(FCDEffectParameter::SURFACE);
			surface->SetInitMethod(new FCDEffectParameterSurfaceInitFrom());
			surface->AddImage(image);
			surface->SetGenerator();
			surface->SetReference(surfaceSid);
		}
		else if (_surface->HasType(FCDEffectParameterSurface::GetClassType()))
		{
			surface = (FCDEffectParameterSurface*) _surface;
		}
		else return;

		// Look for a sampler with the expected sid.
		fm::string samplerSid = image->GetDaeId() + "-sampler";
        const FCDEffectParameter* _sampler = FCDEffectTools::FindEffectParameterByReference(parent, samplerSid);
		if (_sampler == NULL)
		{
			sampler = (FCDEffectParameterSampler*) parent->AddEffectParameter(FCDEffectParameter::SAMPLER);
			sampler->SetSurface(surface);
			sampler->SetGenerator();
			sampler->SetReference(samplerSid);
		}
		else if (_sampler->GetObjectType().Includes(FCDEffectParameterSampler::GetClassType()))
		{
			sampler = (FCDEffectParameterSampler*) const_cast<FCDEffectParameter*>(_sampler);
		}
	}

	SetNewChildFlag(); 
}

// Returns a copy of the texture/sampler, with all the animations attached
FCDTexture* FCDTexture::Clone(FCDTexture* clone) const
{
	if (clone == NULL) clone = new FCDTexture(const_cast<FCDTexture*>(this)->GetDocument(), parent);

	set->Clone(clone->set);
	extra->Clone(clone->extra);

	if (sampler != NULL)
	{
		sampler->Clone(clone->GetSampler());
	}

	return clone;
}
