/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUBoundingBox.h"
#include "FUBoundingSphere.h"

#define FUBOUNDINGBOX_COORDINATE_COUNT 8
#define FUBOUNDINGBOX_COORDINATES(coordinates, boundingBox) \
	FMVector3 coordinates[FUBOUNDINGBOX_COORDINATE_COUNT] = { \
		boundingBox.GetMin(), \
		FMVector3(boundingBox.GetMin().x, boundingBox.GetMin().y, boundingBox.GetMax().z), \
		FMVector3(boundingBox.GetMin().x, boundingBox.GetMax().y, boundingBox.GetMin().z), \
		FMVector3(boundingBox.GetMax().x, boundingBox.GetMin().y, boundingBox.GetMin().z), \
		FMVector3(boundingBox.GetMin().x, boundingBox.GetMax().y, boundingBox.GetMax().z), \
		FMVector3(boundingBox.GetMax().x, boundingBox.GetMin().y, boundingBox.GetMax().z), \
		FMVector3(boundingBox.GetMax().x, boundingBox.GetMax().y, boundingBox.GetMin().z), \
		boundingBox.GetMax() }

//
// FUBoundingSphere
//

FUBoundingSphere::FUBoundingSphere()
:	center(FMVector3::Origin), radius(-1.0f)
{
}

FUBoundingSphere::FUBoundingSphere(const FMVector3& _center, float _radius)
:	center(_center), radius(_radius)
{
}

FUBoundingSphere::FUBoundingSphere(const FUBoundingSphere& copy)
:	center(copy.center), radius(copy.radius)
{
}

FUBoundingSphere::~FUBoundingSphere()
{
}

void FUBoundingSphere::Reset()
{
	center = FMVector3::Origin;
	radius = -1.0f;
}

bool FUBoundingSphere::IsValid() const
{
	return radius >= 0.0f;
}

bool FUBoundingSphere::Contains(const FMVector3& point) const
{
	if (radius >= 0.0f)
	{
		float a = (center-point).LengthSquared();
		float b = (radius*radius);
		return (a < b) || IsEquivalent(a, b);
	}
	else return false;
}

bool FUBoundingSphere::Overlaps(const FUBoundingSphere& boundingSphere, FMVector3* overlapCenter) const
{
	if (radius >= 0.0f)
	{
		FMVector3 centerToCenter = center - boundingSphere.center;
		float distanceSquared = centerToCenter.LengthSquared();
		bool overlaps = distanceSquared < (radius + boundingSphere.radius) * (radius + boundingSphere.radius);
		if (overlaps && overlapCenter != NULL)
		{
			float distance = sqrtf(distanceSquared);
			float overlapDistance = (radius + boundingSphere.radius) - distance;
			float smallerRadius = min(radius, boundingSphere.radius);
			overlapDistance = min(2.0f * smallerRadius, overlapDistance);
			(*overlapCenter) = center + centerToCenter / distance * (radius - overlapDistance / 2.0f);
		}
		return overlaps;
	}
	else return false;
}

bool FUBoundingSphere::Overlaps(const FUBoundingBox& boundingBox, FMVector3* overlapCenter) const
{
	if (radius >= 0.0f)
	{
		float rx, ry, rz;
		if (center.x > boundingBox.GetMax().x) rx = boundingBox.GetMax().x - center.x;
		else if (center.x > boundingBox.GetMin().x) rx = 0.0f;
		else rx = boundingBox.GetMin().x - center.x;
		if (center.y > boundingBox.GetMax().y) ry = boundingBox.GetMax().y - center.y;
		else if (center.y > boundingBox.GetMin().y) ry = 0.0f;
		else ry = boundingBox.GetMin().y - center.y;
		if (center.z > boundingBox.GetMax().z) rz = boundingBox.GetMax().z - center.z;
		else if (center.z > boundingBox.GetMin().z) rz = 0.0f;
		else rz = boundingBox.GetMin().z - center.z;
		bool overlaps = (rx * rx + ry * ry + rz * rz) < (radius * radius);
		if (overlaps && overlapCenter != NULL)
		{
			(*overlapCenter) = center + FMVector3(rx, ry, rz);
		}
		return overlaps;
	}
	return false;
}

void FUBoundingSphere::Include(const FMVector3& point)
{
	if (radius >= 0.0f)
	{
		float distanceSquared = (center - point).LengthSquared();
		if (distanceSquared > (radius * radius))
		{
			radius = sqrtf(distanceSquared);
		}
	}
	else
	{
		center = point;
		radius = 0.0f;
	}
}

void FUBoundingSphere::Include(const FUBoundingSphere& boundingSphere)
{
	if (radius >= 0.0f)
	{
		float distance = (center - boundingSphere.center).Length();
		if (distance + boundingSphere.radius > radius)
		{
			center = ((radius + distance / 2.0f) * center + (boundingSphere.radius + distance / 2.0f) * boundingSphere.center) / (radius + boundingSphere.radius + distance);
			radius = (radius + boundingSphere.radius + distance) / 2.0f;
		}
	}
	else
	{
		center = boundingSphere.center;
		radius = boundingSphere.radius;
	}
}

void FUBoundingSphere::Include(const FUBoundingBox& boundingBox)
{
	if (radius >= 0.0f)
	{
		FUBOUNDINGBOX_COORDINATES(coordinates, boundingBox);
		for (size_t i = 0; i < FUBOUNDINGBOX_COORDINATE_COUNT; ++i)
		{
			Include(coordinates[i]);
		}
	}
	else
	{
		center = boundingBox.GetCenter();
		radius = (boundingBox.GetMax() - center).Length();
	}
}

FUBoundingSphere FUBoundingSphere::Transform(const FMMatrix44& transform) const
{
	if (!IsValid()) return (*this);

	FMVector3 transformedCenter = transform.TransformCoordinate(center);
	FUBoundingSphere transformedSphere(transformedCenter, 0.0f);

	// Calculate the transformed bounding sphere radius using three sample points.
	FMVector3 testPoints[3] =
	{
		FMVector3(radius, 0.0f, 0.0f),
		FMVector3(0.0f, radius, 0.0f),
		FMVector3(0.0f, 0.0f, radius)
	};

	for (size_t i = 0; i < 6; ++i)
	{
		testPoints[i] = transform.TransformVector(testPoints[i]);
		float lengthSquared = testPoints[i].LengthSquared();
		if (lengthSquared > transformedSphere.radius * transformedSphere.radius)
		{
			transformedSphere.radius = sqrtf(lengthSquared);
		}
	}

	return transformedSphere;
}
