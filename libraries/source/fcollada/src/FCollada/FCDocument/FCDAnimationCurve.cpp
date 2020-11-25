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
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimationClip.h"
#include "FCDocument/FCDAnimationKey.h"
#include "FUtils/FUDaeEnum.h"

//
// Local Definitions
//

// Declaring the type of evaluation for curves once.
bool FCDAnimationCurve::is2DEvaluation = true;

// Uses iterative method to accurately pin-point the 't' of the Bezier 
// equation that corresponds to the current time.
float FindT(float cp0x, float cp1x, float cp2x, float cp3x, float input, float initialGuess)
{
	float localTolerance = 0.001f;
	float highT = 1.0f;
	float lowT = 0.0f;

	//Optimize here, start with a more intuitive value than 0.5
	float midT = 0.5f;
	if (initialGuess <= 0.1) midT = 0.1f; //clamp to 10% or 90%, because if miss, the cost is too high.
	else if (initialGuess >= 0.9) midT = 0.9f;
	else midT = initialGuess;
	bool once = true;
	while ((highT-lowT) > localTolerance) {
		if (once) once = false;
		else midT = (highT - lowT) / 2.0f + lowT;
		float ti = 1.0f - midT; // (1 - t)
		float calculatedTime = cp0x*ti*ti*ti + 3*cp1x*midT*ti*ti + 3*cp2x*midT*midT*ti + cp3x*midT*midT*midT; 
		if (fabsf(calculatedTime - input) <= localTolerance) break; //If we 'fall' very close, we like it and break.
		if (calculatedTime > input) highT = midT;
		else lowT = midT;
	}
	return midT;
}

static void ComputeTCBTangent(const FCDAnimationKey* previousKey, const FCDAnimationKey* currentKey, const FCDAnimationKey* nextKey, float tens, float cont, float bias, FMVector2& leftTangent, FMVector2& rightTangent)
{
	FUAssert(currentKey != NULL, return;);

	// Calculate the intervals and allow for time differences of both sides.
	FMVector2 pCurrentMinusPrevious;
	FMVector2 pNextMinusCurrent;

	//If the previous key or the last key is NULL, do make one up...
	if (!previousKey) {
		if (nextKey) pCurrentMinusPrevious.x = nextKey->input - currentKey->input;
		else pCurrentMinusPrevious.x = 0.5f; //Case where there is only one TCB key.. should not happen.
		pCurrentMinusPrevious.y = 0.0f;
	}
	else {
		pCurrentMinusPrevious.x = previousKey->input - currentKey->input;
		pCurrentMinusPrevious.y = previousKey->output - currentKey->output;
	}
	if (!nextKey) {
		if (previousKey) pNextMinusCurrent.x = currentKey->input - previousKey->input;
		else pNextMinusCurrent.x = 0.5f; //Case where there is only one TCB key.. ?
		pNextMinusCurrent.y = 0.0f;
	}
	else {
		pNextMinusCurrent.x = nextKey->input - currentKey->input;
		pNextMinusCurrent.y = nextKey->output - currentKey->output;
	}

	//Calculate the constants applied that contain the continuity, tension, and bias.
	float k1 = ((1.0f - tens) * (1.0f - cont) * (1.0f + bias))/2;
	float k2 = ((1.0f - tens) * (1.0f + cont) * (1.0f - bias))/2;
	float k3 = ((1.0f - tens) * (1.0f + cont) * (1.0f + bias))/2;
	float k4 = ((1.0f - tens) * (1.0f - cont) * (1.0f - bias))/2;

	leftTangent.x = k1 * pCurrentMinusPrevious.x + k2 * pNextMinusCurrent.x;
	leftTangent.y = k1 * pCurrentMinusPrevious.y + k2 * pNextMinusCurrent.y;

	rightTangent.x = k3 * pCurrentMinusPrevious.x + k4 * pNextMinusCurrent.x;
	rightTangent.y = k3 * pCurrentMinusPrevious.y + k4 * pNextMinusCurrent.y;
}

//
// FCDAnimationCurve
//

ImplementObjectType(FCDAnimationCurve);

FCDAnimationCurve::FCDAnimationCurve(FCDocument* document, FCDAnimationChannel* _parent)
 :	FCDObject(document), parent(_parent),
	targetElement(-1),
	preInfinity(FUDaeInfinity::CONSTANT), postInfinity(FUDaeInfinity::CONSTANT),
	inputDriver(NULL), inputDriverIndex(0)
{
	currentClip = NULL;
	currentOffset = 0;
}

FCDAnimationCurve::~FCDAnimationCurve()
{
	CLEAR_POINTER_VECTOR(keys);

	inputDriver = NULL;
	parent = NULL;
	clips.clear();
	clipOffsets.clear();
}

void FCDAnimationCurve::SetKeyCount(size_t count, FUDaeInterpolation::Interpolation interpolation)
{
	size_t oldCount = GetKeyCount();
	if (oldCount < count)
	{
		keys.reserve(count);
		for (; oldCount < count; ++oldCount) AddKey(interpolation);
	}
	else if (count < oldCount)
	{
		for (FCDAnimationKeyList::iterator it = keys.begin() + count; it != keys.end(); ++it) delete (*it);
		keys.resize(count);
	}
	SetDirtyFlag();
}

FCDAnimationKey* FCDAnimationCurve::AddKey(FUDaeInterpolation::Interpolation interpolation)
{
	FCDAnimationKey* key;
	switch (interpolation)
	{
	case FUDaeInterpolation::STEP: key = new FCDAnimationKey; break;
	case FUDaeInterpolation::LINEAR: key = new FCDAnimationKey; break;
	case FUDaeInterpolation::BEZIER: key = new FCDAnimationKeyBezier; break;
	case FUDaeInterpolation::TCB: key = new FCDAnimationKeyTCB; break;
	default: FUFail(key = new FCDAnimationKey; break;);
	}
	key->interpolation = (uint32) interpolation;
	keys.push_back(key);
	SetDirtyFlag();
	return key;
}


// Insert a new key into the ordered array at a certain time
FCDAnimationKey* FCDAnimationCurve::AddKey(FUDaeInterpolation::Interpolation interpolation, float input, size_t& index)
{
	FCDAnimationKey* key;
	switch (interpolation)
	{
	case FUDaeInterpolation::STEP: key = new FCDAnimationKey; break;
	case FUDaeInterpolation::LINEAR: key = new FCDAnimationKey; break;
	case FUDaeInterpolation::BEZIER: key = new FCDAnimationKeyBezier; break;
	case FUDaeInterpolation::TCB: key = new FCDAnimationKeyTCB; break;
	default: FUFail(return NULL);
	}
	key->interpolation = (uint32) interpolation;
	key->input = input;
	FCDAnimationKeyList::iterator insertIdx = keys.begin();
	FCDAnimationKeyList::iterator finalIdx = keys.end();

	// TODO: Not cabbage search :-)
	for (index = 0; insertIdx != finalIdx; insertIdx++, index++)
	{
		if ((*insertIdx)->input > input) break;
	}

	keys.insert(insertIdx, key);
	SetDirtyFlag();
	return key;
}

bool FCDAnimationCurve::DeleteKey(FCDAnimationKey* key)
{
	FCDAnimationKeyList::iterator kitr = keys.find(key);
	if (kitr == keys.end()) return false;

	keys.erase(kitr);
	delete key;
	return true;
}

void FCDAnimationCurve::AddClip(FCDAnimationClip* clip)
{
    clips.push_back(clip);
}

bool FCDAnimationCurve::HasDriver() const
{
	return inputDriver != NULL;
}

void FCDAnimationCurve::GetDriver(FCDAnimated*& driver, int32& index)
{ const_cast<const FCDAnimationCurve*>(this)->GetDriver(const_cast<const FCDAnimated*&>(driver), index); }
void FCDAnimationCurve::GetDriver(const FCDAnimated*& driver, int32& index) const
{
	driver = inputDriver;
	index = inputDriverIndex;
}

void FCDAnimationCurve::SetDriver(FCDAnimated* driver, int32 index)
{
	inputDriver = driver;
	inputDriverIndex = index;
	SetNewChildFlag();
}

FCDAnimationCurve* FCDAnimationCurve::Clone(FCDAnimationCurve* clone, bool includeClips) const
{
	if (clone == NULL) clone = new FCDAnimationCurve(const_cast<FCDocument*>(GetDocument()), parent);

	clone->SetTargetElement(targetElement);
	clone->SetTargetQualifier(targetQualifier);

	// Pre-buffer the list of keys and clone them.
	clone->keys.clear();
	clone->keys.reserve(keys.size());
	for (FCDAnimationKeyList::const_iterator it = keys.begin(); it != keys.end(); ++it)
	{
		FCDAnimationKey* key = clone->AddKey((FUDaeInterpolation::Interpolation) (*it)->interpolation);
		key->input = (*it)->input;
		key->output = (*it)->output;
		if ((*it)->interpolation == FUDaeInterpolation::BEZIER)
		{
			FCDAnimationKeyBezier* bkey1 = (FCDAnimationKeyBezier*) (*it);
			FCDAnimationKeyBezier* bkey2 = (FCDAnimationKeyBezier*) key;
			bkey2->inTangent = bkey1->inTangent;
			bkey2->outTangent = bkey1->outTangent;
		}
		else if ((*it)->interpolation == FUDaeInterpolation::TCB)
		{
			FCDAnimationKeyTCB* tkey1 = (FCDAnimationKeyTCB*) (*it);
			FCDAnimationKeyTCB* tkey2 = (FCDAnimationKeyTCB*) key;
			tkey2->tension = tkey1->tension;
			tkey2->continuity = tkey1->continuity;
			tkey2->bias = tkey1->bias;
			tkey2->easeIn = tkey1->easeIn;
			tkey2->easeOut = tkey1->easeOut;
		}
	}

	clone->preInfinity = preInfinity;
	clone->postInfinity = postInfinity;
	clone->inputDriver = inputDriver;
	clone->inputDriverIndex = inputDriverIndex;

	if (includeClips)
	{
		// Animation clips that depend on this curve
		for (FCDAnimationClipList::const_iterator it = clips.begin(); it != clips.end(); ++it)
		{
			FCDAnimationClip* clonedClip = (FCDAnimationClip*) (*it)->Clone(0, false);
			clonedClip->AddClipCurve(clone);
			clone->AddClip(clonedClip);
		}

		for (FloatList::const_iterator it = clipOffsets.begin(); it != clipOffsets.end(); ++it)
		{
			clone->clipOffsets.push_back(*it);
		}
	}

	return clone;
}

void FCDAnimationCurve::SetCurrentAnimationClip(FCDAnimationClip* clip)
{
	if (currentClip == clip) return;

	currentClip = NULL;
	float clipOffset = 0.0f;
	for (size_t i = 0; i < clips.size(); ++i)
	{
		if (clips[i] == clip)
		{
			currentClip = clips[i];
			clipOffset = clipOffsets[i];
			break;
		}
	}
	
	if (currentClip != NULL)	
	{
		float offset = clipOffset - currentOffset;
		currentOffset = clipOffset;
		for (FCDAnimationKeyList::iterator it = keys.begin(); it != keys.end(); ++it)
		{
			(*it)->input += offset;
		}
	}

	SetDirtyFlag();
}

// Main workhorse for the animation system:
// Evaluates the curve for a given input
float FCDAnimationCurve::Evaluate(float input) const
{
	// Check for empty curves and poses (curves with 1 key).
	if (keys.size() == 0) return 0.0f;
	if (keys.size() == 1) return keys.front()->output;

	float inputStart = keys.front()->input;
	float inputEnd = keys.back()->input;
	float inputSpan = inputEnd - inputStart;
	float outputStart = keys.front()->output;
	float outputEnd = keys.back()->output;
	float outputSpan = outputEnd - outputStart;

	// Account for pre-infinity mode
	float outputOffset = 0.0f;
	if (input < inputStart)
	{
		float inputDifference = inputStart - input;
		switch (preInfinity)
		{
		case FUDaeInfinity::CONSTANT: return outputStart;
		case FUDaeInfinity::LINEAR: return outputStart + inputDifference * (keys[1]->output - outputStart) / (keys[1]->input - inputStart);
		case FUDaeInfinity::CYCLE: { float cycleCount = ceilf(inputDifference / inputSpan); input += cycleCount * inputSpan; break; }
		case FUDaeInfinity::CYCLE_RELATIVE: { float cycleCount = ceilf(inputDifference / inputSpan); input += cycleCount * inputSpan; outputOffset -= cycleCount * outputSpan; break; }
		case FUDaeInfinity::OSCILLATE: { float cycleCount = ceilf(inputDifference / (2.0f * inputSpan)); input += cycleCount * 2.0f * inputSpan; input = inputEnd - fabsf(input - inputEnd); break; }
		case FUDaeInfinity::UNKNOWN: default: return outputStart;
		}
	}

	// Account for post-infinity mode
	else if (input >= inputEnd)
	{
		float inputDifference = input - inputEnd;
		switch (postInfinity)
		{
		case FUDaeInfinity::CONSTANT: return outputEnd;
		case FUDaeInfinity::LINEAR: return outputEnd + inputDifference * (keys[keys.size() - 2]->output - outputEnd) / (keys[keys.size() - 2]->input - inputEnd);
		case FUDaeInfinity::CYCLE: { float cycleCount = ceilf(inputDifference / inputSpan); input -= cycleCount * inputSpan; break; }
		case FUDaeInfinity::CYCLE_RELATIVE: { float cycleCount = ceilf(inputDifference / inputSpan); input -= cycleCount * inputSpan; outputOffset += cycleCount * outputSpan; break; }
		case FUDaeInfinity::OSCILLATE: { float cycleCount = ceilf(inputDifference / (2.0f * inputSpan)); input -= cycleCount * 2.0f * inputSpan; input = inputStart + fabsf(input - inputStart); break; }
		case FUDaeInfinity::UNKNOWN: default: return outputEnd;
		}
	}

	// Find the current interval
	FCDAnimationKeyList::const_iterator it, start = keys.begin(), terminate = keys.end();
	while (terminate - start > 3)
	{ 
		// Binary search.
		it = (const FCDAnimationKey**) ((((size_t) terminate) / 2 + ((size_t) start) / 2) & ~((sizeof(size_t)-1)));
		if ((*it)->input > input) terminate = it;
		else start = it;
	}
	// Linear search is more efficient on the last interval
	for (it = start; it != terminate; ++it)
	{
		if ((*it)->input >= input) break;
	}
	if (it == keys.begin()) return outputOffset + outputStart;

	// Get the keys and values for this interval
	const FCDAnimationKey* startKey = *(it - 1);
	const FCDAnimationKey* endKey = *it;
	float inputInterval = endKey->input - startKey->input;
	float outputInterval = endKey->output - startKey->output;

	// Interpolate the output.
	// Similar code is found in FCDAnimationMultiCurve.cpp. If you update this, update the other one too.
	float output;
	switch (startKey->interpolation)
	{
	case FUDaeInterpolation::LINEAR: {
		output = startKey->output + (input - startKey->input) / inputInterval * outputInterval;
		break; }

	case FUDaeInterpolation::BEZIER: {
		if (endKey->interpolation == FUDaeInterpolation::LINEAR) {
			output = startKey->output + (input - startKey->input) / inputInterval * outputInterval;
			break;
		}
		if (endKey->interpolation == FUDaeInterpolation::DEFAULT || 
			endKey->interpolation == FUDaeInterpolation::STEP ||
			endKey->interpolation == FUDaeInterpolation::UNKNOWN) {
			output = startKey->output;
			break;
		}
		//Code that applies to both whether the endKey is Bezier or TCB.
		FCDAnimationKeyBezier* bkey1 = (FCDAnimationKeyBezier*) startKey;
		FMVector2 inTangent;
		if (endKey->interpolation == FUDaeInterpolation::BEZIER) {
			inTangent = ((FCDAnimationKeyBezier*) endKey)->inTangent;
		}
		else if (endKey->interpolation == FUDaeInterpolation::TCB) {
			FCDAnimationKeyTCB* tkey2 = (FCDAnimationKeyTCB*) endKey;
			FMVector2 tempTangent;
			tempTangent.x = tempTangent.y = 0.0f;
			const FCDAnimationKey* nextKey = (it + 1) < keys.end() ? (*(it + 1)) : NULL;
			ComputeTCBTangent(startKey, endKey, nextKey, tkey2->tension, tkey2->continuity, tkey2->bias, inTangent, tempTangent);
			//Change this when we've figured out the values of the vectors from TCB...
			inTangent.x = endKey->input + inTangent.x; 
			inTangent.y = endKey->output + inTangent.y;
		}
		float t = (input - startKey->input) / inputInterval;
		if (is2DEvaluation) t = FindT(bkey1->input, bkey1->outTangent.x, inTangent.x, endKey->input, input, t);
 		float b = bkey1->outTangent.y;
		float c = inTangent.y;
		float ti = 1.0f - t;
		float br = 3.0f;
		float cr = 3.0f;
		if (!is2DEvaluation) { 
			br = inputInterval / (bkey1->outTangent.x - startKey->input);
			cr = inputInterval / (endKey->input - inTangent.x);
			br = FMath::Clamp(br, 0.01f, 100.0f);
			cr = FMath::Clamp(cr, 0.01f, 100.0f);
		}
		output = startKey->output * ti * ti * ti + br * b * ti * ti * t + cr * c * ti * t * t + endKey->output * t * t * t;
		break; }
	case FUDaeInterpolation::TCB: {
		if (endKey->interpolation == FUDaeInterpolation::LINEAR) {
			output = startKey->output + (input - startKey->input) / inputInterval * outputInterval;
			break;
		}
		if (endKey->interpolation == FUDaeInterpolation::DEFAULT || 
			endKey->interpolation == FUDaeInterpolation::STEP ||
			endKey->interpolation == FUDaeInterpolation::UNKNOWN) {
			output = startKey->output;
			break;
		}
		// Calculate the start key's out-tangent.
		FCDAnimationKeyTCB* tkey1 = (FCDAnimationKeyTCB*) startKey;
		FMVector2 startTangent, tempTangent, endTangent;
		startTangent.x = startTangent.y = tempTangent.x = tempTangent.y = endTangent.x = endTangent.y = 0.0f;
		const FCDAnimationKey* previousKey = (it - 1) > keys.begin() ? (*(it - 2)) : NULL;
		ComputeTCBTangent(previousKey, startKey, endKey, tkey1->tension, tkey1->continuity, tkey1->bias, tempTangent, startTangent);

		// Calculate the end key's in-tangent.
		float by = 0.0f, cy= 0.0f; //will be used in the Bezier equation.
		float bx = 0.0f, cx = 0.0f; //will be used in FindT.. x equivalent of the point at b and c
		if (endKey->interpolation == FUDaeInterpolation::TCB) {
			FCDAnimationKeyTCB* tkey2 = (FCDAnimationKeyTCB*) endKey;
			const FCDAnimationKey* nextKey = (it + 1) < keys.end() ? (*(it + 1)) : NULL;
			ComputeTCBTangent(startKey, endKey, nextKey, tkey2->tension, tkey2->continuity, tkey2->bias, endTangent, tempTangent);
			cy = endKey->output + endTangent.y; //Assuming the tangent is GOING from the point.
			cx = endKey->output + endTangent.x;
		}
		else if (endKey->interpolation == FUDaeInterpolation::BEZIER) {
			FCDAnimationKeyBezier* tkey2 = (FCDAnimationKeyBezier*) endKey;
			endTangent = tkey2->inTangent;
			cy = endTangent.y;
			cx = endTangent.x;
		}
		float t = (input - inputStart) / inputInterval;
		by = startKey->output - startTangent.y; //Assuming the tangent is GOING from the point.
		bx = startKey->input - startTangent.x;

		if (is2DEvaluation) t = FindT(tkey1->input, bx, cx, endKey->input, input, t);
//		else { //Need to figure out algorithm for easing in and out.
//			t = Ease(t, tkey1->easeIn, tkey1->easeOut);
//		}

		float ti = 1.0f - t;
		output = startKey->output*ti*ti*ti +
			3*by*t*ti*ti +
			3*cy*t*t*ti +
			endKey->output*t*t*t;
		break; }
	case FUDaeInterpolation::STEP:
	case FUDaeInterpolation::UNKNOWN:
	default:
		output = startKey->output;
		break;
	}
	return outputOffset + output;
}

// Apply a conversion function on the key values and tangents
void FCDAnimationCurve::ConvertValues(FCDConversionFunction valueConversion, FCDConversionFunction tangentConversion)
{
	if (valueConversion != NULL)
	{
		for (FCDAnimationKeyList::iterator it = keys.begin(); it != keys.end(); ++it)
		{
			(*it)->output = (*valueConversion)((*it)->output);
		}
	}
	if (tangentConversion != NULL)
	{
		for (FCDAnimationKeyList::iterator it = keys.begin(); it != keys.end(); ++it)
		{
			if ((*it)->interpolation == FUDaeInterpolation::BEZIER)
			{
				FCDAnimationKeyBezier* bkey = (FCDAnimationKeyBezier*) (*it);
				bkey->inTangent.v = (*tangentConversion)(bkey->inTangent.v);
				bkey->outTangent.v = (*tangentConversion)(bkey->outTangent.v);
			}
		}
	}
	SetDirtyFlag();
}
void FCDAnimationCurve::ConvertValues(FCDConversionFunctor* valueConversion, FCDConversionFunctor* tangentConversion)
{
	if (valueConversion != NULL)
	{
		for (FCDAnimationKeyList::iterator it = keys.begin(); it != keys.end(); ++it)
		{
			(*it)->output = (*valueConversion)((*it)->output);
		}
	}
	if (tangentConversion != NULL)
	{
		for (FCDAnimationKeyList::iterator it = keys.begin(); it != keys.end(); ++it)
		{
			if ((*it)->interpolation == FUDaeInterpolation::BEZIER)
			{
				FCDAnimationKeyBezier* bkey = (FCDAnimationKeyBezier*) (*it);
				bkey->inTangent.v = (*tangentConversion)(bkey->inTangent.v);
				bkey->outTangent.v = (*tangentConversion)(bkey->outTangent.v);
			}
		}
	}
	SetDirtyFlag();
}

// Apply a conversion function on the key times and tangent weights
void FCDAnimationCurve::ConvertInputs(FCDConversionFunction timeConversion, FCDConversionFunction tangentWeightConversion)
{
	if (timeConversion != NULL)
	{
		for (FCDAnimationKeyList::iterator it = keys.begin(); it != keys.end(); ++it)
		{
			(*it)->input = (*timeConversion)((*it)->input);
		}
	}
	if (tangentWeightConversion != NULL)
	{
		for (FCDAnimationKeyList::iterator it = keys.begin(); it != keys.end(); ++it)
		{
			if ((*it)->interpolation == FUDaeInterpolation::BEZIER)
			{
				FCDAnimationKeyBezier* bkey = (FCDAnimationKeyBezier*) (*it);
				bkey->inTangent.u = (*tangentWeightConversion)(bkey->inTangent.u);
				bkey->outTangent.u = (*tangentWeightConversion)(bkey->outTangent.u);
			}
		}
	}
	SetDirtyFlag();
}
void FCDAnimationCurve::ConvertInputs(FCDConversionFunctor* timeConversion, FCDConversionFunctor* tangentWeightConversion)
{
	if (timeConversion != NULL)
	{
		for (FCDAnimationKeyList::iterator it = keys.begin(); it != keys.end(); ++it)
		{
			(*it)->input = (*timeConversion)((*it)->input);
		}
	}
	if (tangentWeightConversion != NULL)
	{
		for (FCDAnimationKeyList::iterator it = keys.begin(); it != keys.end(); ++it)
		{
			if ((*it)->interpolation == FUDaeInterpolation::BEZIER)
			{
				FCDAnimationKeyBezier* bkey = (FCDAnimationKeyBezier*) (*it);
				bkey->inTangent.u = (*tangentWeightConversion)(bkey->inTangent.u);
				bkey->outTangent.u = (*tangentWeightConversion)(bkey->outTangent.u);
			}
		}
	}
	SetDirtyFlag();
}

void FCDAnimationCurve::SetClipOffset(float offset, const FCDAnimationClip* clip)
{
	for (size_t i = 0; i < clips.size(); ++i)
	{
		if (clips[i] == clip)
		{
			clipOffsets[i] = offset;
			break;
		}
	}
	SetDirtyFlag();
}

void FCDAnimationCurve::RegisterAnimationClip(FCDAnimationClip* clip) 
{ 
	clips.push_back(clip); 
	clipOffsets.push_back(-clip->GetStart());
	SetDirtyFlag();
}
