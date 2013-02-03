/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDPhysicsMaterial.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsRigidBodyInstance.h"
#include "FCDocument/FCDPhysicsRigidBodyParameters.h"
#include "FCDocument/FCDPhysicsRigidConstraint.h"
#include "FCDocument/FCDPhysicsRigidConstraintInstance.h"
#include "FUtils/FUDaeSyntax.h"
#include "FCTestExportImport.h"

static fm::string physicsMaterialId1 = "Pmat";
static fm::string physicsMaterialId2 = "PMat";
static fm::string physicsMaterialId3 = "PMat";
static fm::string physicsModelId1, physicsModelId2, physicsModelId3;
static const char* szTestName = "FCTestExportImportPhysics";

namespace FCTestExportImport
{

	bool FillPhysics(FULogFile& fileOut, FCDocument* document)
	{
		PassIf(FillPhysicsMaterialLibrary(fileOut, document->GetPhysicsMaterialLibrary()));
		PassIf(FillPhysicsModelLibrary(fileOut, document->GetPhysicsModelLibrary()));
		return true;
	}

	bool CheckPhysics(FULogFile& fileOut, FCDocument* document)
	{
		PassIf(CheckPhysicsMaterialLibrary(fileOut, document->GetPhysicsMaterialLibrary()));
		PassIf(CheckPhysicsModelLibrary(fileOut, document->GetPhysicsModelLibrary()));
		return true;
	}

	//
	// Physics Materials
	//

	bool FillPhysicsMaterialLibrary(FULogFile& fileOut, FCDPhysicsMaterialLibrary* library)
	{
		// Create three physics materials
		FCDPhysicsMaterial* material1 = library->AddEntity();
		material1->SetDaeId(physicsMaterialId1);
		physicsMaterialId1 = material1->GetDaeId();

		FCDPhysicsMaterial* material2 = library->AddEntity();
		material2->SetDaeId(physicsMaterialId2);
		physicsMaterialId2 = material2->GetDaeId();

		FCDPhysicsMaterial* material3 = library->AddEntity();
		material3->SetDaeId(physicsMaterialId3);
		physicsMaterialId3 = material3->GetDaeId();

		FailIf(physicsMaterialId1 == physicsMaterialId2);
		FailIf(physicsMaterialId1 == physicsMaterialId3);
		FailIf(physicsMaterialId2 == physicsMaterialId3);

		// Fill the three physics materials with different parameter values
		material1->SetStaticFriction(1.6f);
		material1->SetDynamicFriction(1.7f);
		material1->SetRestitution(0.2f);

		material2->SetStaticFriction(0.6f);
		material2->SetDynamicFriction(7.6f);
		material2->SetRestitution(-0.5f);

		material3->SetStaticFriction(0.002f);
		material3->SetDynamicFriction(1992.5f);
		material3->SetRestitution(33.0f);
		return true;
	}

	bool CheckPhysicsMaterialLibrary(FULogFile& fileOut, FCDPhysicsMaterialLibrary* library)
	{
		// Find the three wanted materials
		FCDPhysicsMaterial* material1 = NULL,* material2 = NULL,* material3 = NULL;
		for (size_t i = 0; i < library->GetEntityCount(); ++i)
		{
			FCDPhysicsMaterial* m = library->GetEntity(i);
			if (m->GetDaeId() == physicsMaterialId1) { FailIf(material1 != NULL); material1 = m; }
			else if (m->GetDaeId() == physicsMaterialId2) { FailIf(material2 != NULL); material2 = m; }
			else if (m->GetDaeId() == physicsMaterialId3) { FailIf(material3 != NULL); material3 = m; }
			else { Fail; }
		}
		PassIf(material1 != NULL && material2 != NULL && material3 != NULL);

		// Verify the material informations
		PassIf(IsEquivalent(material1->GetStaticFriction(), 1.6f));
		PassIf(IsEquivalent(material1->GetDynamicFriction(), 1.7f));
		PassIf(IsEquivalent(material1->GetRestitution(), 0.2f));

		PassIf(IsEquivalent(material2->GetStaticFriction(), 0.6f));
		PassIf(IsEquivalent(material2->GetDynamicFriction(), 7.6f));
		PassIf(IsEquivalent(material2->GetRestitution(), -0.5f));

		PassIf(IsEquivalent(material3->GetStaticFriction(), 0.002f));
		PassIf(IsEquivalent(material3->GetDynamicFriction(), 1992.5f));
		PassIf(IsEquivalent(material3->GetRestitution(), 33.0f));
		return true;
	}

	//
	// Physics Model
	//

	bool FillPhysicsModelLibrary(FULogFile& fileOut, FCDPhysicsModelLibrary* library)
	{
		// Create some physics models
		FCDPhysicsModel* model1 = library->AddEntity();
		physicsModelId1 = model1->GetDaeId();

		FCDPhysicsModel* model2 = library->AddEntity();
		physicsModelId2 = model2->GetDaeId();

		FCDPhysicsModel* model3 = library->AddEntity();
		physicsModelId3 = model3->GetDaeId();

		// Fill in the first model with rigid bodies and constraints
		PassIf(FillPhysicsModel(fileOut, model1));

		// Fill the second model with two instances of the first model.
		model2->AddPhysicsModelInstance(model1);
		model2->AddPhysicsModelInstance(model1);

		// Leave the third model empty.
		return true;
	}

	bool FillPhysicsModel(FULogFile& fileOut, FCDPhysicsModel* model)
	{
		FCDPhysicsRigidBody* body = model->AddRigidBody();
		body->SetSubId("GLAD");
		body = model->AddRigidBody();
		PassIf(FillPhysicsRigidBody(fileOut, body));
		body->SetSubId("DALG");

		FCDPhysicsRigidConstraint* constraint = model->AddRigidConstraint();
		constraint->SetSubId("Pour");
		constraint = model->AddRigidConstraint();
		PassIf(FillPhysicsRigidConstraint(fileOut, constraint));
		constraint->SetSubId("Contre");
		return true;
	}

	bool CheckPhysicsModelLibrary(FULogFile& fileOut, FCDPhysicsModelLibrary* library)
	{
		// Find the three physics models
		FCDPhysicsModel* model1 = NULL,* model2 = NULL,* model3 = NULL;
		for (size_t i = 0; i < library->GetEntityCount(); ++i)
		{
			FCDPhysicsModel* m = library->GetEntity(i);
			if (m->GetDaeId() == physicsModelId1) { FailIf(model1 != NULL); model1 = m; }
			else if (m->GetDaeId() == physicsModelId2) { FailIf(model2 != NULL); model2 = m; }
			else if (m->GetDaeId() == physicsModelId3) { FailIf(model3 != NULL); model3 = m; }
			else { Fail; }
		}
		PassIf(model1 != NULL && model2 != NULL && model3 != NULL);

		// Verify the physics models information. The third model should be empty,
		// the second model should have two instances of the first model.
		PassIf(CheckPhysicsModel(fileOut, model1));

		PassIf(model2->GetInstanceCount() == 2);
		PassIf(model2->GetInstance(0)->GetEntity() == model1);
		PassIf(model2->GetInstance(1)->GetEntity() == model1);
		PassIf(model2->GetRigidBodyCount() == 0);
		PassIf(model2->GetRigidConstraintCount() == 0);

		PassIf(model3->GetInstanceCount() == 0);
		PassIf(model3->GetRigidBodyCount() == 0);
		PassIf(model3->GetRigidConstraintCount() == 0);
		return true;
	}

	bool CheckPhysicsModel(FULogFile& fileOut, FCDPhysicsModel* model)
	{
		PassIf(model->GetInstanceCount() == 0);

		PassIf(model->GetRigidBodyCount() == 2);
		FCDPhysicsRigidBody* body1 = NULL,* body2 = NULL;
		for (size_t i = 0; i < 2; ++i)
		{
			FCDPhysicsRigidBody* b = model->GetRigidBody(i);
			if (b->GetSubId() == "GLAD") { FailIf(body1 != NULL); body1 = b; }
			else if (b->GetSubId() == "DALG") { FailIf(body2 != NULL); body2 = b; }
			else Fail;
		}
		PassIf(body1 != NULL && body2 != NULL);
		PassIf(model->FindRigidBodyFromSid("DALG") == body2);
		PassIf(model->FindRigidBodyFromSid("GLAD") == body1);
		PassIf(CheckPhysicsRigidBody(fileOut, body2));

		PassIf(model->GetRigidConstraintCount() == 2);
		FCDPhysicsRigidConstraint* constraint1 = NULL,* constraint2 = NULL;
		for (size_t i = 0; i < 2; ++i)
		{
			FCDPhysicsRigidConstraint* c = model->GetRigidConstraint(i);
			if (c->GetSubId() == "Pour") { FailIf(constraint1 != NULL); constraint1 = c; }
			else if (c->GetSubId() == "Contre") { FailIf(constraint2 != NULL); constraint2 = c; }
			else Fail;
		}
		PassIf(constraint1 != NULL && constraint2 != NULL);
		PassIf(CheckPhysicsRigidConstraint(fileOut, constraint2));
		return true;
	}

	//
	// Rigid Body
	//

	bool FillPhysicsRigidBody(FULogFile& UNUSED(fileOut), FCDPhysicsRigidBody* rigidBody)
	{
		rigidBody->GetParameters()->SetMass(0.5f);
		return true;
	}

	bool CheckPhysicsRigidBody(FULogFile& fileOut, FCDPhysicsRigidBody* rigidBody)
	{
		PassIf(IsEquivalent(rigidBody->GetParameters()->GetMass(), 0.5f));
		return true;
	}

	//
	// Rigid Constraint
	// 

	bool FillPhysicsRigidConstraint(FULogFile& UNUSED(fileOut), FCDPhysicsRigidConstraint* rigidConstraint)
	{
		rigidConstraint->SetEnabled(false);
		rigidConstraint->SetInterpenetrate(false);
		return true;
	}

	bool CheckPhysicsRigidConstraint(FULogFile& fileOut, FCDPhysicsRigidConstraint* rigidConstraint)
	{
		PassIf(!rigidConstraint->IsEnabled());
		PassIf(!rigidConstraint->IsInterpenetrate());
		return true;
	}
};
