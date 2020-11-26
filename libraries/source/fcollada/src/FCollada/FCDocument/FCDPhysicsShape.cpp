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
#include "FCDocument/FCDGeometryInstance.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDPhysicsAnalyticalGeometry.h"
#include "FCDocument/FCDPhysicsMaterial.h"
#include "FCDocument/FCDPhysicsRigidBody.h"
#include "FCDocument/FCDPhysicsShape.h"
#include "FCDocument/FCDTransform.h"
#include "FUtils/FUBoundingBox.h"

ImplementObjectType(FCDPhysicsShape);

FCDPhysicsShape::FCDPhysicsShape(FCDocument* document) : FCDObject(document)
{
	hollow = true; // COLLADA 1.4.1 no default specified
	physicsMaterial = NULL;
	ownsPhysicsMaterial = false;
	isDensityMoreAccurate = false;
	geometry = NULL;
	analGeom = NULL;
	mass = NULL;
	density = NULL;
	instanceMaterialRef = NULL;
}

FCDPhysicsShape::~FCDPhysicsShape()
{
	SetPhysicsMaterial(NULL);
	SAFE_DELETE(mass);
	SAFE_DELETE(density);
	SAFE_RELEASE(instanceMaterialRef);

	if (ownsPhysicsMaterial) SAFE_RELEASE(physicsMaterial);
	SAFE_RELEASE(geometry);
	geometry = NULL;
}

FCDTransform* FCDPhysicsShape::AddTransform(FCDTransform::Type type, size_t index)
{
	FCDTransform* transform = FCDTFactory::CreateTransform(GetDocument(), NULL, type);
	if (transform != NULL)
	{
		if (index > transforms.size()) transforms.push_back(transform);
		else transforms.insert(transforms.begin() + index, transform);
	}
	SetNewChildFlag();
	return transform;
}

FCDPhysicsMaterial* FCDPhysicsShape::AddOwnPhysicsMaterial()
{
	if (ownsPhysicsMaterial) SAFE_RELEASE(physicsMaterial);

	physicsMaterial = new FCDPhysicsMaterial(GetDocument());
	ownsPhysicsMaterial = true;
	SetNewChildFlag();
	return physicsMaterial;
}

void FCDPhysicsShape::SetPhysicsMaterial(FCDPhysicsMaterial* _physicsMaterial)
{
	if (ownsPhysicsMaterial) SAFE_RELEASE(physicsMaterial);
	ownsPhysicsMaterial = false;
	physicsMaterial = _physicsMaterial;
	SetNewChildFlag();
}

FCDGeometryInstance* FCDPhysicsShape::CreateGeometryInstance(FCDGeometry* geom, bool createConvexMesh)
{
	analGeom = NULL;
	SAFE_RELEASE(geometry);
	
	geometry = (FCDGeometryInstance*)FCDEntityInstanceFactory::CreateInstance(GetDocument(), NULL, FCDEntity::GEOMETRY);

	if (createConvexMesh)
	{
		FCDGeometry* convexHullGeom = GetDocument()->GetGeometryLibrary()->AddEntity();
		fm::string convexId = geom->GetDaeId()+"-convex";
		convexHullGeom->SetDaeId(convexId);
		convexHullGeom->SetName(FUStringConversion::ToFString(convexId));
		FCDGeometryMesh* convexHullGeomMesh = convexHullGeom->CreateMesh();
		convexHullGeomMesh->SetConvexHullOf(geom);
		convexHullGeomMesh->SetConvex(true);
		geometry->SetEntity(convexHullGeom);
	}
	else
	{
		geometry->SetEntity(geom);
	}

	SetNewChildFlag();
	return geometry;
}

FCDPhysicsAnalyticalGeometry* FCDPhysicsShape::CreateAnalyticalGeometry(FCDPhysicsAnalyticalGeometry::GeomType type)
{
	SAFE_RELEASE(geometry);
	analGeom = FCDPASFactory::CreatePAS(GetDocument(), type);
	SetNewChildFlag();
	return analGeom;
}

// Create a copy of this shape
// Note: geometries are just shallow-copied
FCDPhysicsShape* FCDPhysicsShape::Clone(FCDPhysicsShape* clone) const
{
	if (clone == NULL) clone = new FCDPhysicsShape(const_cast<FCDocument*>(GetDocument()));

	if (mass != NULL) clone->SetMass(*mass);
	if (density != NULL) clone->SetDensity(*density);
	clone->SetHollow(hollow);

	// Clone the material instance
	if (instanceMaterialRef != NULL)
	{
		clone->instanceMaterialRef = FCDEntityInstanceFactory::CreateInstance(clone->GetDocument(), NULL, FCDEntity::PHYSICS_MATERIAL);
		instanceMaterialRef->Clone(instanceMaterialRef);
	}
	if (physicsMaterial != NULL)
	{
		FCDPhysicsMaterial* clonedMaterial = clone->AddOwnPhysicsMaterial();
		physicsMaterial->Clone(clonedMaterial);
	}

	// Clone the analytical geometry or the mesh geometry
	if (analGeom != NULL)
	{
		clone->analGeom = FCDPASFactory::CreatePAS(clone->GetDocument(), analGeom->GetGeomType());
		analGeom->Clone(clone->analGeom);
	}
	if (geometry != NULL)
	{
		clone->geometry = (FCDGeometryInstance*)FCDEntityInstanceFactory::CreateInstance(clone->GetDocument(), NULL, geometry->GetEntityType());
		geometry->Clone(clone->geometry);
	}

	// Clone the shape placement transform
	for (size_t i = 0; i < transforms.size(); ++i)
	{
		FCDTransform* clonedTransform = clone->AddTransform(transforms[i]->GetType());
		transforms[i]->Clone(clonedTransform);
	}

	return clone;
}

float FCDPhysicsShape::GetMass() const
{
	if (mass) 
		return *mass;
		
	return 0.f;
}

void FCDPhysicsShape::SetMass(float _mass)
{
	SAFE_DELETE(mass);
	mass = new float;
	*mass = _mass;
	SetDirtyFlag();
}

float FCDPhysicsShape::GetDensity()  const
{
	if (density) 
		return *density; 

	return 0.f;
}

void FCDPhysicsShape::SetDensity(float _density) 
{
	SAFE_DELETE(density);
	density = new float;
	*density = _density;
	SetDirtyFlag();
}

float FCDPhysicsShape::CalculateVolume() const
{
	if (IsGeometryInstance())
	{
		FCDGeometry* geom = ((FCDGeometry*)geometry->GetEntity());
		if (geom->IsMesh())
		{
			FUBoundingBox boundary;
			float countingVolume = 0.0f;
			const FCDGeometryMesh* mesh = geom->GetMesh();

			if (!mesh->GetConvexHullOf().empty())
			{
				mesh = mesh->FindConvexHullOfMesh();
			}
			if (mesh == NULL) return 1.0f; // missing convex hull or of spline

			for (size_t i = 0; i < mesh->GetPolygonsCount(); i++)
			{
				const FCDGeometryPolygons* polygons = mesh->GetPolygons(i);
				const FCDGeometryPolygonsInput* positionInput = polygons->FindInput(FUDaeGeometryInput::POSITION);
				const FCDGeometrySource* positionSource = positionInput->GetSource();
				uint32 positionStride = positionSource->GetStride();
				FUAssert(positionStride == 3, continue;);
				const float* positionData = positionSource->GetData();
				size_t positionDataLength = positionSource->GetDataCount();
				for (size_t pos = 0; pos < positionDataLength;)
				{
					boundary.Include(FMVector3(positionData, (uint32)pos));
					pos += positionStride;
				}

				FMVector3 min = boundary.GetMin();
				FMVector3 max = boundary.GetMax();
				countingVolume += 
						(max.x - min.x) * (max.y - min.y) * (max.z - min.z);
				boundary.Reset();
			}
			return countingVolume;
		}
		// splines have no volume!
		return 1.0f;
	}
	else
	{
		FUAssert(IsAnalyticalGeometry(), return 1.0f;);
		return (analGeom->CalculateVolume());
	}
}
