/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEffectCode.h"
#include "FCDocument/FCDEffectPass.h"
#include "FCDocument/FCDEffectProfileFX.h"
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterFactory.h"
#include "FCDocument/FCDEffectTechnique.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDImage.h"

//
// FCDEffectTechnique
//

ImplementObjectType(FCDEffectTechnique);
ImplementParameterObject(FCDEffectTechnique, FCDEffectCode, codes, new FCDEffectCode(parent->GetDocument()));
ImplementParameterObject(FCDEffectTechnique, FCDEffectPass, passes, new FCDEffectPass(parent->GetDocument(), parent));
ImplementParameterObjectNoCtr(FCDEffectTechnique, FCDEffectParameter, parameters);

FCDEffectTechnique::FCDEffectTechnique(FCDocument* document, FCDEffectProfileFX *_parent)
:	FCDObject(document), parent(_parent)
,	InitializeParameterNoArg(name)
,	InitializeParameterNoArg(codes)
,	InitializeParameterNoArg(passes)
,	InitializeParameterNoArg(parameters)
{
}

FCDEffectTechnique::~FCDEffectTechnique()
{
	parent = NULL;
}

// Adds a new pass to this effect technique.
FCDEffectPass* FCDEffectTechnique::AddPass()
{
	FCDEffectPass* pass = new FCDEffectPass(GetDocument(), this);
	passes.push_back(pass);
	SetNewChildFlag();
	return passes.back();
}

// Adds a new code inclusion to this effect profile.
FCDEffectCode* FCDEffectTechnique::AddCode()
{
	FCDEffectCode* code = new FCDEffectCode(GetDocument());
	codes.push_back(code);
	SetNewChildFlag();
	return code;
}

FCDEffectTechnique* FCDEffectTechnique::Clone(FCDEffectTechnique* clone) const
{
	if (clone == NULL) clone = new FCDEffectTechnique(const_cast<FCDocument*>(GetDocument()), NULL);

	clone->name = name;
	size_t parameterCount = parameters.size();
	for (size_t p = 0; p < parameterCount; ++p)
	{
		FCDEffectParameter* parameter = clone->AddEffectParameter(parameters[p]->GetType());
		parameters[p]->Clone(parameter);
	}

	// Clone the codes: need to happen before the passes are cloned
	clone->codes.reserve(codes.size());
	for (const FCDEffectCode** itC = codes.begin(); itC != codes.end(); ++itC)
	{
		(*itC)->Clone(clone->AddCode());
	}

	// Clone the passes
	clone->passes.reserve(passes.size());
	for (const FCDEffectPass** itP = passes.begin(); itP != passes.end(); ++itP)
	{
		(*itP)->Clone(clone->AddPass());
	}

	return clone;
}

FCDEffectParameter* FCDEffectTechnique::AddEffectParameter(uint32 type)
{
	FCDEffectParameter* parameter = FCDEffectParameterFactory::Create(GetDocument(), type);
	parameters.push_back(parameter);
	SetNewChildFlag();
	return parameter;
}

const FCDEffectCode* FCDEffectTechnique::FindCode(const char* sid) const
{
	for (const FCDEffectCode** itC = codes.begin(); itC != codes.end(); ++itC)
	{
		if ((*itC)->GetSubId() == sid) return (*itC);
	}
	return NULL;
}
