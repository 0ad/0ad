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
#include "maths/FixedVector2D.h"
#include "maths/MathUtil.h"


namespace Geometry
{

/**
 * Checks if a point is inside the given rotated rectangle.
 * Points precisely on an edge are considered to be inside.
 *
 * The rectangle is defined by the four vertexes
 * (+/-u*halfSize.X +/-v*halfSize.Y)
 *
 * The @p u and @p v vectors must be perpendicular.
 */
inline bool PointIsInSquare(CFixedVector2D point, CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize)
{
	return point.Dot(u).Absolute() <= halfSize.X && point.Dot(v).Absolute() <= halfSize.Y;
}

/**
 * Returns a vector (bx,by) such that every point inside
 * the given rotated rectangle has coordinates
 * (x,y) with -bx <= x <= bx, -by <= y < by.
 *
 * The rectangle is defined by the four vertexes
 * (+/-u*halfSize.X +/-v*halfSize.Y).
 */
CFixedVector2D GetHalfBoundingBox(CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize);

/**
 * Returns the minimum Euclidean distance from the given point to
 * any point on the boundary of the given rotated rectangle.
 *
 * If @p countInsideAsZero is true, and the point is inside the rectangle,
 * it will return 0.
 * If @p countInsideAsZero is false, the (positive) distance to the boundary
 * will be returned regardless of where the point is.
 *
 * The rectangle is defined by the four vertexes
 * (+/-u*halfSize.X +/-v*halfSize.Y).
 *
 * The @p u and @p v vectors must be perpendicular and unit length.
 */
fixed DistanceToSquare(CFixedVector2D point,
	CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize,
	bool countInsideAsZero = false);

/**
 * Similar to above but never uses sqrt, so it returns the squared distance.
 */
fixed DistanceToSquareSquared(CFixedVector2D point,
					   CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize,
					   bool countInsideAsZero = false);
/**
 * Returns a point on the boundary of the given rotated rectangle
 * that is closest (or equally closest) to the given point
 * in Euclidean distance.
 *
 * The rectangle is defined by the four vertexes
 * (+/-u*halfSize.X +/-v*halfSize.Y).
 *
 * The @p u and @p v vectors must be perpendicular and unit length.
 */
CFixedVector2D NearestPointOnSquare(CFixedVector2D point,
	CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize);

/**
 * Given a circle of radius @p radius, and a chord of length @p chordLength
 * on this circle, computes the central angle formed by
 * connecting the chord's endpoints to the center of the circle.
 *
 * @param radius Radius of the circle; must be strictly positive.
 */
float ChordToCentralAngle(const float chordLength, const float radius);

bool TestRaySquare(CFixedVector2D a, CFixedVector2D b, CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize);

bool TestRayAASquare(CFixedVector2D a, CFixedVector2D b, CFixedVector2D halfSize);

bool TestSquareSquare(
		CFixedVector2D c0, CFixedVector2D u0, CFixedVector2D v0, CFixedVector2D halfSize0,
		CFixedVector2D c1, CFixedVector2D u1, CFixedVector2D v1, CFixedVector2D halfSize1);

} // namespace

#endif // INCLUDED_HELPER_GEOMETRY
