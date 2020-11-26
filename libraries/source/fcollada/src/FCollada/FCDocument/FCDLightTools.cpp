/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDLightTools.h"
#include "FCDocument/FCDLight.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimationCurveTools.h"
#include "FCDocument/FCDAnimationKey.h"
#include "FCDocument/FCDAnimationMultiCurve.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDLibrary.h"

namespace FCDLightTools
{
	void LoadPenumbra(FCDLight* light, float penumbraValue, FCDAnimated* penumbraAnimated, bool createAnimationChannel)
	{
		bool fallOffIsOuter = false;

		// Take care of the default non-animated values
		
		float fallOffAngle = light->GetFallOffAngle();
		if (penumbraValue < 0)
		{
			penumbraValue = -penumbraValue;

			fallOffAngle -= 2*penumbraValue;
			light->SetFallOffAngle(fallOffAngle);
			fallOffIsOuter = true;
		}

		float outerAngle = fallOffAngle + 2*penumbraValue;
		light->SetOuterAngle(outerAngle);

		// Take care of the animated values
		FCDParameterAnimatableFloat& outerAngleValue = light->GetOuterAngle();
		FCDAnimated* outerAngleAnimated = outerAngleValue.GetAnimated();
		if (penumbraAnimated->HasCurve() && penumbraAnimated != outerAngleAnimated)
		{
			penumbraAnimated->Clone(outerAngleAnimated);
		}

		FCDParameterAnimatableFloat& fallOffAngleValue = light->GetFallOffAngle();
		FCDAnimated* fallOffAngleAnimated = fallOffAngleValue.GetAnimated();

		FCDAnimationCurve* outerAngleCurve = outerAngleAnimated->GetCurve(0);
		FCDAnimationCurve* fallOffAngleCurve = fallOffAngleAnimated->GetCurve(0);

		// no animations on the penumbra nor the fallOffAngle, so no animation needed
		if (outerAngleCurve == NULL && fallOffAngleCurve == NULL) return; 

		// there's animation on the fallOffAngle only, so just need to copy the animation to the outterAngle as well
		if (outerAngleCurve == NULL)
		{
			fallOffAngleAnimated->Clone(outerAngleAnimated);
			FUAssert(outerAngleAnimated != NULL, return); // should not be null because we know fallOffAngle is animated
			outerAngleAnimated->SetTargetObject(light);

			// if we penumbra was negative, fallOffAngle needs to be lowered, else outerAngle needs to be raised
			float offset = 0.0f;
			FCDAnimated* animated = NULL;
			if (fallOffIsOuter)
			{
				offset = -2 * penumbraValue;
				animated = fallOffAngleAnimated;
			}
			else
			{
				offset = 2 * penumbraValue;
				animated = outerAngleAnimated;
			}

			// now the outerAngle has the penumbra on it too, so have to update the keys

			// add twice the penumbra to the fallOffAngle to get the OuterAngle
			FCDConversionOffsetFunctor offsetFunctor(offset);
			FCDAnimationCurveTrackList& curves = animated->GetCurves()[0];
			size_t curvesCount = curves.size();
			for (size_t curvesCounter = 0; curvesCounter < curvesCount; curvesCounter++)
			{
				curves.at(curvesCounter)->ConvertValues(&offsetFunctor, &offsetFunctor);
			}
			return;
		}

		outerAngleAnimated->SetTargetObject(light); // at this point, outerAngleAnimated != NULL

		// FIXME: [aleung] This is not necessarily true, but close enough for now
		size_t outerAngleKeysCount = outerAngleCurve->GetKeyCount();
		FCDAnimationKey** outerAngleKeys = outerAngleCurve->GetKeys();
		bool hasNegPenumbra = fallOffIsOuter;
		if (!hasNegPenumbra)
		{
			for (size_t i = 0; i < outerAngleKeysCount; i++)
			{
				if (outerAngleKeys[i]->output < 0)
				{
					hasNegPenumbra = true;
					break;
				}
			}
		}

		// there's animation on the penumbra only and there's no negative values, so we just need to animate the outerAngle
		if ((fallOffAngleCurve == NULL) && !hasNegPenumbra)
		{
			FCDAnimationCurveListList& curvesList = outerAngleAnimated->GetCurves();

			// fall off is not animated, easy case
			FCDConversionScaleFunctor scaleFunctor(2);
			FCDConversionOffsetFunctor offsetFunctor(fallOffAngleValue);

			FCDAnimationCurveTrackList& curves = curvesList[0];
			size_t curvesCount = curves.size();
			for (size_t curvesCounter = 0; curvesCounter < curvesCount; curvesCounter++)
			{
				curves.at(curvesCounter)->ConvertValues(&scaleFunctor, &scaleFunctor);
				curves.at(curvesCounter)->ConvertValues(&offsetFunctor, &offsetFunctor);
			}
			return;
		}

		// at this point, we need curves for both fallOffAngle and outerAngle!
		if (fallOffAngleCurve == NULL)
		{
			if (createAnimationChannel)
			{
				outerAngleValue.GetAnimated()->Clone(fallOffAngleAnimated);
			}
			FUAssert(fallOffAngleAnimated != NULL, return); // should not be null because outerAngle is animated
			fallOffAngleAnimated->SetTargetObject(light);
			fallOffAngleCurve = fallOffAngleAnimated->GetCurve(0);

			if (fallOffAngleCurve == NULL)
			{
				FUAssert(createAnimationChannel, ;); // should only happen if we created new animated
				fallOffAngleAnimated->RemoveCurve(0);
				FCDAnimation* animation = light->GetDocument()->GetAnimationLibrary()->AddEntity();
				animation->SetDaeId("compensate_falloff_for_penumbra");
				FCDAnimationChannel* channel = animation->AddChannel();
				fallOffAngleCurve = channel->AddCurve();
				outerAngleCurve->Clone(fallOffAngleCurve);
				fallOffAngleAnimated->AddCurve(0, fallOffAngleCurve);
			}

			// remove the keys
			FCDAnimationCurveTrackList& curves = fallOffAngleAnimated->GetCurves()[0];

			size_t curvesCount = curves.size();
			for (size_t i = 0; i < curvesCount; i++)
			{
				curves.at(i)->SetKeyCount(0, FUDaeInterpolation::BEZIER);

				// add keys at the beginning and end with constant pre/post infinity
				// the output value should be outerAngle if fallOffIsOuter, otherwise, it should be fallOffAngle
				curves.at(i)->SetPreInfinity(FUDaeInfinity::CONSTANT);
				curves.at(i)->SetPostInfinity(FUDaeInfinity::CONSTANT);

				FCDAnimationKey* firstKey = curves.at(i)->AddKey(FUDaeInterpolation::LINEAR);
				firstKey->input = outerAngleKeys[0]->input;
				firstKey->output = (fallOffIsOuter)? light->GetOuterAngle() : light->GetFallOffAngle();

				FCDAnimationKey* lastKey = curves.at(i)->AddKey(FUDaeInterpolation::LINEAR);
				lastKey->input = outerAngleKeys[(outerAngleKeysCount > 1)? outerAngleKeysCount - 1 : 0]->input;
				lastKey->output = (fallOffIsOuter)? light->GetOuterAngle() : light->GetFallOffAngle();
			}
		}


		

		// now we have both penumbra and fallOffAngle animated, have to create missing keys by merging the curves
		
		
		FloatList multiCurveValues(2, 0.0f);
		FCDAnimationCurveConstList multiCurveCurves(2);
		multiCurveValues[0] = fallOffAngleValue;
		multiCurveValues[1] = outerAngleValue;
		multiCurveCurves[0] = fallOffAngleCurve;
		multiCurveCurves[1] = outerAngleCurve;

		FCDAnimationMultiCurve* multiCurve = FCDAnimationCurveTools::MergeCurves(multiCurveCurves, multiCurveValues);
		size_t dimension = multiCurve->GetDimension();
		FUAssert(dimension == 2, return);

		// go through the keys to see if there's any switches in signs. If so, we need to insert intermediate keys to the 
		// outerAngleCurve (penumbra) at the penumbra = 0 point to get desired result
		{
			size_t multiKeyCount = multiCurve->GetKeyCount();
			FUAssert(multiKeyCount > 0, return); // should not have no keys
			FCDAnimationMKey** multiKeys = multiCurve->GetKeys();

			size_t additionalKeys = 0;
			bool isNegative = multiKeys[0]->output[1] < 0;
			bool wasZero = IsEquivalent(multiKeys[0]->output[1], 0.0f);
			bool hasFlip = false;
			for (size_t i = 1; i < multiKeyCount; i++) // start from 1 since we won't add key before the first key
			{
				FCDAnimationMKey* key = multiKeys[i];
				if (IsEquivalent(key->output[1], 0.0f))
				{
					i++;
					if (i < multiKeyCount)
					{
						isNegative = multiKeys[i]->output[1] < 0;
						wasZero = IsEquivalent(multiKeys[i]->output[1], 0.0f);
					}
					continue;
				}
				else
				{
					if (wasZero)
					{
						isNegative = key->output[1] < 0;
						wasZero = false;
						continue;
					}
					wasZero = false;
				}

				if ((isNegative && (key->output[1] > 0)) || (!isNegative && (key->output[1] < 0)))
				{
					hasFlip = true;
					isNegative = !isNegative;

					// flip in sign, find out where to add the additional key using binary search
					float higherTime = key->input;
					float lowerTime = multiKeys[i-1]->input; // will not fail since the first sign is taken
					float output[2];
					float currentTime = (higherTime + lowerTime) / 2.0f;

					multiCurve->Evaluate(currentTime, output);
					while (!IsEquivalent(output[1], 0.0f)) // while penumbra output != 0
					{
						if ((isNegative && (output[1] < 0)) || (!isNegative && (output[1] > 0)))
						{
							higherTime = currentTime;
						}
						else
						{
							lowerTime = currentTime;
						}
						currentTime = (higherTime + lowerTime) / 2.0f;
						multiCurve->Evaluate(currentTime, output);
					}

					size_t index = i + additionalKeys;
					FCDAnimationKey* newKey = 
							outerAngleCurve->AddKey((FUDaeInterpolation::Interpolation)multiKeys[i-1]->interpolation,
							currentTime, index);
					additionalKeys++;
					newKey->input = currentTime;
					newKey->output = 0.0f;
					if (key->interpolation == FUDaeInterpolation::BEZIER)
					{
						FCDAnimationKeyBezier* newBkey = (FCDAnimationKeyBezier*)newKey;

						float previousInput = multiKeys[i-1]->input;
						float nextInput = multiKeys[i]->input;
						newBkey->inTangent.x = (previousInput + 2.0f * newBkey->input) / 3.0f;
						newBkey->outTangent.x = (nextInput + 2.0f * newBkey->input) / 3.0f;

						// calculate estimate inTangents
						float prevDistance = newBkey->input - newBkey->inTangent.x;
						float prevDelta = prevDistance / 1000; // small enough to get an estimate
						multiCurve->Evaluate(currentTime - prevDelta, output);
						float slope = output[1] / prevDelta;
						newBkey->inTangent.y = prevDistance * slope;

						// calculate estimate outTangents
						float nextDistance = newBkey->outTangent.x - newBkey->input;
						float nextDelta = nextDistance / 1000; // small enough to get an estimate
						multiCurve->Evaluate(currentTime + nextDelta, output);
						slope = output[1] / nextDelta;
						newBkey->outTangent.y = nextDistance * slope;
					}
				}
			}

			if (hasFlip)
			{
				// recompute the multiCurve with the intermediate values
				multiCurve = FCDAnimationCurveTools::MergeCurves(multiCurveCurves, multiCurveValues);
				dimension = multiCurve->GetDimension();
				FUAssert(dimension == 2, return);
			}
		}


		// erase the current keys
		fallOffAngleCurve->SetKeyCount(0, FUDaeInterpolation::BEZIER);
		outerAngleCurve->SetKeyCount(0, FUDaeInterpolation::BEZIER);

		// add the new keys and set their input and output values
		FCDAnimationMKey** keys = multiCurve->GetKeys();
		size_t keyCount = multiCurve->GetKeyCount();
		for (size_t i = 0; i < keyCount; i++)
		{
			FCDAnimationMKey* key = keys[i];

			FCDAnimationKey* fallOffAngleKey = 
					fallOffAngleCurve->AddKey((FUDaeInterpolation::Interpolation)key->interpolation);
			FCDAnimationKey* outerAngleKey = 
					outerAngleCurve->AddKey((FUDaeInterpolation::Interpolation)key->interpolation);

			fallOffAngleKey->input = key->input;
			outerAngleKey->input = key->input;

			float offset = key->output[0];
			fallOffAngleKey->output = offset;
			outerAngleKey->output = offset + 2*key->output[1];

			if (key->interpolation == FUDaeInterpolation::BEZIER)
			{
				FCDAnimationMKeyBezier* bkey = (FCDAnimationMKeyBezier*)key;

				FCDAnimationKeyBezier* fallOffAngleBkey = (FCDAnimationKeyBezier*) fallOffAngleKey;
				FCDAnimationKeyBezier* outerAngleBkey = (FCDAnimationKeyBezier*) outerAngleKey;

				fallOffAngleBkey->inTangent = bkey->inTangent[0];
				fallOffAngleBkey->outTangent =  bkey->outTangent[0];

				outerAngleBkey->inTangent = bkey->inTangent[1];
				outerAngleBkey->outTangent = bkey->outTangent[1];
				outerAngleBkey->inTangent.v = 2*outerAngleBkey->inTangent.v; // deal with the offset in next pass
				outerAngleBkey->outTangent.v = 2*outerAngleBkey->outTangent.v; // deal with the offset in next pass

				float inTangentsOffset = (fallOffAngleBkey->inTangent.y - fallOffAngleBkey->output) /
						(fallOffAngleBkey->inTangent.x - fallOffAngleBkey->input);
				float outTangentsOffset = (fallOffAngleBkey->outTangent.y - fallOffAngleBkey->output) /
						(fallOffAngleBkey->outTangent.x - fallOffAngleBkey->input);

				outerAngleBkey->outTangent.y += 
						offset + outTangentsOffset * (outerAngleBkey->outTangent.x - outerAngleBkey->input);
				outerAngleBkey->inTangent.y += 
						offset + inTangentsOffset * (outerAngleBkey->inTangent.x - outerAngleBkey->input);
			}
		}

		// flip the keys (and their tangents) if necessary
		FCDAnimationKey** finalFallOffAngleKeys = fallOffAngleCurve->GetKeys();
		FCDAnimationKey** finalOuterAngleKeys = outerAngleCurve->GetKeys();
		bool previousFlipKeys = false;
		FCDAnimationKeyBezier* previousFallOffAngleBkey = NULL;
		FCDAnimationKeyBezier* previousOuterAngleBkey = NULL;
		for (size_t i = 0; i < keyCount; i++)
		{
			FCDAnimationKey* fallOffAngleKey = finalFallOffAngleKeys[i];
			FCDAnimationKey* outerAngleKey = finalOuterAngleKeys[i];

			// FIXME: [aleung] this is not necessarily enough, but close enough for now
			bool flipKeys = false;
			if (outerAngleKey->output < fallOffAngleKey->output)
			{
				float temp = fallOffAngleKey->output;
				fallOffAngleKey->output = outerAngleKey->output;
				outerAngleKey->output = temp;
				flipKeys = true;
			}

			if (fallOffAngleKey->interpolation == FUDaeInterpolation::BEZIER) // both same interpolation
			{
				FCDAnimationKeyBezier* fallOffAngleBkey = (FCDAnimationKeyBezier*) fallOffAngleKey;
				FCDAnimationKeyBezier* outerAngleBkey = (FCDAnimationKeyBezier*) outerAngleKey;

				if (flipKeys)
				{
					// flip outTangents
					FMVector2 temp = outerAngleBkey->outTangent;
					outerAngleBkey->outTangent = fallOffAngleBkey->outTangent;
					fallOffAngleBkey->outTangent = temp;

					temp = outerAngleBkey->inTangent;
					outerAngleBkey->inTangent = fallOffAngleBkey->inTangent;
					fallOffAngleBkey->inTangent = temp;

					if ((!previousFlipKeys) && (i != 0))
					{
						// have to flip their outTangents
						if (previousFallOffAngleBkey != NULL)
						{
							temp = previousOuterAngleBkey->outTangent;
							previousOuterAngleBkey->outTangent = previousFallOffAngleBkey->outTangent;
							previousFallOffAngleBkey->outTangent = temp;
						}
					}
				}
				else if (previousFlipKeys) // we need to flip our inTangents even though we did not flip
				{
					// flip inTangents
					FMVector2 temp = outerAngleBkey->inTangent;
					outerAngleBkey->inTangent = fallOffAngleBkey->inTangent;
					fallOffAngleBkey->inTangent = temp;
				}

				previousFallOffAngleBkey = fallOffAngleBkey;
				previousOuterAngleBkey = outerAngleBkey;
			}
			else
			{
				previousFallOffAngleBkey = NULL;
				previousOuterAngleBkey = NULL;
			}
			previousFlipKeys = flipKeys;
		}
	}
}

