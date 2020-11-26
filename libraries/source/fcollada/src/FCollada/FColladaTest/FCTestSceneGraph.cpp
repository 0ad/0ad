/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSceneNodeIterator.h"

TESTSUITE_START(FCDSceneNode)

TESTSUITE_TEST(0, Iterator)
	
	// None of the previous tests should be leaving dangling documents.
	PassIf(FCollada::GetTopDocumentCount() == 0);

	FUObjectRef<FCDocument> doc = FCollada::NewTopDocument();
	FCDSceneNode* top = doc->AddVisualScene();
	FCDSceneNode* second = top->AddChildNode();
	FCDSceneNode* third = doc->AddVisualScene();
	second->AddInstance(third);

	// Check the standard iterator, in breadth-first mode
	FCDSceneNodeIterator it1(top, FCDSceneNodeIterator::BREADTH_FIRST);
	PassIf(!it1.IsDone());
	PassIf((*it1) == top);
	++it1;
	PassIf(!it1.IsDone());
	PassIf((*it1) == second);
	++it1;
	PassIf(!it1.IsDone());
	PassIf(it1.GetNode() == third);
	++it1;
	PassIf(it1.IsDone());

	// Check the const iterator, in breadth-first mode
	FCDSceneNodeConstIterator it2(top, FCDSceneNodeConstIterator::BREADTH_FIRST);
	PassIf(!it2.IsDone());
	PassIf((*it2) == top);
	++it2;
	PassIf(!it2.IsDone());
	PassIf((*it2) == second);
	++it2;
	PassIf(!it2.IsDone());
	PassIf(it2.GetNode() == third);
	++it2;
	PassIf(it2.IsDone());

	// Check the standard iterator, in depth-first mode
	FCDSceneNodeIterator it3(top, FCDSceneNodeIterator::DEPTH_FIRST_POSTORDER);
	PassIf(!it3.IsDone());
	PassIf((*it3) == third);
	++it3;
	PassIf(!it3.IsDone());
	PassIf((*it3) == second);
	++it3;
	PassIf(!it3.IsDone());
	PassIf(it3.GetNode() == top);
	++it3;
	PassIf(it3.IsDone());

	// Check the const iterator, in depth-first mode
	FCDSceneNodeConstIterator it4(top, FCDSceneNodeConstIterator::DEPTH_FIRST_POSTORDER);
	PassIf(!it4.IsDone());
	PassIf((*it4) == third);
	++it4;
	PassIf(!it4.IsDone());
	PassIf((*it4) == second);
	++it4;
	PassIf(!it4.IsDone());
	PassIf(it4.GetNode() == top);
	++it4;
	PassIf(it4.IsDone());

TESTSUITE_END

