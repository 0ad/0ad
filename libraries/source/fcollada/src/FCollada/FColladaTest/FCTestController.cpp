/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSkinController.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDLibrary.h"

TESTSUITE_START(FCDControllers)

TESTSUITE_TEST(0, ReduceInfluences)

	// None of the previous tests should be leaving dangling documents.
	PassIf(FCollada::GetTopDocumentCount() == 0);

	// Create the COLLADA skin controller.
	FUObjectRef<FCDocument> document = FCollada::NewTopDocument();
	FCDController* testController = document->GetControllerLibrary()->AddEntity();
	FCDSkinController* skinController = testController->CreateSkinController();

	// Add some named joints, so we don't need the FCDSceneNode objects.
	skinController->AddJoint("Test1");
	skinController->AddJoint("Test2");
	skinController->AddJoint("Test3");
	skinController->AddJoint("Test4");
	skinController->AddJoint("Test5");

	// Create one vertex with a bunch of influences.
	skinController->SetInfluenceCount(1);
	FCDSkinControllerVertex* vertex = skinController->GetVertexInfluence(0);
	vertex->AddPair(0, 0.20f);
	vertex->AddPair(1, 0.40f);
	vertex->AddPair(4, 0.10f);
	vertex->AddPair(2, 0.12f);
	vertex->AddPair(3, 0.18f);
	PassIf(vertex->GetPairCount() == 5);

	// A first influence reduction: reduce to 4.
	skinController->ReduceInfluences(4);
	PassIf(vertex->GetPairCount() == 4);

	// This ensures all the joints checked below are unique.
	// simple enough sum test: joint 4 should have been dropped.
	uint32 sum = 0;
	for (size_t i = 0; i < 4; ++i) sum += vertex->GetPair(i)->jointIndex;
	PassIf(sum == 6);
	for (size_t i = 0; i < 4; ++i)
	{
		switch (vertex->GetPair(i)->jointIndex)
		{
		case 0: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 0.22222f)); break;
		case 1: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 0.44444f)); break;
		case 2: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 0.13333f)); break;
		case 3: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 0.20000f)); break;
		default: FUFail("");
		}
	}

	// A second influence reduction: reduce anything below 0.21f.
	skinController->ReduceInfluences(4, 0.21f);
	PassIf(vertex->GetPairCount() == 2);
	sum = 0;
	for (size_t i = 0; i < 2; ++i) sum += vertex->GetPair(i)->jointIndex;
	PassIf(sum == 1);
	for (size_t i = 0; i < 2; ++i)
	{
		switch (vertex->GetPair(i)->jointIndex)
		{
		case 0: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 1.0f / 3.0f)); break;
		case 1: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 2.0f / 3.0f)); break;
		default: FUFail("");
		}
	}

	// A third influence reduction: reduce to the most important joint.
	skinController->ReduceInfluences(1);
	PassIf(vertex->GetPairCount() == 1);
	PassIf(vertex->GetPair(0)->jointIndex == 1);
	PassIf(IsEquivalent(vertex->GetPair(0)->weight, 1.0f));

TESTSUITE_TEST(1, ReduceInfluences_LargestLast)

	// Create the COLLADA skin controller.
	FUObjectRef<FCDocument> document = FCollada::NewTopDocument();
	FCDController* testController = document->GetControllerLibrary()->AddEntity();
	FCDSkinController* skinController = testController->CreateSkinController();

	// Add some named joints, so we don't need the FCDSceneNode objects.
	skinController->AddJoint("Test1");
	skinController->AddJoint("Test2");
	skinController->AddJoint("Test3");
	skinController->AddJoint("Test4");
	skinController->AddJoint("Test5");

	// Create one vertex with a bunch of influences.
	skinController->SetInfluenceCount(1);
	FCDSkinControllerVertex* vertex = skinController->GetVertexInfluence(0);
	vertex->AddPair(0, 0.10f);
	vertex->AddPair(1, 0.18f);
	vertex->AddPair(4, 0.12f);
	vertex->AddPair(2, 0.20f);
	vertex->AddPair(3, 0.40f);
	PassIf(vertex->GetPairCount() == 5);

	// A first influence reduction: reduce to 4.
	skinController->ReduceInfluences(4);
	PassIf(vertex->GetPairCount() == 4);

	// This ensures all the joints checked below are unique.
	// simple enough sum test: joint 0 should have been dropped.
	uint32 sum = 0;
	for (size_t i = 0; i < 4; ++i) sum += vertex->GetPair(i)->jointIndex;
	PassIf(sum == 10);
	for (size_t i = 0; i < 4; ++i)
	{
		switch (vertex->GetPair(i)->jointIndex)
		{
		case 2: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 0.22222f)); break;
		case 3: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 0.44444f)); break;
		case 4: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 0.13333f)); break;
		case 1: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 0.20000f)); break;
		default: FUFail("");
		}
	}

	// A second influence reduction: reduce anything below 0.21f.
	skinController->ReduceInfluences(4, 0.21f);
	PassIf(vertex->GetPairCount() == 2);
	sum = 0;
	for (size_t i = 0; i < 2; ++i) sum += vertex->GetPair(i)->jointIndex;
	PassIf(sum == 5);
	for (size_t i = 0; i < 2; ++i)
	{
		switch (vertex->GetPair(i)->jointIndex)
		{
		case 2: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 1.0f / 3.0f)); break;
		case 3: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 2.0f / 3.0f)); break;
		default: FUFail("");
		}
	}

	// A third influence reduction: reduce to the most important joint.
	skinController->ReduceInfluences(1);
	PassIf(vertex->GetPairCount() == 1);
	PassIf(vertex->GetPair(0)->jointIndex == 3);
	PassIf(IsEquivalent(vertex->GetPair(0)->weight, 1.0f));

TESTSUITE_TEST(2, ReduceInfluence_Bulvan)

	// http://www.feelingsoftware.com/component/option,com_smf/Itemid,86/topic,407.0
	//
	// Pair1 - jointIndex = 0    weight = 0.1f
	// Pair2 - jointIndex = 1    weight = 0.1f
	// Pair3 - jointIndex = 2    weight = 0.3f
	// Pair4 - jointIndex = 3    weight = 0.3f
	// Pair5 - jointIndex = 4    weight = 0.2f

	// Create the COLLADA skin controller.
	FUObjectRef<FCDocument> document = FCollada::NewTopDocument();
	FCDController* testController = document->GetControllerLibrary()->AddEntity();
	FCDSkinController* skinController = testController->CreateSkinController();

	// Add some named joints, so we don't need the FCDSceneNode objects.
	skinController->AddJoint("Test1");
	skinController->AddJoint("Test2");
	skinController->AddJoint("Test3");
	skinController->AddJoint("Test4");
	skinController->AddJoint("Test5");

	// Create one vertex with a bunch of influences.
	skinController->SetInfluenceCount(1);
	FCDSkinControllerVertex* vertex = skinController->GetVertexInfluence(0);
	vertex->AddPair(0, 0.1f);
	vertex->AddPair(1, 0.1f);
	vertex->AddPair(2, 0.3f);
	vertex->AddPair(3, 0.3f);
	vertex->AddPair(4, 0.2f);
	PassIf(vertex->GetPairCount() == 5);

	// One influence reduction: reduce to 4.
	skinController->ReduceInfluences(4);
	PassIf(vertex->GetPairCount() == 4);

	uint32 sum = 0;
	for (size_t i = 0; i < 4; ++i) sum += vertex->GetPair(i)->jointIndex;
	PassIf(sum == 10 || sum == 9); // either 0 or 1 is dropped.
	for (size_t i = 0; i < 4; ++i)
	{
		switch (vertex->GetPair(i)->jointIndex)
		{
		case 0: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 0.11111f)); break;
		case 1: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 0.11111f)); break;
		case 2: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 0.33333f)); break;
		case 3: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 0.33333f)); break;
		case 4: PassIf(IsEquivalent(vertex->GetPair(i)->weight, 0.22222f)); break;
		default: FUFail("");
		}
	}
	
TESTSUITE_END
