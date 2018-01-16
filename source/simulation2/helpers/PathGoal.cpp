/* Copyright (C) 2018 Wildfire Games.
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

#include "PathGoal.h"

#include "graphics/Terrain.h"
#include "Pathfinding.h"

#include "Geometry.h"

static bool NavcellContainsCircle(int i, int j, fixed x, fixed z, fixed r, bool inside)
{
	// Accept any navcell (i,j) that contains a point which is inside[/outside]
	// (or on the edge of) the circle

	// Get world-space bounds of navcell
	entity_pos_t x0 = entity_pos_t::FromInt(i).Multiply(Pathfinding::NAVCELL_SIZE);
	entity_pos_t z0 = entity_pos_t::FromInt(j).Multiply(Pathfinding::NAVCELL_SIZE);
	entity_pos_t x1 = x0 + Pathfinding::NAVCELL_SIZE;
	entity_pos_t z1 = z0 + Pathfinding::NAVCELL_SIZE;

	if (inside)
	{
		// Get the point inside the navcell closest to (x,z)
		entity_pos_t nx = Clamp(x, x0, x1);
		entity_pos_t nz = Clamp(z, z0, z1);
		// Check if that point is inside the circle
		return (CFixedVector2D(nx, nz) - CFixedVector2D(x, z)).CompareLength(r) <= 0;
	}
	else
	{
		// If any corner of the navcell is outside the circle, return true.
		// Otherwise, since the circle is convex, there cannot be any other point
		// in the navcell that is outside the circle.
		return (
		    (CFixedVector2D(x0, z0) - CFixedVector2D(x, z)).CompareLength(r) >= 0
		 || (CFixedVector2D(x1, z0) - CFixedVector2D(x, z)).CompareLength(r) >= 0
		 || (CFixedVector2D(x0, z1) - CFixedVector2D(x, z)).CompareLength(r) >= 0
		 || (CFixedVector2D(x1, z1) - CFixedVector2D(x, z)).CompareLength(r) >= 0
		);
	}
}

static bool NavcellContainsSquare(int i, int j,
	fixed x, fixed z, CFixedVector2D u, CFixedVector2D v, fixed hw, fixed hh,
	bool inside)
{
	// Accept any navcell (i,j) that contains a point which is inside[/outside]
	// (or on the edge of) the square

	// Get world-space bounds of navcell
	entity_pos_t x0 = entity_pos_t::FromInt(i).Multiply(Pathfinding::NAVCELL_SIZE);
	entity_pos_t z0 = entity_pos_t::FromInt(j).Multiply(Pathfinding::NAVCELL_SIZE);
	entity_pos_t x1 = x0 + Pathfinding::NAVCELL_SIZE;
	entity_pos_t z1 = z0 + Pathfinding::NAVCELL_SIZE;

	if (inside)
	{
		// Get the point inside the navcell closest to (x,z)
		entity_pos_t nx = Clamp(x, x0, x1);
		entity_pos_t nz = Clamp(z, z0, z1);
		// Check if that point is inside the circle
		return Geometry::PointIsInSquare(CFixedVector2D(nx - x, nz - z), u, v, CFixedVector2D(hw, hh));
	}
	else
	{
		// If any corner of the navcell is outside the square, return true.
		// Otherwise, since the square is convex, there cannot be any other point
		// in the navcell that is outside the square.
		return (
		    !Geometry::PointIsInSquare(CFixedVector2D(x0 - x, z0 - z), u, v, CFixedVector2D(hw, hh))
		 || !Geometry::PointIsInSquare(CFixedVector2D(x1 - x, z0 - z), u, v, CFixedVector2D(hw, hh))
		 || !Geometry::PointIsInSquare(CFixedVector2D(x0 - x, z1 - z), u, v, CFixedVector2D(hw, hh))
		 || !Geometry::PointIsInSquare(CFixedVector2D(x1 - x, z1 - z), u, v, CFixedVector2D(hw, hh))
		);
	}
}

bool PathGoal::NavcellContainsGoal(int i, int j) const
{
	switch (type)
	{
	case POINT:
	{
		// Only accept a single navcell
		int gi = (x >> Pathfinding::NAVCELL_SIZE_LOG2).ToInt_RoundToNegInfinity();
		int gj = (z >> Pathfinding::NAVCELL_SIZE_LOG2).ToInt_RoundToNegInfinity();
		return gi == i && gj == j;
	}
	case CIRCLE:
		return NavcellContainsCircle(i, j, x, z, hw, true);
	case INVERTED_CIRCLE:
		return NavcellContainsCircle(i, j, x, z, hw, false);
	case SQUARE:
		return NavcellContainsSquare(i, j, x, z, u, v, hw, hh, true);
	case INVERTED_SQUARE:
		return NavcellContainsSquare(i, j, x, z, u, v, hw, hh, false);
	NODEFAULT;
	}
}

bool PathGoal::NavcellRectContainsGoal(int i0, int j0, int i1, int j1, int* gi, int* gj) const
{
	// Get min/max to simplify range checks
	int imin = std::min(i0, i1);
	int imax = std::max(i0, i1);
	int jmin = std::min(j0, j1);
	int jmax = std::max(j0, j1);

	// Direction to iterate from (i0,j0) towards (i1,j1)
	int di = i1 < i0 ? -1 : +1;
	int dj = j1 < j0 ? -1 : +1;

	switch (type)
	{
	case POINT:
	{
		// Calculate the navcell that contains the point goal
		int i = (x >> Pathfinding::NAVCELL_SIZE_LOG2).ToInt_RoundToNegInfinity();
		int j = (z >> Pathfinding::NAVCELL_SIZE_LOG2).ToInt_RoundToNegInfinity();
		// If that goal navcell is in the given range, return it
		if (imin <= i && i <= imax && jmin <= j && j <= jmax)
		{
			if (gi)
				*gi = i;
			if (gj)
				*gj = j;
			return true;
		}
		return false;
	}

	case CIRCLE:
	{
		// Loop over all navcells in the given range (starting at (i0,j0) since
		// this function is meant to find the goal navcell nearest to there
		// assuming jmin==jmax || imin==imax),
		// and check whether any point in each navcell is within the goal circle.
		// (TODO: this is pretty inefficient.)
		for (int j = j0; jmin <= j && j <= jmax; j += dj)
		{
			for (int i = i0; imin <= i && i <= imax; i += di)
			{
				if (NavcellContainsCircle(i, j, x, z, hw, true))
				{
					if (gi)
						*gi = i;
					if (gj)
						*gj = j;
					return true;
				}
			}
		}
		return false;
	}

	case INVERTED_CIRCLE:
	{
		// Loop over all navcells in the given range (starting at (i0,j0) since
		// this function is meant to find the goal navcell nearest to there
		// assuming jmin==jmax || imin==imax),
		// and check whether any point in each navcell is outside the goal circle.
		// (TODO: this is pretty inefficient.)
		for (int j = j0; jmin <= j && j <= jmax; j += dj)
		{
			for (int i = i0; imin <= i && i <= imax; i += di)
			{
				if (NavcellContainsCircle(i, j, x, z, hw, false))
				{
					if (gi)
						*gi = i;
					if (gj)
						*gj = j;
					return true;
				}
			}
		}
		return false;
	}

	case SQUARE:
	{
		// Loop over all navcells in the given range (starting at (i0,j0) since
		// this function is meant to find the goal navcell nearest to there
		// assuming jmin==jmax || imin==imax),
		// and check whether any point in each navcell is within the goal square.
		// (TODO: this is pretty inefficient.)
		for (int j = j0; jmin <= j && j <= jmax; j += dj)
		{
			for (int i = i0; imin <= i && i <= imax; i += di)
			{
				if (NavcellContainsSquare(i, j, x, z, u, v, hw, hh, true))
				{
					if (gi)
						*gi = i;
					if (gj)
						*gj = j;
					return true;
				}
			}
		}
		return false;
	}

	case INVERTED_SQUARE:
	{
		// Loop over all navcells in the given range (starting at (i0,j0) since
		// this function is meant to find the goal navcell nearest to there
		// assuming jmin==jmax || imin==imax),
		// and check whether any point in each navcell is outside the goal square.
		// (TODO: this is pretty inefficient.)
		for (int j = j0; jmin <= j && j <= jmax; j += dj)
		{
			for (int i = i0; imin <= i && i <= imax; i += di)
			{
				if (NavcellContainsSquare(i, j, x, z, u, v, hw, hh, false))
				{
					if (gi)
						*gi = i;
					if (gj)
						*gj = j;
					return true;
				}
			}
		}
		return false;
	}

	NODEFAULT;
	}
}

bool PathGoal::RectContainsGoal(entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1) const
{
	switch (type)
	{
	case POINT:
		return x0 <= x && x <= x1 && z0 <= z && z <= z1;

	case CIRCLE:
	{
		entity_pos_t nx = Clamp(x, x0, x1);
		entity_pos_t nz = Clamp(z, z0, z1);
		return (CFixedVector2D(nx, nz) - CFixedVector2D(x, z)).CompareLength(hw) <= 0;
	}

	case INVERTED_CIRCLE:
	{
		return (
		    (CFixedVector2D(x0, z0) - CFixedVector2D(x, z)).CompareLength(hw) >= 0
		 || (CFixedVector2D(x1, z0) - CFixedVector2D(x, z)).CompareLength(hw) >= 0
		 || (CFixedVector2D(x0, z1) - CFixedVector2D(x, z)).CompareLength(hw) >= 0
		 || (CFixedVector2D(x1, z1) - CFixedVector2D(x, z)).CompareLength(hw) >= 0
		);
	}

	case SQUARE:
	{
		entity_pos_t nx = Clamp(x, x0, x1);
		entity_pos_t nz = Clamp(z, z0, z1);
		return Geometry::PointIsInSquare(CFixedVector2D(nx - x, nz - z), u, v, CFixedVector2D(hw, hh));
	}

	case INVERTED_SQUARE:
	{
		return (
		    !Geometry::PointIsInSquare(CFixedVector2D(x0 - x, z0 - z), u, v, CFixedVector2D(hw, hh))
		 || !Geometry::PointIsInSquare(CFixedVector2D(x1 - x, z0 - z), u, v, CFixedVector2D(hw, hh))
		 || !Geometry::PointIsInSquare(CFixedVector2D(x0 - x, z1 - z), u, v, CFixedVector2D(hw, hh))
		 || !Geometry::PointIsInSquare(CFixedVector2D(x1 - x, z1 - z), u, v, CFixedVector2D(hw, hh))
		);
	}

	NODEFAULT;
	}
}

fixed PathGoal::DistanceToPoint(CFixedVector2D pos) const
{
	CFixedVector2D d(pos.X - x, pos.Y - z);

	switch (type)
	{
	case POINT:
		return d.Length();

	case CIRCLE:
		return d.CompareLength(hw) <= 0 ? fixed::Zero() : d.Length() - hw;

	case INVERTED_CIRCLE:
		return d.CompareLength(hw) >= 0 ? fixed::Zero() : hw - d.Length();

	case SQUARE:
	{
		CFixedVector2D halfSize(hw, hh);
		return Geometry::PointIsInSquare(d, u, v, halfSize) ?
			fixed::Zero() : Geometry::DistanceToSquare(d, u, v, halfSize);
	}

	case INVERTED_SQUARE:
	{
		CFixedVector2D halfSize(hw, hh);
		return !Geometry::PointIsInSquare(d, u, v, halfSize) ?
			fixed::Zero() : Geometry::DistanceToSquare(d, u, v, halfSize);
	}

	NODEFAULT;
	}
}

CFixedVector2D PathGoal::NearestPointOnGoal(CFixedVector2D pos) const
{
	CFixedVector2D g(x, z);

	switch (type)
	{
	case POINT:
		return g;

	case CIRCLE:
	{
		CFixedVector2D d(pos.X - x, pos.Y - z);
		if (d.CompareLength(hw) <= 0)
			return pos;

		d.Normalize(hw);
		return g + d;
	}

	case INVERTED_CIRCLE:
	{
		CFixedVector2D d(pos.X - x, pos.Y - z);
		if (d.CompareLength(hw) >= 0)
			return pos;

		if (d.IsZero())
			d = CFixedVector2D(fixed::FromInt(1), fixed::Zero()); // some arbitrary direction
		d.Normalize(hw);
		return g + d;
	}

	case SQUARE:
	{
		CFixedVector2D halfSize(hw, hh);
		CFixedVector2D d(pos.X - x, pos.Y - z);
		return Geometry::PointIsInSquare(d, u, v, halfSize) ?
			pos : g + Geometry::NearestPointOnSquare(d, u, v, halfSize);
	}

	case INVERTED_SQUARE:
	{
		CFixedVector2D halfSize(hw, hh);
		CFixedVector2D d(pos.X - x, pos.Y - z);
		return !Geometry::PointIsInSquare(d, u, v, halfSize) ?
			pos : g + Geometry::NearestPointOnSquare(d, u, v, halfSize);
	}

	NODEFAULT;
	}
}
