/* Copyright (C) 2020 Wildfire Games.
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
 * Provides an interface for a vector in R3 and allows vector and
 * scalar operations on it
 */

#include "precompiled.h"

#include "Vector3D.h"

#include "MathUtil.h"
#include "FixedVector3D.h"

#include <cmath>
#include <algorithm>
#include <limits>

CVector3D::CVector3D(const CFixedVector3D& v) :
	X(v.X.ToFloat()), Y(v.Y.ToFloat()), Z(v.Z.ToFloat())
{
}

// static
CVector3D CVector3D::Max()
{
	const float max_float = std::numeric_limits<float>::max();
	return CVector3D(max_float, max_float, max_float);
}

// static
CVector3D CVector3D::Min()
{
	const float max_float = std::numeric_limits<float>::max();
	return CVector3D(-max_float, -max_float, -max_float);
}

int CVector3D::operator ! () const
{
	if (X != 0.0f ||
		Y != 0.0f ||
		Z != 0.0f)

		return 0;

	return 1;
}

float CVector3D::LengthSquared () const
{
	return ( SQR(X) + SQR(Y) + SQR(Z) );
}

float CVector3D::Length () const
{
	return sqrtf ( LengthSquared() );
}

void CVector3D::Normalize ()
{
	float scale = 1.0f/Length ();

	X *= scale;
	Y *= scale;
	Z *= scale;
}

CVector3D CVector3D::Normalized () const
{
	float scale = 1.0f/Length ();

	return CVector3D(X * scale, Y * scale, Z * scale);
}


//-----------------------------------------------------------------------------

float MaxComponent(const CVector3D& v)
{
	return std::max({v.X, v.Y, v.Z});
}
