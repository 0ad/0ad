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

#include "precompiled.h"

#include "Geometry.h"

using namespace Geometry;

// TODO: all of these things could be optimised quite easily

CFixedVector2D Geometry::GetHalfBoundingBox(const CFixedVector2D& u, const CFixedVector2D& v, const CFixedVector2D& halfSize)
{
	return CFixedVector2D(
		u.X.Multiply(halfSize.X).Absolute() + v.X.Multiply(halfSize.Y).Absolute(),
		u.Y.Multiply(halfSize.X).Absolute() + v.Y.Multiply(halfSize.Y).Absolute()
	);
}

fixed Geometry::DistanceToSquare(const CFixedVector2D& point, const CFixedVector2D& u, const CFixedVector2D& v, const CFixedVector2D& halfSize, bool countInsideAsZero)
{
	/*
	 * Relative to its own coordinate system, we have a square like:
	 *
	 *  A  :    B    :  C
	 *     :         :
	 * - - ########### - -
	 *     #         #
	 *     # I       #
	 *  D  #    0    #  E     v
	 *     #         #        ^
	 *     #         #        |
	 * - - ########### - -      -->u
	 *     :         :
	 *  F  :    G    :  H
	 *
	 * where 0 is the center, u and v are unit axes,
	 * and the square is hw*2 by hh*2 units in size.
	 *
	 * Points in the BIG regions should check distance to horizontal edges.
	 * Points in the DIE regions should check distance to vertical edges.
	 * Points in the ACFH regions should check distance to the corresponding corner.
	 *
	 * So we just need to check all of the regions to work out which calculations to apply.
	 *
	 */

	// By symmetry (taking absolute values), we work only in the 0-B-C-E quadrant
	// du, dv are the location of the point in the square's coordinate system
	fixed du = point.Dot(u).Absolute();
	fixed dv = point.Dot(v).Absolute();

	fixed hw = halfSize.X;
	fixed hh = halfSize.Y;

	if (du < hw) // regions B, I, G
	{
		if (dv < hh) // region I
			return countInsideAsZero ? fixed::Zero() : std::min(hw - du, hh - dv);
		else
			return dv - hh;
	}
	else if (dv < hh) // regions D, E
	{
		return du - hw; // vertical edges
	}
	else // regions A, C, F, H
	{
		CFixedVector2D distance(du - hw, dv - hh);
		return distance.Length();
	}
}

// Same as above except it does not use Length
// For explanations refer to DistanceToSquare
fixed Geometry::DistanceToSquareSquared(const CFixedVector2D& point, const CFixedVector2D& u, const CFixedVector2D& v, const CFixedVector2D& halfSize, bool countInsideAsZero)
{
	fixed du = point.Dot(u).Absolute();
	fixed dv = point.Dot(v).Absolute();

	fixed hw = halfSize.X;
	fixed hh = halfSize.Y;

	if (du < hw) // regions B, I, G
	{
		if (dv < hh) // region I
			return countInsideAsZero ? fixed::Zero() : std::min((hw - du).Square(), (hh - dv).Square());
		else
			return (dv - hh).Square(); // horizontal edges
	}
	else if (dv < hh) // regions D, E
	{
		return (du - hw).Square(); // vertical edges
	}
	else // regions A, C, F, H
	{
		return (du - hw).Square() + (dv - hh).Square();
	}
}

CFixedVector2D Geometry::NearestPointOnSquare(const CFixedVector2D& point, const CFixedVector2D& u, const CFixedVector2D& v, const CFixedVector2D& halfSize)
{
	/*
	 * Relative to its own coordinate system, we have a square like:
	 *
	 *  A  :         :  C
	 *     :         :
	 * - - #### B #### - -
	 *     #\       /#
	 *     # \     / #
	 *     D  --0--  E        v
	 *     # /     \ #        ^
	 *     #/       \#        |
	 * - - #### G #### - -      -->u
	 *     :         :
	 *  F  :         :  H
	 *
	 * where 0 is the center, u and v are unit axes,
	 * and the square is hw*2 by hh*2 units in size.
	 *
	 * Points in the BDEG regions are nearest to the corresponding edge.
	 * Points in the ACFH regions are nearest to the corresponding corner.
	 *
	 * So we just need to check all of the regions to work out which calculations to apply.
	 *
	 */

	// du, dv are the location of the point in the square's coordinate system
	fixed du = point.Dot(u);
	fixed dv = point.Dot(v);

	fixed hw = halfSize.X;
	fixed hh = halfSize.Y;

	if (-hw < du && du < hw) // regions B, G; or regions D, E inside the square
	{
		if (-hh < dv && dv < hh && (du.Absolute() - hw).Absolute() < (dv.Absolute() - hh).Absolute()) // regions D, E
		{
			if (du >= fixed::Zero()) // E
				return u.Multiply(hw) + v.Multiply(dv);
			else // D
				return -u.Multiply(hw) + v.Multiply(dv);
		}
		else // B, G
		{
			if (dv >= fixed::Zero()) // B
				return v.Multiply(hh) + u.Multiply(du);
			else // G
				return -v.Multiply(hh) + u.Multiply(du);
		}
	}
	else if (-hh < dv && dv < hh) // regions D, E outside the square
	{
		if (du >= fixed::Zero()) // E
			return u.Multiply(hw) + v.Multiply(dv);
		else // D
			return -u.Multiply(hw) + v.Multiply(dv);
	}
	else // regions A, C, F, H
	{
		CFixedVector2D corner;
		if (du < fixed::Zero()) // A, F
			corner -= u.Multiply(hw);
		else // C, H
			corner += u.Multiply(hw);
		if (dv < fixed::Zero()) // F, H
			corner -= v.Multiply(hh);
		else // A, C
			corner += v.Multiply(hh);

		return corner;
	}
}

fixed Geometry::DistanceSquareToSquare(const CFixedVector2D& relativePos, const CFixedVector2D& u1, const CFixedVector2D& v1, const CFixedVector2D& halfSize1, const CFixedVector2D& u2, const CFixedVector2D& v2, const CFixedVector2D& halfSize2)
{
	/*
	 * The shortest distance between two non colliding squares equals the distance between a corner
	 * and other square. Thus calculating all 8 those distances and taking the smallest.
	 * For colliding squares we simply return 0. When one of the points is inside the other square
	 * we depend on DistanceToSquare's countInsideAsZero. When no point is inside the other square,
	 * it is enough to check that two adjacent edges of one square does not collide with the other square.
	 */
	fixed hw1 = halfSize1.X;
	fixed hh1 = halfSize1.Y;
	fixed hw2 = halfSize2.X;
	fixed hh2 = halfSize2.Y;
	if (TestRaySquare(relativePos + u1.Multiply(hw1) + v1.Multiply(hh1), relativePos - u1.Multiply(hw1) + v1.Multiply(hh1), u2, v2, halfSize2) ||
	    TestRaySquare(relativePos + u1.Multiply(hw1) + v1.Multiply(hh1), relativePos + u1.Multiply(hw1) - v1.Multiply(hh1), u2, v2, halfSize2))
		return fixed::Zero();

	return std::min(std::min(std::min(
			DistanceToSquare(relativePos + u1.Multiply(hw1) + v1.Multiply(hh1), u2, v2, halfSize2, true),
			DistanceToSquare(relativePos + u1.Multiply(hw1) - v1.Multiply(hh1), u2, v2, halfSize2, true)),
		std::min(
			DistanceToSquare(relativePos - u1.Multiply(hw1) + v1.Multiply(hh1), u2, v2, halfSize2, true),
			DistanceToSquare(relativePos - u1.Multiply(hw1) - v1.Multiply(hh1), u2, v2, halfSize2, true))),
		std::min(std::min(
			DistanceToSquare(relativePos + u2.Multiply(hw2) + v2.Multiply(hh2), u1, v1, halfSize1, true),
			DistanceToSquare(relativePos + u2.Multiply(hw2) - v2.Multiply(hh2), u1, v1, halfSize1, true)),
		std::min(
			DistanceToSquare(relativePos - u2.Multiply(hw2) + v2.Multiply(hh2), u1, v1, halfSize1, true),
			DistanceToSquare(relativePos - u2.Multiply(hw2) - v2.Multiply(hh2), u1, v1, halfSize1, true))));
}

fixed Geometry::MaxDistanceToSquare(const CFixedVector2D& point, const CFixedVector2D& u, const CFixedVector2D& v, const CFixedVector2D& halfSize, bool countInsideAsZero)
{
	fixed hw = halfSize.X;
	fixed hh = halfSize.Y;

	if (point.Dot(u).Absolute() < hw && point.Dot(v).Absolute() < hh && countInsideAsZero)
		return fixed::Zero();

	/*
	 * The maximum distance from a point to an edge of a square equals the greatest distance
	 * from the point to the a corner. Thus calculating all and taking the greatest.
	 */
	return std::max(std::max(
			(point + u.Multiply(hw) + v.Multiply(hh)).Length(),
			(point + u.Multiply(hw) - v.Multiply(hh)).Length()),
		std::max(
			(point - u.Multiply(hw) + v.Multiply(hh)).Length(),
			(point - u.Multiply(hw) - v.Multiply(hh)).Length()));
}

fixed Geometry::MaxDistanceSquareToSquare(const CFixedVector2D& relativePos, const CFixedVector2D& u1, const CFixedVector2D& v1, const CFixedVector2D& halfSize1, const CFixedVector2D& u2, const CFixedVector2D& v2, const CFixedVector2D& halfSize2)
{
	/*
	 * The maximum distance from an edge of a square to the edge of another square
	 * equals the greatest distance from the any of the 16 corner corner distances.
	*/
	fixed hw1 = halfSize1.X;
	fixed hh1 = halfSize1.Y;

	return std::max(std::max(
			MaxDistanceToSquare(relativePos + u1.Multiply(hw1) + v1.Multiply(hh1), u2, v2, halfSize2, true),
			MaxDistanceToSquare(relativePos + u1.Multiply(hw1) - v1.Multiply(hh1), u2, v2, halfSize2, true)),
		std::max(MaxDistanceToSquare(relativePos - u1.Multiply(hw1) + v1.Multiply(hh1), u2, v2, halfSize2, true),
			MaxDistanceToSquare(relativePos - u1.Multiply(hw1) - v1.Multiply(hh1), u2, v2, halfSize2, true)));
}

bool Geometry::TestRaySquare(const CFixedVector2D& a, const CFixedVector2D& b, const CFixedVector2D& u, const CFixedVector2D& v, const CFixedVector2D& halfSize)
{
	/*
	 * We only consider collisions to be when the ray goes from outside to inside the shape (and possibly out again).
	 * Various cases to consider:
	 *   'a' inside, 'b' inside -> no collision
	 *   'a' inside, 'b' outside -> no collision
	 *   'a' outside, 'b' inside -> collision
	 *   'a' outside, 'b' outside -> depends; use separating axis theorem:
	 *     if the ray's bounding box is outside the square -> no collision
	 *     if the whole square is on the same side of the ray -> no collision
	 *     otherwise -> collision
	 * (Points on the edge are considered 'inside'.)
	 */

	fixed hw = halfSize.X;
	fixed hh = halfSize.Y;

	fixed au = a.Dot(u);
	fixed av = a.Dot(v);

	if (-hw <= au && au <= hw && -hh <= av && av <= hh)
		return false; // a is inside

	fixed bu = b.Dot(u);
	fixed bv = b.Dot(v);

	if (-hw <= bu && bu <= hw && -hh <= bv && bv <= hh) // TODO: isn't this subsumed by the next checks?
		return true; // a is outside, b is inside

	if ((au < -hw && bu < -hw) || (au > hw && bu > hw) || (av < -hh && bv < -hh) || (av > hh && bv > hh))
		return false; // ab is entirely above/below/side the square

	CFixedVector2D abp = (b - a).Perpendicular();
	fixed s0 = abp.Dot((u.Multiply(hw) + v.Multiply(hh)) - a);
	fixed s1 = abp.Dot((u.Multiply(hw) - v.Multiply(hh)) - a);
	fixed s2 = abp.Dot((-u.Multiply(hw) - v.Multiply(hh)) - a);
	fixed s3 = abp.Dot((-u.Multiply(hw) + v.Multiply(hh)) - a);
	if (s0.IsZero() || s1.IsZero() || s2.IsZero() || s3.IsZero())
		return true; // ray intersects the corner

	bool sign = (s0 < fixed::Zero());
	if ((s1 < fixed::Zero()) != sign || (s2 < fixed::Zero()) != sign || (s3 < fixed::Zero()) != sign)
		return true; // ray cuts through the square

	return false;
}

// Exactly like TestRaySquare with u=(1,0), v=(0,1)
bool Geometry::TestRayAASquare(const CFixedVector2D& a, const CFixedVector2D& b, const CFixedVector2D& halfSize)
{
	fixed hw = halfSize.X;
	fixed hh = halfSize.Y;

	if (-hw <= a.X && a.X <= hw && -hh <= a.Y && a.Y <= hh)
		return false; // a is inside

	if (-hw <= b.X && b.X <= hw && -hh <= b.Y && b.Y <= hh) // TODO: isn't this subsumed by the next checks?
		return true; // a is outside, b is inside

	if ((a.X < -hw && b.X < -hw) || (a.X > hw && b.X > hw) || (a.Y < -hh && b.Y < -hh) || (a.Y > hh && b.Y > hh))
		return false; // ab is entirely above/below/side the square

	CFixedVector2D abp = (b - a).Perpendicular();
	fixed s0 = abp.Dot(CFixedVector2D(hw, hh) - a);
	fixed s1 = abp.Dot(CFixedVector2D(hw, -hh) - a);
	fixed s2 = abp.Dot(CFixedVector2D(-hw, -hh) - a);
	fixed s3 = abp.Dot(CFixedVector2D(-hw, hh) - a);
	if (s0.IsZero() || s1.IsZero() || s2.IsZero() || s3.IsZero())
		return true; // ray intersects the corner

	bool sign = (s0 < fixed::Zero());
	if ((s1 < fixed::Zero()) != sign || (s2 < fixed::Zero()) != sign || (s3 < fixed::Zero()) != sign)
		return true; // ray cuts through the square

	return false;
}

/**
 * Separating axis test; returns true if the square defined by u/v/halfSize at the origin
 * is not entirely on the clockwise side of a line in direction 'axis' passing through 'a'
 */
static bool SquareSAT(const CFixedVector2D& a, const CFixedVector2D& axis, const CFixedVector2D& u, const CFixedVector2D& v, const CFixedVector2D& halfSize)
{
	fixed hw = halfSize.X;
	fixed hh = halfSize.Y;

	CFixedVector2D p = axis.Perpendicular();
	if (p.Dot((u.Multiply(hw) + v.Multiply(hh)) - a) <= fixed::Zero())
		return true;
	if (p.Dot((u.Multiply(hw) - v.Multiply(hh)) - a) <= fixed::Zero())
		return true;
	if (p.Dot((-u.Multiply(hw) - v.Multiply(hh)) - a) <= fixed::Zero())
		return true;
	if (p.Dot((-u.Multiply(hw) + v.Multiply(hh)) - a) <= fixed::Zero())
		return true;

	return false;
}

bool Geometry::TestSquareSquare(
		const CFixedVector2D& c0, const CFixedVector2D& u0, const CFixedVector2D& v0, const CFixedVector2D& halfSize0,
		const CFixedVector2D& c1, const CFixedVector2D& u1, const CFixedVector2D& v1, const CFixedVector2D& halfSize1)
{
	// TODO: need to test this carefully

	CFixedVector2D corner0a = c0 + u0.Multiply(halfSize0.X) + v0.Multiply(halfSize0.Y);
	CFixedVector2D corner0b = c0 - u0.Multiply(halfSize0.X) - v0.Multiply(halfSize0.Y);
	CFixedVector2D corner1a = c1 + u1.Multiply(halfSize1.X) + v1.Multiply(halfSize1.Y);
	CFixedVector2D corner1b = c1 - u1.Multiply(halfSize1.X) - v1.Multiply(halfSize1.Y);

	// Do a SAT test for each square vs each edge of the other square
	if (!SquareSAT(corner0a - c1, -u0, u1, v1, halfSize1))
		return false;
	if (!SquareSAT(corner0a - c1, v0, u1, v1, halfSize1))
		return false;
	if (!SquareSAT(corner0b - c1, u0, u1, v1, halfSize1))
		return false;
	if (!SquareSAT(corner0b - c1, -v0, u1, v1, halfSize1))
		return false;
	if (!SquareSAT(corner1a - c0, -u1, u0, v0, halfSize0))
		return false;
	if (!SquareSAT(corner1a - c0, v1, u0, v0, halfSize0))
		return false;
	if (!SquareSAT(corner1b - c0, u1, u0, v0, halfSize0))
		return false;
	if (!SquareSAT(corner1b - c0, -v1, u0, v0, halfSize0))
		return false;

	return true;
}

int Geometry::GetPerimeterDistance(int x_max, int y_max, int x, int y)
{
	if (x_max <= 0 || y_max <= 0)
		return 0;

	int quarter = x_max + y_max;
	if (x == x_max && y >= 0)
		return y;
	if (y == y_max)
		return quarter - x;
	if (x == -x_max)
		return 2 * quarter - y;
	if (y == -y_max)
		return 3 * quarter + x;
	if (x == x_max)
		return 4 * quarter + y;
	return 0;
}

std::pair<int, int> Geometry::GetPerimeterCoordinates(int x_max, int y_max, int k)
{
	if (x_max <= 0 || y_max <= 0)
		return std::pair<int, int>(0, 0);

	int quarter = x_max + y_max;
	k %= 4 * quarter;
	if (k < 0)
		k += 4 * quarter;

	if (k < y_max)
		return std::pair<int, int>(x_max, k);
	if (k < quarter + x_max)
		return std::pair<int, int>(quarter - k, y_max);
	if (k < 2 * quarter + y_max)
		return std::pair<int, int>(-x_max, 2 * quarter - k);
	if (k < 3 * quarter + x_max)
		return std::pair<int, int>(k - 3 * quarter, -y_max);
	return std::pair<int, int>(x_max, k - 4 * quarter);
}
