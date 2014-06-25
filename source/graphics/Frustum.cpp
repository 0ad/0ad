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
 * CFrustum is a collection of planes which define a viewing space.
 */

/*
Usually associated with the camera, there are 6 planes which define the
view pyramid. But we allow more planes per frustum which may be used for
portal rendering, where a portal may have 3 or more edges.
*/

#include "precompiled.h"

#include "Frustum.h"
#include "maths/BoundingBoxAligned.h"
#include "maths/MathUtil.h"
#include "maths/Matrix3D.h"

CFrustum::CFrustum ()
{
	m_NumPlanes = 0;
}

CFrustum::~CFrustum ()
{
}

void CFrustum::SetNumPlanes (size_t num)
{
	m_NumPlanes = num;

	//clip it
	if (m_NumPlanes >= MAX_NUM_FRUSTUM_PLANES)
	{
		debug_warn(L"CFrustum::SetNumPlanes: Too many planes");
		m_NumPlanes = MAX_NUM_FRUSTUM_PLANES-1;
	}
}

void CFrustum::AddPlane (const CPlane& plane)
{
	if (m_NumPlanes >= MAX_NUM_FRUSTUM_PLANES)
	{
		debug_warn(L"CFrustum::AddPlane: Too many planes");
		return;
	}

	m_aPlanes[m_NumPlanes++] = plane;
}

void CFrustum::Transform(CMatrix3D& m)
{
	for (size_t i = 0; i < m_NumPlanes; i++)
	{
		CVector3D n = m.Rotate(m_aPlanes[i].m_Norm);
		CVector3D p = m.Transform(m_aPlanes[i].m_Norm * -m_aPlanes[i].m_Dist);
		m_aPlanes[i].Set(n, p);
		m_aPlanes[i].Normalize();
	}
}

bool CFrustum::IsPointVisible (const CVector3D &point) const
{
	PLANESIDE Side;

	for (size_t i=0; i<m_NumPlanes; i++)
	{
		Side = m_aPlanes[i].ClassifyPoint (point);

		if (Side == PS_BACK)
			return false;
	}

	return true;
}

bool CFrustum::DoesSegmentIntersect(const CVector3D& startRef, const CVector3D &endRef)
{
	CVector3D start = startRef;
	CVector3D end = endRef;

	if(IsPointVisible(start) || IsPointVisible(end))
		return true;

	CVector3D intersect;
	for ( size_t i = 0; i<m_NumPlanes; ++i )
	{
		if ( m_aPlanes[i].FindLineSegIntersection(start, end, &intersect) )
		{
			if ( IsPointVisible( intersect ) )
				return true;
		}
	}
	return false;
}

bool CFrustum::IsSphereVisible (const CVector3D &center, float radius) const
{
	for (size_t i = 0; i < m_NumPlanes; i++)
	{
		float Dist = m_aPlanes[i].DistanceToPlane(center);
		// If none of the sphere is in front of the plane, then
		// it is outside the frustum
		if (-Dist > radius)
			return false;
	}

	return true;
}

bool CFrustum::IsBoxVisible (const CVector3D &position,const CBoundingBoxAligned &bounds) const
{
	//basically for every plane we calculate the furthest point
	//in the box to that plane. If that point is beyond the plane
	//then the box is not visible
	CVector3D FarPoint;
	PLANESIDE Side;
	CVector3D Min = position+bounds[0];
	CVector3D Max = position+bounds[1];

	for (size_t i=0; i<m_NumPlanes; i++)
	{
		if (m_aPlanes[i].m_Norm.X > 0.0f)
		{
			if (m_aPlanes[i].m_Norm.Y > 0.0f)
			{
				if (m_aPlanes[i].m_Norm.Z > 0.0f)
				{
					FarPoint.X = Max.X; FarPoint.Y = Max.Y;	FarPoint.Z = Max.Z;
				}
				else
				{
					FarPoint.X = Max.X;	FarPoint.Y = Max.Y; FarPoint.Z = Min.Z;
				}
			}
			else
			{
				if (m_aPlanes[i].m_Norm.Z > 0.0f)
				{
					FarPoint.X = Max.X; FarPoint.Y = Min.Y;	FarPoint.Z = Max.Z;
				}
				else
				{
					FarPoint.X = Max.X;	FarPoint.Y = Min.Y; FarPoint.Z = Min.Z;
				}
			}
		}
		else
		{
			if (m_aPlanes[i].m_Norm.Y > 0.0f)
			{
				if (m_aPlanes[i].m_Norm.Z > 0.0f)
				{
					FarPoint.X = Min.X; FarPoint.Y = Max.Y;	FarPoint.Z = Max.Z;
				}
				else
				{
					FarPoint.X = Min.X;	FarPoint.Y = Max.Y; FarPoint.Z = Min.Z;
				}
			}
			else
			{
				if (m_aPlanes[i].m_Norm.Z > 0.0f)
				{
					FarPoint.X = Min.X; FarPoint.Y = Min.Y;	FarPoint.Z = Max.Z;
				}
				else
				{
					FarPoint.X = Min.X;	FarPoint.Y = Min.Y; FarPoint.Z = Min.Z;
				}
			}
		}
		
		Side = m_aPlanes[i].ClassifyPoint (FarPoint);

		if (Side == PS_BACK)
			return false;
	}

	return true;
}
