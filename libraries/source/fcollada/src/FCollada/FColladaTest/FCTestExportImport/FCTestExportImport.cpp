/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDCamera.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDGeometrySpline.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDTransform.h"
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEffectTools.h"
#include "FCDocument/FCDEffectStandard.h"
#include "FCDocument/FCDEffectParameter.h"

#include "FCTestExportImport.h"
using namespace FCTestExportImport;

static fm::string sceneNode1Id, sceneNode2Id, sceneNode3Id;

// Test import of a code-generated scene, with library entities.
// Does the export, re-import and validates that the information is intact.
TESTSUITE_START(FCDExportReimport)

TESTSUITE_TEST(0, Export)
	// Write out a simple document with three visual scenes
	FCDocument* doc = FCollada::NewTopDocument();
	FCDSceneNode* sceneNode1 = doc->AddVisualScene();
	sceneNode1->SetName(FC("Scene1"));
	FCDSceneNode* sceneNode2 = doc->AddVisualScene();
	sceneNode2->SetName(FC("Scene2"));
	FCDSceneNode* sceneNode3 = doc->AddVisualScene();
	sceneNode3->SetName(FC("Scene3"));

	// Fill in the other libraries
	PassIf(FillLayers(fileOut, doc));
	PassIf(FillImageLibrary(fileOut, doc->GetImageLibrary()));
	PassIf(FillCameraLibrary(fileOut, doc->GetCameraLibrary()));
	PassIf(FillLightLibrary(fileOut, doc->GetLightLibrary()));
	PassIf(FillForceFieldLibrary(fileOut, doc->GetForceFieldLibrary()));
	PassIf(FillEmitterLibrary(fileOut, doc->GetEmitterLibrary())); // must occur after FillForceFieldLibrary;
	PassIf(FillGeometryLibrary(fileOut, doc->GetGeometryLibrary()));
	PassIf(FillControllerLibrary(fileOut, doc->GetControllerLibrary())); // must occur after FillGeometryLibrary;
	PassIf(FillMaterialLibrary(fileOut, doc->GetMaterialLibrary())); // must occur after FillImageLibrary;
	PassIf(FillVisualScene(fileOut, sceneNode2)); // must occur after lights, cameras, geometries and controllers;
	PassIf(FillPhysics(fileOut, doc)); // must occur after the visual scene is filled in;
	PassIf(FillAnimationLibrary(fileOut, doc->GetAnimationLibrary())); // must occur last;

	// Add a bit of extra data to the FCDocument to verify that this feature works too.
	FCDEType* type = doc->GetExtra()->AddType("TOTO");
	FCDETechnique* technique = type->AddTechnique("TOTO_TECHNIQUE");
	technique->AddParameter("AParameter", FC("AValue"));

	// Write out the document.
	FCollada::SaveDocument(doc, FC("TestOut.dae"));

	// Check the dae id that were automatically generated: they should be unique
	FailIf(sceneNode1->GetDaeId() == sceneNode2->GetDaeId());
	FailIf(sceneNode1->GetDaeId() == sceneNode3->GetDaeId());
	FailIf(sceneNode2->GetDaeId() == sceneNode3->GetDaeId());
	sceneNode1Id = sceneNode1->GetDaeId();
	sceneNode2Id = sceneNode2->GetDaeId();
	sceneNode3Id = sceneNode3->GetDaeId();
	SAFE_RELEASE(doc);

TESTSUITE_TEST(1, Reimport)
	// Import back this document
	FUErrorSimpleHandler errorHandler;
	FCDocument* idoc = FCollada::NewTopDocument();
	PassIf(FCollada::LoadDocumentFromFile(idoc, FC("TestOut.dae")));
	PassIf(idoc != NULL);
	
#ifdef _WIN32
	OutputDebugStringA(errorHandler.GetErrorString());
#endif
	PassIf(errorHandler.IsSuccessful());

	// Verify that all the data we pushed is still available
	// Note that visual scenes may be added by other tests: such as for joints.
	FCDVisualSceneNodeLibrary* vsl = idoc->GetVisualSceneLibrary();
	PassIf(vsl->GetEntityCount() >= 3);

	// Verify that the visual scene ids are unique.
	for (size_t i = 0; i < vsl->GetEntityCount(); ++i)
	{
		FCDSceneNode* inode = vsl->GetEntity(i);
		for (size_t j = 0; j < i; ++j)
		{
			FCDSceneNode* jnode = vsl->GetEntity(j);
			FailIf(inode->GetDaeId() == jnode->GetDaeId());
		}
	}

	// Verify that the three wanted visual scene ids exist and find the one we fill in.
	bool found1 = false, found3 = false;
	FCDSceneNode* found2 = NULL;
	for (size_t i = 0; i < vsl->GetEntityCount(); ++i)
	{
		FCDSceneNode* inode = vsl->GetEntity(i);
		if (inode->GetDaeId() == sceneNode1Id)
		{
			FailIf(found1);
			PassIf(inode->GetName() == FC("Scene1"));
			found1 = true;
		}
		else if (inode->GetDaeId() == sceneNode2Id)
		{
			FailIf(found2 != NULL);
			PassIf(inode->GetName() == FC("Scene2")); 
			found2 = inode;
		}
		else if (inode->GetDaeId() == sceneNode3Id)
		{
			FailIf(found3);
			PassIf(inode->GetName() == FC("Scene3"));
			found3 = true;
		}
	}
	PassIf(found2 != NULL);

	// Compare all these re-imported library contents
	PassIf(CheckLayers(fileOut, idoc));
	PassIf(CheckVisualScene(fileOut, found2));
	PassIf(CheckImageLibrary(fileOut, idoc->GetImageLibrary()));
	PassIf(CheckCameraLibrary(fileOut, idoc->GetCameraLibrary()));
	PassIf(CheckEmitterLibrary(fileOut, idoc->GetEmitterLibrary()));
	PassIf(CheckForceFieldLibrary(fileOut, idoc->GetForceFieldLibrary()));
	PassIf(CheckLightLibrary(fileOut, idoc->GetLightLibrary()));
	PassIf(CheckGeometryLibrary(fileOut, idoc->GetGeometryLibrary()));
	PassIf(CheckControllerLibrary(fileOut, idoc->GetControllerLibrary()));
	PassIf(CheckMaterialLibrary(fileOut, idoc->GetMaterialLibrary()));
	PassIf(CheckAnimationLibrary(fileOut, idoc->GetAnimationLibrary()));
	PassIf(CheckPhysics(fileOut, idoc));

	// Check that the document extra data is available and intact.
	FCDEType* type = idoc->GetExtra()->FindType("TOTO");
	FailIf(type == NULL);
	FCDETechnique* technique = type->FindTechnique("TOTO_TECHNIQUE");
	FailIf(technique == NULL);
	FCDENode* extraNode = technique->FindParameter("AParameter");
	PassIf(extraNode != NULL);
	PassIf(IsEquivalent(extraNode->GetContent(), FC("AValue"))); 
	
	SAFE_RELEASE(idoc);

TESTSUITE_TEST(2, paramImport)

	// Import this document.
	FUErrorSimpleHandler errorHandler;
	FUObjectRef<FCDocument> idoc = FCollada::NewTopDocument();
	PassIf(FCollada::LoadDocumentFromFile(idoc, FC("TestSphere.dae")));
#ifdef _WIN32
	OutputDebugStringA(errorHandler.GetErrorString());
#endif
	PassIf(errorHandler.IsSuccessful());

	// Verify the data.
	FCDEffectLibrary* effectLibrary = idoc->GetEffectLibrary();
	FCDEffect* effect = effectLibrary->GetEntity(0);
	FCDEffectProfile* profile = effect->GetProfile(0);
	FCDEffectStandard* effectStandard = (FCDEffectStandard*) profile;

	// Verify a sample of the regular data.
	FCDEffectParameterColor4* specular = effectStandard->GetSpecularColorParam();
	PassIf(IsEquivalent(specular->GetValue(), FMVector4(0.9f, 0.9f, 0.9f, 1.0f)));
	FCDEffectParameterFloat* indexOfRefraction = effectStandard->GetIndexOfRefractionParam();
	PassIf(IsEquivalent(indexOfRefraction->GetValue(), 1.0f));

	// Verify the referenced data one by one...
	FCDEffectParameterColor4* ambient = effectStandard->GetAmbientColorParam();
	PassIf(IsEquivalent("myAmbient", ambient->GetReference()));
	PassIf(ambient->IsReferencer());
	FCDEffectParameterFloat* shininess = effectStandard->GetShininessParam();
	PassIf(IsEquivalent("myShininess", shininess->GetReference()));
	PassIf(shininess->IsReferencer());
	FCDEffectParameterColor4* reflective = effectStandard->GetReflectivityColorParam();
	PassIf(IsEquivalent("myReflective", reflective->GetReference()));
	PassIf(reflective->IsReferencer());

	// Verify default values
	PassIf(IsEquivalent(FMVector4::AlphaOne, ambient->GetValue()));
	PassIf(IsEquivalent(FMVector4::One, reflective->GetValue()));
	PassIf(IsEquivalent(20.0f, shininess->GetValue()));

	// Verify the values in the param's list
	FCDEffectParameterFloat* profileShininess = (FCDEffectParameterFloat*) FCDEffectTools::FindEffectParameterByReference(profile, "myShininess", true);
	PassIf(IsEquivalent(profileShininess->GetValue(), 10.0f));
	PassIf(profileShininess->IsGenerator());
	FCDEffectParameterFloat3* profileAmbient = (FCDEffectParameterFloat3*) FCDEffectTools::FindEffectParameterByReference(profile, "myAmbient", true);
	PassIf(IsEquivalent(profileAmbient->GetValue()->x, 0.59f));
	PassIf(IsEquivalent(profileAmbient->GetValue()->y, 0.59f));
	PassIf(IsEquivalent(profileAmbient->GetValue()->z, 0.59f));
	PassIf(profileAmbient->IsGenerator());
	FCDEffectParameterColor4* effectReflectivityRef = (FCDEffectParameterColor4*) FCDEffectTools::FindEffectParameterByReference(effect, "myReflective", true);
	FCDEffectParameterColor4* effectReflectivitySem = (FCDEffectParameterColor4*) FCDEffectTools::FindEffectParameterBySemantic(effect, "REFLECTIVITY", true);
	PassIf(effectReflectivityRef == effectReflectivitySem);
	PassIf(IsEquivalent(effectReflectivityRef->GetValue(), FMVector4(0.5f, 0.25f, 0.125f, 1.0f)));
	PassIf(effectReflectivityRef->IsGenerator());

	// Verify the modifier that's in the material instance parameter's list
	FCDMaterialLibrary* materialLibrary = idoc->GetMaterialLibrary();
	FCDMaterial* material = materialLibrary->GetEntity(0);

	FCDEffectParameterFloat* materialShininess = (FCDEffectParameterFloat*) FCDEffectTools::FindEffectParameterByReference(material, "myShininess", true);
	PassIf(IsEquivalent(materialShininess->GetValue(), 0.2f));
	PassIf(materialShininess->IsModifier());

	// Verify the animator that's in the geometry instance parameter's list
	FCDVisualSceneNodeLibrary* visualScene = idoc->GetVisualSceneLibrary();
	PassIf(IsEquivalent((uint32)1, (uint32)visualScene->GetEntityCount()));
	FCDSceneNode* sceneNode = visualScene->GetEntity(0);
	FCDSceneNode* childNode = sceneNode->GetChild(0);
	FCDEntityInstance* instance = childNode->GetInstance(0);
	FCDGeometryInstance* geometryInstance = (FCDGeometryInstance*) instance;

	FCDEffectParameterFloat* instanceShininess = (FCDEffectParameterFloat*) FCDEffectTools::FindEffectParameterBySemantic(geometryInstance, "SHININESS");
	PassIf(instanceShininess != NULL);
	PassIf(IsEquivalent(instanceShininess->GetType(), FCDEffectParameter::FLOAT));
	PassIf(instanceShininess == FCDEffectTools::FindEffectParameterByReference(geometryInstance, "myShininessAnimated"));
	PassIf(instanceShininess->IsAnimator());

	//Write it out again.
	FCollada::SaveDocument(idoc, FC("TestSphereOut.dae"));

#ifdef _WIN32
	OutputDebugStringA(errorHandler.GetErrorString());
#endif
	PassIf(errorHandler.IsSuccessful());

	SAFE_RELEASE(idoc);

	FUObjectRef<FCDocument> idoc2 = FCollada::NewTopDocument();
	PassIf(FCollada::LoadDocumentFromFile(idoc2, FC("TestSphereOut.dae")));
#ifdef _WIN32
	OutputDebugStringA(errorHandler.GetErrorString());
#endif
	PassIf(errorHandler.IsSuccessful());

	// Verify the data.
	FCDEffectLibrary* effectLibrary2 = idoc2->GetEffectLibrary();
	FCDEffect* effect2 = effectLibrary2->GetEntity(0);
	FCDEffectProfile* profile2 = effect2->GetProfile(0);
	FCDEffectStandard* effectStandard2 = (FCDEffectStandard*) profile2;

	// Verify a sample of the regular data.
	FCDEffectParameterColor4* specular2 = effectStandard2->GetSpecularColorParam();
	PassIf(IsEquivalent(specular2->GetValue(), FMVector4(0.9f, 0.9f, 0.9f, 1.0f)));
	FCDEffectParameterFloat* indexOfRefraction2 = effectStandard2->GetIndexOfRefractionParam();
	PassIf(IsEquivalent(indexOfRefraction2->GetValue(), 1.0f));

	// Verify the referenced data one by one...
	FCDEffectParameterColor4* ambient2 = effectStandard2->GetAmbientColorParam();
	PassIf(IsEquivalent("myAmbient", ambient2->GetReference()));
	PassIf(ambient2->IsReferencer());
	FCDEffectParameterFloat* shininess2 = effectStandard2->GetShininessParam();
	PassIf(IsEquivalent("myShininess", shininess2->GetReference()));
	PassIf(shininess2->IsReferencer());
	FCDEffectParameterColor4* reflective2 = effectStandard2->GetReflectivityColorParam();
	PassIf(IsEquivalent("myReflective", reflective2->GetReference()));
	PassIf(reflective2->IsReferencer());

	// Verify default values
	PassIf(IsEquivalent(FMVector4::AlphaOne, ambient2->GetValue()));
	PassIf(IsEquivalent(FMVector4::One, reflective2->GetValue()));
	PassIf(IsEquivalent(20.0f, shininess2->GetValue()));

	// Verify the values in the param's list
	FCDEffectParameterFloat* profileShininess2 = (FCDEffectParameterFloat*) FCDEffectTools::FindEffectParameterByReference(profile2, "myShininess", true);
	PassIf(IsEquivalent(profileShininess2->GetValue(), 10.0f));
	PassIf(profileShininess2->IsGenerator());
	FCDEffectParameterFloat3* profileAmbient2 = (FCDEffectParameterFloat3*) FCDEffectTools::FindEffectParameterByReference(profile2, "myAmbient", true);
	PassIf(IsEquivalent(profileAmbient2->GetValue()->x, 0.59f));
	PassIf(IsEquivalent(profileAmbient2->GetValue()->y, 0.59f));
	PassIf(IsEquivalent(profileAmbient2->GetValue()->z, 0.59f));
	PassIf(profileAmbient2->IsGenerator());
	FCDEffectParameterColor4* effectReflectivityRef2 = (FCDEffectParameterColor4*) FCDEffectTools::FindEffectParameterByReference(effect2, "myReflective", true);
	FCDEffectParameterColor4* effectReflectivitySem2 = (FCDEffectParameterColor4*) FCDEffectTools::FindEffectParameterBySemantic(effect2, "REFLECTIVITY", true);
	PassIf(effectReflectivityRef2 == effectReflectivitySem2);
	PassIf(IsEquivalent(effectReflectivityRef2->GetValue(), FMVector4(0.5f, 0.25f, 0.125f, 1.0f)));
	PassIf(effectReflectivityRef2->IsGenerator());

	// Verify the modifier that's in the material instance parameter's list
	FCDMaterialLibrary* materialLibrary2 = idoc2->GetMaterialLibrary();
	FCDMaterial* material2 = materialLibrary2->GetEntity(0);

	FCDEffectParameterFloat* materialShininess2 = (FCDEffectParameterFloat*) FCDEffectTools::FindEffectParameterByReference(material2, "myShininess", true);
	PassIf(IsEquivalent(materialShininess2->GetValue(), 0.2f));
	PassIf(materialShininess2->IsModifier());

	// Verify the animator that's in the geometry instance parameter's list
	FCDVisualSceneNodeLibrary* visualScene2 = idoc2->GetVisualSceneLibrary();
	PassIf(IsEquivalent((uint32)1, (uint32)visualScene2->GetEntityCount()));
	FCDSceneNode* sceneNode2 = visualScene2->GetEntity(0);
	FCDSceneNode* childNode2 = sceneNode2->GetChild(0);
	FCDEntityInstance* instance2 = childNode2->GetInstance(0);
	FCDGeometryInstance* geometryInstance2 = (FCDGeometryInstance*) instance2;

	FCDEffectParameterFloat* instanceShininess2 = (FCDEffectParameterFloat*) FCDEffectTools::FindEffectParameterBySemantic(geometryInstance2, "SHININESS");
	PassIf(IsEquivalent(instanceShininess2->GetType(), FCDEffectParameter::FLOAT));
	PassIf(instanceShininess2 == FCDEffectTools::FindEffectParameterByReference(geometryInstance2, "myShininessAnimated"));
	PassIf(instanceShininess2->IsAnimator());

TESTSUITE_END
