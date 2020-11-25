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
#include "FCDocument/FCDEffectTools.h"
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterFactory.h"
#include "FCDocument/FCDEntityReference.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDMaterial.h"

//
// FCDMaterial
//

ImplementObjectType(FCDMaterial);
ImplementParameterObjectNoCtr(FCDMaterial, FCDEntityReference, effect);
ImplementParameterObjectNoCtr(FCDMaterial, FCDEffectParameter, parameters);
	
FCDMaterial::FCDMaterial(FCDocument* document)
:	FCDEntity(document, "VisualMaterial")
,	ownsEffect(false)
,	InitializeParameterNoArg(effect)
,	InitializeParameterNoArg(parameters)
{
	effect = new FCDEntityReference(document, this);
}

FCDMaterial::~FCDMaterial()
{
	if (ownsEffect)
	{
		FCDEntity* _effect = effect->GetEntity();
		SAFE_RELEASE(_effect);
	}
	SAFE_RELEASE(effect);
	techniqueHints.clear();
}

FCDEffectParameter* FCDMaterial::AddEffectParameter(uint32 type)
{
	FCDEffectParameter* parameter = FCDEffectParameterFactory::Create(GetDocument(), type);
	parameters.push_back(parameter);
	SetNewChildFlag();
	return parameter;
}

const FCDEffect* FCDMaterial::GetEffect() const
{
	FUAssert(effect != NULL, return NULL);
	const FCDEntity* entity = effect->GetEntity();
	if (entity != NULL && entity->HasType(FCDEffect::GetClassType())) return (const FCDEffect*) entity;
	else return NULL;
}

void FCDMaterial::SetEffect(FCDEffect* _effect)
{
	effect->SetEntity(_effect);
	SetNewChildFlag();
}

// Cloning
FCDEntity* FCDMaterial::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDMaterial* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDMaterial(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDMaterial::GetClassType())) clone = (FCDMaterial*) _clone;

	Parent::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		// Clone the effect and the local list of parameters
		const FCDEffect* _effect = GetEffect();
		if (_effect != NULL)
		{
			if (cloneChildren)
			{
				clone->ownsEffect = true;
				FCDEffect* clonedEffect = clone->GetDocument()->GetEffectLibrary()->AddEntity();
				_effect->Clone(clonedEffect, cloneChildren);
			}
			else
			{
				clone->SetEffect(const_cast<FCDEffect*>(_effect));
			}
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
