/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterFactory.h"
#include "FCDocument/FCDEffectParameterSampler.h"
#include "FCDocument/FCDEffectParameterSurface.h"

// Creates a new effect parameter, given a type.
FCDEffectParameter* FCDEffectParameterFactory::Create(FCDocument* document, uint32 type)
{
	FCDEffectParameter* parameter = NULL;

	switch (type)
	{
	case FCDEffectParameter::SAMPLER: parameter = new FCDEffectParameterSampler(document); break;
	case FCDEffectParameter::INTEGER: parameter = new FCDEffectParameterInt(document); break;
	case FCDEffectParameter::BOOLEAN: parameter = new FCDEffectParameterBool(document); break;
	case FCDEffectParameter::FLOAT: parameter = new FCDEffectParameterFloat(document); break;
	case FCDEffectParameter::FLOAT2: parameter = new FCDEffectParameterFloat2(document); break;
	case FCDEffectParameter::FLOAT3: parameter = new FCDEffectParameterFloat3(document); break;
	case FCDEffectParameter::VECTOR: parameter = new FCDEffectParameterVector(document); break;
	case FCDEffectParameter::MATRIX: parameter = new FCDEffectParameterMatrix(document); break;
	case FCDEffectParameter::STRING: parameter = new FCDEffectParameterString(document); break;
	case FCDEffectParameter::SURFACE: parameter = new FCDEffectParameterSurface(document); break;
	default: break;
	}

	return parameter;
}

