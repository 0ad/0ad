/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_HELPER_GEOMETRY
#define INCLUDED_HELPER_GEOMETRY

/**
 * @file
 * Helper functions related to geometry algorithms
 */

#include "maths/Fixed.h"

class CFixedVector2D;

namespace Geometry
{

bool PointIsInSquare(CFixedVector2D point, CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize);

CFixedVector2D GetHalfBoundingBox(CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize);

fixed DistanceToSquare(CFixedVector2D point, CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize);

CFixedVector2D NearestPointOnSquare(CFixedVector2D point, CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize);

bool TestRaySquare(CFixedVector2D a, CFixedVector2D b, CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize);

bool TestRayAASquare(CFixedVector2D a, CFixedVector2D b, CFixedVector2D halfSize);

bool TestSquareSquare(
		CFixedVector2D c0, CFixedVector2D u0, CFixedVector2D v0, CFixedVector2D halfSize0,
		CFixedVector2D c1, CFixedVector2D u1, CFixedVector2D v1, CFixedVector2D halfSize1);

} // namespace

#endif // INCLUDED_HELPER_GEOMETRY
