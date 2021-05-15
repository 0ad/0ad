/* Copyright (C) 2021 Wildfire Games.
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
 * Axis-aligned bounding box
 */

#include "precompiled.h"

#include "BoundingBoxAligned.h"

#include "maths/BoundingBoxOriented.h"
#include "maths/Brush.h"
#include "maths/Frustum.h"
#include "maths/Matrix3D.h"

#include <limits>

const CBoundingBoxAligned CBoundingBoxAligned::EMPTY = CBoundingBoxAligned(); // initializes to an empty bound

///////////////////////////////////////////////////////////////////////////////
// RayIntersect: intersect ray with this bound; return true
// if ray hits (and store entry and exit times), or false
// otherwise
// note: incoming ray direction must be normalised
bool CBoundingBoxAligned::RayIntersect(
	const CVector3D& origin, const CVector3D& dir, float& tmin, float& tmax) const
{
	float t1, t2;
	float tnear, tfar;

	if (dir[0] == 0)
	{
		if (origin[0] < m_Data[0][0] || origin[0] > m_Data[1][0])
			return false;
		else
		{
			tnear = -std::numeric_limits<float>::max();
			tfar = std::numeric_limits<float>::max();
		}
	}
	else
	{
		t1 = (m_Data[0][0] - origin[0]) / dir[0];
		t2 = (m_Data[1][0] - origin[0]) / dir[0];

		if (dir[0] < 0)
		{
			tnear = t2;
			tfar = t1;
		}
		else
		{
			tnear = t1;
			tfar = t2;
		}

		if (tfar < 0)
			return false;
	}

	if (dir[1] == 0 && (origin[1] < m_Data[0][1] || origin[1] > m_Data[1][1]))
		return false;
	else
	{
		t1 = (m_Data[0][1] - origin[1]) / dir[1];
		t2 = (m_Data[1][1] - origin[1]) / dir[1];

		if (dir[1] < 0)
		{
			if (t2 > tnear)
				tnear = t2;
			if (t1 < tfar)
				tfar = t1;
		}
		else
		{
			if (t1 > tnear)
				tnear = t1;
			if (t2 < tfar)
				tfar = t2;
		}

		if (tnear > tfar || tfar < 0)
			return false;
	}

	if (dir[2] == 0 && (origin[2] < m_Data[0][2] || origin[2] > m_Data[1][2]))
		return false;
	else
	{
		t1 = (m_Data[0][2] - origin[2]) / dir[2];
		t2 = (m_Data[1][2] - origin[2]) / dir[2];

		if (dir[2] < 0)
		{
			if (t2 > tnear)
				tnear = t2;
			if (t1 < tfar)
				tfar = t1;
		}
		else
		{
			if (t1 > tnear)
				tnear = t1;
			if (t2 < tfar)
				tfar = t2;
		}

		if (tnear > tfar || tfar < 0)
			return false;
	}

	tmin = tnear;
	tmax = tfar;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// SetEmpty: initialise this bound as empty
void CBoundingBoxAligned::SetEmpty()
{
	m_Data[0] = CVector3D::Max();
	m_Data[1] = CVector3D::Min();
}

///////////////////////////////////////////////////////////////////////////////
// IsEmpty: tests whether this bound is empty
bool CBoundingBoxAligned::IsEmpty() const
{
	return m_Data[0] == CVector3D::Max() && m_Data[1] == CVector3D::Min();
}

///////////////////////////////////////////////////////////////////////////////
// Transform: transform this bound by given matrix; return transformed bound
// in 'result' parameter - slightly modified version of code in Graphic Gems
// (can't remember which one it was, though)
void CBoundingBoxAligned::Transform(const CMatrix3D& m, CBoundingBoxAligned& result) const
{
	ENSURE(this != &result);

	for (int i = 0; i < 3; ++i)
	{
		// handle translation
		result[0][i] = result[1][i] = m(i, 3);

		// Now find the extreme points by considering the product of the
		// min and max with each component of matrix
		for (int j = 0; j < 3; ++j)
		{
			float a = m(i, j) * m_Data[0][j];
			float b = m(i, j) * m_Data[1][j];

			if (a >= b)
				std::swap(a, b);

			result[0][i] += a;
			result[1][i] += b;
		}
	}
}

void CBoundingBoxAligned::Transform(const CMatrix3D& transform, CBoundingBoxOriented& result) const
{
	const CVector3D& pMin = m_Data[0];
	const CVector3D& pMax = m_Data[1];

	// the basis vectors of the OBB are the normalized versions of the transformed AABB basis vectors, which
	// are the columns of the identity matrix, so the unnormalized OBB basis vectors are the transformation
	// matrix columns:
	CVector3D u(transform._11, transform._21, transform._31);
	CVector3D v(transform._12, transform._22, transform._32);
	CVector3D w(transform._13, transform._23, transform._33);

	// the half-sizes are scaled by whatever factor the AABB unit vectors end up scaled by
	result.m_HalfSizes = CVector3D(
		(pMax.X - pMin.X) / 2.f * u.Length(),
		(pMax.Y - pMin.Y) / 2.f * v.Length(),
		(pMax.Z - pMin.Z) / 2.f * w.Length()
	);

	u.Normalize();
	v.Normalize();
	w.Normalize();

	result.m_Basis[0] = u;
	result.m_Basis[1] = v;
	result.m_Basis[2] = w;

	result.m_Center = transform.Transform((pMax + pMin) * 0.5f);
}

///////////////////////////////////////////////////////////////////////////////
// Intersect with the given frustum in a conservative manner
void CBoundingBoxAligned::IntersectFrustumConservative(const CFrustum& frustum)
{
	// if this bound is empty, then the result must be empty (we should not attempt to intersect with
	// a brush, may cause crashes due to the numeric representation of empty bounds -- see
	// http://trac.wildfiregames.com/ticket/1027)
	if (IsEmpty())
		return;

	CBrush brush(*this);
	CBrush buf;

	brush.Intersect(frustum, buf);

	buf.Bounds(*this);
}

///////////////////////////////////////////////////////////////////////////////
CFrustum CBoundingBoxAligned::ToFrustum() const
{
	CFrustum frustum;

	frustum.SetNumPlanes(6);

	// get the LEFT plane
	frustum[0].m_Norm = CVector3D(1, 0, 0);
	frustum[0].m_Dist = -m_Data[0].X;

	// get the RIGHT plane
	frustum[1].m_Norm = CVector3D(-1, 0, 0);
	frustum[1].m_Dist = m_Data[1].X;

	// get the BOTTOM plane
	frustum[2].m_Norm = CVector3D(0, 1, 0);
	frustum[2].m_Dist = -m_Data[0].Y;

	// get the TOP plane
	frustum[3].m_Norm = CVector3D(0, -1, 0);
	frustum[3].m_Dist = m_Data[1].Y;

	// get the NEAR plane
	frustum[4].m_Norm = CVector3D(0, 0, 1);
	frustum[4].m_Dist = -m_Data[0].Z;

	// get the FAR plane
	frustum[5].m_Norm = CVector3D(0, 0, -1);
	frustum[5].m_Dist = m_Data[1].Z;

	return frustum;
}

///////////////////////////////////////////////////////////////////////////////
void CBoundingBoxAligned::Expand(float amount)
{
	m_Data[0] -= CVector3D(amount, amount, amount);
	m_Data[1] += CVector3D(amount, amount, amount);
}

bool CBoundingBoxAligned::IsPointInside(const CVector3D& point) const
{
	return
		m_Data[0].X <= point.X && point.X <= m_Data[1].X &&
		m_Data[0].Y <= point.Y && point.Y <= m_Data[1].Y &&
		m_Data[0].Z <= point.Z && point.Z <= m_Data[1].Z;
}
