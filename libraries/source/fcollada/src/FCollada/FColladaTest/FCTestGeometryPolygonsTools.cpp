/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometryPolygonsTools.h"

TESTSUITE_START(FCDGeometryPolygonsTools)

TESTSUITE_TEST(0, FitIndexBuffers)
	FUErrorSimpleHandler errorHandler;

	// Import of the Eagle sample and retrieve its mesh.
	FUObjectRef<FCDocument> document = FCollada::NewTopDocument();
	PassIf(FCollada::LoadDocumentFromFile(document, FC("Eagle.DAE")));
	PassIf(errorHandler.IsSuccessful());
	FailIf(document->GetGeometryLibrary()->GetEntityCount() == 0);
	FCDGeometry* geometry = document->GetGeometryLibrary()->GetEntity(0);
	FailIf(geometry == NULL || !geometry->IsMesh());
	FCDGeometryMesh* mesh = geometry->GetMesh();
	FailIf(mesh == NULL);
	PassIf(mesh->GetPolygonsCount() == 1);
	size_t originalInputCount = mesh->GetPolygons(0)->GetInputCount();
	PassIf(originalInputCount > 0);
	FCDGeometryPolygonsTools::FitIndexBuffers(mesh, 90);

	// Verify the output.
	PassIf(mesh->GetPolygonsCount() == 3);
	size_t expectedCounts[3] = { 90, 90, 72 };
	for (size_t i = 0; i < 3; ++i)
	{
		FCDGeometryPolygons* p = mesh->GetPolygons(i);
		PassIf(p->GetFaceVertexCount() == expectedCounts[i]);
		size_t inputCount = p->GetInputCount();
		PassIf(originalInputCount == inputCount); // Make sure no inputs were lost.
		for (size_t k = 0; k < inputCount; ++k)
		{
			FCDGeometryPolygonsInput* input = p->GetInput(k);
			PassIf(input->GetIndexCount() == expectedCounts[i]);
		}
	}

TESTSUITE_TEST(1, GenerateUniqueIndices)
	FUErrorSimpleHandler errorHandler;

	// Import of the Eagle sample and retrieve its mesh.
	FUObjectRef<FCDocument> document = FCollada::NewTopDocument();
	PassIf(FCollada::LoadDocumentFromFile(document, FC("Eagle.DAE")));
	PassIf(errorHandler.IsSuccessful());

	FailIf(document->GetGeometryLibrary()->GetEntityCount() == 0);
	FCDGeometry* geometry = document->GetGeometryLibrary()->GetEntity(0);
	FailIf(geometry == NULL || !geometry->IsMesh());

	FCDGeometryMesh* mesh = geometry->GetMesh();
	FailIf(mesh == NULL);
	PassIf(mesh->GetPolygonsCount() == 1);

	FCDGeometryPolygons* polygons = mesh->GetPolygons(0);
	FailIf(polygons == NULL);

	PassIf(polygons->GetInputCount() == 2);
	uint32* vertexList = polygons->GetInput(0)->GetIndices();
	uint32* normalList = polygons->GetInput(1)->GetIndices();
	size_t vertexIndexCount = polygons->GetInput(0)->GetIndexCount();
	size_t normalIndexCount = polygons->GetInput(1)->GetIndexCount();
	FailIf(vertexIndexCount != normalIndexCount);

	// pass if there's at least one different index
	bool found = false;
	for (size_t i = 0; i < normalIndexCount; i++)
	{
		if (vertexList[i] != normalList[i])
		{
			found = true;
			break;
		}
	}
	PassIf(found);

	FCDGeometryIndexTranslationMap translationMap;
	FCDGeometryPolygonsTools::GenerateUniqueIndices(mesh, NULL, &translationMap);

	PassIf(polygons->GetInputCount() == 2);
	uint32* newVertexList = polygons->GetInput(0)->GetIndices();
	uint32* newNormalList = polygons->GetInput(1)->GetIndices();
	size_t newVertexIndexCount = polygons->GetInput(0)->GetIndexCount();
	size_t newNormalIndexCount = polygons->GetInput(1)->GetIndexCount();
	PassIf(newVertexIndexCount == newNormalIndexCount);
	PassIf(newVertexList == newNormalList);

TESTSUITE_END
