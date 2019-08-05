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

#include "BoundingBoxOriented.h"
#include "maths/BoundingBoxAligned.h"

#include <float.h>

const CBoundingBoxOriented CBoundingBoxOriented::EMPTY = CBoundingBoxOriented();

CBoundingBoxOriented::CBoundingBoxOriented(const CBoundingBoxAligned& bound)
{
	if (bound.IsEmpty())
	{
		SetEmpty();
	}
	else
	{
		bound.GetCenter(m_Center);

		// the axes of an AABB are the world-space axes
		m_Basis[0].X = 1.f; m_Basis[0].Y = 0.f; m_Basis[0].Z = 0.f;
		m_Basis[1].X = 0.f; m_Basis[1].Y = 1.f; m_Basis[1].Z = 0.f;
		m_Basis[2].X = 0.f; m_Basis[2].Y = 0.f; m_Basis[2].Z = 1.f;

		// element-wise division by two to get half sizes (remember, [1] and [0] are the max and min coord points)
		m_HalfSizes = (bound[1] - bound[0]) * 0.5f;
	}
}

bool CBoundingBoxOriented::RayIntersect(const CVector3D& origin, const CVector3D& dir, float& tMin_out, float& tMax_out) const
{
	// See Real-Time Rendering, Third Edition, p. 743
	float tMin = -FLT_MAX;
	float tMax = FLT_MAX;

	CVector3D p = m_Center - origin;

	for (int i = 0; i < 3; ++i)
	{
		// test the ray for intersections with the slab whose normal vector is m_Basis[i]
		float e = m_Basis[i].Dot(p); // distance between the ray origin and the box center projected onto the slab normal
		float f = m_Basis[i].Dot(dir); // cosine of the angle between the slab normal and the ray direction

		if(fabsf(f) > 1e-10f)
		{
			// Determine the distances t1 and t2 from the origin of the ray to the points where it intersects
			// the slab. See docs/ray_intersect.pdf for why/how this works.
			float invF = 1.f/f;
			float t1 = (e + m_HalfSizes[i]) * invF;
			float t2 = (e - m_HalfSizes[i]) * invF;

			// make sure t1 <= t2, swap if necessary
			if (t1 > t2)
			{
				float tmp = t1;
				t1 = t2;
				t2 = tmp;
			}

			// update the overall tMin and tMax if necessary
			if (t1 > tMin) tMin = t1;
			if (t2 < tMax) tMax = t2;

			// try to break out of the loop as fast as possible by checking for some conditions
			if (tMin > tMax) return false; // ray misses the box
			if (tMax < 0) return false; // box is behind the ray origin
		}
		else
		{
			// the ray is parallel to the slab currently being tested, or is as close to parallel
			// as makes no difference; return false if the ray is outside of the slab.
			if (e > m_HalfSizes[i] || -e > m_HalfSizes[i])
				return false;
		}
	}

	tMin_out = tMin;
	tMax_out = tMax;
	return true;
}
