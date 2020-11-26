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
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimationKey.h"
#include "FCDocument/FCDAnimationMultiCurve.h"
#include "FUtils/FUDaeEnum.h"

// Implemented in FCDAnimationCurve.cpp
extern float FindT(float cp0x, float cp1x, float cp2x, float cp3x, float input, float initialGuess);

// Declaring the type of evaluation for curves once.
bool FCDAnimationMultiCurve::is2DEvaluation = true;

//
// FCDAnimationMultiCurve
//

ImplementObjectType(FCDAnimationMultiCurve);

FCDAnimationMultiCurve::FCDAnimationMultiCurve(FCDocument* document, uint32 _dimension)
:	FCDObject(document), dimension(_dimension)
,	targetElement(-1)
,	preInfinity(FUDaeInfinity::CONSTANT), postInfinity(FUDaeInfinity::CONSTANT)
{
	if (dimension == 0) dimension = 1;
}

FCDAnimationMultiCurve::~FCDAnimationMultiCurve()
{
	CLEAR_POINTER_VECTOR(keys);
}

void FCDAnimationMultiCurve::SetKeyCount(size_t count, FUDaeInterpolation::Interpolation interpolation)
{
	size_t oldCount = GetKeyCount();
	if (oldCount < count)
	{
		keys.reserve(count);
		for (; oldCount < count; ++oldCount) AddKey(interpolation);
	}
	else if (count < oldCount)
	{
		for (FCDAnimationMKeyList::iterator it = keys.begin() + count; it != keys.end(); ++it) delete (*it);
		keys.resize(count);
	}
	SetDirtyFlag();
}

FCDAnimationMKey* FCDAnimationMultiCurve::AddKey(FUDaeInterpolation::Interpolation interpolation)
{
	FCDAnimationMKey* key;
	switch (interpolation)
	{
	case FUDaeInterpolation::STEP: key = new FCDAnimationMKey(dimension); break;
	case FUDaeInterpolation::LINEAR: key = new FCDAnimationMKey(dimension); break;
	case FUDaeInterpolation::BEZIER: key = new FCDAnimationMKeyBezier(dimension); break;
	case FUDaeInterpolation::TCB: key = new FCDAnimationMKeyTCB(dimension); break;
	default: FUFail(key = new FCDAnimationMKey(dimension); break;);
	}
	key->interpolation = (uint32) interpolation;
	keys.push_back(key);
	SetDirtyFlag();
	return key;
}

// Samples all the curves for a given input
void FCDAnimationMultiCurve::Evaluate(float input, float* output) const
{
	// Check for empty curves and poses (curves with 1 key).
	if (keys.size() == 0)
	{
		for (uint32 i = 0; i < dimension; ++i) output[i] = 0.0f;
	}
	else if (keys.size() == 1)
	{
		for (uint32 i = 0; i < dimension; ++i) output[i] = keys.front()->output[i];
	}
	else
	{
		// Find the current interval
		FCDAnimationMKeyList::const_iterator it, start = keys.begin(), terminate = keys.end();
		while (terminate - start > 3)
		{ 
			// Binary search.
			it = (const FCDAnimationMKey**) ((((size_t) terminate) / 2 + ((size_t) start) / 2) & ~((sizeof(size_t)-1)));
			if ((*it)->input > input) terminate = it;
			else start = it;
		}
		// Linear search is more efficient on the last interval
		for (it = start; it != terminate; ++it)
		{
			if ((*it)->input > input) break;
		}

		if (it == keys.end())
		{
			// We're sampling after the curve, return the last values
			const FCDAnimationMKey* lastKey = keys.back();
			for (uint32 i = 0; i < dimension; ++i) output[i] = lastKey->output[i];
		}
		else if (it == keys.begin())
		{
			// We're sampling before the curve, return the first values
			const FCDAnimationMKey* firstKey = keys.front();
			for (uint32 i = 0; i < dimension; ++i) output[i] = firstKey->output[i];
		}
		else
		{
			// Get the keys and values for this interval
			const FCDAnimationMKey* startKey = *(it - 1);
			const FCDAnimationMKey* endKey = *it;
			float inputInterval = endKey->input - startKey->input;

			// Interpolate the outputs.
			// Similar code is found in FCDAnimationCurve.cpp. If you update this, update the other one too.
			switch (startKey->interpolation)
			{
			case FUDaeInterpolation::LINEAR:
				for (uint32 i = 0; i < dimension; ++i)
				{
					output[i] = startKey->output[i] + (input - startKey->input) / inputInterval * (endKey->output[i] - startKey->output[i]); 
				}
				break;

			case FUDaeInterpolation::BEZIER: {
				FCDAnimationMKeyBezier* bkey1 = (FCDAnimationMKeyBezier*) startKey;
				for (uint32 i = 0; i < dimension; ++i)
				{
					FMVector2 inTangent;
					if (endKey->interpolation == FUDaeInterpolation::BEZIER) inTangent = ((FCDAnimationMKeyBezier*) endKey)->inTangent[i];
					else inTangent = FMVector2(endKey->input, 0.0f);

					float t = (input - startKey->input) / inputInterval;
					if (is2DEvaluation) t = FindT(startKey->input, bkey1->outTangent[i].x, inTangent.x, endKey->input, input, t);
					float b = bkey1->outTangent[i].v;
					float c = inTangent.v;
					float ti = 1.0f - t;
					float br = inputInterval / (bkey1->outTangent[i].u - startKey->input);
					float cr = inputInterval / (endKey->input - inTangent.u);
					br = FMath::Clamp(br, 0.01f, 100.0f);
					cr = FMath::Clamp(cr, 0.01f, 100.0f);

					output[i] = startKey->output[i] * ti * ti * ti + br* b * ti * ti * t + cr * c * ti * t * t + endKey->output[i] * t * t * t;
				}
				break; }

			case FUDaeInterpolation::TCB: // Not implemented..
			case FUDaeInterpolation::UNKNOWN:
			case FUDaeInterpolation::STEP:
			default:
				for (uint32 i = 0; i < dimension; ++i) output[i] = startKey->output[i];
				break;
			}
		}
	}
}

