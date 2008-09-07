/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDCamera.h"
#include "FCTestExportImport.h"

namespace FCTestExportImport
{
	static const char* szTestName = "FCTestExportImportCamera";

	bool FillCameraLibrary(FULogFile& fileOut, FCDCameraLibrary* library)
	{
		// Export a perspective camera.
		FCDCamera* persp = library->AddEntity();
		PassIf(library->GetEntityCount() == 1);
		PassIf(library->GetEntity(0) == persp);
		PassIf(persp->GetType() == FCDEntity::CAMERA);
		persp->SetProjectionType(FCDCamera::PERSPECTIVE);
		PassIf(persp->GetProjectionType() == FCDCamera::PERSPECTIVE);

		persp->SetAspectRatio(1.5f);
		persp->SetFarZ(128.0f);
		persp->SetNearZ(0.5f);
		persp->SetNote(FC("Testing Camera support."));
		persp->SetFovX(1.5f);
		PassIf(!persp->HasVerticalFov());

		// Export an orthographic camera.
		FCDCamera* ortho = library->AddEntity();
		PassIf(library->GetEntityCount() == 2);
		ortho->SetProjectionType(FCDCamera::ORTHOGRAPHIC);
		ortho->SetMagY(1.5f);
		ortho->SetAspectRatio(0.3f);
		ortho->SetNearZ(0.01f);
		ortho->SetFarZ(41.2f);
		PassIf(ortho->GetProjectionType() == FCDCamera::ORTHOGRAPHIC);
		return true;
	}

	bool CheckCameraLibrary(FULogFile& fileOut, FCDCameraLibrary* library)
	{
		PassIf(library->GetEntityCount() == 2);

		// Find the perspective and the orthographic camera.
		FCDCamera* persp = NULL,* ortho = NULL;
		for (size_t i = 0; i < 2; ++i)
		{
			FCDCamera* camera = library->GetEntity(i);
			PassIf(camera->GetType() == FCDEntity::CAMERA);
			switch (camera->GetProjectionType())
			{
			case FCDCamera::PERSPECTIVE: { FailIf(persp != NULL); persp = camera; } break;
			case FCDCamera::ORTHOGRAPHIC: { FailIf(ortho != NULL); ortho = camera; }  break;
			default: Fail;
			}
		}
		PassIf(persp != NULL && ortho != NULL);

		// Verify the perspective camera parameters
		PassIf(IsEquivalent(persp->GetAspectRatio(), 1.5f));
		PassIf(IsEquivalent(persp->GetFarZ(), 128.0f));
		PassIf(IsEquivalent(persp->GetNearZ(), 0.5f));
		PassIf(persp->GetNote() == FC("Testing Camera support."));
		PassIf(IsEquivalent(persp->GetFovX(), 1.5f));
		PassIf(!persp->HasVerticalFov());

		// Verify the orthographic camera parameters
		PassIf(IsEquivalent(ortho->GetAspectRatio(), 0.3f));
		PassIf(IsEquivalent(ortho->GetFarZ(), 41.2f));
		PassIf(IsEquivalent(ortho->GetNearZ(), 0.01f));
		PassIf(IsEquivalent(ortho->GetMagY(), 1.5f));
		PassIf(ortho->HasVerticalMag());
		FailIf(ortho->HasHorizontalMag());
		return true;
	}
};

