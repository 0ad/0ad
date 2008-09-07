/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FMVolume.h"

namespace FMVolume
{
	float CalculateBoxVolume(const FMVector3& halfExtents)
	{
		return (halfExtents.x*2) * (halfExtents.y*2) * (halfExtents.z*2);
	}

	float CalculateSphereVolume(float radius)
	{
		return CalculateEllipsoidVolume(radius, radius, radius);
	}

	float CalculateEllipsoidVolume(float radius1, float radius2, float radius3)
	{
		return (float)(4.0f * FMath::Pi * radius1 * radius2 * radius3) / 3.0f;
	}

	float CalculateEllipsoidEndVolume(const FMVector2& radius)
	{
		return CalculateEllipsoidVolume(radius.x, radius.y, max(radius.x, radius.y));
	}

	float CalculateCylinderVolume(const FMVector2& radius, float height)
	{
		return  (float)(FMath::Pi * radius.x * radius.y * height);
	}

	float CalculateCapsuleVolume(const FMVector2& radius, float height)
	{
		// 1 cylinder + 1 ellipsoid (or sphere if radius.x and radius.y are equivalent)
		return CalculateCylinderVolume(radius, height) + CalculateEllipsoidEndVolume(radius);
	}

	float CalculateConeVolume(const FMVector2& radius, float height)
	{
		return (float) FMath::Pi * radius.x * radius.y * height / 3.0f;
	}

	float CalculateTaperedCylinderVolume(const FMVector2& radius, const FMVector2& radius2, float height)
	{
		// calculate the integral from 0 to height assuming linear interpolation of area from one surface to the other
		return (2 * radius.x * radius.y + radius.x * radius2.y + radius2.x * radius.y + 2 * radius2.x * radius2.y) *
				(float)FMath::Pi * height / 6.0f;
	}
}

