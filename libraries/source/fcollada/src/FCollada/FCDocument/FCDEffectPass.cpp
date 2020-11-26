/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDEffectTechnique.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEffectPass.h"
#include "FCDocument/FCDEffectPassShader.h"
#include "FCDocument/FCDEffectPassState.h"
#include "FCDocument/FCDEffectParameter.h"

//
// FCDEffectPass
//

ImplementObjectType(FCDEffectPass);
ImplementParameterObject(FCDEffectPass, FCDEffectPassShader, shaders, new FCDEffectPassShader(parent->GetDocument(), parent));
ImplementParameterObjectNoCtr(FCDEffectPass, FCDEffectPassState, states);

FCDEffectPass::FCDEffectPass(FCDocument* document, FCDEffectTechnique *_parent)
:	FCDObject(document), parent(_parent)
,	InitializeParameterNoArg(name)
,	InitializeParameterNoArg(shaders)
,	InitializeParameterNoArg(states)
{
}

FCDEffectPass::~FCDEffectPass()
{
	parent = NULL;
}

// Adds a new shader to the pass.
FCDEffectPassShader* FCDEffectPass::AddShader()
{
	FCDEffectPassShader* shader = new FCDEffectPassShader(GetDocument(), this);
	shaders.push_back(shader);
	SetNewChildFlag();
	return shader;
}

// Adds a new vertex shader to the pass.
// If a vertex shader already exists within the pass, it will be released.
FCDEffectPassShader* FCDEffectPass::AddVertexShader()
{
	FCDEffectPassShader* vertexShader;
	for (vertexShader = GetVertexShader(); vertexShader != NULL; vertexShader = GetVertexShader())
	{
		SAFE_RELEASE(vertexShader);
	}

	vertexShader = AddShader();
	vertexShader->AffectsVertices();
	SetNewChildFlag();
	return vertexShader;
}

// Adds a new fragment shader to the pass.
// If a fragment shader already exists within the pass, it will be released.
FCDEffectPassShader* FCDEffectPass::AddFragmentShader()
{
	FCDEffectPassShader* fragmentShader;
	for (fragmentShader = GetFragmentShader(); fragmentShader != NULL; fragmentShader = GetFragmentShader())
	{
		SAFE_RELEASE(fragmentShader);
	}

	fragmentShader = AddShader();
	fragmentShader->AffectsFragments();
	SetNewChildFlag();
	return fragmentShader;
}

FCDEffectPass* FCDEffectPass::Clone(FCDEffectPass* clone) const
{
	if (clone == NULL) clone = new FCDEffectPass(const_cast<FCDocument*>(GetDocument()), parent);

	clone->name = name;

	// Clone the shaders
	clone->shaders.reserve(shaders.size());
	for (const FCDEffectPassShader** itS = shaders.begin(); itS != shaders.end(); ++itS)
	{
		FCDEffectPassShader* clonedShader = clone->AddShader();
		(*itS)->Clone(clonedShader);
	}

	// Clone the states
	clone->states.reserve(states.size());
	for (const FCDEffectPassState** itS = states.begin(); itS != states.end(); ++itS)
	{
		FCDEffectPassState* clonedState = clone->AddRenderState((*itS)->GetType());
		(*itS)->Clone(clonedState);
	}

	return clone;
}

// Retrieve the type-specific shaders
const FCDEffectPassShader* FCDEffectPass::GetVertexShader() const
{
	for (const FCDEffectPassShader** itS = shaders.begin(); itS != shaders.end(); ++itS)
	{
		if ((*itS)->IsVertexShader()) return (*itS);
	}
	return NULL;
}

const FCDEffectPassShader* FCDEffectPass::GetFragmentShader() const
{
	for (const FCDEffectPassShader** itS = shaders.begin(); itS != shaders.end(); ++itS)
	{
		if ((*itS)->IsFragmentShader()) return (*itS);
	}
	return NULL;
}

FCDEffectPassState* FCDEffectPass::AddRenderState(FUDaePassState::State type)
{
	FCDEffectPassState* state = new FCDEffectPassState(GetDocument(), type);

	// Ordered-insertion into the states container.
	size_t stateCount = states.size();
	size_t insertIndex;
	for (insertIndex = 0; insertIndex < stateCount; ++insertIndex)
	{
		if (type < states[insertIndex]->GetType()) break;
	}
	states.insert(insertIndex, state);

	SetNewChildFlag();
	return state;
}

const FCDEffectPassState* FCDEffectPass::FindRenderState(FUDaePassState::State type) const
{
	for (const FCDEffectPassState** itS = states.begin(); itS != states.end(); ++itS)
	{
		if ((*itS)->GetType() == type) return (*itS);
	}
	return NULL;
}
