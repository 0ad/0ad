/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FArchiveXML.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDEntityInstance.h"
#include "FCDocument/FCDPhysicsForceFieldInstance.h"
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDPhysicsModelInstance.h"
#include "FCDocument/FCDPhysicsRigidBodyInstance.h"
#include "FCDocument/FCDPhysicsRigidBodyParameters.h"
#include "FCDocument/FCDPhysicsAnalyticalGeometry.h"
#include "FCDocument/FCDPhysicsMaterial.h"
#include "FCDocument/FCDPhysicsModel.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsRigidConstraint.h"
#include "FCDocument/FCDPhysicsScene.h"
#include "FCDocument/FCDPhysicsShape.h"

xmlNode* FArchiveXML::WritePhysicsShape(FCDObject* object, xmlNode* parentNode)
{
	FCDPhysicsShape* physicsShape = (FCDPhysicsShape*)object;

	xmlNode* physicsShapeNode = AddChild(parentNode, DAE_SHAPE_ELEMENT);

	AddChild(physicsShapeNode, DAE_HOLLOW_ELEMENT, physicsShape->IsHollow()?"true":"false");
	if (physicsShape->GetMass() && !physicsShape->IsDensityMoreAccurate())
		AddChild(physicsShapeNode, DAE_MASS_ELEMENT, FUStringConversion::ToString(physicsShape->GetMass()));
	if (physicsShape->GetDensity())
		AddChild(physicsShapeNode, DAE_DENSITY_ELEMENT, FUStringConversion::ToString(physicsShape->GetDensity()));

	if (physicsShape->OwnsPhysicsMaterial() && physicsShape->GetPhysicsMaterial())
	{
		xmlNode* materialNode = AddChild(physicsShapeNode, DAE_PHYSICS_MATERIAL_ELEMENT);
		FArchiveXML::LetWriteObject(physicsShape->GetPhysicsMaterial(), materialNode);
	}
	else if (physicsShape->GetInstanceMaterial())
	{
		FArchiveXML::LetWriteObject(physicsShape->GetInstanceMaterial(), physicsShapeNode);
	}
	
	if (physicsShape->GetGeometryInstance())
		FArchiveXML::LetWriteObject(physicsShape->GetGeometryInstance(), physicsShapeNode);
	if (physicsShape->GetAnalyticalGeometry())
		FArchiveXML::LetWriteObject(physicsShape->GetAnalyticalGeometry(), physicsShapeNode);

	for (size_t i = 0; i < physicsShape->GetTransforms().size(); ++i)
	{
		FArchiveXML::LetWriteObject(physicsShape->GetTransforms()[i], physicsShapeNode);
	}

	return physicsShapeNode;
}

xmlNode* FArchiveXML::WritePhysicsAnalyticalGeometry(FCDObject* object, xmlNode* UNUSED(parentNode))
{
	FCDPhysicsAnalyticalGeometry* physicsAnalyticalGeometry = (FCDPhysicsAnalyticalGeometry*)object;
	(void) physicsAnalyticalGeometry;

	//
	// Currently not reachable
	//
	FUBreak;
	return NULL;
}

xmlNode* FArchiveXML::WritePASBox(FCDObject* object, xmlNode* node)
{
	FCDPASBox* pASBox = (FCDPASBox*)object;

	xmlNode* geomNode = AddChild(node, DAE_BOX_ELEMENT);
	fm::string s = FUStringConversion::ToString(pASBox->halfExtents);
	AddChild(geomNode, DAE_HALF_EXTENTS_ELEMENT, s);
	return geomNode;
}

xmlNode* FArchiveXML::WritePASCapsule(FCDObject* object, xmlNode* node)
{
	FCDPASCapsule* pASCapsule = (FCDPASCapsule*)object;

	xmlNode* geomNode = AddChild(node, DAE_CAPSULE_ELEMENT);
	AddChild(geomNode, DAE_HEIGHT_ELEMENT, pASCapsule->height);
	AddChild(geomNode, DAE_RADIUS_ELEMENT, FUStringConversion::ToString(pASCapsule->radius));
	return geomNode;
}

xmlNode* FArchiveXML::WritePASTaperedCapsule(FCDObject* object, xmlNode* node)
{
	FCDPASTaperedCapsule* pASTaperedCapsule = (FCDPASTaperedCapsule*)object;

	xmlNode* geomNode = AddChild(node, DAE_TAPERED_CAPSULE_ELEMENT);
	AddChild(geomNode, DAE_HEIGHT_ELEMENT, pASTaperedCapsule->height);
	AddChild(geomNode, DAE_RADIUS1_ELEMENT, FUStringConversion::ToString(pASTaperedCapsule->radius));
	AddChild(geomNode, DAE_RADIUS2_ELEMENT, FUStringConversion::ToString(pASTaperedCapsule->radius2));
	return geomNode;
	
}

xmlNode* FArchiveXML::WritePASCylinder(FCDObject* object, xmlNode* node)
{
	FCDPASCylinder* pASCylinder = (FCDPASCylinder*)object;
	
	xmlNode* geomNode = AddChild(node, DAE_CYLINDER_ELEMENT);
	AddChild(geomNode, DAE_HEIGHT_ELEMENT, pASCylinder->height);
	AddChild(geomNode, DAE_RADIUS_ELEMENT, FUStringConversion::ToString(pASCylinder->radius));
	return geomNode;
}

xmlNode* FArchiveXML::WritePASTaperedCylinder(FCDObject* object, xmlNode* node)
{
	FCDPASTaperedCylinder* pASTaperedCylinder = (FCDPASTaperedCylinder*)object;

	xmlNode* geomNode = AddChild(node, DAE_TAPERED_CYLINDER_ELEMENT);
	AddChild(geomNode, DAE_HEIGHT_ELEMENT, pASTaperedCylinder->height);
	AddChild(geomNode, DAE_RADIUS1_ELEMENT, FUStringConversion::ToString(pASTaperedCylinder->radius));
	AddChild(geomNode, DAE_RADIUS2_ELEMENT, FUStringConversion::ToString(pASTaperedCylinder->radius2));
	return geomNode;
}

xmlNode* FArchiveXML::WritePASPlane(FCDObject* object, xmlNode* node)
{
	FCDPASPlane* pASPlane = (FCDPASPlane*)object;

	xmlNode* geomNode = AddChild(node, DAE_PLANE_ELEMENT);
	FMVector4 equation;
	equation.w = pASPlane->normal.x; equation.x = pASPlane->normal.y; equation.y = pASPlane->normal.z; equation.z = pASPlane->d;
	fm::string s = FUStringConversion::ToString(equation);
	AddChild(geomNode, DAE_EQUATION_ELEMENT, s);
	return geomNode;
}

xmlNode* FArchiveXML::WritePASSphere(FCDObject* object, xmlNode* node)
{
	FCDPASSphere* pASSphere = (FCDPASSphere*)object;

	xmlNode* geomNode = AddChild(node, DAE_SPHERE_ELEMENT);
	AddChild(geomNode, DAE_RADIUS_ELEMENT, pASSphere->radius);
	return geomNode;
}

xmlNode* FArchiveXML::WritePhysicsMaterial(FCDObject* object, xmlNode* parentNode)
{
	FCDPhysicsMaterial* physicsMaterial = (FCDPhysicsMaterial*)object;

	xmlNode* physicsMaterialNode = FArchiveXML::WriteToEntityXMLFCDEntity(physicsMaterial, parentNode, DAE_PHYSICS_MATERIAL_ELEMENT);
	xmlNode* commonTechniqueNode = AddChild(physicsMaterialNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	AddChild(commonTechniqueNode, DAE_PHYSICS_DYNAMIC_FRICTION, physicsMaterial->GetDynamicFriction());
	AddChild(commonTechniqueNode, DAE_PHYSICS_RESTITUTION, physicsMaterial->GetRestitution());
	AddChild(commonTechniqueNode, DAE_PHYSICS_STATIC_FRICTION, physicsMaterial->GetStaticFriction());

	FArchiveXML::WriteEntityExtra(physicsMaterial, physicsMaterialNode);
	return physicsMaterialNode;
}

xmlNode* FArchiveXML::WritePhysicsModel(FCDObject* object, xmlNode* parentNode)
{
	FCDPhysicsModel* physicsModel = (FCDPhysicsModel*)object;

	xmlNode* physicsModelNode = FArchiveXML::WriteToEntityXMLFCDEntity(physicsModel, parentNode, DAE_PHYSICS_MODEL_ELEMENT);
	for (size_t i = 0; i < physicsModel->GetInstanceCount(); ++i)
	{
		FArchiveXML::LetWriteObject(physicsModel->GetInstance(i), physicsModelNode);
	}
	for (size_t i = 0; i < physicsModel->GetRigidBodyCount(); ++i)
	{
		FArchiveXML::LetWriteObject(physicsModel->GetRigidBody(i), physicsModelNode);
	}
	for (size_t i = 0; i < physicsModel->GetRigidConstraintCount(); i++)
	{
		FArchiveXML::LetWriteObject(physicsModel->GetRigidConstraint(i), physicsModelNode);
	}

	FArchiveXML::WriteEntityExtra(physicsModel, physicsModelNode);
	return physicsModelNode;
}

xmlNode* FArchiveXML::WritePhysicsRigidBody(FCDObject* object, xmlNode* parentNode)
{
	FCDPhysicsRigidBody* physicsRigidBody = (FCDPhysicsRigidBody*)object;

	xmlNode* physicsRigidBodyNode = FArchiveXML::WriteToEntityXMLFCDEntity(physicsRigidBody, parentNode, DAE_RIGID_BODY_ELEMENT, false);
	physicsRigidBody->SetSubId(AddNodeSid(physicsRigidBodyNode, physicsRigidBody->GetSubId().c_str()));

	xmlNode* baseNode = AddChild(physicsRigidBodyNode, DAE_TECHNIQUE_COMMON_ELEMENT);

	FArchiveXML::WritePhysicsRigidBodyParameters(physicsRigidBody->GetParameters(), baseNode);

	FArchiveXML::WriteEntityExtra(physicsRigidBody, physicsRigidBodyNode);
	return physicsRigidBodyNode;
}

xmlNode* FArchiveXML::WritePhysicsRigidConstraint(FCDObject* object, xmlNode* parentNode)
{
	FCDPhysicsRigidConstraint* physicsRigidConstraint = (FCDPhysicsRigidConstraint*)object;

	xmlNode* physicsRigidConstraintNode = FArchiveXML::WriteToEntityXMLFCDEntity(physicsRigidConstraint, parentNode, DAE_RIGID_CONSTRAINT_ELEMENT, false);

	physicsRigidConstraint->SetSubId(AddNodeSid(physicsRigidConstraintNode, physicsRigidConstraint->GetSubId().c_str()));

	xmlNode* refNode = AddChild(physicsRigidConstraintNode, DAE_REF_ATTACHMENT_ELEMENT);
	fm::string referenceId = (physicsRigidConstraint->GetReferenceRigidBody() != NULL) ? physicsRigidConstraint->GetReferenceRigidBody()->GetSubId() : (physicsRigidConstraint->GetReferenceNode() != NULL) ? physicsRigidConstraint->GetReferenceNode()->GetDaeId() : "";
	AddAttribute(refNode, DAE_RIGID_BODY_ELEMENT, referenceId);
	for (FCDTransformContainer::iterator itT = physicsRigidConstraint->GetTransformsRef().begin(); itT != physicsRigidConstraint->GetTransformsRef().end(); ++itT)
	{
		FArchiveXML::LetWriteObject((*itT), refNode);
	}

	xmlNode* tarNode = AddChild(physicsRigidConstraintNode, DAE_ATTACHMENT_ELEMENT);
	fm::string targetId = (physicsRigidConstraint->GetTargetRigidBody() != NULL) ? physicsRigidConstraint->GetTargetRigidBody()->GetSubId() : (physicsRigidConstraint->GetTargetNode() != NULL) ? physicsRigidConstraint->GetTargetNode()->GetDaeId() : "";
	AddAttribute(tarNode, DAE_RIGID_BODY_ELEMENT, targetId);
	for (FCDTransformContainer::iterator itT = physicsRigidConstraint->GetTransformsTar().begin(); itT != physicsRigidConstraint->GetTransformsTar().end(); ++itT)
	{
		FArchiveXML::LetWriteObject((*itT), tarNode);
	}

	xmlNode* baseNode = AddChild(physicsRigidConstraintNode, DAE_TECHNIQUE_COMMON_ELEMENT);
	xmlNode* enabledNode = AddChild(baseNode, DAE_ENABLED_ELEMENT, physicsRigidConstraint->GetEnabled());
	if (physicsRigidConstraint->GetEnabled().IsAnimated())
	{
		FArchiveXML::WriteAnimatedValue(&physicsRigidConstraint->GetEnabled(), enabledNode, 
			!physicsRigidConstraint->GetSubId().empty() ? physicsRigidConstraint->GetSubId().c_str() : "constrain");
	}
	xmlNode* interpenetrateNode = AddChild(baseNode, DAE_INTERPENETRATE_ELEMENT, physicsRigidConstraint->GetInterpenetrate());
	if (physicsRigidConstraint->GetInterpenetrate().IsAnimated())
	{
		FArchiveXML::WriteAnimatedValue(&physicsRigidConstraint->GetInterpenetrate(), interpenetrateNode, 
			!physicsRigidConstraint->GetSubId().empty() ? physicsRigidConstraint->GetSubId().c_str() : "interpenetrate");
	}
	xmlNode* limitsNode = AddChild(baseNode, DAE_LIMITS_ELEMENT);
	
	xmlNode* sctNode = AddChild(limitsNode, DAE_SWING_CONE_AND_TWIST_ELEMENT);
	AddChild(sctNode, DAE_MIN_ELEMENT, (const FMVector3&) physicsRigidConstraint->GetLimitsSCTMin());
	AddChild(sctNode, DAE_MAX_ELEMENT, (const FMVector3&) physicsRigidConstraint->GetLimitsSCTMax());

	xmlNode* linearNode = AddChild(limitsNode, DAE_LINEAR_ELEMENT);
	AddChild(linearNode, DAE_MIN_ELEMENT, (FMVector3&) physicsRigidConstraint->GetLimitsLinearMin());
	AddChild(linearNode, DAE_MAX_ELEMENT, (FMVector3&) physicsRigidConstraint->GetLimitsLinearMax());

	xmlNode* springNode = AddChild(baseNode, DAE_SPRING_ELEMENT);
	xmlNode* sAngularNode = AddChild(springNode, DAE_ANGULAR_ELEMENT);
	AddChild(sAngularNode, DAE_STIFFNESS_ELEMENT, physicsRigidConstraint->GetSpringAngularStiffness());
	AddChild(sAngularNode, DAE_DAMPING_ELEMENT, physicsRigidConstraint->GetSpringAngularDamping());
	AddChild(sAngularNode, DAE_TARGET_VALUE_ELEMENT, physicsRigidConstraint->GetSpringAngularTargetValue());

	xmlNode* sLinearNode = AddChild(springNode, DAE_LINEAR_ELEMENT);
	AddChild(sLinearNode, DAE_STIFFNESS_ELEMENT, physicsRigidConstraint->GetSpringLinearStiffness());
	AddChild(sLinearNode, DAE_DAMPING_ELEMENT, physicsRigidConstraint->GetSpringLinearDamping());
	AddChild(sLinearNode, DAE_TARGET_VALUE_ELEMENT, physicsRigidConstraint->GetSpringLinearTargetValue());
	
	//FIXME: what about <technique> and <extra>?
	FArchiveXML::WriteEntityExtra(physicsRigidConstraint, physicsRigidConstraintNode);
	return physicsRigidConstraintNode;
}

xmlNode* FArchiveXML::WritePhysicsScene(FCDObject* object, xmlNode* parentNode)
{
	FCDPhysicsScene* physicsScene = (FCDPhysicsScene*)object;

	xmlNode* physicsSceneNode = FArchiveXML::WriteToEntityXMLFCDEntity(physicsScene, parentNode, DAE_PHYSICS_SCENE_ELEMENT);
	if (physicsSceneNode == NULL) return physicsSceneNode;
	
	// Write out the instantiation: force fields, then physics models
	for (size_t i = 0; i < physicsScene->GetForceFieldInstancesCount(); ++i)
	{
		FCDEntityInstance* instance = physicsScene->GetForceFieldInstance(i);
		FArchiveXML::LetWriteObject(instance, physicsSceneNode);
	}

	for (size_t i = 0; i < physicsScene->GetPhysicsModelInstancesCount(); ++i)
	{
		FCDEntityInstance* instance = physicsScene->GetPhysicsModelInstance(i);
		FArchiveXML::LetWriteObject(instance, physicsSceneNode);
	}

	// Add COMMON technique.
	xmlNode* techniqueNode = AddChild(physicsSceneNode, 
			DAE_TECHNIQUE_COMMON_ELEMENT);
	AddChild(techniqueNode, DAE_GRAVITY_ATTRIBUTE, TO_STRING(physicsScene->GetGravity()));
	AddChild(techniqueNode, DAE_TIME_STEP_ATTRIBUTE, physicsScene->GetTimestep());

	// Write out the extra information
	FArchiveXML::WriteEntityExtra(physicsScene, physicsSceneNode);

	return physicsSceneNode;
}


void FArchiveXML::WritePhysicsRigidBodyParameters(FCDPhysicsRigidBodyParameters* physicsRigidBodyParameters, xmlNode* techniqueNode)
{	
	FArchiveXML::AddPhysicsParameter(techniqueNode, DAE_DYNAMIC_ELEMENT, physicsRigidBodyParameters->GetDynamic());
	FArchiveXML::AddPhysicsParameter(techniqueNode, DAE_MASS_ELEMENT, physicsRigidBodyParameters->GetMass());
	xmlNode* massFrameNode = AddChild(techniqueNode, DAE_MASS_FRAME_ELEMENT);
	FArchiveXML::AddPhysicsParameter(massFrameNode, DAE_TRANSLATE_ELEMENT, physicsRigidBodyParameters->GetMassFrameTranslate());
	FMVector4 massFrameRotate(physicsRigidBodyParameters->GetMassFrameRotateAxis(), physicsRigidBodyParameters->GetMassFrameRotateAngle());
	AddChild(massFrameNode, DAE_ROTATE_ELEMENT, massFrameRotate);
	if (physicsRigidBodyParameters->IsInertiaAccurate())
	{
		FArchiveXML::AddPhysicsParameter(techniqueNode, DAE_INERTIA_ELEMENT, physicsRigidBodyParameters->GetInertia());
	}

	if (physicsRigidBodyParameters->GetPhysicsMaterial() != NULL)
	{
		if (physicsRigidBodyParameters->OwnsPhysicsMaterial())
		{
			FArchiveXML::LetWriteObject(physicsRigidBodyParameters->GetPhysicsMaterial(), techniqueNode);
		}
		else if (physicsRigidBodyParameters->GetInstanceMaterial() != NULL)
		{
			FArchiveXML::LetWriteObject(physicsRigidBodyParameters->GetInstanceMaterial(), techniqueNode);
		}
		else
		{
			xmlNode* instanceNode = AddChild(techniqueNode, 
					DAE_INSTANCE_PHYSICS_MATERIAL_ELEMENT);
			AddAttribute(instanceNode, DAE_URL_ATTRIBUTE, 
				fm::string("#") + physicsRigidBodyParameters->GetPhysicsMaterial()->GetDaeId());
		}
	}

	for (size_t i = 0; i < physicsRigidBodyParameters->GetPhysicsShapeCount(); ++i)
	{
		FArchiveXML::LetWriteObject(physicsRigidBodyParameters->GetPhysicsShape(i), techniqueNode);
	}
}

template <class TYPE, int QUAL>
xmlNode* FArchiveXML::AddPhysicsParameter(xmlNode* parentNode, const char* name, FCDParameterAnimatableT<TYPE,QUAL>& value)
{
	xmlNode* paramNode = AddChild(parentNode, name);
	AddContent(paramNode, FUStringConversion::ToString((TYPE&) value));
	if (value.IsAnimated())
	{
		const FCDAnimated* animated = value.GetAnimated();
		FArchiveXML::WriteAnimatedValue(animated, paramNode, name);
	}
	return paramNode;
}
