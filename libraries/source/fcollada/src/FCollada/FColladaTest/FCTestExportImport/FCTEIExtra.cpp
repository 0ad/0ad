/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDExtra.h"
#include "FCTestExportImport.h"

namespace FCTestExportImport
{
	static const char* szTestName = "FCTestExportImportExtra";

	bool FillExtraTree(FULogFile& fileOut, FCDExtra* extra, bool hasTypes)
	{
		FailIf(extra == NULL);

		// Add a test technique.
		PassIf(extra->GetDefaultType()->GetTechniqueCount() == 0);
		FCDETechnique* technique1 = extra->GetDefaultType()->AddTechnique("FCTEI_TestProfile");
		FCDETechnique* technique2 = extra->GetDefaultType()->AddTechnique("FCTEI_TestProfile");
		FailIf(technique1 == NULL);
		FailIf(technique2 == NULL);
		PassIf(technique1 == technique2);
		PassIf(extra->GetDefaultType()->GetTechniqueCount() == 1);

		// Add a parent parameter to the technique and two subsequent parameters with the same name.
		FCDENode* parameterTree = technique1->AddChildNode();
		parameterTree->SetName("MainParameterTree");
		FCDENode* firstParameter = parameterTree->AddChildNode();
		firstParameter->SetName("SomeParameter");
		firstParameter->SetContent(FS("Test_SomeParameter"));
		firstParameter->AddAttribute("Guts", 0);
		FCDENode* secondParameter = parameterTree->AddChildNode();
		secondParameter->SetName("SomeParameter");
		secondParameter->AddAttribute("Guts", 3);
		secondParameter->SetContent(FS("Test_ThatParameter!"));
		PassIf(parameterTree->GetChildNodeCount() == 2);

		// Add some attributes to the parameter tree
		parameterTree->AddAttribute("Vicious", FC("Squirrel"));
		parameterTree->AddAttribute("Gross", 1002);

		if (hasTypes)
		{
			// Add a second extra type
			// Empty named-types should be supported.
			FCDEType* secondType = extra->AddType("verificator");
			PassIf(secondType != NULL);
			PassIf(secondType != extra->GetDefaultType());
			PassIf(secondType->GetTechniqueCount() == 0);
		}
		return true;
	}

	bool CheckExtraTree(FULogFile& fileOut, FCDExtra* extra, bool hasTypes)
	{
		FailIf(extra == NULL);

		// Find and verify the one technique
		FailIf(extra->GetDefaultType()->GetTechniqueCount() < 1); // note that FCDLight adds some <extra> elements of its own.
		FCDETechnique* technique = extra->GetDefaultType()->FindTechnique("FCTEI_TestProfile");
		FailIf(technique == NULL);

		// Find and verify the base parameter tree node
		FailIf(technique->GetChildNodeCount() != 1);
		FCDENode* baseNode = technique->GetChildNode(0);
		PassIf(baseNode != NULL);
		PassIf(extra->GetDefaultType()->FindRootNode("MainParameterTree") == baseNode);

		// Verify the base node attributes
		PassIf(baseNode->GetAttributeCount() == 2);
		FCDEAttribute* a1 = baseNode->FindAttribute("Vicious");
		FCDEAttribute* a2 = baseNode->FindAttribute("Gross");
		FailIf(a1 == NULL);
		FailIf(a2 == NULL);
		FailIf(a1 == a2);
		PassIf(IsEquivalent(a1->GetValue(), FC("Squirrel")));
		PassIf(IsEquivalent(FUStringConversion::ToUInt32(a2->GetValue()), (uint32)1002));

		// Identify the base node leaves
		PassIf(baseNode->GetChildNodeCount() == 2);
		FCDENode* leaf0 = NULL,* leaf3 = NULL;
		for (size_t i = 0; i < 2; ++i)
		{
			FCDENode* leaf = baseNode->GetChildNode(i);
			PassIf(IsEquivalent(leaf->GetName(), "SomeParameter"));
			FCDEAttribute* guts = leaf->FindAttribute("Guts");
			FailIf(guts == NULL || guts->GetValue().empty());
			uint32 gutsIndex = FUStringConversion::ToUInt32(guts->GetValue());
			if (gutsIndex == 0) { FailIf(leaf0 != NULL); leaf0 = leaf; }
			else if (gutsIndex == 3) { FailIf(leaf3 != NULL); leaf3 = leaf; }
			else Fail;
		}
		FailIf(leaf0 == NULL || leaf3 == NULL);

		// Verify the base node leaves
		PassIf(leaf0->GetChildNodeCount() == 0);
		PassIf(leaf3->GetChildNodeCount() == 0);
		PassIf(leaf0->GetAttributeCount() == 1);
		PassIf(leaf3->GetAttributeCount() == 1);
		PassIf(IsEquivalent(leaf0->GetContent(), FC("Test_SomeParameter")));
		PassIf(IsEquivalent(leaf3->GetContent(), FS("Test_ThatParameter!")));

		if (hasTypes)
		{
			// Verify the second extra type
			// Empty named-types should be imported without complaints or merging.
			FCDEType* secondType = extra->FindType("verificator");
			PassIf(secondType != NULL);
			PassIf(secondType != extra->GetDefaultType());
			PassIf(secondType->GetTechniqueCount() == 0);
		}
		return true;
	}
}

