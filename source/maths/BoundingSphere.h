/* Copyright (C) 2014 Wildfire Games.
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

#ifndef INCLUDED_BOUNDINGSPHERE
#define INCLUDED_BOUNDINGSPHERE

#include "maths/BoundingBoxAligned.h"
#include "maths/Vector3D.h"

class CBoundingSphere
{
private:
	CVector3D m_Center;
	float m_Radius;

public:
	CBoundingSphere() : m_Radius(0) { }

	CBoundingSphere(const CVector3D& center, float radius) : m_Center(center), m_Radius(radius) { }

	/**
	 * Construct a bounding sphere that encompasses a bounding box
	 * swept through all possible rotations around the origin.
	 */
	static CBoundingSphere FromSweptBox(const CBoundingBoxAligned& bbox)
	{
		float maxX = std::max(fabsf(bbox[0].X), fabsf(bbox[1].X));
		float maxY = std::max(fabsf(bbox[0].Y), fabsf(bbox[1].Y));
		float maxZ = std::max(fabsf(bbox[0].Z), fabsf(bbox[1].Z));
		float radius = sqrtf(maxX*maxX + maxY*maxY + maxZ*maxZ);

		return CBoundingSphere(CVector3D(0.f, 0.f, 0.f), radius);
	}

	const CVector3D& GetCenter()
	{
		return m_Center;
	}

	float GetRadius() const
	{
		return m_Radius;
	}
};

#endif // INCLUDED_BOUNDINGSPHERE
