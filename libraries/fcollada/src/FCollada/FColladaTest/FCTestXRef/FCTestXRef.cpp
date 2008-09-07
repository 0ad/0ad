/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDAsset.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDLight.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDExternalReferenceManager.h"
#include "FCDocument/FCDSceneNode.h"

TESTSUITE_START(FCTestXRef)

TESTSUITE_TEST(0, SubTests)
	RUN_TESTSUITE(FCTestXRefSimple);
	RUN_TESTSUITE(FCTestXRefTree);
	RUN_TESTSUITE(FCTestXRefAcyclic);

	// Make sure our XRef tests have not left dangling documents.
	PassIf(FCollada::GetTopDocumentCount() == 0);

TESTSUITE_END
