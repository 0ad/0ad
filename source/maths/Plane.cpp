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

#include "precompiled.h"

#include "Plane.h"
#include "MathUtil.h"

CPlane::CPlane ()
{
	m_Dist = 0.0f;
}

//sets the plane equation from 3 points on that plane
void CPlane::Set (const CVector3D &p1, const CVector3D &p2, const CVector3D &p3)
{
	CVector3D D1, D2;
	CVector3D Norm;

	//calculate two vectors on the surface of the plane
	D1 = p2-p1;
	D2 = p3-p1;

	//cross multiply gives normal
	Norm = D2.Cross(D1);

	Set (Norm, p1);
}

//sets the plane equation from a normal and a point on 
//that plane
void CPlane::Set (const CVector3D &norm, const CVector3D &point)
{
	m_Norm = norm;

	m_Dist = - (norm.X * point.X +
				norm.Y * point.Y +
				norm.Z * point.Z);

//	Normalize ();
}

//normalizes the plane equation
void CPlane::Normalize ()
{
	float Scale;

	Scale = 1.0f/m_Norm.Length ();

	m_Norm.X *= Scale;
	m_Norm.Y *= Scale;
	m_Norm.Z *= Scale;
	m_Dist *= Scale;
}

//returns the side of the plane on which this point
//lies.
PLANESIDE CPlane::ClassifyPoint (const CVector3D &point) const
{
	float Dist;

	Dist = m_Norm.X * point.X +
		   m_Norm.Y * point.Y +
		   m_Norm.Z * point.Z +
		   m_Dist;

	const float EPS = 0.001f;

	if (Dist > EPS)
		return PS_FRONT;
	else if (Dist < -EPS)
		return PS_BACK;

	return PS_ON;
}

//calculates the intersection point of a line with this
//plane. Returns false if there is no intersection
bool CPlane::FindLineSegIntersection (const CVector3D &start, const CVector3D &end, CVector3D *intsect)
{
	float dist1 = DistanceToPlane( start );
	float dist2 = DistanceToPlane( end );

	if( (dist1 < 0 && dist2 < 0) || (dist1 >= 0 && dist2 >= 0) )
		return false;

	float t = (-dist1) / (dist2-dist1);
	*intsect = Interpolate( start, end, t );

	return true;
}

bool CPlane::FindRayIntersection (const CVector3D &start, const CVector3D &direction, CVector3D *intsect)
{
	float dot = m_Norm.Dot (direction);
	if (dot == 0.0f)
		return false;

	*intsect = start - (direction * (DistanceToPlane (start)/dot));
	return true;
}
