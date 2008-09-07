/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDGeometrySpline.h"
#include "FCDocument/FCDMorphController.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSkinController.h"
#include "FCTestExportImport.h"

static const float positionData[12] = { 0.0f, 0.0f, 3.0f, 5.0f, 0.0f, -2.0f, -3.0f, 4.0f, -2.0f, -3.0f, -4.0f, -2.0f };
static const float colorData[12] = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f };
static const float dummyData[10] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
static const uint32 positionIndices[12] = { 0, 1, 2, 0, 2, 3, 0, 3, 1, 3, 2, 1 };
static const uint32 colorIndices[12] = { 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2 };

static const float sampleBindPose1[16] = { 1.0f, 0.4f, 0.4f, 0.0f, 7.77f, 0.0f, 0.3f, 2.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
static const float sampleBindPose2[16] = { 0.3f, 0.0f, -0.3f, -21.0f, 0.96f, 0.0f, 2.0f, 2.5f, 0.0f, -5.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
static fm::string splineId, meshId, jointId1, jointId2;

namespace FCTestExportImport
{
	static const char* szTestName = "FCTestExportImportGeometry";

	bool FillGeometryLibrary(FULogFile& fileOut, FCDGeometryLibrary* library)
	{
		FCDGeometry* geometry = library->AddEntity();
		PassIf(geometry->GetType() == FCDEntity::GEOMETRY);
		FailIf(geometry->IsMesh());
		FailIf(geometry->IsSpline());
		meshId = geometry->GetDaeId();
		FailIf(meshId.empty());

		// Creates a mesh to export
		FCDGeometryMesh* mesh = geometry->CreateMesh();
		FailIf(geometry->IsSpline());
		PassIf(geometry->IsMesh());
		PassIf(geometry->GetMesh() == mesh);
		PassIf(geometry->GetSpline() == NULL);
		FillGeometryMesh(fileOut, mesh);

		// Create a spline to export
		geometry = library->AddEntity();
		geometry->CreateMesh();
		FCDGeometrySpline* spline = geometry->CreateSpline();
		PassIf(geometry->IsSpline());
		FailIf(geometry->IsMesh());
		PassIf(geometry->GetMesh() == NULL);
		PassIf(geometry->GetSpline() == spline);
		FillGeometrySpline(fileOut, spline);
		splineId = geometry->GetDaeId();
		FailIf(splineId.empty());
		return true;
	}

	bool FillGeometryMesh(FULogFile& fileOut, FCDGeometryMesh* mesh)
	{
		FCDGeometrySource* posSource = mesh->AddVertexSource();
		FailIf(posSource == NULL);
		posSource->SetName(FC("TestPositionSource"));
		posSource->SetSourceType(FUDaeGeometryInput::POSITION);
		posSource->SetData(FloatList(positionData, 12), 3);

		FCDGeometrySource* colorSource = mesh->AddSource();
		FailIf(colorSource == NULL);
		colorSource->SetName(FC("TestColorSource"));
		colorSource->SetSourceType(FUDaeGeometryInput::COLOR);
		colorSource->SetData(FloatList(colorData, 12), 4);

		FCDGeometrySource* dummySource = mesh->AddSource();
		FailIf(dummySource == NULL);
		dummySource->SetName(FC("TestDummySource"));
		dummySource->SetSourceType(FUDaeGeometryInput::EXTRA);
		dummySource->SetData(FloatList(dummyData, 10), 3);
		PassIf(FillExtraTree(fileOut, dummySource->GetExtra(), false));

		FCDGeometryPolygons* polys1 = mesh->AddPolygons();
		FailIf(polys1 == NULL || polys1->GetInputCount() != 1);
		FCDGeometryPolygons* polys2 = mesh->AddPolygons();
		FailIf(polys2 == NULL || polys2->GetInputCount() != 1);
		FCDGeometryPolygonsInput* pInput1 = polys1->AddInput(colorSource, 1);
		PassIf(polys1->GetInputCount() == 2);
		PassIf(pInput1 != NULL && pInput1->GetSource() == colorSource && pInput1->GetOffset() == 1);
		FCDGeometryPolygonsInput* pInput2 = polys2->AddInput(colorSource, 1);
		PassIf(polys2->GetInputCount() == 2);
		PassIf(pInput2 != NULL && pInput2->GetSource() == colorSource && pInput2->GetOffset() == 1);
		FCDGeometryPolygonsInput* pInput3 = polys1->AddInput(dummySource, 2);
		PassIf(pInput3 != NULL && pInput3->GetSource() == dummySource && pInput3->GetOffset() == 2);

		// Write some extra tree in there too.
		FillExtraTree(fileOut, polys1->GetExtra(), true);
		FillExtraTree(fileOut, polys2->GetExtra(), true);

		// Fill in some indices in order to form a tetrahedron
		polys1->AddFace(3); polys1->AddFace(3); polys1->AddFace(3); polys1->AddFace(3);
		FailIf(polys1->FindInput(posSource) == NULL);
		polys1->FindInput(posSource)->SetIndices(positionIndices, 12);
		FailIf(polys1->FindInput(posSource) == NULL);
		polys1->FindInput(colorSource)->SetIndices(colorIndices, 12);
		return true;
	}

	bool FillGeometrySpline(FULogFile& fileOut, FCDGeometrySpline* spline)
	{
		spline->SetType(FUDaeSplineType::BEZIER);
		FCDBezierSpline* splineData = (FCDBezierSpline*) spline->AddSpline();
		PassIf(splineData->IsType(FCDBezierSpline::GetClassType()));
		/*
		FCDCVs cvs;
		cvs.push_back(FMVector3(1.0f, 2.0f, 4.0f));
		cvs.push_back(FMVector3::XAxis);
		spline->SetCVs(cvs);
		PassIf(spline->GetCVCount() == 2);

		FCDKnots knots;
		knots.push_back(1.4);
		knots.push_back(7.4);
		spline->SetKnots(knots);
		PassIf(spline->GetKnotCount() == 2); */
		return true;
	}

	bool CheckGeometryLibrary(FULogFile& fileOut, FCDGeometryLibrary* library)
	{
		PassIf(library->GetEntityCount() == 2);

		// Find the one mesh and the one spline geometries.
		FCDGeometryMesh* mesh = NULL; FCDGeometrySpline* spline = NULL;
		for (size_t i = 0; i < 2; ++i)
		{
			FCDGeometry* g = library->GetEntity(i);
			if (g->IsMesh()) mesh = g->GetMesh();
			else if (g->IsSpline()) spline = g->GetSpline();
			else Fail;
		}
		FailIf(mesh == NULL || spline == NULL);

		CheckGeometryMesh(fileOut, mesh);
		CheckGeometrySpline(fileOut, spline);
		return true;
	}

	bool CheckGeometryMesh(FULogFile& fileOut, FCDGeometryMesh* mesh)
	{
		// Verify the mesh and its sources
		PassIf(mesh->GetSourceCount() == 3);
		FCDGeometrySource* posSource = NULL,* colorSource = NULL,* dummySource = NULL;
		for (size_t i = 0; i < 3; ++i)
		{
			FCDGeometrySource* source = mesh->GetSource(i);
			FailIf(source == NULL);
			switch (source->GetType())
			{
			case FUDaeGeometryInput::POSITION: posSource = source; PassIf(source->GetName() == FC("TestPositionSource")); break;
			case FUDaeGeometryInput::COLOR: colorSource = source; PassIf(source->GetName() == FC("TestColorSource")); break;
			case FUDaeGeometryInput::EXTRA: dummySource = source; PassIf(source->GetName() == FC("TestDummySource")); break;
			default: Fail; break;
			}
		}
		FailIf(posSource == NULL || colorSource == NULL || dummySource == NULL);
		PassIf(IsEquivalent(posSource->GetData(), posSource->GetDataCount(), positionData, 12));
		PassIf(posSource->GetStride() == 3);
		PassIf(IsEquivalent(colorSource->GetData(), colorSource->GetDataCount(), colorData, 12));
		PassIf(colorSource->GetStride() == 4);
		PassIf(IsEquivalent(dummySource->GetData(), dummySource->GetDataCount(), dummyData, 10));
		PassIf(dummySource->GetStride() == 3);
		PassIf(CheckExtraTree(fileOut, dummySource->GetExtra(), false));

		// Find the non-empty polygon set and verify that one of the polygon set is, in fact, empty.
		FCDGeometryPolygons* polys1 = NULL,* polysEmpty = NULL;
		for (size_t i = 0; i < mesh->GetPolygonsCount(); ++i)
		{
			FCDGeometryPolygons* p = mesh->GetPolygons(i);
			if (p->GetFaceCount() == 0) { PassIf(polysEmpty == NULL); polysEmpty = p; }
			else { PassIf(polys1 == NULL); polys1 = p; }

			CheckExtraTree(fileOut, p->GetExtra(), true);
		}
		PassIf(polys1 != NULL && polysEmpty != NULL);

		// Check that we have the wanted tetrahedron in the non-empty polygon set.
		PassIf(polys1->GetFaceCount() == 4);
		PassIf(polys1->GetHoleCount() == 0);
		PassIf(polys1->GetFaceVertexCount(0) == 3 && polys1->GetFaceVertexCount(1) == 3 && polys1->GetFaceVertexCount(2) == 3 && polys1->GetFaceVertexCount(3) == 3);
		FCDGeometryPolygonsInput* posInput = polys1->FindInput(posSource);
		FailIf(posInput == NULL || posInput->GetIndexCount() != 12);
		FCDGeometryPolygonsInput* colorInput = polys1->FindInput(colorSource);
		FailIf(colorInput == NULL || colorInput == posInput || colorInput->GetIndexCount() != 12);
		PassIf(IsEquivalent(posInput->GetIndices(), 12, positionIndices, 12));
		PassIf(IsEquivalent(colorInput->GetIndices(), 12, colorIndices, 12));
		return true;
	}

	bool CheckGeometrySpline(FULogFile& UNUSED(fileOut), FCDGeometrySpline* UNUSED(spline))
	{/*
		// Verify the spline and its data
		FailIf(spline->GetCVCount() != 2);
		FailIf(spline->GetKnotCount() != 2);
		PassIf(IsEquivalent(*spline->GetCV(0), FMVector3(1.0f, 2.0f, 4.0f)));
		PassIf(IsEquivalent(*spline->GetCV(1), FMVector3::XAxis));
		PassIf(IsEquivalent(spline->GetKnot(0), 1.4));
		PassIf(IsEquivalent(spline->GetKnot(1), 7.4));*/
		return true;
	}

	bool FillControllerLibrary(FULogFile& fileOut, FCDControllerLibrary* library)
	{
		FailIf(library == NULL);

		// Create a morph controller on the previously created spline.
		FCDController* morpher = library->AddEntity();
		PassIf(FillControllerMorph(fileOut, morpher->CreateMorphController()));

		// Create a skin controller on the previously created mesh.
		FCDController* skin = library->AddEntity();
		skin->SetNote(FS("A nicey skinny controller. "));
		PassIf(FillControllerSkin(fileOut, skin->CreateSkinController()));
		return true;
	}

	bool FillControllerMorph(FULogFile& fileOut, FCDMorphController* controller)
	{
		FailIf(controller == NULL);
		controller->SetMethod(FUDaeMorphMethod::RELATIVE);

		// Retrieve the base spline entity that will be morphed
		// (and for this test only: it'll be the morph targets)
		// Also retrieve the mesh, for similarity checks.
		FCDGeometry* spline = controller->GetDocument()->FindGeometry(splineId);
		FailIf(spline == NULL);
		FCDGeometry* mesh = controller->GetDocument()->FindGeometry(meshId);
		FailIf(mesh == NULL);
		FailIf(controller->IsSimilar(mesh));
		FailIf(controller->IsSimilar(spline));

		controller->SetBaseTarget(spline);
		PassIf(controller->IsSimilar(spline));
		FailIf(controller->IsSimilar(mesh));

		controller->AddTarget(spline, 0.3f);
		controller->AddTarget(spline, 0.6f);
		PassIf(controller->GetTargetCount() == 2);
		return true;
	}

	bool FillControllerSkin(FULogFile& fileOut, FCDSkinController* controller)
	{
		FailIf(controller == NULL);

		// Create two joints.
		FCDSceneNode* visualScene = controller->GetDocument()->GetVisualSceneLibrary()->AddEntity();
		PassIf(visualScene != NULL);
		FCDSceneNode* joint1 = visualScene->AddChildNode();
		PassIf(joint1 != NULL);
		FCDSceneNode* joint2 = joint1->AddChildNode();
		PassIf(joint2 != NULL);
		// In the standard course of operations, we would either load a file
		// with subId Info, or link it in when loading the instance.  Here we have no instance
		fm::string jid1("joint1");
		fm::string jid2("joint2");
		joint1->SetSubId(jid1);
		joint2->SetSubId(jid2);
		jointId1 = joint1->GetSubId();
		jointId2 = joint2->GetSubId();
		FailIf(jointId1.empty() || jointId2.empty());
		controller->AddJoint(joint1->GetSubId(), FMMatrix44::Identity);
		controller->AddJoint(joint2->GetSubId(), FMMatrix44(sampleBindPose1).Inverted());
		controller->SetBindShapeTransform(FMMatrix44(sampleBindPose2));
		PassIf(controller->GetJointCount() == 2);

		// Retrieve and assign the base target
		FCDGeometry* geometricTarget = controller->GetDocument()->FindGeometry(meshId);
		controller->SetTarget(geometricTarget);

		// Set some influences
		PassIf(controller->GetInfluenceCount() == 4);
		FCDSkinControllerVertex* influence = controller->GetVertexInfluence(0);
		FailIf(influence == NULL);
		influence->AddPair(0, 0.5f);
		influence->AddPair(1, 0.5f);

		influence = controller->GetVertexInfluence(3);
		FailIf(influence == NULL);
		influence->AddPair(1, 1.0f);

		influence = controller->GetVertexInfluence(2);
		FailIf(influence == NULL);
		influence->AddPair(0, 0.1f);
		return true;
	}

	bool CheckControllerLibrary(FULogFile& fileOut, FCDControllerLibrary* library)
	{
		FailIf(library == NULL);

		FCDController* morpher = NULL,* skin = NULL;
		for (size_t i = 0; i < library->GetEntityCount(); ++i)
		{
			FCDController* c = library->GetEntity(i);
			if (c->IsMorph()) { PassIf(morpher == NULL); morpher = c; }
			else if (c->IsSkin()) { PassIf(skin == NULL); skin = c; }
			else Fail;
		}
		PassIf(morpher != NULL && skin != NULL);

		// Check the morpher
		FailIf(morpher->IsSkin());
		PassIf(morpher->IsMorph());
		PassIf(morpher->GetMorphController() != NULL);
		PassIf(CheckControllerMorph(fileOut, morpher->GetMorphController()));

		// Check the skin
		PassIf(skin->GetSkinController() != NULL);
		//FailIf(skin->HasMorphController());
		FailIf(skin->IsMorph());
		PassIf(skin->GetNote() == FC("A nicey skinny controller. "));
		PassIf(CheckControllerSkin(fileOut, skin->GetSkinController()));
		return true;
	}

	bool CheckControllerMorph(FULogFile& fileOut, FCDMorphController* controller)
	{
		FailIf(controller == NULL);
		PassIf(controller->GetMethod() == FUDaeMorphMethod::RELATIVE);

		// Check that there are two targets, that the weights are correct, as well as the ids.
		PassIf(controller->GetTargetCount() == 2);
		PassIf(controller->GetBaseTarget() != NULL);
		PassIf(controller->GetBaseTarget()->GetDaeId() == splineId);

		FCDMorphTarget* target1 = controller->GetTarget(0);
		FailIf(target1 == NULL);
		FCDMorphTarget* target2 = controller->GetTarget(1);
		FailIf(target2 == NULL);
		PassIf(target1->GetGeometry() == controller->GetBaseTarget());
		PassIf(target2->GetGeometry() == controller->GetBaseTarget());
		PassIf(IsEquivalent(target1->GetWeight(), 0.6f) || IsEquivalent(target1->GetWeight(), 0.3f));
		PassIf(IsEquivalent(target2->GetWeight(), 0.6f) || IsEquivalent(target2->GetWeight(), 0.3f));
		FailIf(IsEquivalent(target1->GetWeight(), target2->GetWeight()));
		return true;
	}

	bool CheckControllerSkin(FULogFile& fileOut, FCDSkinController* controller)
	{
		FailIf(controller == NULL);

		// Check the base target's identity
		FailIf(controller->GetTarget() == NULL);
		PassIf(controller->GetTarget()->GetType() == FCDEntity::GEOMETRY);
		PassIf(controller->GetTarget()->GetDaeId() == meshId);

		// Retrieve the two joints and verify their ids/bind-pose.
		PassIf(controller->GetJointCount() == 2);
		const FMMatrix44* joint1 = NULL,* joint2 = NULL;
		for (size_t i = 0; i < 2; ++i)
		{
			FCDSkinControllerJoint* joint = controller->GetJoint(i);
			if (joint->GetId() == jointId1) { FailIf(joint1 != NULL); joint1 = &joint->GetBindPoseInverse(); }
			else if (joint->GetId() == jointId2) { FailIf(joint2 != NULL); joint2 = &joint->GetBindPoseInverse(); }
			else Fail;
		}
		FailIf(joint1 == NULL || joint2 == NULL);
		PassIf(IsEquivalent(*joint1, FMMatrix44::Identity));
		FMMatrix44 sbp = FMMatrix44(sampleBindPose1).Inverted();
		PassIf(IsEquivalent(*joint2, FMMatrix44(sampleBindPose1).Inverted()));

		// Verify the influences
		PassIf(controller->GetInfluenceCount() == 4);
		FCDSkinControllerVertex* influence = controller->GetVertexInfluence(0);
		FailIf(influence == NULL);
		PassIf(influence->GetPairCount() == 2);
		PassIf(IsEquivalent(influence->GetPair(0)->jointIndex, 0));
		PassIf(IsEquivalent(influence->GetPair(0)->weight, 0.5f));
		PassIf(IsEquivalent(influence->GetPair(1)->jointIndex, 1));
		PassIf(IsEquivalent(influence->GetPair(1)->weight, 0.5f));

		influence = controller->GetVertexInfluence(1);
		FailIf(influence == NULL);
		PassIf(influence->GetPairCount() == 0);

		influence = controller->GetVertexInfluence(2);
		FailIf(influence == NULL);
		PassIf(influence->GetPairCount() == 1);
		PassIf(IsEquivalent(influence->GetPair(0)->jointIndex, 0));
		PassIf(IsEquivalent(influence->GetPair(0)->weight, 1.0f)); // the weight should have been normalized.

		influence = controller->GetVertexInfluence(3);
		FailIf(influence == NULL);
		PassIf(influence->GetPairCount() == 1);
		PassIf(IsEquivalent(influence->GetPair(0)->jointIndex, 1));
		PassIf(IsEquivalent(influence->GetPair(0)->weight, 1.0f));
		return true;
	}
};

