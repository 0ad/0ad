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

TESTSUITE_START(FCTestXRefSimple)

TESTSUITE_TEST(0, Export)
	PassIf(FCollada::GetTopDocumentCount() == 0);
	FCDocument* doc1 = FCollada::NewTopDocument();
	FCDocument* doc2 = FCollada::NewTopDocument();
	FCDSceneNode* sceneNode = doc1->AddVisualScene()->AddChildNode();
	FCDLight* light = doc2->GetLightLibrary()->AddEntity();
	light->SetLightType(FCDLight::AMBIENT);
	light->SetColor(FMVector3::XAxis);
	sceneNode->AddInstance(light);
	FCollada::SaveDocument(doc2, FC("XRefDoc2.dae"));
	FCollada::SaveDocument(doc1, FC("XRefDoc1.dae"));
	SAFE_RELEASE(doc1);
	SAFE_RELEASE(doc2);

TESTSUITE_TEST(1, ReimportSceneNode)
	FUErrorSimpleHandler errorHandler;
	PassIf(FCollada::GetTopDocumentCount() == 0);
	FCollada::SetDereferenceFlag(true);
	FCDocument* doc1 = FCollada::NewTopDocument();
	PassIf(FCollada::LoadDocumentFromFile(doc1, FC("XRefDoc1.dae")));
	FailIf(!errorHandler.IsSuccessful());
	FCDSceneNode* visualScene = doc1->GetVisualSceneInstance();
	FailIf(visualScene == NULL || visualScene->GetChildrenCount() == 0);
	visualScene = visualScene->GetChild(0);
	FailIf(visualScene == NULL || visualScene->GetInstanceCount() == 0);
	FCDEntityInstance* instance = visualScene->GetInstance(0);
	FailIf(instance == NULL);
	PassIf(instance->GetEntityType() == FCDEntity::LIGHT);
	FCDLight* light = (FCDLight*) instance->GetEntity();
	FailIf(light == NULL);
	PassIf(light->GetLightType() == FCDLight::AMBIENT);
	PassIf(IsEquivalent(light->GetColor(), FMVector3::XAxis));
	SAFE_RELEASE(doc1);

TESTSUITE_TEST(2, ReimportBothEasyOrder)
	PassIf(FCollada::GetTopDocumentCount() == 0);
	FCollada::SetDereferenceFlag(false);
	FCDocument* doc2 = FCollada::NewTopDocument();
	PassIf(FCollada::LoadDocumentFromFile(doc2, FC("XRefDoc2.dae")));
	FCDocument* doc1 = FCollada::NewTopDocument();
	PassIf(FCollada::LoadDocumentFromFile(doc1, FC("XRefDoc1.dae")));
	FUErrorSimpleHandler errorHandler;
		
	FailIf(!errorHandler.IsSuccessful());

	FCDSceneNode* visualScene = doc1->GetVisualSceneInstance();
	FailIf(visualScene == NULL || visualScene->GetChildrenCount() == 0);
	visualScene = visualScene->GetChild(0);
	FailIf(visualScene == NULL || visualScene->GetInstanceCount() == 0);
	FCDEntityInstance* instance = visualScene->GetInstance(0);
	FailIf(instance == NULL);
	PassIf(instance->GetEntityType() == FCDEntity::LIGHT);
	FCDLight* light = (FCDLight*) instance->GetEntity();
	FailIf(light == NULL);
	PassIf(light->GetLightType() == FCDLight::AMBIENT);
	PassIf(IsEquivalent(light->GetColor(), FMVector3::XAxis));
	SAFE_RELEASE(doc1);
	SAFE_RELEASE(doc2);

TESTSUITE_TEST(3, ReimportBothHardOrder)
	FUErrorSimpleHandler errorHandler;

	// Import the scene node document only, first.
	// And verify that the instance is created, but NULL.
	PassIf(FCollada::GetTopDocumentCount() == 0);
	FCollada::SetDereferenceFlag(false);
	FCDocument* doc1 = FCollada::NewTopDocument();
	PassIf(FCollada::LoadDocumentFromFile(doc1, FC("XRefDoc1.dae")));
	PassIf(errorHandler.IsSuccessful());
	FCDSceneNode* visualScene = doc1->GetVisualSceneInstance();
	FailIf(visualScene == NULL || visualScene->GetChildrenCount() == 0);
	visualScene = visualScene->GetChild(0);
	FailIf(visualScene == NULL || visualScene->GetInstanceCount() == 0);
	FCDEntityInstance* instance = visualScene->GetInstance(0);
	FailIf(instance == NULL);
	PassIf(instance->GetEntityType() == FCDEntity::LIGHT);
	FCDLight* light = (FCDLight*) instance->GetEntity();
	PassIf(light == NULL);

	// Now, import the second document and verify that the instance is now valid.
	FCDocument* doc2 = FCollada::NewTopDocument();
	PassIf(FCollada::LoadDocumentFromFile(doc2, FC("XRefDoc2.dae")));
	doc2->GetAsset()->SetUpAxis(FMVector3::XAxis);
	PassIf(errorHandler.IsSuccessful());
	light = (FCDLight*) instance->GetEntity();
	FailIf(light == NULL);
	PassIf(light->GetLightType() == FCDLight::AMBIENT);
	PassIf(IsEquivalent(light->GetColor(), FMVector3::XAxis));
	SAFE_RELEASE(doc1);

	// Verify that the second document is still valid.
	const FMVector3& upAxis = doc2->GetAsset()->GetUpAxis();
	PassIf(IsEquivalent(upAxis, FMVector3::XAxis));
	SAFE_RELEASE(doc2);

TESTSUITE_END
