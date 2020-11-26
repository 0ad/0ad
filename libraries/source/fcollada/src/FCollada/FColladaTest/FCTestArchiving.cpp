/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDLight.h"
#include "FUtils/FUFile.h"

TESTSUITE_START(FColladaArchiving)

TESTSUITE_TEST(0, FileArchiving)
	FUErrorSimpleHandler errorHandler;

	// Create a simple document with a sole light inside that we can use to verify I/O.
	FUObjectRef<FCDocument> document = FCollada::NewTopDocument();
	FCDLight* light = document->GetLightLibrary()->AddEntity();
	light->SetLightType(FCDLight::POINT);
	light->SetName(FC("ATestLight"));
	light->SetIntensity(0.4f);

	// Save and load this document.
	FCollada::SaveDocument(document, FC("./TestOut.dae"));
	FUObjectRef<FCDocument> document2 = FCollada::NewTopDocument();
	PassIf(FCollada::LoadDocumentFromFile(document2, FC("./TestOut.dae")));
	PassIf(errorHandler.IsSuccessful());
	PassIf(document2->GetLightLibrary()->GetEntityCount() == 1);
	FCDLight* light2 = document2->GetLightLibrary()->GetEntity(0);
	PassIf(IsEquivalent(light2->GetName(), light->GetName()));
	PassIf(light2->GetLightType() == light->GetLightType());
	PassIf(IsEquivalent(light->GetIntensity(), light->GetIntensity()));

	// Read in this document using a FUFile and manually load it in FCollada.
	FUFile file1(FC("./TestOut.dae"), FUFile::READ);
	size_t dataLength = file1.GetLength();
	uint8* buffer = new uint8[dataLength];
	PassIf(file1.Read(buffer, dataLength));
	file1.Close();
	FUObjectRef<FCDocument> document3 = FCollada::NewTopDocument();
	FCollada::LoadDocumentFromMemory(FC("./TestOut.dae"), document3, buffer, dataLength);
	PassIf(errorHandler.IsSuccessful());
	PassIf(document3->GetLightLibrary()->GetEntityCount() == 1);
	FCDLight* light3 = document3->GetLightLibrary()->GetEntity(0);
	PassIf(IsEquivalent(light3->GetName(), light->GetName()));
	PassIf(light3->GetLightType() == light->GetLightType());
	PassIf(IsEquivalent(light->GetIntensity(), light->GetIntensity()));

TESTSUITE_END
