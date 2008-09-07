/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDEmitter.h"
#include "FCDocument/FCDEmitterInstance.h"
#include "FCDocument/FCDForceField.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCTestExportImport.h"

namespace FCTestExportImport
{
	static const char* szTestName = "FCTestExportImportEmitter";

	bool FillForceFieldLibrary(FULogFile& fileOut, FCDForceFieldLibrary* library)
	{
		// Export a simple force field.
		FCDForceField* field = library->AddEntity();
		field->SetName(FC("SomeField"));
		PassIf(field != NULL);


		return true;
	}

	bool FillEmitterLibrary(FULogFile& fileOut, FCDEmitterLibrary* library)
	{
		// Export a first emitter.
		FCDEmitter* emitter1 = library->AddEntity();
		PassIf(emitter1 != NULL);
		PassIf(library->GetEntityCount() == 1);
		PassIf(library->GetEntity(0) == emitter1);
		PassIf(emitter1->GetType() == FCDEntity::EMITTER);


		// Export a second emitter.
		FCDEmitter* emitter2 = library->AddEntity();
		PassIf(library->GetEntityCount() == 2);
		PassIf(emitter2 != NULL);


		return true;
	}

	bool FillEmitterInstance(FULogFile& fileOut, FCDEmitterInstance* instance)
	{
		PassIf(instance != NULL);


		return true;
	}

	bool CheckForceFieldLibrary(FULogFile& fileOut, FCDForceFieldLibrary* library)
	{
		PassIf(library->GetEntityCount() == 1);

		FCDForceField* field = library->GetEntity(0);
		PassIf(IsEquivalent(field->GetName(), FC("SomeField")));


		return true;
	}

	bool CheckEmitterLibrary(FULogFile& fileOut, FCDEmitterLibrary* library)
	{
		return true;
		PassIf(library->GetEntityCount() == 2);


		return true;
	}

	bool CheckEmitterInstance(FULogFile& fileOut, FCDEmitterInstance* instance)
	{
		PassIf(instance != NULL);


		return true;
	}
};

