/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDPhysicsAnalyticalGeometry.h"
#include "FCDocument/FCDocument.h"
#include "FMath/FMVolume.h"

//
// FCDPhysicsAnalyticalGeometry
//

ImplementObjectType(FCDPhysicsAnalyticalGeometry);

FCDPhysicsAnalyticalGeometry::FCDPhysicsAnalyticalGeometry(FCDocument* document)
:	FCDEntity(document, "AnalyticalGeometry")
{
}

FCDPhysicsAnalyticalGeometry::~FCDPhysicsAnalyticalGeometry()
{
}

FCDEntity* FCDPhysicsAnalyticalGeometry::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	// FCDPhysicsAnalyticalGeometry has no data values to clone.
	// It is also abstract and cannot be created if (_clone == NULL).
	return Parent::Clone(_clone, cloneChildren);
}

//
// FCDPASBox
//

ImplementObjectType(FCDPASBox);

FCDPASBox::FCDPASBox(FCDocument* document)
:	FCDPhysicsAnalyticalGeometry(document)
{
	halfExtents.x = halfExtents.y = halfExtents.z = 0.f;
}

FCDEntity* FCDPASBox::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPASBox* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPASBox(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPASBox::GetClassType())) clone = (FCDPASBox*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->halfExtents = halfExtents;
	}
	return _clone;
}

float FCDPASBox::CalculateVolume() const
{
	return FMVolume::CalculateBoxVolume(halfExtents);
}

//
// FCDPASPlane
//

ImplementObjectType(FCDPASPlane);

FCDPASPlane::FCDPASPlane(FCDocument* document) : FCDPhysicsAnalyticalGeometry(document)
{
	normal.x = normal.y = normal.z = d = 0.f;
}

FCDEntity* FCDPASPlane::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPASPlane* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPASPlane(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPASPlane::GetClassType())) clone = (FCDPASPlane*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->normal = normal;
	}
	return _clone;
}

float FCDPASPlane::CalculateVolume() const
{
	return 1.0f;
}

//
// FCDPASSphere
//

ImplementObjectType(FCDPASSphere);

FCDPASSphere::FCDPASSphere(FCDocument* document) : FCDPhysicsAnalyticalGeometry(document)
{
	radius = 0.f;
}

FCDEntity* FCDPASSphere::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPASSphere* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPASSphere(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPASSphere::GetClassType())) clone = (FCDPASSphere*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->radius = radius;
	}
	return _clone;
}

float FCDPASSphere::CalculateVolume() const
{
	return FMVolume::CalculateSphereVolume(radius);
}

//
// FCDPASCylinder
//

ImplementObjectType(FCDPASCylinder);

FCDPASCylinder::FCDPASCylinder(FCDocument* document) : FCDPhysicsAnalyticalGeometry(document)
{
	height = 0.f;
	radius.x = 0.f;
	radius.y = 0.f;
}

FCDEntity* FCDPASCylinder::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPASCylinder* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPASCylinder(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPASCylinder::GetClassType())) clone = (FCDPASCylinder*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->radius = radius;
		clone->height = height;
	}
	return _clone;
}

float FCDPASCylinder::CalculateVolume() const
{
	return FMVolume::CalculateCylinderVolume(radius, height);
}

//
// FCDPASCapsule
//

ImplementObjectType(FCDPASCapsule);

FCDPASCapsule::FCDPASCapsule(FCDocument* document) : FCDPhysicsAnalyticalGeometry(document)
{
	height = 0.f;
	radius.x = 0.f;
	radius.y = 0.f;
}

FCDEntity* FCDPASCapsule::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPASCapsule* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPASCapsule(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPASCapsule::GetClassType())) clone = (FCDPASCapsule*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->radius = radius;
		clone->height = height;
	}
	return _clone;
}

float FCDPASCapsule::CalculateVolume() const
{
	return FMVolume::CalculateCapsuleVolume(radius, height);
}

//
// FCDPASTaperedCapsule
//

ImplementObjectType(FCDPASTaperedCapsule);

FCDPASTaperedCapsule::FCDPASTaperedCapsule(FCDocument* document) : FCDPASCapsule(document)
{
	radius2.x = 0.f;
	radius2.y = 0.f;
}

FCDPhysicsAnalyticalGeometry* FCDPASTaperedCapsule::Clone(FCDPhysicsAnalyticalGeometry* _clone, bool cloneChildren) const
{
	FCDPASTaperedCapsule* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPASTaperedCapsule(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPASTaperedCapsule::GetClassType())) clone = (FCDPASTaperedCapsule*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->radius2 = radius2;
	}
	return _clone;
}

float FCDPASTaperedCapsule::CalculateVolume() const
{
	if (IsEquivalent(radius, radius2)) // this is a capsule
	{
		return FMVolume::CalculateCapsuleVolume(radius, height);
	}

	// 1 tapered cylinder + 1/2 ellipsoid + 1/2 other ellipsoid
	return FMVolume::CalculateTaperedCylinderVolume(radius, radius2, height) 
			+ FMVolume::CalculateEllipsoidEndVolume(radius) / 2.0f +
			FMVolume::CalculateEllipsoidEndVolume(radius2) / 2.0f;
}

//
// FCDPASTaperedCylinder
//

ImplementObjectType(FCDPASTaperedCylinder);

FCDPASTaperedCylinder::FCDPASTaperedCylinder(FCDocument* document) : FCDPASCylinder(document)
{
	radius2.x = 0.f;
	radius2.y = 0.f;
}

FCDEntity* FCDPASTaperedCylinder::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDPASTaperedCylinder* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPASTaperedCylinder(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDPASTaperedCylinder::GetClassType())) clone = (FCDPASTaperedCylinder*) _clone;

	Parent::Clone(clone, cloneChildren);

	if (clone != NULL)
	{
		clone->radius2 = radius2;
	}
	return _clone;
}

float FCDPASTaperedCylinder::CalculateVolume() const
{
	if (IsEquivalent(radius, radius2)) // this is a cylinder
	{
		return FMVolume::CalculateCylinderVolume(radius, height);
	}

	return FMVolume::CalculateTaperedCylinderVolume(radius, radius2, height);
}

//
// FCDPASFactory
//

FCDPhysicsAnalyticalGeometry* FCDPASFactory::CreatePAS(FCDocument* document, FCDPhysicsAnalyticalGeometry::GeomType type)
{
	switch (type)
	{
	case FCDPhysicsAnalyticalGeometry::BOX: return new FCDPASBox(document);
	case FCDPhysicsAnalyticalGeometry::PLANE: return new FCDPASPlane(document);
	case FCDPhysicsAnalyticalGeometry::SPHERE: return new FCDPASSphere(document);
	case FCDPhysicsAnalyticalGeometry::CYLINDER: return new FCDPASCylinder(document);
	case FCDPhysicsAnalyticalGeometry::CAPSULE: return new FCDPASCapsule(document);
	case FCDPhysicsAnalyticalGeometry::TAPERED_CYLINDER: return new FCDPASTaperedCylinder(document);
	case FCDPhysicsAnalyticalGeometry::TAPERED_CAPSULE: return new FCDPASTaperedCapsule(document);
	default: return NULL;
	}
}
