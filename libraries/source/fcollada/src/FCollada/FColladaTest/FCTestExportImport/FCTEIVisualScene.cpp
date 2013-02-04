/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDCamera.h"
#include "FCDocument/FCDController.h"
#include "FCDocument/FCDControllerInstance.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDEmitter.h"
#include "FCDocument/FCDEmitterInstance.h"
#include "FCDocument/FCDForceField.h"
#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDLight.h"
#include "FCDocument/FCDMaterialInstance.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDTransform.h"
#include "FMath/FMRandom.h"
#include "FCTestExportImport.h"

static const float sampleMatrix[16] = { 0.0f, 2.0f, 0.4f, 2.0f, 7.0f, 0.2f, 991.0f, 2.5f, 11.0f, 25.0f, 1.55f, 0.02f, 0.001f, 12.0f, 1.02e-3f };
static fm::string transformChildId = "TMChild";

namespace FCTestExportImport
{
	static const char* szTestName = "FCTestExportImportVisualScene";

	bool FillVisualScene(FULogFile& fileOut, FCDSceneNode* scene)
	{
		// Create a child node with transforms.
		FCDSceneNode* tmChild = scene->AddChildNode();
		tmChild->SetDaeId(transformChildId);
		FillTransforms(fileOut, tmChild);

		// Create two more child nodes for instances
		FCDSceneNode* instanceNode1 = scene->AddChildNode();
		FailIf(instanceNode1 == NULL);
		FCDSceneNode* instanceNode2 = scene->AddChildNode();
		FailIf(instanceNode1 == NULL);

		// The first instance is a simple geometry instance: we'll need to instantiate the material as well.
		FCDGeometry* meshGeometry = NULL;
		size_t geometryCount = scene->GetDocument()->GetGeometryLibrary()->GetEntityCount();
		for (size_t i = 0; i < geometryCount; ++i)
		{
			FCDGeometry* g = scene->GetDocument()->GetGeometryLibrary()->GetEntity(i);
			FailIf(g == NULL);
			if (g->IsMesh()) { meshGeometry = g; break; }
		}
		FailIf(meshGeometry == NULL);
		FCDGeometry* geometry = scene->GetDocument()->GetGeometryLibrary()->GetEntity(0);
		FailIf(geometry == NULL);
		FCDEntityInstance* meshInstance = instanceNode1->AddInstance(geometry);
		PassIf(meshInstance != NULL);
		FailIf(meshInstance->GetType() != FCDEntityInstance::GEOMETRY);
		FillGeometryInstance(fileOut, (FCDGeometryInstance*) meshInstance);

		// The second instance is a light instance.
		size_t lightCount = scene->GetDocument()->GetLightLibrary()->GetEntityCount();
		PassIf(lightCount > 0);
		FCDLight* light = scene->GetDocument()->GetLightLibrary()->GetEntity(lightCount / 2);
		FCDEntityInstance* lightInstance = instanceNode1->AddInstance(light);
		PassIf(lightInstance != NULL);
		FailIf(lightInstance->GetType() != FCDEntityInstance::SIMPLE);

		// The third instance is a camera instance.
		size_t cameraCount = scene->GetDocument()->GetCameraLibrary()->GetEntityCount();
		PassIf(cameraCount > 0);
		FCDCamera* camera = scene->GetDocument()->GetCameraLibrary()->GetEntity(cameraCount - 1);
		FCDEntityInstance* cameraInstance = instanceNode1->AddInstance(camera);
		PassIf(cameraInstance != NULL);
		FailIf(cameraInstance->GetType() != FCDEntityInstance::SIMPLE);

		// The fourth instance is a controller instance.
		size_t controllerCount = scene->GetDocument()->GetControllerLibrary()->GetEntityCount();
		FCDController* controller = NULL;
		for (size_t i = 0; i < controllerCount; ++i)
		{
			FCDController* c = scene->GetDocument()->GetControllerLibrary()->GetEntity(i);
			PassIf(c != NULL);
			FCDGeometry* g  = c->GetBaseGeometry();
			if (g != NULL && g->IsMesh()) { controller = c; break;}
		}
		FailIf(controller == NULL);
		FCDEntityInstance* controllerInstance = instanceNode2->AddInstance(controller);
		FailIf(controllerInstance == NULL);
		PassIf(controllerInstance->HasType(FCDGeometryInstance::GetClassType()));
		PassIf(controllerInstance->HasType(FCDControllerInstance::GetClassType()));
		FillControllerInstance(fileOut, (FCDGeometryInstance*) controllerInstance);

		// The fifth instance is an emitter instance.
		size_t emitterCount = scene->GetDocument()->GetEmitterLibrary()->GetEntityCount();
		PassIf(emitterCount > 0);
		FCDEmitter* emitter = scene->GetDocument()->GetEmitterLibrary()->GetEntity(0);
		FCDEntityInstance* emitterInstance = instanceNode2->AddInstance(emitter);
		PassIf(emitterInstance != NULL);
		PassIf(emitterInstance->HasType(FCDEmitterInstance::GetClassType()));
		FillEmitterInstance(fileOut, (FCDEmitterInstance*) emitterInstance);

		// The sixth instance is a force field instance.
		size_t fieldCount = scene->GetDocument()->GetForceFieldLibrary()->GetEntityCount();
		PassIf(fieldCount > 0);
		FCDForceField* field = scene->GetDocument()->GetForceFieldLibrary()->GetEntity(0);
		FCDEntityInstance* fieldInstance = instanceNode2->AddInstance(field);
		PassIf(fieldInstance != NULL);
		return true;
	}

	bool FillTransforms(FULogFile& UNUSED(fileOut), FCDSceneNode* child)
	{
		FCDTRotation* rotation = (FCDTRotation*) child->AddTransform(FCDTransform::ROTATION);
		rotation->SetAxis(FMVector3::ZAxis);
		rotation->SetAngle(45.0f);
		FCDTTranslation* translation = (FCDTTranslation*) child->AddTransform(FCDTransform::TRANSLATION);
		translation->SetTranslation(0.0f, 4.0f, 6.0f);
		FCDTScale* scale = (FCDTScale*) child->AddTransform(FCDTransform::SCALE);
		scale->SetScale(FMVector3(3.0f, 0.5f, 2.0f));
		FCDTMatrix* matrix = (FCDTMatrix*) child->AddTransform(FCDTransform::MATRIX);
		matrix->SetTransform(FMMatrix44(sampleMatrix));
		FCDTLookAt* lookAt = (FCDTLookAt*) child->AddTransform(FCDTransform::LOOKAT);
		lookAt->SetPosition(1.0f, 2.0f, 3.0f);
		lookAt->SetTarget(5.0f, 6.0f, 9.0f);
		lookAt->SetUp(12.0f, 0.3f, 0.4f);
		FCDTSkew* skew = (FCDTSkew*) child->AddTransform(FCDTransform::SKEW);
		skew->SetAroundAxis(FMVector3::ZAxis);
		skew->SetRotateAxis(FMVector3::XAxis);
		skew->SetAngle(60.0f);
		return true;
	}

	bool FillControllerInstance(FULogFile& fileOut, FCDGeometryInstance* instance)
	{
		FillGeometryInstance(fileOut, instance);
		return true;
	}

	bool FillGeometryInstance(FULogFile& fileOut, FCDGeometryInstance* instance)
	{
		FailIf(instance == NULL);
		size_t materialCount = instance->GetDocument()->GetMaterialLibrary()->GetEntityCount();
		PassIf(materialCount > 0);

		// Retrieve the mesh associated with the controller and assign its polygons some materials.
		FCDEntity* e = instance->GetEntity();
		FailIf(e == NULL);
		if (e->GetType() == FCDEntity::CONTROLLER) e = ((FCDController*) e)->GetBaseGeometry();
		FailIf(e == NULL || e->GetType() != FCDEntity::GEOMETRY);
		FCDGeometry* g = (FCDGeometry*) e;
		PassIf(g != NULL && g->IsMesh());
		FCDGeometryMesh* mesh = g->GetMesh();
		PassIf(mesh != NULL);

		for (size_t i = 0; i < mesh->GetPolygonsCount(); ++i)
		{
            // Retrieve a random material
			FCDMaterial* material = instance->GetDocument()->GetMaterialLibrary()->GetEntity(FMRandom::GetUInt32((uint32) materialCount));
			instance->AddMaterialInstance(material, mesh->GetPolygons(i));
		}

		// Add a last material instance, with some bindings.
		FCDMaterialInstance* lastMatInstance = instance->AddMaterialInstance();
		lastMatInstance->SetSemantic(FS("DUMMY"));
		lastMatInstance->AddBinding();
		lastMatInstance->AddBinding("Toto", "Targeted");
		lastMatInstance->AddBinding("Gallant", "Pierto");
		return true;
	}

	bool CheckVisualScene(FULogFile& fileOut, FCDSceneNode* imported)
	{
		// Find the child with the ordered transforms, by id.
		FCDSceneNode* tmChild = (FCDSceneNode*) imported->FindDaeId(transformChildId);
		PassIf(tmChild != NULL);
		PassIf(imported->GetParent() == NULL);
		PassIf(tmChild->GetParent() == imported);
		CheckTransforms(fileOut, tmChild);

		// Make a list of all the instances in the child nodes.
		fm::pvector<FCDEntityInstance> instances;
		for (size_t i = 0; i < imported->GetChildrenCount(); ++i)
		{
			FCDSceneNode* n = imported->GetChild(i);
			for (size_t j = 0; j < n->GetInstanceCount(); ++j)
			{
				instances.push_back(n->GetInstance(j));
			}
		}

		// Process the instances, there should be one light, one camera, one mesh and one controller!
		bool foundLight = false, foundCamera = false, foundMesh = false, foundController = false, foundEmitter = false, foundForceField = false;
		for (size_t k = 0; k < instances.size(); ++k)
		{
			FCDEntityInstance* instance = instances[k];
			FailIf(instance == NULL);
			FailIf(instance->GetEntity() == NULL);
			switch (instance->GetEntity()->GetType())
			{
			case FCDEntity::LIGHT:
				FailIf(foundLight); foundLight = true;
				FailIf(instance->GetType() != FCDEntityInstance::SIMPLE);
				break;

			case FCDEntity::CAMERA:
				FailIf(foundCamera); foundCamera = true;
				FailIf(instance->GetType() != FCDEntityInstance::SIMPLE);
				break;

			case FCDEntity::GEOMETRY:
				FailIf(foundMesh); foundMesh = true;
				FailIf(instance->GetType() != FCDEntityInstance::GEOMETRY);
				CheckGeometryInstance(fileOut, (FCDGeometryInstance*) instance);
				break;

			case FCDEntity::CONTROLLER:
				FailIf(foundController); foundController = true;
				FailIf(instance->GetType() != FCDEntityInstance::CONTROLLER);
				CheckControllerInstance(fileOut, (FCDGeometryInstance*) instance);
				break;

			case FCDEntity::EMITTER:
				FailIf(foundEmitter); foundEmitter = true;
				PassIf(instance->HasType(FCDEmitterInstance::GetClassType()));
				CheckEmitterInstance(fileOut, (FCDEmitterInstance*) instance);
				break;

			case FCDEntity::FORCE_FIELD:
				FailIf(foundForceField); foundForceField = true;
				PassIf(instance->GetEntityType() == FCDEntity::FORCE_FIELD);
				break;

			default: Fail; break;
			}
		}
		PassIf(foundLight && foundCamera && foundMesh && foundController && foundEmitter && foundForceField);
		return true;
	}

	bool CheckTransforms(FULogFile& fileOut, FCDSceneNode* child)
	{
		PassIf(child != NULL);
		PassIf(child->GetTransformCount() == 6);

		// The transforms MUST be in the same order as the exported order.
		FCDTransform* transform = child->GetTransform(0);
		FailIf(transform == NULL);
		FailIf(transform->GetParent() != child);
		PassIf(transform->GetType() == FCDTransform::ROTATION);
		FCDTRotation* rotation = (FCDTRotation*) transform;
		PassIf(IsEquivalent(rotation->GetAxis(), FMVector3::ZAxis));
		PassIf(IsEquivalent(rotation->GetAngle(), 45.0f));

		transform = child->GetTransform(1);
		FailIf(transform == NULL);
		FailIf(transform->GetParent() != child);
		PassIf(transform->GetType() == FCDTransform::TRANSLATION);
		FCDTTranslation* translation = (FCDTTranslation*) transform;
		PassIf(IsEquivalent(translation->GetTranslation(), FMVector3(0.0f, 4.0f, 6.0f)));

		transform = child->GetTransform(2);
		FailIf(transform == NULL);
		FailIf(transform->GetParent() != child);
		PassIf(transform->GetType() == FCDTransform::SCALE);
		FCDTScale* scale = (FCDTScale*) transform;
		PassIf(IsEquivalent(scale->GetScale(), FMVector3(3.0f, 0.5f, 2.0f)));

		transform = child->GetTransform(3);
		FailIf(transform == NULL);
		FailIf(transform->GetParent() != child);
		PassIf(transform->GetType() == FCDTransform::MATRIX);
		FCDTMatrix* mx = (FCDTMatrix*) transform;
		PassIf(IsEquivalent(mx->GetTransform(), FMMatrix44(sampleMatrix)));

		transform = child->GetTransform(4);
		FailIf(transform == NULL);
		FailIf(transform->GetParent() != child);
		PassIf(transform->GetType() == FCDTransform::LOOKAT);
		FCDTLookAt* lookAt = (FCDTLookAt*) transform;
		PassIf(IsEquivalent(lookAt->GetPosition(), FMVector3(1.0f, 2.0f, 3.0f)));
		PassIf(IsEquivalent(lookAt->GetTarget(), FMVector3(5.0f, 6.0f, 9.0f)));
		PassIf(IsEquivalent(lookAt->GetUp(), FMVector3(12.0f, 0.3f, 0.4f)));

		transform = child->GetTransform(5);
		FailIf(transform == NULL);
		FailIf(transform->GetParent() != child);
		PassIf(transform->GetType() == FCDTransform::SKEW);
		FCDTSkew* skew = (FCDTSkew*) transform;
		PassIf(IsEquivalent(skew->GetAroundAxis(), FMVector3::ZAxis));
		PassIf(IsEquivalent(skew->GetRotateAxis(), FMVector3::XAxis));
		PassIf(IsEquivalent(skew->GetAngle(), 60.0f));
		return true;
	}

	bool CheckControllerInstance(FULogFile& fileOut, FCDGeometryInstance* instance)
	{
		FailIf(instance == NULL);
		CheckGeometryInstance(fileOut, instance);
		return true;
	}

	bool CheckGeometryInstance(FULogFile& fileOut, FCDGeometryInstance* instance)
	{
		FailIf(instance == NULL);

		// Retrieve the mesh instantiated.
		FCDEntity* e = instance->GetEntity();
		FailIf(e == NULL);
		if (e->GetType() == FCDEntity::CONTROLLER) e = ((FCDController*) e)->GetBaseGeometry();
		FailIf(e == NULL || e->GetType() != FCDEntity::GEOMETRY);
		FCDGeometry* g = (FCDGeometry*) e;
		PassIf(g != NULL && g->IsMesh());
		FCDGeometryMesh* mesh = g->GetMesh();
		PassIf(mesh != NULL);

		// Verify that all the polygons have a material instantiated for them.
		for (size_t i = 0; i < mesh->GetPolygonsCount(); ++i)
		{
			PassIf(mesh->GetPolygons(i) != NULL);
			const fstring& semantic = mesh->GetPolygons(i)->GetMaterialSemantic();
			PassIf(!semantic.empty());
			FCDMaterialInstance* material = instance->FindMaterialInstance(semantic);
			PassIf(material != NULL);
			PassIf(material->GetMaterial() != NULL);
		}

		// Verify the last material instance and its bindings.
		FCDMaterialInstance* lastMatInstance = instance->FindMaterialInstance(FC("DUMMY"));
		PassIf(lastMatInstance != NULL);
		PassIf(lastMatInstance->GetMaterial() == NULL);
		PassIf(lastMatInstance->GetBindingCount() == 3);
		bool foundEmpty = false, foundToto = false, foundGallant = false;
		for (size_t i = 0; i < 3; ++i)
		{
			FCDMaterialInstanceBind* b = lastMatInstance->GetBinding(i);
			if (b->semantic->empty()) { PassIf(!foundEmpty); foundEmpty = true; PassIf(b->target->empty()); }
			else if (IsEquivalent(b->semantic, "Toto")) { PassIf(!foundToto); foundToto = true; PassIf(IsEquivalent(b->target, "Targeted")); }
			else if (IsEquivalent(b->semantic, "Gallant")) { PassIf(!foundGallant); foundGallant = true; PassIf(IsEquivalent(b->target, "Pierto")); }
			else Fail;
		}
		PassIf(foundEmpty && foundToto && foundGallant);
		return true;
	}

	bool FillLayers(FULogFile& UNUSED(fileOut), FCDocument* doc)
	{
		FCDLayer* layer = doc->AddLayer();
		layer->name = "TestL1";

		layer = doc->AddLayer();
		layer->name = "TestL2";
		layer->objects.push_back("Obj1");
		layer->objects.push_back("Obj2");
		return true;
	}

	bool CheckLayers(FULogFile& fileOut, FCDocument* doc)
	{
		PassIf(doc->GetLayerCount() == 2);
		
		// There should be two layers: one is empty and the other will
		// be verified later on.
		FCDLayer* emptyLayer = NULL,* fullLayer = NULL;
		for (size_t i = 0; i < 2; ++i)
		{
			FCDLayer* l = doc->GetLayer(i);
			if (l->objects.empty()) { FailIf(emptyLayer != NULL); emptyLayer = l; }
			else { FailIf(fullLayer != NULL); fullLayer = l; }
		}
		PassIf(emptyLayer != NULL && fullLayer != NULL);

		// Verify the layer names.
		PassIf(emptyLayer->name == "TestL1");
		PassIf(fullLayer->name == "TestL2");
		FailIf(fullLayer->objects.size() != 2);
		PassIf(fullLayer->objects[0] == "Obj1");
		PassIf(fullLayer->objects[1] == "Obj2");
		return true;
	}
};

