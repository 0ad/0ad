/* Copyright (C) 2011 Wildfire Games.
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

#ifndef INCLUDED_BOX
#define INCLUDED_BOX

#include "maths/Vector3D.h"

class CBoundingBoxAligned;

/*
 * Generic oriented box. Originally intended to be used an Oriented Bounding Box (OBB),
 * as opposed to CBoundingBoxAligned which is always aligned to the world-space axes (AABB).
 * However, it could also be used to represent more generic shapes, such as parallelepipeds.
 */
class CBoundingBoxOriented
{
public:

	/// Empty constructor; creates an empty box
	CBoundingBoxOriented() { SetEmpty(); }

	/**
	 * Constructs a new oriented box centered at @p center and with normalized side vectors @p u,
	 * @p v and @p w. These vectors should be mutually orthonormal for a proper rectangular box.
	 * The half-widths of the box in each dimension are given by the corresponding components of
	 * @p halfSizes.
	 */
	CBoundingBoxOriented(const CVector3D& center, const CVector3D& u, const CVector3D& v, const CVector3D& w, const CVector3D& halfSizes)
		: m_Center(center), m_HalfSizes(halfSizes)
	{
		m_Basis[0] = u;
		m_Basis[1] = v;
		m_Basis[2] = w;
	}

	/// Constructs a new box from an axis-aligned bounding box (AABB).
	explicit CBoundingBoxOriented(const CBoundingBoxAligned& bound);

	/**
	 * Check if a given ray intersects this box. Must not be used if IsEmpty() is true.
	 * See Real-Time Rendering, Third Edition by T. Akenine-Moller, p. 741--744.
	 *
	 * @param[in] origin Origin of the ray.
	 * @param[in] dir Direction vector of the ray, defining the positive direction of the ray.
	 *            Must be of unit length.
	 * @param[out] tMin,tMax Distance in the positive direction from the origin of the ray to the
	 *             entry and exit points in the box, provided that the ray intersects the box. if
	 *             the ray does not intersect the box, no values are written to these variables.
	 *             If the origin is inside the box, then this is counted as an intersection and one
	 *             of @p tMin and @p tMax may be negative.
	 *
	 * @return true If the ray originating in @p origin and with unit direction vector @p dir intersects
	 *         this box, false otherwise.
	 */
	bool RayIntersect(const CVector3D& origin, const CVector3D& dir, float& tMin, float& tMax) const;

	/**
	 * Returns the corner at coordinate (@p u, @p v, @p w). Each of @p u, @p v and @p w must be exactly 1 or -1.
	 * Must not be used if IsEmpty() is true.
	 */
	void GetCorner(int u, int v, int w, CVector3D& out) const
	{
		out = m_Center + m_Basis[0]*(u*m_HalfSizes[0]) + m_Basis[1]*(v*m_HalfSizes[1]) + m_Basis[2]*(w*m_HalfSizes[2]);
	}

	void SetEmpty()
	{
		// everything is zero
		m_Center = CVector3D();
		m_Basis[0] = CVector3D();
		m_Basis[1] = CVector3D();
		m_Basis[2] = CVector3D();
		m_HalfSizes = CVector3D();
	}

	bool IsEmpty() const
	{
		CVector3D empty;
		return (m_Center == empty &&
			    m_Basis[0] == empty &&
				m_Basis[1] == empty &&
				m_Basis[2] == empty &&
				m_HalfSizes == empty);
	}

public:
	CVector3D m_Center; ///< Centroid location of the box
	CVector3D m_HalfSizes; ///< Half the sizes of the box in each dimension (u,v,w). Positive values are expected.
	/// Basis vectors (u,v,w) of the sides. Must always be normalized, and should be
	/// orthogonal for a proper rectangular cuboid.
	CVector3D m_Basis[3];

	static const CBoundingBoxOriented EMPTY;
};

#endif // INCLUDED_BOX
