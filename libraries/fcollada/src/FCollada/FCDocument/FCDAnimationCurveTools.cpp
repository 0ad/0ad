/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAnimationCurveTools.h"
#include "FCDocument/FCDAnimationKey.h"
#include "FCDocument/FCDAnimationMultiCurve.h"

#define SMALL_DELTA	0.001f

namespace FCDAnimationCurveTools
{
	// Non-standard constructor used to merge together animation curves
	FCDAnimationMultiCurve* MergeCurves(const FCDAnimationCurveConstList& toMerge, const FloatList& defaultValues)
	{
		size_t dimension = toMerge.size();
		if (dimension == 0) return NULL;

		// Look for the document pointer and select the parent curve.
		FCDocument* document = NULL;
		int32 targetElement = -1;
		FUDaeInfinity::Infinity preInfinity = FUDaeInfinity::CONSTANT, postInfinity = FUDaeInfinity::CONSTANT;
		for (size_t i = 0; i < dimension; ++i)
		{
			if (toMerge[i] != NULL)
			{
				document = const_cast<FCDocument*>(toMerge[i]->GetDocument());
				targetElement = toMerge[i]->GetTargetElement();
				preInfinity = toMerge[i]->GetPreInfinity();
				postInfinity = toMerge[i]->GetPostInfinity();
				break;
			}
		}
		if (document == NULL) return NULL;

		// Allocate the output multiCurve.
		FCDAnimationMultiCurve* multiCurve = new FCDAnimationMultiCurve(document, (uint32) dimension);
		multiCurve->SetTargetElement(targetElement);
		multiCurve->SetPreInfinity(preInfinity);
		multiCurve->SetPostInfinity(postInfinity);

		// Calculate the merged input keys and their wanted interpolations.
		FloatList mergedInputs;
		UInt32List mergedInterpolations;
		for (size_t i = 0; i < dimension; ++i)
		{
			const FCDAnimationCurve* curve = toMerge[i];
			if (curve == NULL) continue;
			const FCDAnimationKey** curveKeys = curve->GetKeys();

			// Merge each curve's keys, which should already be sorted, into the multi-curve's
			size_t multiCurveKeyCount = mergedInputs.size(), m = 0;
			size_t curveKeyCount = curve->GetKeyCount(), c = 0;
			while (m < multiCurveKeyCount && c < curveKeyCount)
			{
				if (IsEquivalent(mergedInputs[m], curveKeys[c]->input))
				{
					if (mergedInterpolations[m] != curveKeys[c]->interpolation) mergedInterpolations[m] = FUDaeInterpolation::BEZIER;
					++c; ++m;
				}
				else if (mergedInputs[m] < curveKeys[c]->input) { ++m; }
				else
				{
					// Insert this new key within the merged list.
					mergedInputs.insert(mergedInputs.begin() + m, curveKeys[c]->input);
					mergedInterpolations.insert(mergedInterpolations.begin() + m, curveKeys[c]->interpolation);
					++multiCurveKeyCount;
					++m; ++c;
				}
			}
			while (c < curveKeyCount)
			{
				// Insert all these extra keys at the end of the merged list.
				mergedInputs.push_back(curveKeys[c]->input);
				mergedInterpolations.push_back(curveKeys[c]->interpolation);
				++c;
			}
		}
		size_t keyCount = mergedInputs.size();

		// Create the multi-dimensional keys.
		for (size_t i = 0; i < keyCount; ++i)
		{
			FCDAnimationMKey* key = multiCurve->AddKey((FUDaeInterpolation::Interpolation) mergedInterpolations[i]);
			key->input = mergedInputs[i];
		}
		FCDAnimationMKey** keys = multiCurve->GetKeys();

		// Merge the curves one by one into the multi-curve
		for (size_t i = 0; i < dimension; ++i)
		{
			const FCDAnimationCurve* curve = toMerge[i];
			if (curve == NULL || curve->GetKeyCount() == 0)
			{
				// No curve, or an empty curve, set the default value on all the keys
				float defaultValue = (i < defaultValues.size()) ? defaultValues[i] : 0.0f;
				for (size_t k = 0; k < keyCount; ++k)
				{
					keys[k]->output[i] = defaultValue;
					if (keys[k]->interpolation == FUDaeInterpolation::BEZIER)
					{
						float previousSpan = (k > 0 ? mergedInputs[k] - mergedInputs[k - 1] : 1.0f) / 3.0f;
						float nextSpan = (k < keyCount - 1 ? mergedInputs[k + 1] - mergedInputs[k] : 1.0f) / 3.0f;

						FCDAnimationMKeyBezier* bkey = (FCDAnimationMKeyBezier*) keys[k];
						bkey->inTangent[i] = FMVector2(keys[k]->input - previousSpan, defaultValue);
						bkey->outTangent[i] = FMVector2(keys[k]->input + nextSpan, defaultValue);
					}
				}
				
				continue;
			}

			// Merge in this curve's values, sampling when the multi-curve's key is not present in the curve.
			// Calculate and retrieve the tangents in a polar-like form.
			const FCDAnimationKey** curveKeys = curve->GetKeys();
			size_t curveKeyCount = curve->GetKeyCount();
			for (size_t k = 0, c = 0; k < keyCount; ++k)
			{
				float input = keys[k]->input;
				float previousSpan = (k > 0 ? input - keys[k - 1]->input : 1.0f) / 3.0f;
				float nextSpan = (k < keyCount - 1 ? keys[k + 1]->input - input : 1.0f) / 3.0f;

				if (c >= curveKeyCount || !IsEquivalent(keys[k]->input, curveKeys[c]->input))
				{
					// Sample the curve
					float value = keys[k]->output[i] = curve->Evaluate(input);
					if (keys[k]->interpolation == FUDaeInterpolation::BEZIER)
					{
						// Calculate the slope at the sampled point.
						// Since the curve should be smooth: the in/out tangents should be equal.
						FCDAnimationMKeyBezier* bkey = (FCDAnimationMKeyBezier*) keys[k];
						float slope = (value - curve->Evaluate(input - SMALL_DELTA)) / SMALL_DELTA;
						bkey->inTangent[i] = FMVector2(input - previousSpan, value - slope * previousSpan);
						bkey->outTangent[i] = FMVector2(input + nextSpan, value + slope * nextSpan);
					}
					else if (keys[k]->interpolation == FUDaeInterpolation::TCB)
					{
						// Don't fool around: just set default values.
						FCDAnimationMKeyTCB* tkey = (FCDAnimationMKeyTCB*) keys[k];
						tkey->tension[i] = tkey->continuity[i] = tkey->bias[i] = 0.5f;
						tkey->easeIn[i] = tkey->easeOut[i] = 0.0f;
					}
				}
				else
				{
					// Keys match, grab the value directly
					keys[k]->output[i] = curveKeys[c]->output;

					// Check the wanted interpolation type to retrieve/calculate the extra necessary information.
					if (keys[k]->interpolation == FUDaeInterpolation::BEZIER)
					{
						float oldPreviousSpan = (c > 0 ? curveKeys[c]->input - curveKeys[c - 1]->input : 1.0f) / 3.0f;
						float oldNextSpan = (c < curveKeyCount - 1 ? curveKeys[c + 1]->input - curveKeys[c]->input : 1.0f) / 3.0f;

						// Calculate the new tangent: keep the slope proportional
						FCDAnimationMKeyBezier* bkey = (FCDAnimationMKeyBezier*) keys[k];
						if (curveKeys[c]->interpolation == FUDaeInterpolation::BEZIER)
						{
							FCDAnimationKeyBezier* bkey2 = (FCDAnimationKeyBezier*) curveKeys[c];
							FMVector2 absolute(bkey->input, bkey->output[i]);
							bkey->inTangent[i] = absolute + (bkey2->inTangent - absolute) / oldPreviousSpan * previousSpan;
							bkey->outTangent[i] = absolute + (bkey2->outTangent - absolute) / oldNextSpan * nextSpan;
						}
						else
						{
							// Default to flat tangents.
							bkey->inTangent[i] = FMVector2(bkey->input - previousSpan, bkey->output[i]);
							bkey->outTangent[i] = FMVector2(bkey->input + nextSpan, bkey->output[i]);
						}
					}
					else if (keys[k]->interpolation == FUDaeInterpolation::TCB)
					{
						FCDAnimationMKeyTCB* tkey = (FCDAnimationMKeyTCB*) keys[k];
						if (curveKeys[c]->interpolation == FUDaeInterpolation::TCB)
						{
							FCDAnimationKeyTCB* tkey2 = (FCDAnimationKeyTCB*) curveKeys[c];
							tkey->tension[i] = tkey2->tension;
							tkey->continuity[i] = tkey2->continuity;
							tkey->bias[i] = tkey2->bias;
							tkey->easeIn[i] = tkey2->easeIn;
							tkey->easeOut[i] = tkey2->easeOut;
						}
						else
						{
							// Default to flat values.
							tkey->tension[i] = tkey->continuity[i] = tkey->bias[i] = 0.5f;
							tkey->easeIn[i] = tkey->easeOut[i] = 0.0f;
						}
					}

					// Go to the next existing key in the 1D curve.
					++c;
				}
			}
		}
		return multiCurve;
	}

	// Collapse this multi-dimensional curve into a one-dimensional curve, given a collapsing function
	FCDAnimationCurve* Collapse(const FCDAnimationMultiCurve* curve, FCDCollapsingFunction collapse)
	{
		size_t keyCount = curve->GetKeyCount();
		size_t dimension = curve->GetDimension();
		if (keyCount == 0 || dimension== 0) return NULL;
		if (collapse == NULL) collapse = Average;
		const FCDAnimationMKey** inKeys = curve->GetKeys();

		// Create the output one-dimensional curve and create the keys.
		FCDAnimationCurve* out = new FCDAnimationCurve(const_cast<FCDocument*>(curve->GetDocument()), NULL);
		for (size_t i = 0; i < keyCount; ++i)
		{
			out->AddKey((FUDaeInterpolation::Interpolation) inKeys[i]->interpolation);
		}
		FCDAnimationKey** outKeys = out->GetKeys();

		// Copy the key data over, collapsing the values
		float* buffer = new float[dimension];
		for (size_t i = 0; i < keyCount; ++i)
		{
			outKeys[i]->input = inKeys[i]->input;

			// Collapse the values and the tangents
			for (uint32 j = 0; j < dimension; ++j) buffer[j] = inKeys[i]->output[j];
			outKeys[i]->output = (*collapse)(buffer, (uint32) dimension);

			if (outKeys[i]->interpolation == FUDaeInterpolation::BEZIER)
			{
				FCDAnimationMKeyBezier* inBKey = (FCDAnimationMKeyBezier*) inKeys + i;
				FCDAnimationKeyBezier* outBKey = (FCDAnimationKeyBezier*) outKeys + i;
				for (uint32 j = 0; j < dimension; ++j) buffer[j] = inBKey->inTangent[j].v;
				outBKey->inTangent = FMVector2(inBKey->inTangent[0].u, (*collapse)(buffer, (uint32) dimension));
				for (uint32 j = 0; j < dimension; ++j) buffer[j] = inBKey->outTangent[j].v;
				outBKey->outTangent = FMVector2(inBKey->outTangent[0].u, (*collapse)(buffer, (uint32) dimension));
			}
		}
		SAFE_DELETE_ARRAY(buffer);

		return out;
	}

	float TakeFirst(float* values, uint32 count)
	{
		return (count > 0) ? *values : 0.0f;
	}
	
	float Average(float* values, uint32 count)
	{
		float v = 0.0f;
		for (uint32 i = 0; i < count; ++i) v += values[i]; v /= float(count);
		return v;
	}
};
