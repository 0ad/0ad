/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEffectCode.h"
#include "FCDocument/FCDEffectPass.h"
#include "FCDocument/FCDEffectPassShader.h"
#include "FCDocument/FCDEffectProfileFX.h"
#include "FCDocument/FCDEffectTechnique.h"

//
// FCDEffectPassBind
//

ImplementObjectType(FCDEffectPassBind);

FCDEffectPassBind::FCDEffectPassBind(FCDocument* document)
:	FCDObject(document)
,	InitializeParameterNoArg(reference)
,	InitializeParameterNoArg(symbol)
{
}

//
// FCDEffectPassShader
//

ImplementObjectType(FCDEffectPassShader);
ImplementParameterObject(FCDEffectPassShader, FCDEffectPassBind, bindings, new FCDEffectPassBind(parent->GetDocument()));
ImplementParameterObjectNoCtr(FCDEffectPassShader, FCDEffectCode, code);

FCDEffectPassShader::FCDEffectPassShader(FCDocument* document, FCDEffectPass* _parent)
:	FCDObject(document), parent(_parent)
,	InitializeParameterNoArg(name)
,	InitializeParameterNoArg(compilerTarget)
,	InitializeParameterNoArg(compilerOptions)
,	InitializeParameterNoArg(bindings)
,	InitializeParameterNoArg(code)
,	InitializeParameter(isFragment, false)
{
}

FCDEffectPassShader::~FCDEffectPassShader()
{
	parent = NULL;
	code = NULL;
}

// Retrieve the shader binding, given a COLLADA parameter reference.
const FCDEffectPassBind* FCDEffectPassShader::FindBindingReference(const char* reference) const
{
	for (const FCDEffectPassBind** it = bindings.begin(); it != bindings.end(); ++it)
	{
		if (IsEquivalent((*it)->reference, reference)) return (*it);
	}
	return NULL;
}

const FCDEffectPassBind* FCDEffectPassShader::FindBindingSymbol(const char* symbol) const
{
	for (const FCDEffectPassBind** it = bindings.begin(); it != bindings.end(); ++it)
	{
		if (IsEquivalent((*it)->symbol, symbol)) return (*it);
	}
	return NULL;
}

// Adds a new binding to this shader.
FCDEffectPassBind* FCDEffectPassShader::AddBinding()
{
	bindings.push_back(new FCDEffectPassBind(GetDocument()));
	SetNewChildFlag();
	return bindings.back();
}

// Cloning
FCDEffectPassShader* FCDEffectPassShader::Clone(FCDEffectPassShader* clone) const
{
	if (clone == NULL) clone = new FCDEffectPassShader(const_cast<FCDocument*>(GetDocument()), parent);

	clone->isFragment = isFragment;
	size_t bindingCount = bindings.size();
	for (size_t b = 0; b < bindingCount; ++b)
	{
		FCDEffectPassBind* binding = clone->AddBinding();
		binding->reference = bindings[b]->reference;
		binding->symbol = bindings[b]->symbol;
	}
	clone->compilerTarget = compilerTarget;
	clone->compilerOptions = compilerOptions;
	clone->name = name;

	// Look for the new code within the parent.
	if (code != NULL)
	{
		clone->code = clone->parent->GetParent()->FindCode(code->GetSubId());
		if (clone->code == NULL) clone->code = clone->parent->GetParent()->GetParent()->FindCode(code->GetSubId());
	}
	return clone;
}
