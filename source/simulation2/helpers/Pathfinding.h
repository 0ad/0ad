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

#ifndef INCLUDED_PATHFINDING
#define INCLUDED_PATHFINDING

#include "maths/MathUtil.h"
#include "ps/CLogger.h"

#include "simulation2/system/Entity.h"
#include "simulation2/system/ParamNode.h"
#include "graphics/Terrain.h"
#include "Grid.h"
#include "PathGoal.h"

typedef u16 pass_class_t;

struct LongPathRequest
{
	u32 ticket;
	entity_pos_t x0;
	entity_pos_t z0;
	PathGoal goal;
	pass_class_t passClass;
	entity_id_t notify;
};

struct ShortPathRequest
{
	u32 ticket;
	entity_pos_t x0;
	entity_pos_t z0;
	entity_pos_t clearance;
	entity_pos_t range;
	PathGoal goal;
	pass_class_t passClass;
	bool avoidMovingUnits;
	entity_id_t group;
	entity_id_t notify;
};

struct Waypoint
{
	entity_pos_t x, z;
};

/**
 * Returned path.
 * Waypoints are in *reverse* order (the earliest is at the back of the list)
 */
struct WaypointPath
{
	std::vector<Waypoint> m_Waypoints;
};

/**
 * Represents the cost of a path consisting of horizontal/vertical and
 * diagonal movements over a uniform-cost grid.
 * Maximum path length before overflow is about 45K steps.
 */
struct PathCost
{
	PathCost() : data(0) { }

	/// Construct from a number of horizontal/vertical and diagonal steps
	PathCost(u16 hv, u16 d)
		: data(hv * 65536 + d * 92682) // 2^16 * sqrt(2) == 92681.9
	{
	}

	/// Construct for horizontal/vertical movement of given number of steps
	static PathCost horizvert(u16 n)
	{
		return PathCost(n, 0);
	}

	/// Construct for diagonal movement of given number of steps
	static PathCost diag(u16 n)
	{
		return PathCost(0, n);
	}

	PathCost operator+(const PathCost& a) const
	{
		PathCost c;
		c.data = data + a.data;
		return c;
	}

	PathCost& operator+=(const PathCost& a)
	{
		data += a.data;
		return *this;
	}

	bool operator<=(const PathCost& b) const { return data <= b.data; }
	bool operator< (const PathCost& b) const { return data <  b.data; }
	bool operator>=(const PathCost& b) const { return data >= b.data; }
	bool operator>(const PathCost& b) const { return data >  b.data; }

	u32 ToInt()
	{
		return data;
	}

private:
	u32 data;
};

static const int PASS_CLASS_BITS = 16;
typedef u16 NavcellData; // 1 bit per passability class (up to PASS_CLASS_BITS)
#define IS_PASSABLE(item, classmask) (((item) & (classmask)) == 0)
#define PASS_CLASS_MASK_FROM_INDEX(id) ((pass_class_t)(1u << id))
#define SPECIAL_PASS_CLASS PASS_CLASS_MASK_FROM_INDEX((PASS_CLASS_BITS-1)) // 16th bit, used for special in-place computations

namespace Pathfinding
{
	/**
	 * The long-range pathfinder operates primarily over a navigation grid (a uniform-cost
	 * 2D passability grid, with horizontal/vertical (not diagonal) connectivity).
	 * This is based on the terrain tile passability, plus the rasterized shapes of
	 * obstructions, all expanded outwards by the radius of the units.
	 * Since units are much smaller than terrain tiles, the nav grid should be
	 * higher resolution than the tiles.
	 * We therefore split each terrain tile into NxN "nav cells" (for some integer N,
	 * preferably a power of two).
	 */
	const int NAVCELLS_PER_TILE = 4;

	/**
	 * Size of a navcell in metres ( = TERRAIN_TILE_SIZE / NAVCELLS_PER_TILE)
	 */
	const fixed NAVCELL_SIZE = fixed::FromInt((int)TERRAIN_TILE_SIZE) / Pathfinding::NAVCELLS_PER_TILE;
	const int NAVCELL_SIZE_INT = 1;
	const int NAVCELL_SIZE_LOG2 = 0;

	/**
	 * For extending the goal outwards/inwards a little bit
	 * NOTE: keep next to the definition of NAVCELL_SIZE to avoid init order problems
	 *	between translation units.
	 * TODO: figure out whether this is actually needed. It was added back in r8751 (in 2010) for unclear reasons
	 * and it does not seem to really improve behavior today
	 */
	const entity_pos_t GOAL_DELTA = NAVCELL_SIZE/8;

	/**
	 * To make sure the long-range pathfinder is more strict than the short-range one,
	 * we need to slightly over-rasterize. So we extend the clearance radius by 1.
	 */
	const entity_pos_t CLEARANCE_EXTENSION_RADIUS = fixed::FromInt(1);

	/**
	 * Compute the navcell indexes on the grid nearest to a given point
	 * w, h are the grid dimensions, i.e. the number of navcells per side
	 */
	inline void NearestNavcell(entity_pos_t x, entity_pos_t z, u16& i, u16& j, u16 w, u16 h)
	{
		// Use NAVCELL_SIZE_INT to save the cost of dividing by a fixed
		i = (u16)clamp((x / NAVCELL_SIZE_INT).ToInt_RoundToNegInfinity(), 0, w - 1);
		j = (u16)clamp((z / NAVCELL_SIZE_INT).ToInt_RoundToNegInfinity(), 0, h - 1);
	}

	/**
	 * Returns the position of the center of the given tile
	 */
	inline void TileCenter(u16 i, u16 j, entity_pos_t& x, entity_pos_t& z)
	{
		cassert(TERRAIN_TILE_SIZE % 2 == 0);
		x = entity_pos_t::FromInt(i*(int)TERRAIN_TILE_SIZE + (int)TERRAIN_TILE_SIZE / 2);
		z = entity_pos_t::FromInt(j*(int)TERRAIN_TILE_SIZE + (int)TERRAIN_TILE_SIZE / 2);
	}

	inline void NavcellCenter(u16 i, u16 j, entity_pos_t& x, entity_pos_t& z)
	{
		x = entity_pos_t::FromInt(i * 2 + 1).Multiply(NAVCELL_SIZE / 2);
		z = entity_pos_t::FromInt(j * 2 + 1).Multiply(NAVCELL_SIZE / 2);
	}

	/*
	 * Checks that the line (x0,z0)-(x1,z1) does not intersect any impassable navcells.
	 */
	inline bool CheckLineMovement(entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1,
		pass_class_t passClass, const Grid<NavcellData>& grid)
	{
		// We shouldn't allow lines between diagonally-adjacent navcells.
		// It doesn't matter whether we allow lines precisely along the edge
		// of an impassable navcell.

		// To rasterise the line:
		// If the line is (e.g.) aiming up-right, then we start at the navcell
		// containing the start point and the line must either end in that navcell
		// or else exit along the top edge or the right edge (or through the top-right corner,
		// which we'll arbitrary treat as the horizontal edge).
		// So we jump into the adjacent navcell across that edge, and continue.

		// To handle the special case of units that are stuck on impassable cells,
		// we allow them to move from an impassable to a passable cell (but not
		// vice versa).

		u16 i0, j0, i1, j1;
		NearestNavcell(x0, z0, i0, j0, grid.m_W, grid.m_H);
		NearestNavcell(x1, z1, i1, j1, grid.m_W, grid.m_H);

		// Find which direction the line heads in
		int di = (i0 < i1 ? +1 : i1 < i0 ? -1 : 0);
		int dj = (j0 < j1 ? +1 : j1 < j0 ? -1 : 0);

		u16 i = i0;
		u16 j = j0;

		bool currentlyOnImpassable = !IS_PASSABLE(grid.get(i0, j0), passClass);

		while (true)
		{
			// Make sure we are still in the limits
			ENSURE(
				((di > 0 && i0 <= i && i <= i1) || (di < 0 && i1 <= i && i <= i0) || (di == 0 && i == i0)) &&
				((dj > 0 && j0 <= j && j <= j1) || (dj < 0 && j1 <= j && j <= j0) || (dj == 0 && j == j0)));

			// Fail if we're moving onto an impassable navcell
			bool passable = IS_PASSABLE(grid.get(i, j), passClass);
			if (passable)
				currentlyOnImpassable = false;
			else if (!currentlyOnImpassable)
				return false;

			// Succeed if we're at the target
			if (i == i1 && j == j1)
				return true;

			// If we can only move horizontally/vertically, then just move in that direction
			// If we are reaching the limits, we can go straight to the end
			if (di == 0 || i == i1)
			{
				j += dj;
				continue;
			}
			else if (dj == 0 || j == j1)
			{
				i += di;
				continue;
			}

			// Otherwise we need to check which cell to move into:

			// Check whether the line intersects the horizontal (top/bottom) edge of
			// the current navcell.
			// Horizontal edge is (i, j + (dj>0?1:0)) .. (i + 1, j + (dj>0?1:0))
			// Since we already know the line is moving from this navcell into a different
			// navcell, we simply need to test that the edge's endpoints are not both on the
			// same side of the line.

			// If we are crossing exactly a vertex of the grid, we will get dota or dotb equal
			// to 0. In that case we arbitrarily choose to move of dj.
			// This only works because we handle the case (i == i1 || j == j1) beforehand.
			// Otherwise we could go outside the j limits and never reach the final navcell.

			entity_pos_t xia = entity_pos_t::FromInt(i).Multiply(Pathfinding::NAVCELL_SIZE);
			entity_pos_t xib = entity_pos_t::FromInt(i+1).Multiply(Pathfinding::NAVCELL_SIZE);
			entity_pos_t zj = entity_pos_t::FromInt(j + (dj+1)/2).Multiply(Pathfinding::NAVCELL_SIZE);

			CFixedVector2D perp = CFixedVector2D(x1 - x0, z1 - z0).Perpendicular();
			entity_pos_t dota = (CFixedVector2D(xia, zj) - CFixedVector2D(x0, z0)).Dot(perp);
			entity_pos_t dotb = (CFixedVector2D(xib, zj) - CFixedVector2D(x0, z0)).Dot(perp);

			// If the horizontal edge is fully on one side of the line, so the line doesn't
			// intersect it, we should move across the vertical edge instead
			if ((dota < entity_pos_t::Zero() && dotb < entity_pos_t::Zero()) ||
				(dota > entity_pos_t::Zero() && dotb > entity_pos_t::Zero()))
				i += di;
			else
				j += dj;
		}
	}
}

/*
 * For efficient pathfinding we want to try hard to minimise the per-tile search cost,
 * so we precompute the tile passability flags and movement costs for the various different
 * types of unit.
 * We also want to minimise memory usage (there can easily be 100K tiles so we don't want
 * to store many bytes for each).
 *
 * To handle passability efficiently, we have a small number of passability classes
 * (e.g. "infantry", "ship"). Each unit belongs to a single passability class, and
 * uses that for all its pathfinding.
 * Passability is determined by water depth, terrain slope, forestness, buildingness.
 * We need at least one bit per class per tile to represent passability.
 *
 * Not all pass classes are used for actual pathfinding. The pathfinder calls
 * CCmpObstructionManager's Rasterize() to add shapes onto the passability grid.
 * Which shapes are rasterized depend on the value of the m_Obstructions of each passability
 * class.
 *
 * Passabilities not used for unit pathfinding should not use the Clearance attribute, and
 * will get a zero clearance value.
 */
class PathfinderPassability
{
public:
	PathfinderPassability(pass_class_t mask, const CParamNode& node) :
		m_Mask(mask)
	{
		if (node.GetChild("MinWaterDepth").IsOk())
			m_MinDepth = node.GetChild("MinWaterDepth").ToFixed();
		else
			m_MinDepth = std::numeric_limits<fixed>::min();

		if (node.GetChild("MaxWaterDepth").IsOk())
			m_MaxDepth = node.GetChild("MaxWaterDepth").ToFixed();
		else
			m_MaxDepth = std::numeric_limits<fixed>::max();

		if (node.GetChild("MaxTerrainSlope").IsOk())
			m_MaxSlope = node.GetChild("MaxTerrainSlope").ToFixed();
		else
			m_MaxSlope = std::numeric_limits<fixed>::max();

		if (node.GetChild("MinShoreDistance").IsOk())
			m_MinShore = node.GetChild("MinShoreDistance").ToFixed();
		else
			m_MinShore = std::numeric_limits<fixed>::min();

		if (node.GetChild("MaxShoreDistance").IsOk())
			m_MaxShore = node.GetChild("MaxShoreDistance").ToFixed();
		else
			m_MaxShore = std::numeric_limits<fixed>::max();

		if (node.GetChild("Clearance").IsOk())
		{
			m_Clearance = node.GetChild("Clearance").ToFixed();

			/* According to Philip who designed the original doc (in docs/pathfinder.pdf),
			 * clearance should usually be integer to ensure consistent behavior when rasterizing
			 * the passability map.
			 * This seems doubtful to me and my pathfinder fix makes having a clearance of 0.8 quite convenient
			 * so I comment out this check, but leave it here for the purpose of documentation should a bug arise.

			if (!(m_Clearance % Pathfinding::NAVCELL_SIZE).IsZero())
			{
				// If clearance isn't an integer number of navcells then we'll
				// probably get weird behaviour when expanding the navcell grid
				// by clearance, vs expanding static obstructions by clearance
				LOGWARNING("Pathfinder passability class has clearance %f, should be multiple of %f",
					m_Clearance.ToFloat(), Pathfinding::NAVCELL_SIZE.ToFloat());
			}*/
		}
		else
			m_Clearance = fixed::Zero();

		if (node.GetChild("Obstructions").IsOk())
		{
			std::wstring obstructions = node.GetChild("Obstructions").ToString();
			if (obstructions == L"none")
				m_Obstructions = NONE;
			else if (obstructions == L"pathfinding")
				m_Obstructions = PATHFINDING;
			else if (obstructions == L"foundation")
				m_Obstructions = FOUNDATION;
			else
			{
				LOGERROR("Invalid value for Obstructions in pathfinder.xml for pass class %d", mask);
				m_Obstructions = NONE;
			}
		}
		else
			m_Obstructions = NONE;
	}

	bool IsPassable(fixed waterdepth, fixed steepness, fixed shoredist)
	{
		return ((m_MinDepth <= waterdepth && waterdepth <= m_MaxDepth) && (steepness < m_MaxSlope) && (m_MinShore <= shoredist && shoredist <= m_MaxShore));
	}

	pass_class_t m_Mask;

	fixed m_Clearance; // min distance from static obstructions

	enum ObstructionHandling
	{
		NONE,
		PATHFINDING,
		FOUNDATION
	};
	ObstructionHandling m_Obstructions;

private:
	fixed m_MinDepth;
	fixed m_MaxDepth;
	fixed m_MaxSlope;
	fixed m_MinShore;
	fixed m_MaxShore;
};

#endif // INCLUDED_PATHFINDING
