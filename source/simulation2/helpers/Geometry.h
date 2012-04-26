/* Copyright (C) 2012 Wildfire Games.
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
#include "maths/MathUtil.h"

class CFixedVector2D;

namespace Geometry
{

/**
 * Checks if a point is inside the given rotated square or rectangle.
 *
 * @note Currently assumes the @p u and @p v vectors are perpendicular.
 * @param point point vector of the point that is to be tested relative to the origin (center) of the shape.
 * @param u rotated X axis unit vector relative to the absolute XZ plane. Indicates the orientation of the rectangle. If not rotated,
 *          this value is the absolute X axis unit vector (1,0). If rotated by angle theta, this should be (cos theta, -sin theta), as
 *          the absolute Z axis points down in the unit circle.
 * @param v rotated Z axis unit vector relative to the absolute XZ plane. Indicates the orientation of the rectangle. If not rotated,
 *          this value is the absolute Z axis unit vector (0,1). If rotated by angle theta, this should be (sin theta, cos theta), as
 *          the absolute Z axis points down in the unit circle.
 * @param halfSize Holds half the dimensions of the shape along the u and v vectors, respectively.
 *
 * @return true if @p point is inside the square with rotated X axis unit vector @p u and rotated Z axis unit vector @p v,
 * and half dimensions specified by @p halfSizes.
 */
bool PointIsInSquare(CFixedVector2D point, CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize);

CFixedVector2D GetHalfBoundingBox(CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize);

fixed DistanceToSquare(CFixedVector2D point, CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize);

/**
 * Given a circle of radius @p radius, and a chord of length @p chordLength on this circle, computes the central angle formed by 
 * connecting the chord's endpoints to the center of the circle.
 * 
 * @param radius Radius of the circle; must be strictly positive.
 */
float ChordToCentralAngle(const float chordLength, const float radius);

/**
 * Find point closest to the given point on the edge of the given square or rectangle.
 *
 * @note Currently assumes the @p u and @p v vectors are perpendicular.
 * @param point point vector of the point we want to get the nearest edge point for, relative to the origin (center) of the shape.
 * @param u rotated X axis unit vector, relative to the absolute XZ plane. Indicates the orientation of the shape. If not rotated,
 *          this value is the absolute X axis unit vector (1,0). If rotated by angle theta, this should be (cos theta, -sin theta).
 * @param v rotated Z axis unit vector, relative to the absolute XZ plane. Indicates the orientation of the shape. If not rotated,
 *          this value is the absolute Z axis unit vector (0,1). If rotated by angle theta, this should be (sin theta, cos theta).
 * @param halfSize Holds half the dimensions of the shape along the u and v vectors, respectively.
 *
 * @return point that is closest to @p point on the edge of the square specified by orientation unit vectors @p u and @p v and half
 *	dimensions @p halfSize, relative to the center of the square
 */
CFixedVector2D NearestPointOnSquare(CFixedVector2D point, CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize);

bool TestRaySquare(CFixedVector2D a, CFixedVector2D b, CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize);

bool TestRayAASquare(CFixedVector2D a, CFixedVector2D b, CFixedVector2D halfSize);

bool TestSquareSquare(
		CFixedVector2D c0, CFixedVector2D u0, CFixedVector2D v0, CFixedVector2D halfSize0,
		CFixedVector2D c1, CFixedVector2D u1, CFixedVector2D v1, CFixedVector2D halfSize1);

} // namespace

#endif // INCLUDED_HELPER_GEOMETRY
