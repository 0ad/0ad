/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDAnimationCurveTools.h"
#include "FCDocument/FCDAnimationMultiCurve.h"
#include "FCDocument/FCDAnimationKey.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSceneNodeTools.h"

TESTSUITE_START(FCDAnimation)

TESTSUITE_TEST(0, Sampling)
	FUErrorSimpleHandler errorHandler;

	// Test import of the Eagle sample
	// Retrieves the "Bone09" joint and does the import sampling to verify its correctness.
	FUObjectRef<FCDocument> document = FCollada::NewTopDocument();
	PassIf(FCollada::LoadDocumentFromFile(document, FC("Eagle.DAE")));
	PassIf(errorHandler.IsSuccessful());
	PassIf(document->GetCameraLibrary()->GetEntityCount() == 1);
	FCDSceneNode* node = document->FindSceneNode("Bone09");
	FailIf(node == NULL);

	FCDSceneNodeTools::GenerateSampledAnimation(node);
	const FloatList& keys = FCDSceneNodeTools::GetSampledAnimationKeys();
	const FMMatrix44List& values = FCDSceneNodeTools::GetSampledAnimationMatrices();
	FailIf(keys.size() > 30);
	PassIf(keys.size() == values.size());
	FCDSceneNodeTools::ClearSampledAnimation();

TESTSUITE_TEST(1, CurveMerging)
	// Test the merge of single curves into multiple curves.
	FUObjectRef<FCDocument> document = FCollada::NewTopDocument();
	FCDAnimation* animation = document->GetAnimationLibrary()->AddEntity();
	FCDAnimationChannel* channel = animation->AddChannel();

	// Create a first curve.
	static const size_t curve1KeyCount = 3;
	static const float curve1Keys[curve1KeyCount] = { 0.0f, 2.0f, 3.0f };
	static const float curve1Values[curve1KeyCount] = { 0.0f, -2.0f, 0.0f };
	static const FMVector2 curve1Intan[curve1KeyCount] = { FMVector2(-0.3f, 0.0f), FMVector2(1.2f, 1.0f), FMVector2(2.8f, -1.0f) };
	static const FMVector2 curve1Outtan[curve1KeyCount] = { FMVector2(0.5f, -4.0f), FMVector2(2.5f, 3.0f), FMVector2(3.0f, 0.0f) };
	FCDAnimationCurve* c1 = channel->AddCurve();
	for (size_t i = 0; i < curve1KeyCount; ++i)
	{
		FCDAnimationKeyBezier* k = (FCDAnimationKeyBezier*) c1->AddKey(FUDaeInterpolation::BEZIER);
		k->input = curve1Keys[i]; k->output = curve1Values[i];
		k->inTangent = curve1Intan[i]; k->outTangent = curve1Outtan[i];
	}

	// Create a second curve.
	static const size_t curve2KeyCount = 3;
	static const float curve2Keys[curve2KeyCount] = { 0.0f, 1.0f, 3.0f };
	static const float curve2Values[curve2KeyCount] = { -10.0f, -12.0f, -10.0f };
	static const FMVector2 curve2Intan[curve2KeyCount] = { FMVector2(0.0f, 0.0f), FMVector2(0.8f, -2.0f), FMVector2(2.7f, -2.0f) };
	static const FMVector2 curve2Outtan[curve2KeyCount] = { FMVector2(0.2f, 0.0f), FMVector2(1.8f, -1.0f), FMVector2(3.5f, 2.0f) };
	FCDAnimationCurve* c2 = channel->AddCurve();
	for (size_t i = 0; i < curve2KeyCount; ++i)
	{
		FCDAnimationKeyBezier* k = (FCDAnimationKeyBezier*) c2->AddKey(FUDaeInterpolation::BEZIER);
		k->input = curve2Keys[i]; k->output = curve2Values[i];
		k->inTangent = curve2Intan[i]; k->outTangent = curve2Outtan[i];
	}

	// Merge the curves
	FCDAnimationCurveList curves;
	curves.push_back(c1);
	curves.push_back(c2);
	FloatList defaultValues(2, 0.0f);
	FCDAnimationMultiCurve* multiCurve = FCDAnimationCurveTools::MergeCurves(curves, defaultValues);
	FailIf(multiCurve == NULL);

	// Verify the created multi-curve
	static const size_t multiCurveKeyCount = 4;
	static const float multiCurveKeys[multiCurveKeyCount] = { 0.0f, 1.0f, 2.0f, 3.0f };
	static const float multiCurveValues[2][multiCurveKeyCount] = { { 0.0f, -1.25025f, -2.0f, 0.0f }, { -10.0f, -12.0f, -4.14197f, -10.0f } };
	static const FMVector2 multiCurveIntans[2][multiCurveKeyCount] = { { FMVector2(-0.3f, 0.0f), FMVector2(0.6666f, -1.5860f), FMVector2(1.6f, -0.5f), FMVector2(2.8f, -1.0f) }, { FMVector2(0.0f, 0.0f), FMVector2(0.8f, -2.0f), FMVector2(1.6666f, -4.92637f), FMVector2(2.85f, -6.0f) } };
	static const FMVector2 multiCurveOuttans[2][multiCurveKeyCount] = { { FMVector2(0.25f, -2.0f), FMVector2(1.3333f, -0.91447f), FMVector2(2.5f, 3.0f), FMVector2(3.0f, 0.0f) }, { FMVector2(0.2f, 0.0f), FMVector2(1.4f, -6.5f), FMVector2(2.3333f, -3.35757f), FMVector2(3.5f, 2.0f) } };
	PassIf(multiCurve->GetDimension() == 2);
	PassIf(multiCurve->GetKeyCount() == multiCurveKeyCount);
	for (size_t i = 0; i < multiCurveKeyCount; ++i)
	{
		FCDAnimationMKeyBezier* bk = (FCDAnimationMKeyBezier*) multiCurve->GetKey(i);
		PassIf(IsEquivalent(bk->input, multiCurveKeys[i]));
		PassIf(bk->interpolation == FUDaeInterpolation::BEZIER);
		PassIf(bk->GetDimension() == 2);
		for (size_t d = 0; d < 2; ++d)
		{
			PassIf(IsEquivalent(bk->output[d], multiCurveValues[d][i]));
			PassIf(IsEquivalent(bk->inTangent[d], multiCurveIntans[d][i]));
			PassIf(IsEquivalent(bk->outTangent[d], multiCurveOuttans[d][i]));
		}
	}

	SAFE_RELEASE(c1);
	SAFE_RELEASE(c2);
	SAFE_RELEASE(multiCurve);

TESTSUITE_END
