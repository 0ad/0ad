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
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDExternalReferenceManager.h"
#include "FCDocument/FCDSceneNode.h"

TESTSUITE_START(FCTestXRefAcyclic)

TESTSUITE_TEST(0, Export)
	// None of the previous tests should be leaving dangling documents.
	PassIf(FCollada::GetTopDocumentCount() == 0);

	FCDocument* firstDoc = FCollada::NewTopDocument();
	FCDocument* secondDoc = FCollada::NewTopDocument();

	FCDGeometry* mesh1 = firstDoc->GetGeometryLibrary()->AddEntity();
	FCDGeometry* mesh2 = secondDoc->GetGeometryLibrary()->AddEntity();

	FCDSceneNode* node1 = firstDoc->AddVisualScene();
	node1 = node1->AddChildNode();
	FCDSceneNode* node2 = secondDoc->AddVisualScene();
	node2 = node2->AddChildNode();

	node2->AddInstance(mesh1);
	node1->AddInstance(mesh2);

	firstDoc->SetFileUrl(FS("XRefDoc1.dae"));
	FCollada::SaveDocument(secondDoc, FC("XRefDoc2.dae"));
	FCollada::SaveDocument(firstDoc, FC("XRefDoc1.dae"));

	SAFE_RELEASE(firstDoc);
	SAFE_RELEASE(secondDoc);

TESTSUITE_TEST(1, ImportOne)
	// None of the previous tests should be leaving dangling documents.
	PassIf(FCollada::GetTopDocumentCount() == 0);

	FUErrorSimpleHandler errorHandler;
	FCollada::SetDereferenceFlag(false);
	FCDocument* firstDoc = FCollada::NewTopDocument();
	PassIf(FCollada::LoadDocumentFromFile(firstDoc, FC("XRefDoc1.dae")));
	FCDocument* secondDoc = FCollada::NewTopDocument();
	PassIf(FCollada::LoadDocumentFromFile(secondDoc, FC("XRefDoc2.dae")));
	PassIf(errorHandler.IsSuccessful());

	FCDSceneNode* node1 = firstDoc->GetVisualSceneInstance();
	FCDSceneNode* node2 = secondDoc->GetVisualSceneInstance();
	FailIf(node1 == NULL || node2 == NULL || node1->GetChildrenCount() == 0 || node2->GetChildrenCount() == 0);
	node1 = node1->GetChild(0);
	node2 = node2->GetChild(0);
	FailIf(node1 == NULL || node2 == NULL);
	PassIf(node1->GetInstanceCount() == 1 && node2->GetInstanceCount() == 1);
	FCDEntityInstance* instance1 = node1->GetInstance(0);
	FCDEntityInstance* instance2 = node2->GetInstance(0);
	PassIf(instance1 != NULL && instance2 != NULL);
	PassIf(instance1->GetEntityType() == FCDEntity::GEOMETRY && instance2->GetEntityType() == FCDEntity::GEOMETRY);
	FCDGeometry* mesh1 = (FCDGeometry*) instance1->GetEntity();
	FCDGeometry* mesh2 = (FCDGeometry*) instance2->GetEntity();
	PassIf(mesh1 != NULL && mesh2 != NULL);
	PassIf(mesh1->GetDocument() == secondDoc);
	PassIf(mesh2->GetDocument() == firstDoc);

	SAFE_RELEASE(firstDoc);
	SAFE_RELEASE(secondDoc);	

TESTSUITE_END
