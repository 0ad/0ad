/* Copyright (C) 2015 Wildfire Games.
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

CFixedVector2D Geometry::GetHalfBoundingBox(CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize)
{
	return CFixedVector2D(
		u.X.Multiply(halfSize.X).Absolute() + v.X.Multiply(halfSize.Y).Absolute(),
		u.Y.Multiply(halfSize.X).Absolute() + v.Y.Multiply(halfSize.Y).Absolute()
	);
}

float Geometry::ChordToCentralAngle(const float chordLength, const float radius)
{
	return acosf(1.f - SQR(chordLength)/(2.f*SQR(radius))); // cfr. law of cosines
}

fixed Geometry::DistanceToSquare(CFixedVector2D point, CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize, bool countInsideAsZero)
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

	// du, dv are the location of the point in the square's coordinate system
	fixed du = point.Dot(u);
	fixed dv = point.Dot(v);

	fixed hw = halfSize.X;
	fixed hh = halfSize.Y;

	if (-hw < du && du < hw) // regions B, I, G
	{
		fixed closest = (dv.Absolute() - hh).Absolute(); // horizontal edges

		if (-hh < dv && dv < hh) // region I
			closest = countInsideAsZero ? fixed::Zero() : std::min(closest, (du.Absolute() - hw).Absolute()); // vertical edges

		return closest;
	}
	else if (-hh < dv && dv < hh) // regions D, E
	{
		return (du.Absolute() - hw).Absolute(); // vertical edges
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

		return (corner - point).Length();
	}
}

// Same as above except it does not use Length.
// For explanations refer to DistanceToSquare
fixed Geometry::DistanceToSquareSquared(CFixedVector2D point, CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize, bool countInsideAsZero)
{
	fixed du = point.Dot(u);
	fixed dv = point.Dot(v);
	
	fixed hw = halfSize.X;
	fixed hh = halfSize.Y;
	
	if (-hw < du && du < hw) // regions B, I, G
	{
		fixed closest = (dv.Absolute() - hh).Multiply(dv.Absolute() - hh); // horizontal edges
		
		if (-hh < dv && dv < hh) // region I
			closest = countInsideAsZero ? fixed::Zero() : std::min(closest, (du.Absolute() - hw).Multiply(du.Absolute() - hw)); // vertical edges
		
		return closest;
	}
	else if (-hh < dv && dv < hh) // regions D, E
	{
		return (du.Absolute() - hw).Multiply(du.Absolute() - hw); // vertical edges
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
		
		CFixedVector2D result(corner-point);
		return (result.X.Multiply(result.X) + result.Y.Multiply(result.Y));
	}
}

CFixedVector2D Geometry::NearestPointOnSquare(CFixedVector2D point, CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize)
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

bool Geometry::TestRaySquare(CFixedVector2D a, CFixedVector2D b, CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize)
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

bool Geometry::TestRayAASquare(CFixedVector2D a, CFixedVector2D b, CFixedVector2D halfSize)
{
	// Exactly like TestRaySquare with u=(1,0), v=(0,1)

	// Assume the compiler is clever enough to inline and simplify all this
	// (TODO: stop assuming that)
	CFixedVector2D u (fixed::FromInt(1), fixed::Zero());
	CFixedVector2D v (fixed::Zero(), fixed::FromInt(1));

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

/**
 * Separating axis test; returns true if the square defined by u/v/halfSize at the origin
 * is not entirely on the clockwise side of a line in direction 'axis' passing through 'a'
 */
static bool SquareSAT(CFixedVector2D a, CFixedVector2D axis, CFixedVector2D u, CFixedVector2D v, CFixedVector2D halfSize)
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
		CFixedVector2D c0, CFixedVector2D u0, CFixedVector2D v0, CFixedVector2D halfSize0,
		CFixedVector2D c1, CFixedVector2D u1, CFixedVector2D v1, CFixedVector2D halfSize1)
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
