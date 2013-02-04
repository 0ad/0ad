/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDEntityReference.h"
#include "FCDocument/FCDPhysicsForceFieldInstance.h"
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDPhysicsRigidBodyParameters.h"
#include "FCDocument/FCDPhysicsAnalyticalGeometry.h"
#include "FCDocument/FCDPhysicsMaterial.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsRigidConstraint.h"
#include "FCDocument/FCDPhysicsScene.h"
#include "FCDocument/FCDPhysicsShape.h"

bool FArchiveXML::LoadPhysicsRigidBodyParameters(FCDPhysicsRigidBodyParameters* parameters, xmlNode* techniqueNode, FCDPhysicsRigidBodyParameters* defaultParameters)
{
	bool status = true;

	xmlNode* param = FindChildByType(techniqueNode, DAE_DYNAMIC_ELEMENT);
	if (param)
	{
		parameters->SetDynamic(FUStringConversion::ToBoolean(ReadNodeContentDirect(param)));
		FArchiveXML::LoadAnimatable(&parameters->GetDynamic(), param);
	}
	else if (defaultParameters != NULL)
	{
		parameters->SetDynamic(defaultParameters->GetDynamic() > 0.5f);
		if (defaultParameters->GetDynamic().IsAnimated())
		{
			defaultParameters->GetDynamic().GetAnimated()->Clone(parameters->GetDynamic().GetAnimated());
		}
	}

	xmlNode* massFrame;
	massFrame = FindChildByType(techniqueNode, DAE_MASS_FRAME_ELEMENT);
	if (massFrame)
	{
		param = FindChildByType(massFrame, DAE_TRANSLATE_ELEMENT);
		if (param)
		{
			parameters->SetMassFrameTranslate(FUStringConversion::ToVector3(ReadNodeContentDirect(param)));
			FArchiveXML::LoadAnimatable(&parameters->GetMassFrameTranslate(), param);
		}
		else if (defaultParameters != NULL)
		{
			parameters->SetMassFrameTranslate(defaultParameters->GetMassFrameTranslate());
			if (defaultParameters->GetMassFrameTranslate().IsAnimated())
			{
				defaultParameters->GetMassFrameTranslate().GetAnimated()->Clone(parameters->GetMassFrameTranslate().GetAnimated());
			}
		}
		else
		{
			// no movement
			parameters->SetMassFrameTranslate(FMVector3::Zero);
		}

		param = FindChildByType(massFrame, DAE_ROTATE_ELEMENT);
		if (param)
		{
			FMVector4 temp = FUStringConversion::ToVector4(ReadNodeContentDirect(param));
			parameters->SetMassFrameOrientation(FMAngleAxis(FMVector3(temp.x, temp.y, temp.z), temp.w));
			LoadAnimatable(&parameters->GetMassFrameOrientation(), param);
		}
		else if (defaultParameters != NULL)
		{
			parameters->SetMassFrameOrientation(defaultParameters->GetMassFrameOrientation());
			if (defaultParameters->GetMassFrameOrientation().IsAnimated())
			{
				defaultParameters->GetMassFrameOrientation().GetAnimated()->Clone(parameters->GetMassFrameOrientation().GetAnimated());
			}
		}
		else
		{
			// no movement
			parameters->SetMassFrameOrientation(FMAngleAxis(FMVector3::XAxis, 0.0f));
		}
	}
	else if (defaultParameters != NULL)
	{
		parameters->SetMassFrameTranslate(defaultParameters->GetMassFrameTranslate());
		parameters->SetMassFrameOrientation(defaultParameters->GetMassFrameOrientation());
		if (defaultParameters->GetMassFrameTranslate().IsAnimated())
		{
			defaultParameters->GetMassFrameTranslate().GetAnimated()->Clone(parameters->GetMassFrameTranslate().GetAnimated());
		}
		if (defaultParameters->GetMassFrameOrientation().IsAnimated())
		{
			defaultParameters->GetMassFrameOrientation().GetAnimated()->Clone(parameters->GetMassFrameOrientation().GetAnimated());
		}
	}
	else
	{
		// no movement
		parameters->SetMassFrameTranslate(FMVector3::Zero);
		parameters->SetMassFrameOrientation(FMAngleAxis(FMVector3::XAxis, 0.0f));
	}

	xmlNodeList shapeNodes;
	FindChildrenByType(techniqueNode, DAE_SHAPE_ELEMENT, shapeNodes);
	if (shapeNodes.empty())
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_SHAPE_NODE_MISSING, techniqueNode->line);
	}
	for (xmlNodeList::iterator itS = shapeNodes.begin(); itS != shapeNodes.end(); ++itS)
	{
		FCDPhysicsShape* shape = parameters->AddPhysicsShape();
		status &= (FArchiveXML::LoadPhysicsShape(shape, *itS));
	}
	// shapes are not taken from the default parameters

	param = FindChildByType(techniqueNode, DAE_PHYSICS_MATERIAL_ELEMENT);
	if (param != NULL) 
	{
		FCDPhysicsMaterial* material = parameters->AddOwnPhysicsMaterial();
		FArchiveXML::LoadPhysicsMaterial(material, param);
	}
	else
	{
		param = FindChildByType(techniqueNode, DAE_INSTANCE_PHYSICS_MATERIAL_ELEMENT);
		if (param != NULL)
		{
			FCDEntityInstance* physicsMaterialInstance = FCDEntityInstanceFactory::CreateInstance(parameters->GetDocument(), NULL, FCDEntity::PHYSICS_MATERIAL);
			parameters->SetInstanceMaterial(physicsMaterialInstance);
			FArchiveXML::LoadSwitch(physicsMaterialInstance, &physicsMaterialInstance->GetObjectType(), param);
			FCDPhysicsMaterial* material = (FCDPhysicsMaterial*) physicsMaterialInstance->GetEntity();
			if (material == NULL)
			{
				FUError::Error(FUError::ERROR_LEVEL, FUError::WARNING_MISSING_URI_TARGET, param->line);
			}
			parameters->SetPhysicsMaterial(material);
		}
		else
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_PHYS_MAT_DEF_MISSING, techniqueNode->line);
		}
	}
	// material is not taken fromt he default parameters

	param = FindChildByType(techniqueNode, DAE_MASS_ELEMENT);
	if (param)
	{
		parameters->SetMass(FUStringConversion::ToFloat(ReadNodeContentDirect(param)));
		parameters->SetDensityMoreAccurate(false);
		parameters->SetDensity(0.0f);
		FArchiveXML::LoadAnimatable(&parameters->GetMass(), param);
	}
	else if (defaultParameters != NULL)
	{
		parameters->SetMass(defaultParameters->GetMass());
		parameters->SetDensity(defaultParameters->GetDensity());
		parameters->SetDensityMoreAccurate(defaultParameters->IsDensityMoreAccurate());
		if (defaultParameters->GetMass().IsAnimated())
		{
			defaultParameters->GetMass().GetAnimated()->Clone(parameters->GetMass().GetAnimated());
		}
	}
	else
	{
		/* Default value for mass is density x total shape volume, but 
		   since our shape's mass is already calculated with respect to the
		   volume, we can just read it from there. If the user specified a 
		   mass, then this overrides the calculation of density x volume, 
		   as expected. */
		parameters->SetMass(0.0f);
		float totalDensity = 0.0f;
		parameters->SetDensityMoreAccurate(false);
		for (size_t i = 0; i < parameters->GetPhysicsShapeCount(); ++i)
		{
			FCDPhysicsShape* shape = parameters->GetPhysicsShape(i);
			parameters->SetMass(parameters->GetMass() + shape->GetMass());
			totalDensity += shape->GetDensity();
			parameters->SetDensityMoreAccurate(parameters->IsDensityMoreAccurate() || shape->IsDensityMoreAccurate()); // common case: 1 shape, density = 1.0f
		}
		parameters->SetDensity(totalDensity / parameters->GetPhysicsShapeCount());
	}
	

	param = FindChildByType(techniqueNode, DAE_INERTIA_ELEMENT);
	if (param) 
	{
		parameters->SetInertia(FUStringConversion::ToVector3(ReadNodeContentDirect(param)));
		parameters->SetInertiaAccurate(true);
		FArchiveXML::LoadAnimatable(&parameters->GetInertia(), param);
	}
	else if (defaultParameters != NULL)
	{
		parameters->SetInertia(defaultParameters->GetInertia());
		parameters->SetInertiaAccurate(defaultParameters->IsInertiaAccurate());
		if (defaultParameters->GetInertia().IsAnimated())
		{
			defaultParameters->GetInertia().GetAnimated()->Clone(parameters->GetInertia().GetAnimated());
		}
	}
	else
	{
		/* FIXME: Approximation: sphere shape, with mass distributed 
		   equally across the volume and center of mass is at the center of
		   the sphere. Real moments of inertia call for complex 
		   integration. Sphere it is simply I = k * m * r^2 on all axes. */
		float volume = 0.0f;
		for (size_t i = 0; i < parameters->GetPhysicsShapeCount(); ++i)
		{
			volume += parameters->GetPhysicsShape(i)->CalculateVolume();
		}

		float radiusCubed = 0.75f * volume / (float)FMath::Pi;
		float I = 0.4f * parameters->GetMass() * pow(radiusCubed, 2.0f / 3.0f);
		parameters->SetInertia(FMVector3(I, I, I));
		parameters->SetInertiaAccurate(false);
	}

	return status;
}

bool FArchiveXML::AttachModelInstancesFCDPhysicsModel(FCDPhysicsModel* physicsModel)
{
	bool status = true;
	
	FCDPhysicsModelDataMap::iterator it = FArchiveXML::documentLinkDataMap[physicsModel->GetDocument()].physicsModelDataMap.find(physicsModel);
	FUAssert(it != FArchiveXML::documentLinkDataMap[physicsModel->GetDocument()].physicsModelDataMap.end(),);
	FCDPhysicsModelData& data = it->second;

	for (ModelInstanceNameNodeMap::iterator it = data.modelInstancesMap.begin(); it != data.modelInstancesMap.end(); ++it)
	{
		FCDPhysicsModelInstance* instance = physicsModel->AddPhysicsModelInstance();
		status &= FArchiveXML::LoadPhysicsModelInstance(instance, it->first);
	}
	data.modelInstancesMap.clear();
	return status;
}

bool FArchiveXML::LoadPhysicsShape(FCDObject* object, xmlNode* physicsShapeNode)
{ 
	FCDPhysicsShape* physicsShape = (FCDPhysicsShape*)object;

	bool status = true;
	if (!IsEquivalent(physicsShapeNode->name, DAE_SHAPE_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOW_PS_LIB_ELEMENT, physicsShapeNode->line);
		return status;
	}

	// Read in the first valid child element found
	for (xmlNode* child = physicsShapeNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_HOLLOW_ELEMENT))
		{
			physicsShape->SetHollow(FUStringConversion::ToBoolean(ReadNodeContentDirect(child)));
		}
		else if (IsEquivalent(child->name, DAE_MASS_ELEMENT))
		{
			physicsShape->SetMass(FUStringConversion::ToFloat(ReadNodeContentDirect(child)));
			physicsShape->SetDensityMoreAccurate(false);
		}
		else if (IsEquivalent(child->name, DAE_DENSITY_ELEMENT))
		{
			physicsShape->SetDensity(FUStringConversion::ToFloat(ReadNodeContentDirect(child)));
			physicsShape->SetDensityMoreAccurate(physicsShape->GetMassPointer() == NULL); // mass before density in COLLADA 1.4.1
		}
		else if (IsEquivalent(child->name, DAE_PHYSICS_MATERIAL_ELEMENT))
		{
			FCDPhysicsMaterial* material = physicsShape->AddOwnPhysicsMaterial();
			FArchiveXML::LoadPhysicsMaterial(material, child);
		}
		else if (IsEquivalent(child->name, 
				DAE_INSTANCE_PHYSICS_MATERIAL_ELEMENT))
		{
			physicsShape->SetInstanceMaterial(FCDEntityInstanceFactory::CreateInstance(physicsShape->GetDocument(), NULL, FCDEntity::PHYSICS_MATERIAL));
			FArchiveXML::LoadSwitch(physicsShape->GetInstanceMaterial(),
										&physicsShape->GetInstanceMaterial()->GetObjectType(),
										child);

			if (!HasNodeProperty(child, DAE_URL_ATTRIBUTE))
			{
				//inline definition of physics_material
				FCDPhysicsMaterial* material = physicsShape->AddOwnPhysicsMaterial();
				FArchiveXML::LoadPhysicsMaterial(material, child);
				physicsShape->GetInstanceMaterial()->SetEntity(material);
			}
		}
		else if (IsEquivalent(child->name, DAE_INSTANCE_GEOMETRY_ELEMENT))
		{
			FUUri url = ReadNodeUrl(child);
			if (!url.IsFile())
			{
				FCDGeometry* entity = physicsShape->GetDocument()->FindGeometry(TO_STRING(url.GetFragment()));
				if (entity != NULL)
				{
					physicsShape->SetAnalyticalGeometry(NULL);
					physicsShape->SetGeometryInstance((FCDGeometryInstance*)FCDEntityInstanceFactory::CreateInstance(physicsShape->GetDocument(), NULL, FCDEntity::GEOMETRY));
					physicsShape->GetGeometryInstance()->SetEntity((FCDEntity*)entity);
					status &= (FArchiveXML::LoadGeometryInstance(physicsShape->GetGeometryInstance(), child));
					continue; 
				}
			}
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_FCDGEOMETRY_INST_MISSING, child->line);
		}

#define PARSE_ANALYTICAL_SHAPE(type, nodeName) \
		else if (IsEquivalent(child->name, nodeName)) { \
		FCDPhysicsAnalyticalGeometry* analytical = physicsShape->CreateAnalyticalGeometry(FCDPhysicsAnalyticalGeometry::type); \
		status = FArchiveXML::LoadPhysicsAnalyticalGeometry(analytical, child); \
			if (!status) { \
				FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_SHAPE_NODE, child->line); break; \
			} }

		PARSE_ANALYTICAL_SHAPE(BOX, DAE_BOX_ELEMENT)
		PARSE_ANALYTICAL_SHAPE(PLANE, DAE_PLANE_ELEMENT)
		PARSE_ANALYTICAL_SHAPE(SPHERE, DAE_SPHERE_ELEMENT)
		PARSE_ANALYTICAL_SHAPE(CYLINDER, DAE_CYLINDER_ELEMENT)
		PARSE_ANALYTICAL_SHAPE(CAPSULE, DAE_CAPSULE_ELEMENT)
		PARSE_ANALYTICAL_SHAPE(TAPERED_CAPSULE, DAE_TAPERED_CAPSULE_ELEMENT)
		PARSE_ANALYTICAL_SHAPE(TAPERED_CYLINDER, DAE_TAPERED_CYLINDER_ELEMENT)
#undef PARSE_ANALYTICAL_SHAPE


		// Parse the physics shape transforms <rotate>, <translate> are supported.
		else if (IsEquivalent(child->name, DAE_ASSET_ELEMENT)) {}
		else if (IsEquivalent(child->name, DAE_EXTRA_ELEMENT)) {}
		else
		{
			uint32 transformType = FArchiveXML::GetTransformType(child);
			if (transformType == FCDTransform::TRANSLATION || transformType == FCDTransform::ROTATION)
			{
				FCDTransform* transform = physicsShape->AddTransform((FCDTransform::Type) transformType);
				status &= (FArchiveXML::LoadSwitch(transform, &transform->GetObjectType(), child));
			}
		}
	}

	if ((physicsShape->GetMassPointer() == NULL) && (physicsShape->GetDensityPointer() == NULL))
	{
		physicsShape->SetDensity(1.0f);
		physicsShape->SetDensityMoreAccurate(true);
	}

	// default value if only one is defined.
	if ((physicsShape->GetMassPointer() == NULL) && (physicsShape->GetDensityPointer() != NULL))
	{
		physicsShape->SetMass(physicsShape->GetDensity() * physicsShape->CalculateVolume());
	}
	else if ((physicsShape->GetMassPointer() != NULL) && (physicsShape->GetDensityPointer() == NULL))
	{
		physicsShape->SetDensity(physicsShape->GetMass() / physicsShape->CalculateVolume());
	}

	physicsShape->SetDirtyFlag();
	return status;
}		

bool FArchiveXML::LoadPhysicsAnalyticalGeometry(FCDObject* object, xmlNode* node)
{ 
	return FArchiveXML::LoadSwitch(object, &object->GetObjectType(), node);
}

bool FArchiveXML::LoadPASBox(FCDObject* object, xmlNode* node)
{
	FCDPASBox* pASBox = (FCDPASBox*)object;

	bool status = true;

	if (!IsEquivalent(node->name, DAE_BOX_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_BOX_TYPE, node->line);
		return status;
	}

	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_HALF_EXTENTS_ELEMENT))
		{
			const char* halfExt = ReadNodeContentDirect(child);
			pASBox->halfExtents.x = FUStringConversion::ToFloat(&halfExt);
			pASBox->halfExtents.y = FUStringConversion::ToFloat(&halfExt);
			pASBox->halfExtents.z = FUStringConversion::ToFloat(&halfExt);
		}
	}

	pASBox->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadPASCapsule(FCDObject* object, xmlNode* node)
{
	FCDPASCapsule* pASCapsule = (FCDPASCapsule*)object;

	bool status = true;

	if (!IsEquivalent(node->name, DAE_CAPSULE_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_CAPSULE_TYPE, node->line);
		return status;
	}

	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_HEIGHT_ELEMENT))
		{
			pASCapsule->height = FUStringConversion::ToFloat(ReadNodeContentDirect(child));
		}
		else if (IsEquivalent(child->name, DAE_RADIUS_ELEMENT))
		{
			const char* stringRadius = ReadNodeContentDirect(child);
			pASCapsule->radius.x = FUStringConversion::ToFloat(&stringRadius);
			pASCapsule->radius.y = FUStringConversion::ToFloat(&stringRadius);
		}
	}

	pASCapsule->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadPASTaperedCapsule(FCDObject* object, xmlNode* node)
{
	FCDPASTaperedCapsule* pASTaperedCapsule = (FCDPASTaperedCapsule*)object;

	bool status = true;

	if (!IsEquivalent(node->name, DAE_TAPERED_CAPSULE_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_TCAPSULE_TYPE, node->line);
		return status;
	}

	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_HEIGHT_ELEMENT))
		{
			const char* h = ReadNodeContentDirect(child);
			pASTaperedCapsule->height = FUStringConversion::ToFloat(&h);
		}
		else if (IsEquivalent(child->name, DAE_RADIUS1_ELEMENT))
		{
			const char* stringRadius = ReadNodeContentDirect(child);
			pASTaperedCapsule->radius.x = FUStringConversion::ToFloat(&stringRadius);
			pASTaperedCapsule->radius.y = FUStringConversion::ToFloat(&stringRadius);
		}
		else if (IsEquivalent(child->name, DAE_RADIUS2_ELEMENT))
		{
			const char* stringRadius = ReadNodeContentDirect(child);
			pASTaperedCapsule->radius2.x = FUStringConversion::ToFloat(&stringRadius);
			pASTaperedCapsule->radius2.y = FUStringConversion::ToFloat(&stringRadius);
		}
	}

	pASTaperedCapsule->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadPASCylinder(FCDObject* object, xmlNode* node)
{
	FCDPASCylinder* pASCylinder = (FCDPASCylinder*)object;

	bool status = true;

	if (!IsEquivalent(node->name, DAE_CYLINDER_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_SPHERE_TYPE, node->line);
		return status;
	}

	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_HEIGHT_ELEMENT))
		{
			pASCylinder->height = FUStringConversion::ToFloat(ReadNodeContentDirect(child));
		}
		else if (IsEquivalent(child->name, DAE_RADIUS_ELEMENT))
		{
			const char* stringRadius = ReadNodeContentDirect(child);
			pASCylinder->radius.x = FUStringConversion::ToFloat(&stringRadius);
			pASCylinder->radius.y = FUStringConversion::ToFloat(&stringRadius);
		}
	}

	pASCylinder->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadPASTaperedCylinder(FCDObject* object, xmlNode* node)
{
	FCDPASTaperedCylinder* pASTaperedCylinder = (FCDPASTaperedCylinder*)object;

	bool status = true;

	if (!IsEquivalent(node->name, DAE_TAPERED_CYLINDER_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_TCYLINDER_TYPE, node->line);
		return status;
	}

	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;
		
		if (IsEquivalent(child->name, DAE_HEIGHT_ELEMENT))
		{
			const char* h = ReadNodeContentDirect(child);
			pASTaperedCylinder->height = FUStringConversion::ToFloat(&h);
		}
		else if (IsEquivalent(child->name, DAE_RADIUS1_ELEMENT))
		{
			const char* stringRadius = ReadNodeContentDirect(child);
			pASTaperedCylinder->radius.x = FUStringConversion::ToFloat(&stringRadius);
			pASTaperedCylinder->radius.y = FUStringConversion::ToFloat(&stringRadius);
		}
		else if (IsEquivalent(child->name, DAE_RADIUS2_ELEMENT))
		{
			const char* stringRadius = ReadNodeContentDirect(child);
			pASTaperedCylinder->radius2.x = FUStringConversion::ToFloat(&stringRadius);
			pASTaperedCylinder->radius2.y = FUStringConversion::ToFloat(&stringRadius);
		}
	}

	pASTaperedCylinder->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadPASPlane(FCDObject* object, xmlNode* node)
{
	FCDPASPlane* pASPlane = (FCDPASPlane*)object;

	bool status = true;

	if (!IsEquivalent(node->name, DAE_PLANE_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_PLANE_TYPE, node->line);
		return status;
	}

	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_EQUATION_ELEMENT))
		{
			const char* eq = ReadNodeContentDirect(child);
			pASPlane->normal.x = FUStringConversion::ToFloat(&eq);
			pASPlane->normal.y = FUStringConversion::ToFloat(&eq);
			pASPlane->normal.z = FUStringConversion::ToFloat(&eq);
			pASPlane->d = FUStringConversion::ToFloat(&eq);
		}
	}

	pASPlane->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadPASSphere(FCDObject* object, xmlNode* node)
{
	FCDPASSphere* pASSphere = (FCDPASSphere*)object;

	bool status = true;

	if (!IsEquivalent(node->name, DAE_SPHERE_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_SPHERE_TYPE, node->line);
		return status;
	}

	for (xmlNode* child = node->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_RADIUS_ELEMENT))
		{
			pASSphere->radius = FUStringConversion::ToFloat(ReadNodeContentDirect(child));
		}
	}

	pASSphere->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadPhysicsMaterial(FCDObject* object, xmlNode* physicsMaterialNode)
{
	if (!FArchiveXML::LoadEntity(object, physicsMaterialNode)) return false;

	bool status = true;
	FCDPhysicsMaterial* physicsMaterial = (FCDPhysicsMaterial*)object;
	if (!IsEquivalent(physicsMaterialNode->name, DAE_PHYSICS_MATERIAL_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_PHYS_MAT_LIB_ELEMENT, physicsMaterialNode->line);
		return status;
	}

	// Read in the <technique_common> element
	xmlNode* commonTechniqueNode = FindChildByType(physicsMaterialNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	if (commonTechniqueNode == NULL)
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_COMMON_TECHNIQUE_MISSING, physicsMaterialNode->line);
	}

	xmlNode* paramNode = FindChildByType(commonTechniqueNode, DAE_PHYSICS_STATIC_FRICTION);
	if (paramNode != NULL) 
	{
		const char* content = ReadNodeContentDirect(paramNode);
		physicsMaterial->SetStaticFriction(FUStringConversion::ToFloat(content));
	}

	paramNode = FindChildByType(commonTechniqueNode, DAE_PHYSICS_DYNAMIC_FRICTION);
	if (paramNode != NULL) 
	{
		const char* content = ReadNodeContentDirect(paramNode);
		physicsMaterial->SetDynamicFriction(FUStringConversion::ToFloat(content));
	}

	paramNode = FindChildByType(commonTechniqueNode, DAE_PHYSICS_RESTITUTION);
	if (paramNode != NULL)
	{
		const char* content = ReadNodeContentDirect(paramNode);
		physicsMaterial->SetRestitution(FUStringConversion::ToFloat(content));
	}

	physicsMaterial->SetDirtyFlag(); 
	return status;
}

bool FArchiveXML::LoadPhysicsModel(FCDObject* object, xmlNode* physicsModelNode)
{
	if (!FArchiveXML::LoadEntity(object, physicsModelNode)) return false;

	bool status = true;
	FCDPhysicsModel* physicsModel = (FCDPhysicsModel*)object;
	FCDPhysicsModelData& data = FArchiveXML::documentLinkDataMap[physicsModel->GetDocument()].physicsModelDataMap[physicsModel];
	if (!IsEquivalent(physicsModelNode->name, DAE_PHYSICS_MODEL_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_PHYS_LIB_ELEMENT, physicsModelNode->line);
		return status;
	}

	// Read in the first valid child element found
	for (xmlNode* child = physicsModelNode->children; child != NULL; child = child->next)
	{
		if (child->type != XML_ELEMENT_NODE) continue;

		if (IsEquivalent(child->name, DAE_RIGID_BODY_ELEMENT))
		{
			FCDPhysicsRigidBody* rigidBody = physicsModel->AddRigidBody();
			status &= (FArchiveXML::LoadPhysicsRigidBody(rigidBody, child));
		}
		else if (IsEquivalent(child->name, DAE_RIGID_CONSTRAINT_ELEMENT))
		{
			FCDPhysicsRigidConstraint* rigidConstraint = physicsModel->AddRigidConstraint();
			status &= (FArchiveXML::LoadPhysicsRigidConstraint(rigidConstraint, child));
		}
		else if (IsEquivalent(child->name, DAE_INSTANCE_PHYSICS_MODEL_ELEMENT))
		{
			data.modelInstancesMap.insert(child, ReadNodeUrl(child));
		}
	}

	physicsModel->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadPhysicsRigidBody(FCDObject* object, xmlNode* physicsRigidBodyNode)
{
	if (!FArchiveXML::LoadEntity(object, physicsRigidBodyNode)) return false;

	bool status = true;
	FCDPhysicsRigidBody* physicsRigidBody = (FCDPhysicsRigidBody*)object;
	if (!IsEquivalent(physicsRigidBodyNode->name, DAE_RIGID_BODY_ELEMENT)) 
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_PRB_LIB_ELEMENT, physicsRigidBodyNode->line);
		return status;
	}

	physicsRigidBody->SetSubId(FUDaeParser::ReadNodeSid(physicsRigidBodyNode));

	xmlNode* techniqueNode = FindChildByType(physicsRigidBodyNode, 
			DAE_TECHNIQUE_COMMON_ELEMENT);
	if (techniqueNode != NULL)
	{
		FArchiveXML::LoadPhysicsRigidBodyParameters(physicsRigidBody->GetParameters(), techniqueNode);
	}
	else
	{
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_COMMON_TECHNIQUE_MISSING,
				physicsRigidBodyNode->line);
	}

	return status;
}

bool FArchiveXML::LoadPhysicsRigidConstraint(FCDObject* object, xmlNode* physicsRigidConstraintNode)
{
	if (!FArchiveXML::LoadEntity(object, physicsRigidConstraintNode)) return false;

	bool status = true;
	FCDPhysicsRigidConstraint* physicsRigidConstraint = (FCDPhysicsRigidConstraint*)object;
	if (!IsEquivalent(physicsRigidConstraintNode->name, DAE_RIGID_CONSTRAINT_ELEMENT))
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_UNKNOWN_RGC_LIB_ELEMENT, physicsRigidConstraintNode->line);
		return status;
	}

	physicsRigidConstraint->SetSubId(FUDaeParser::ReadNodeSid(physicsRigidConstraintNode));

#define PARSE_TRANSFORM(node, className, nodeName, transforms) { \
	xmlNodeList transformNodes; \
	FindChildrenByType(node, nodeName, transformNodes); \
	for (xmlNodeList::iterator itT = transformNodes.begin(); itT != transformNodes.end(); ++itT) \
	{ \
		if (IsEquivalent((*itT)->name, nodeName)) { \
			className* transform = new className(physicsRigidConstraint->GetDocument(), NULL); \
			transforms.push_back(transform); \
			status = FArchiveXML::LoadSwitch(transform, &transform->GetObjectType(), *itT); \
			if (!status) { \
				FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_INVALID_NODE_TRANSFORM, (*itT)->line);} \
		} \
	}  }



	//Reference-frame body
	xmlNode* referenceBodyNode = FindChildByType(physicsRigidConstraintNode, DAE_REF_ATTACHMENT_ELEMENT);
	if (referenceBodyNode == NULL)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_RF_NODE_MISSING, physicsRigidConstraintNode->line);
	}
	fm::string strRigidBody = ReadNodeProperty(referenceBodyNode, DAE_RIGID_BODY_ELEMENT);
	physicsRigidConstraint->SetReferenceRigidBody(physicsRigidConstraint->GetParent()->FindRigidBodyFromSid(strRigidBody));
	if (physicsRigidConstraint->GetReferenceRigidBody() == NULL)
	{
		physicsRigidConstraint->SetReferenceNode(physicsRigidConstraint->GetDocument()->FindSceneNode(strRigidBody));
		if ((physicsRigidConstraint->GetReferenceNode() == NULL) && (referenceBodyNode != NULL))
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_RF_REF_NODE_MISSING, referenceBodyNode->line);
		}
	}
	// Parse the node's transforms: <rotate>, <translate>
	PARSE_TRANSFORM(referenceBodyNode, FCDTRotation, DAE_ROTATE_ELEMENT, physicsRigidConstraint->GetTransformsRef())
	PARSE_TRANSFORM(referenceBodyNode, FCDTTranslation, DAE_TRANSLATE_ELEMENT, physicsRigidConstraint->GetTransformsRef())

	// target body
	xmlNode* bodyNode = FindChildByType(physicsRigidConstraintNode, DAE_ATTACHMENT_ELEMENT);
	if (bodyNode == NULL)
	{
		FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_TARGET_BS_NODE_MISSING, physicsRigidConstraintNode->line);
	}
	strRigidBody = ReadNodeProperty(bodyNode, DAE_RIGID_BODY_ELEMENT);
	physicsRigidConstraint->SetTargetRigidBody(physicsRigidConstraint->GetParent()->FindRigidBodyFromSid(strRigidBody));
	if (physicsRigidConstraint->GetTargetRigidBody() == NULL)
	{
		physicsRigidConstraint->SetTargetNode(physicsRigidConstraint->GetDocument()->FindSceneNode(strRigidBody));
		if ((physicsRigidConstraint->GetTargetNode() == NULL) && (bodyNode != NULL))
		{
			FUError::Error(FUError::WARNING_LEVEL, FUError::WARNING_TARGE_BS_REF_NODE_MISSING, bodyNode->line);
		}
	}
	// Parse the node's transforms: <rotate>, <scale>, <translate>
	PARSE_TRANSFORM(bodyNode, FCDTRotation, DAE_ROTATE_ELEMENT, physicsRigidConstraint->GetTransformsTar())
	PARSE_TRANSFORM(bodyNode, FCDTTranslation, DAE_TRANSLATE_ELEMENT, physicsRigidConstraint->GetTransformsTar())

#undef PARSE_TRANSFORM

	//technique_common
	xmlNode* techniqueNode = FindChildByType(physicsRigidConstraintNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	if (techniqueNode == NULL)
	{
		//return status.Fail(FS("Technique node not specified in rigid_constraint ") + TO_FSTRING(GetDaeId()), physicsRigidConstraintNode->line);
		FUError::Error(FUError::ERROR_LEVEL, FUError::ERROR_TECHNIQUE_NODE_MISSING, physicsRigidConstraintNode->line);
		return status;
	}

	xmlNode* enabledNode = FindChildByType(techniqueNode, DAE_ENABLED_ELEMENT);
	if (enabledNode != NULL)
	{
		physicsRigidConstraint->SetEnabled(FUStringConversion::ToBoolean(ReadNodeContentDirect(enabledNode)));
		FArchiveXML::LoadAnimatable(&physicsRigidConstraint->GetEnabled(), enabledNode);
	}
	xmlNode* interpenetrateNode = FindChildByType(techniqueNode, DAE_INTERPENETRATE_ELEMENT);
	if (interpenetrateNode != NULL)
	{
		physicsRigidConstraint->SetInterpenetrate(FUStringConversion::ToBoolean(ReadNodeContentDirect(interpenetrateNode)));
		FArchiveXML::LoadAnimatable(&physicsRigidConstraint->GetInterpenetrate(), interpenetrateNode);
	}

	xmlNode* limitsNode = FindChildByType(techniqueNode, DAE_LIMITS_ELEMENT);
	if (limitsNode != NULL)
	{
		xmlNode* linearNode = FindChildByType(limitsNode, DAE_LINEAR_ELEMENT);
		if (linearNode != NULL)
		{
			xmlNode* linearMinNode = FindChildByType(linearNode, DAE_MIN_ELEMENT);
			if (linearMinNode != NULL)
			{
				const char* min = ReadNodeContentDirect(linearMinNode);
				physicsRigidConstraint->SetLimitsLinearMin(FUStringConversion::ToVector3(min));
			}
			xmlNode* linearMaxNode = FindChildByType(linearNode, DAE_MAX_ELEMENT);
			if (linearMaxNode != NULL)
			{
				const char* max = ReadNodeContentDirect(linearMaxNode);
				physicsRigidConstraint->SetLimitsLinearMax(FUStringConversion::ToVector3(max));
			}
		}

		xmlNode* sctNode = FindChildByType(limitsNode, DAE_SWING_CONE_AND_TWIST_ELEMENT);
		if (sctNode != NULL)
		{
			xmlNode* sctMinNode = FindChildByType(sctNode, DAE_MIN_ELEMENT);
			if (sctMinNode != NULL)
			{
				const char* min = ReadNodeContentDirect(sctMinNode);
				physicsRigidConstraint->SetLimitsSCTMin(FUStringConversion::ToVector3(min));
			}
			xmlNode* sctMaxNode = FindChildByType(sctNode, DAE_MAX_ELEMENT);
			if (sctMaxNode != NULL)
			{
				const char* max = ReadNodeContentDirect(sctMaxNode);
				physicsRigidConstraint->SetLimitsSCTMax(FUStringConversion::ToVector3(max));
			}
		}
	}

	xmlNode* spring = FindChildByType(physicsRigidConstraintNode, DAE_SPRING_ELEMENT);
	if (spring)
	{
		xmlNode* linearSpring = FindChildByType(spring, DAE_LINEAR_ELEMENT);
		if (linearSpring)
		{
			xmlNode* param = FindChildByType(linearSpring, DAE_DAMPING_ELEMENT);
			if (param) physicsRigidConstraint->SetSpringLinearDamping(FUStringConversion::ToFloat(ReadNodeContentDirect(param)));
			param = FindChildByType(linearSpring, DAE_STIFFNESS_ELEMENT);
			if (param) physicsRigidConstraint->SetSpringLinearStiffness(FUStringConversion::ToFloat(ReadNodeContentDirect(param)));
			param = FindChildByType(linearSpring, DAE_TARGET_VALUE_ELEMENT);
			if (!param) param = FindChildByType(linearSpring, DAE_REST_LENGTH_ELEMENT1_3); // COLLADA 1.3 backward compatibility
			if (param) physicsRigidConstraint->SetSpringLinearTargetValue(FUStringConversion::ToFloat(ReadNodeContentDirect(param)));
		}

		xmlNode* angularSpring = FindChildByType(spring, DAE_ANGULAR_ELEMENT);
		if (angularSpring)
		{
			xmlNode* param = FindChildByType(angularSpring, DAE_DAMPING_ELEMENT);
			if (param) physicsRigidConstraint->SetSpringAngularDamping(FUStringConversion::ToFloat(ReadNodeContentDirect(param)));
			param = FindChildByType(angularSpring, DAE_STIFFNESS_ELEMENT);
			if (param) physicsRigidConstraint->SetSpringAngularStiffness(FUStringConversion::ToFloat(ReadNodeContentDirect(param)));
			param = FindChildByType(angularSpring, DAE_TARGET_VALUE_ELEMENT);
			if (!param) param = FindChildByType(angularSpring, DAE_REST_LENGTH_ELEMENT1_3); // COLLADA 1.3 backward compatibility
			if (param) physicsRigidConstraint->SetSpringAngularTargetValue(FUStringConversion::ToFloat(ReadNodeContentDirect(param)));
		}
	}

	physicsRigidConstraint->SetDirtyFlag();
	return status;
}

bool FArchiveXML::LoadPhysicsScene(FCDObject* object, xmlNode* sceneNode)
{
	if (!FArchiveXML::LoadEntity(object, sceneNode)) return false;

	bool status = true;
	FCDPhysicsScene* physicsScene = (FCDPhysicsScene*)object;
	if (IsEquivalent(sceneNode->name, DAE_PHYSICS_SCENE_ELEMENT))
	{
		for (xmlNode* child = sceneNode->children; child != NULL; child = child->next)
		{
			if (child->type != XML_ELEMENT_NODE) continue;

			// Look for instantiation elements
			if (IsEquivalent(child->name, DAE_INSTANCE_PHYSICS_MODEL_ELEMENT)) 
			{
				FCDPhysicsModelInstance* instance = physicsScene->AddPhysicsModelInstance(NULL);
				status &= (FArchiveXML::LoadPhysicsModelInstance(instance, child));
				continue; 
			}
			else if (IsEquivalent(child->name, DAE_TECHNIQUE_COMMON_ELEMENT))
			{
				xmlNode* gravityNode = FindChildByType(child, DAE_GRAVITY_ATTRIBUTE);
				if (gravityNode)
				{
					const char* gravityVal = ReadNodeContentDirect(gravityNode);
					FMVector3 gravity;
					gravity.x = FUStringConversion::ToFloat(&gravityVal);
					gravity.y = FUStringConversion::ToFloat(&gravityVal);
					gravity.z = FUStringConversion::ToFloat(&gravityVal);
					physicsScene->SetGravity(gravity);
				}
				xmlNode* timestepNode = FindChildByType(child, DAE_TIME_STEP_ATTRIBUTE);
				if (timestepNode)
				{
					physicsScene->SetTimestep(FUStringConversion::ToFloat(ReadNodeContentDirect(timestepNode)));
				}
			}
			else if (IsEquivalent(child->name, 
					DAE_INSTANCE_FORCE_FIELD_ELEMENT))
			{
				FCDPhysicsForceFieldInstance* instance = physicsScene->AddForceFieldInstance(NULL);
				status &= (FArchiveXML::LoadPhysicsForceFieldInstance(instance, child));
			}
			else if (IsEquivalent(child->name, DAE_EXTRA_ELEMENT))
			{
				// The extra information is loaded by the FCDEntity class.
			}
		}
	}

	physicsScene->SetDirtyFlag();
	return status;
}

