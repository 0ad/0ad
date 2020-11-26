/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSceneNodeTools.h"
#include "FCDocument/FCDTransform.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimationKey.h"

namespace FCDSceneNodeTools
{
	static FloatList sampleKeys;
	static FMMatrix44List sampleValues;

	void GenerateSampledAnimation(FCDSceneNode* node)
	{
		sampleKeys.clear();
		sampleValues.clear();

		FCDAnimatedList animateds;
		// Special case for rotation angles: need to check for changes that are greater than 180 degrees.
		Int32List angleIndices;
		
		// Collect all the animation curves
		size_t transformCount = node->GetTransformCount();
		for (size_t t = 0; t < transformCount; ++t)
		{
			FCDTransform* transform = node->GetTransform(t);
			FCDAnimated* animated = transform->GetAnimated();
			if (animated != NULL)
			{
				if (animated->HasCurve()) animateds.push_back(animated);

				// Figure out whether this is a rotation and then, which animated value contains the angle.
				if (!transform->HasType(FCDTRotation::GetClassType())) angleIndices.push_back(-1);
				else angleIndices.push_back((int32) animated->FindQualifier(".ANGLE"));
			}
		}
		if (animateds.empty()) return;

		// Make a list of the ordered key times to sample
		size_t animatedsCount = animateds.size();
		for (size_t i = 0; i < animatedsCount; ++i)
		{
			FCDAnimated* animated = animateds[i];
			int32 angleIndex = angleIndices[i];

			const FCDAnimationCurveListList& allCurves = animated->GetCurves();
			size_t valueCount = allCurves.size();
			for (size_t curveIndex = 0; curveIndex < valueCount; ++curveIndex)
			{
				const FCDAnimationCurveTrackList& curves = allCurves[curveIndex];
				if (curves.empty()) continue;

				size_t curveKeyCount = curves.front()->GetKeyCount();
				const FCDAnimationKey** curveKeys = curves.front()->GetKeys();
				size_t sampleKeyCount = sampleKeys.size();
				
				// Merge this curve's keys in with the sample keys
				// This assumes both key lists are in increasing order
				size_t s = 0, c = 0;
				while (s < sampleKeyCount && c < curveKeyCount)
				{
					float sampleKey = sampleKeys[s], curveKey = curveKeys[c]->input;
					if (IsEquivalent(sampleKey, curveKey)) { ++s; ++c; }
					else if (sampleKey < curveKey) { ++s; }
					else
					{
						// Add this curve key to the sampling key list
						sampleKeys.insert(sampleKeys.begin() + (s++), curveKeys[c++]->input);
						sampleKeyCount++;
					}
				}

				// Add all the left-over curve keys to the sampling key list
				while (c < curveKeyCount) sampleKeys.push_back(curveKeys[c++]->input);

				// Check for large angular rotations..
				if (angleIndex == (intptr_t) curveIndex)
				{
					for (size_t c = 1; c < curveKeyCount; ++c)
					{
						const FCDAnimationKey* previousKey = curveKeys[c - 1];
						const FCDAnimationKey* currentKey = curveKeys[c];
						float halfWrapAmount = (currentKey->output - previousKey->output) / 180.0f;
						halfWrapAmount *= FMath::Sign(halfWrapAmount);
						if (halfWrapAmount >= 1.0f)
						{
							// Need to add sample times.
							size_t addSampleCount = (size_t) floorf(halfWrapAmount);
							for (size_t d = 1; d <= addSampleCount; ++d)
							{
								float fd = (float) d;
								float fid = (float) (addSampleCount + 1 - d);
								float addSampleTime = (currentKey->input * fd + previousKey->input * fid) / (fd + fid);
								
								// Sorted insert.
								float* endIt = sampleKeys.end();
								for (float* sampleKeyTime = sampleKeys.begin(); sampleKeyTime != endIt; ++sampleKeyTime)
								{
									if (IsEquivalent(*sampleKeyTime, addSampleTime)) break;
									else if (*sampleKeyTime > addSampleTime)
									{
										sampleKeys.insert(sampleKeyTime, addSampleTime);
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		size_t sampleKeyCount = sampleKeys.size();
		if (sampleKeyCount == 0) return;

		// Pre-allocate the value array;
		sampleValues.reserve(sampleKeyCount);
		
		// Sample the scene node transform
		for (size_t i = 0; i < sampleKeyCount; ++i)
		{
			float sampleTime = sampleKeys[i];
			for (FCDAnimatedList::iterator it = animateds.begin(); it != animateds.end(); ++it)
			{
				// Sample each animated, which changes the transform values directly
				(*it)->Evaluate(sampleTime);
			}

			// Retrieve the new transform matrix for the COLLADA scene node
			sampleValues.push_back(node->ToMatrix());
		}
	}

	const FloatList& GetSampledAnimationKeys()
	{
		return sampleKeys;
	}

	const FMMatrix44List& GetSampledAnimationMatrices()
	{
		return sampleValues;
	}

	void ClearSampledAnimation()
	{
		sampleKeys.clear();
		sampleValues.clear();
	}
};

