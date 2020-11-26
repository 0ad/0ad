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
// FUBoundingBox
//

// static variables initialization
const FUBoundingBox FUBoundingBox::Infinity(FMVector3(-FLT_MAX, -FLT_MAX, -FLT_MAX), FMVector3(FLT_MAX, FLT_MAX, FLT_MAX));

FUBoundingBox::FUBoundingBox()
:	minimum(FLT_MAX, FLT_MAX, FLT_MAX)
,	maximum(-FLT_MAX, -FLT_MAX, -FLT_MAX)
{
}

FUBoundingBox::FUBoundingBox(const FMVector3& _min, const FMVector3& _max)
:	minimum(_min), maximum(_max)
{
}

FUBoundingBox::FUBoundingBox(const FUBoundingBox& copy)
:	minimum(copy.minimum), maximum(copy.maximum)
{
}

FUBoundingBox::~FUBoundingBox()
{
}

void FUBoundingBox::Reset()
{
	minimum = FMVector3(FLT_MAX, FLT_MAX, FLT_MAX);
	maximum = FMVector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}

bool FUBoundingBox::IsValid() const
{
	return !(minimum.x > maximum.x || minimum.y > maximum.y || minimum.z > maximum.z);
}

bool FUBoundingBox::Contains(const FMVector3& point) const
{
	return minimum.x <= point.x && point.x <= maximum.x
		&& minimum.y <= point.y && point.y <= maximum.y
		&& minimum.z <= point.z && point.z <= maximum.z;
}

bool FUBoundingBox::Overlaps(const FUBoundingBox& boundingBox, FMVector3* overlapCenter) const
{
	bool overlaps = minimum.x <= boundingBox.maximum.x && boundingBox.minimum.x <= maximum.x
		&& minimum.y <= boundingBox.maximum.y && boundingBox.minimum.y <= maximum.y
		&& minimum.z <= boundingBox.maximum.z && boundingBox.minimum.z <= maximum.z;
	if (overlaps && overlapCenter != NULL)
	{
		float overlapMinX = max(minimum.x, boundingBox.minimum.x);
		float overlapMaxX = min(maximum.x, boundingBox.maximum.x);
		float overlapMinY = max(minimum.y, boundingBox.minimum.y);
		float overlapMaxY = min(maximum.y, boundingBox.maximum.y);
		float overlapMinZ = max(minimum.z, boundingBox.minimum.z);
		float overlapMaxZ = min(maximum.z, boundingBox.maximum.z);
		(*overlapCenter) = FMVector3((overlapMaxX + overlapMinX) / 2.0f, (overlapMaxY + overlapMinY) / 2.0f, (overlapMaxZ + overlapMinZ) / 2.0f);
	}
	return overlaps;
}

bool FUBoundingBox::Overlaps(const FUBoundingSphere& boundingSphere, FMVector3* overlapCenter) const
{
	// already implemented in bounding sphere code.
	return boundingSphere.Overlaps(*this, overlapCenter);
}

void FUBoundingBox::Include(const FUBoundingBox& boundingBox)
{
	 const FMVector3& n = boundingBox.minimum;
	 const FMVector3& x = boundingBox.maximum; 
	 if (n.x < minimum.x) minimum.x = n.x;
	 if (n.y < minimum.y) minimum.y = n.y;
	 if (n.z < minimum.z) minimum.z = n.z;
	 if (x.x > maximum.x) maximum.x = x.x;
	 if (x.y > maximum.y) maximum.y = x.y;
	 if (x.z > maximum.z) maximum.z = x.z;
}

void FUBoundingBox::Include(const FMVector3& point)
{
	if (point.x < minimum.x) minimum.x = point.x;
	else if (point.x > maximum.x) maximum.x = point.x;
	if (point.y < minimum.y) minimum.y = point.y;
	else if (point.y > maximum.y) maximum.y = point.y;
	if (point.z < minimum.z) minimum.z = point.z;
	else if (point.z > maximum.z) maximum.z = point.z;
}

FUBoundingBox FUBoundingBox::Transform(const FMMatrix44& transform) const
{
	if (!IsValid() || Equals(Infinity)) return (*this);

	FUBoundingBox transformedBoundingBox;

	FMVector3 testPoints[6] =
	{
		FMVector3(minimum.x, maximum.y, minimum.z), FMVector3(minimum.x, maximum.y, maximum.z),
		FMVector3(maximum.x, maximum.y, minimum.z), FMVector3(minimum.x, minimum.y, maximum.z),
		FMVector3(maximum.x, minimum.y, minimum.z), FMVector3(maximum.x, minimum.y, maximum.z)
	};

	for (size_t i = 0; i < 6; ++i)
	{
		testPoints[i] = transform.TransformCoordinate(testPoints[i]);
		transformedBoundingBox.Include(testPoints[i]);
	}
	transformedBoundingBox.Include(transform.TransformCoordinate(minimum));
	transformedBoundingBox.Include(transform.TransformCoordinate(maximum));

	return transformedBoundingBox;
}

bool FUBoundingBox::Equals(const FUBoundingBox& right) const
{
	return (minimum.x == right.minimum.x && 
		maximum.x == right.maximum.x &&
		minimum.y == right.minimum.y &&
		maximum.y == right.maximum.y &&
		minimum.z == right.minimum.z &&
		maximum.z == right.maximum.z);
}
