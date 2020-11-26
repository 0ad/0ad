/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEffectPass.h"
#include "FCDocument/FCDEffectProfile.h"
#include "FCDocument/FCDEffectTechnique.h"
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterSurface.h"
#include "FCDocument/FCDEffectParameterSampler.h"
#include "FCDocument/FCDImage.h"

//
// FCDEffectParameterSampler
//

ImplementObjectType(FCDEffectParameterSampler);
ImplementParameterObjectNoCtr(FCDEffectParameterSampler, FCDEffectParameterSurface, surface);

FCDEffectParameterSampler::FCDEffectParameterSampler(FCDocument* document)
:	FCDEffectParameter(document)
,	InitializeParameter(samplerType, SAMPLER2D)
,	InitializeParameterNoArg(surface)
,	InitializeParameter(wrap_s, FUDaeTextureWrapMode::DEFAULT)
,	InitializeParameter(wrap_t, FUDaeTextureWrapMode::DEFAULT)
,	InitializeParameter(wrap_p, FUDaeTextureWrapMode::DEFAULT)
,	InitializeParameter(minFilter, FUDaeTextureFilterFunction::DEFAULT)
,	InitializeParameter(magFilter, FUDaeTextureFilterFunction::DEFAULT)
,	InitializeParameter(mipFilter, FUDaeTextureFilterFunction::DEFAULT)
{
}

FCDEffectParameterSampler::~FCDEffectParameterSampler()
{
}

// Sets the surface parameter for the surface to sample.
void FCDEffectParameterSampler::SetSurface(FCDEffectParameterSurface* _surface)
{
	surface = _surface;
	SetNewChildFlag();
}

// compare value
bool FCDEffectParameterSampler::IsValueEqual(FCDEffectParameter* parameter)
{
	if (!FCDEffectParameter::IsValueEqual(parameter)) return false;
	if (parameter->GetObjectType() != FCDEffectParameterSampler::GetClassType()) return false;
	FCDEffectParameterSampler *param = (FCDEffectParameterSampler*)parameter;
	
	if (GetSamplerType() != param->GetSamplerType()) return false;
	if (param->GetSurface() == NULL && surface == NULL) {}
	else if (param->GetSurface() == NULL || surface == NULL) return false;
	else if (!IsEquivalent(param->GetSurface()->GetReference(), surface->GetReference())) return false;

	return true;
}

// Clone
FCDEffectParameter* FCDEffectParameterSampler::Clone(FCDEffectParameter* _clone) const
{
	FCDEffectParameterSampler* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDEffectParameterSampler(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->GetObjectType() == FCDEffectParameterSampler::GetClassType()) clone = (FCDEffectParameterSampler*) _clone;

	if (_clone != NULL) FCDEffectParameter::Clone(_clone);
	if (clone != NULL)
	{
		clone->surface = const_cast<FCDEffectParameterSurface*>((const FCDEffectParameterSurface*)(surface));
		clone->samplerType = samplerType;
	}
	return _clone;
}

// Flattening: overwrite the target parameter with this parameter
void FCDEffectParameterSampler::Overwrite(FCDEffectParameter* target)
{
	if (target->GetType() == SAMPLER)
	{
		FCDEffectParameterSampler* s = (FCDEffectParameterSampler*) target;
		if (samplerType == s->samplerType)
		{
			s->surface = surface;
			SetNewChildFlag();
		}
	}
}
