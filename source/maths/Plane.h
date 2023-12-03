/* Copyright (C) 2009 Wildfire Games.
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

/*
 * A Plane in R3 and several utility methods.
 */

// Note that the format used for the plane equation is
// Ax + By + Cz + D = 0, where <A,B,C> is the normal vector.

#ifndef INCLUDED_PLANE
#define INCLUDED_PLANE

#include "Vector3D.h"
#include "Vector4D.h"

enum PLANESIDE
{
	PS_FRONT,
	PS_BACK,
	PS_ON
};

class CPlane
{
	public:
		CPlane();
		CPlane(const CVector4D& coeffs) : m_Norm(coeffs.X, coeffs.Y, coeffs.Z), m_Dist(coeffs.W) { }

		//sets the plane equation from 3 points on that plane
		void Set(const CVector3D& p1, const CVector3D& p2, const CVector3D& p3);

		//sets the plane equation from a normal and a point on
		//that plane
		void Set(const CVector3D& norm, const CVector3D& point);

		//normalizes the plane equation
		void Normalize();

		//returns the side of the plane on which this point lies
		PLANESIDE ClassifyPoint(const CVector3D& point) const;
		//returns true if this point is on the back side
		inline bool IsPointOnBackSide(const CVector3D& point) const;

		//solves the plane equation for a particular point
		inline float DistanceToPlane(const CVector3D& point) const;

		//calculates the intersection point of a line with this
		//plane. Returns false if there is no intersection
		bool FindLineSegIntersection(const CVector3D& start, const CVector3D& end, CVector3D* intsect) const;
		bool FindRayIntersection(const CVector3D& start, const CVector3D& direction, CVector3D* intsect) const;

	public:
		CVector3D m_Norm;	//normal vector of the plane
		float m_Dist;		//Plane distance (ie D in the plane eq.)
		static const float m_EPS;
};

float CPlane::DistanceToPlane(const CVector3D& point) const
{
	return m_Norm.X * point.X + m_Norm.Y * point.Y + m_Norm.Z * point.Z + m_Dist;
}

bool CPlane::IsPointOnBackSide(const CVector3D& point) const
{
	return DistanceToPlane(point) < -m_EPS;
}

#endif
