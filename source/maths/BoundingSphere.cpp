/* Copyright (C) 2019 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "maths/BoundingBoxAligned.h"
#include "maths/BoundingSphere.h"

CBoundingSphere CBoundingSphere::FromSweptBox(const CBoundingBoxAligned& bbox)
{
	float maxX = std::max(fabsf(bbox[0].X), fabsf(bbox[1].X));
	float maxY = std::max(fabsf(bbox[0].Y), fabsf(bbox[1].Y));
	float maxZ = std::max(fabsf(bbox[0].Z), fabsf(bbox[1].Z));

	float radius = sqrtf(maxX*maxX + maxY*maxY + maxZ*maxZ);

	return CBoundingSphere(CVector3D(0.f, 0.f, 0.f), radius);
}

bool CBoundingSphere::RayIntersect(const CVector3D& origin, const CVector3D& dir) const
{
	// Vector v from the origin of the ray to the center of the sphere
	CVector3D v = m_Center - origin;
	// Length of the projection of v onto the direction vector of the ray
	const float pcLen = dir.Dot(v);
	if (pcLen > 0.0f)
	{
		// Get the shortest distance from the center of the sphere to the ray
		v = dir * pcLen - v;
	}
	return v.LengthSquared() <= m_Radius * m_Radius;
}
