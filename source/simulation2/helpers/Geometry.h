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
inline bool PointIsInSquare(const CFixedVector2D& point, const CFixedVector2D& u, const CFixedVector2D& v, const CFixedVector2D& halfSize)
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
CFixedVector2D GetHalfBoundingBox(const CFixedVector2D& u, const CFixedVector2D& v, const CFixedVector2D& halfSize);

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
fixed DistanceToSquare(const CFixedVector2D& point,
	const CFixedVector2D& u, const CFixedVector2D& v, const CFixedVector2D& halfSize,
	bool countInsideAsZero = false);

/**
 * Similar to above but never uses sqrt, so it returns the squared distance.
 */
fixed DistanceToSquareSquared(const CFixedVector2D& point,
					   const CFixedVector2D& u, const CFixedVector2D& v, const CFixedVector2D& halfSize,
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
CFixedVector2D NearestPointOnSquare(const CFixedVector2D& point,
	const CFixedVector2D& u, const CFixedVector2D& v, const CFixedVector2D& halfSize);

/**
 * Returns the shortest distance between two squares.
 */
fixed DistanceSquareToSquare(const CFixedVector2D& relativePos,
	const CFixedVector2D& u1, const CFixedVector2D& v1, const CFixedVector2D& halfSize1,
	const CFixedVector2D& u2, const CFixedVector2D& v2, const CFixedVector2D& halfSize2);

/**
 * Returns the greatest straight line distance from a point to a square.
 *
 * If @p countInsideAsZero is true, and the point is inside the rectangle,
 * it will return 0.
 * If @p countInsideAsZero is false, the greatest (positive) distance to the boundary
 * will be returned regardless of where the point is.
 */
fixed MaxDistanceToSquare(const CFixedVector2D& point,
	const CFixedVector2D& u, const CFixedVector2D& v, const CFixedVector2D& halfSize,
	bool countInsideAsZero = false);

/**
 * Return the greatest straight line distance between two squares.
 */
fixed MaxDistanceSquareToSquare(const CFixedVector2D& relativePos,
	const CFixedVector2D& u1, const CFixedVector2D& v1, const CFixedVector2D& halfSize1,
	const CFixedVector2D& u2, const CFixedVector2D& v2, const CFixedVector2D& halfSize2);

bool TestRaySquare(const CFixedVector2D& a, const CFixedVector2D& b, const CFixedVector2D& u, const CFixedVector2D& v, const CFixedVector2D& halfSize);

bool TestRayAASquare(const CFixedVector2D& a, const CFixedVector2D& b, const CFixedVector2D& halfSize);

bool TestSquareSquare(
		const CFixedVector2D& c0, const CFixedVector2D& u0, const CFixedVector2D& v0, const CFixedVector2D& halfSize0,
		const CFixedVector2D& c1, const CFixedVector2D& u1, const CFixedVector2D& v1, const CFixedVector2D& halfSize1);

/**
 * Used in Footprint when spawning units:
 * Given a grid point (x, y) on the rectangle [-x_max, x_max] x [-y_max, y_max],
 * this returns the distance travelled in moving from (x_max, 0) to the the point while
 * walking counter-clockwise along the perimeter of the rectangle.
 */
int GetPerimeterDistance(int x_max, int y_max, int x, int y);

/**
 * Used in Footprint when spawning units:
 * This returns the grid point on the rectangle [-x_max, x_max] x [-y_max, y_max]
 * reached after starting at (x_max, 0) and walking a distance k
 * counter-clockwise along the perimeter of the rectangle.
 */
std::pair<int, int> GetPerimeterCoordinates(int x_max, int y_max, int k);

} // namespace

#endif // INCLUDED_HELPER_GEOMETRY
