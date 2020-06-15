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

#include "graphics/Terrain.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"

#include "simulation2/system/Entity.h"
#include "simulation2/system/ParamNode.h"
#include "PathGoal.h"

typedef u16 pass_class_t;
template<typename T>
class Grid;

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
		i = static_cast<u16>(Clamp((x / NAVCELL_SIZE_INT).ToInt_RoundToNegInfinity(), 0, w - 1));
		j = static_cast<u16>(Clamp((z / NAVCELL_SIZE_INT).ToInt_RoundToNegInfinity(), 0, h - 1));
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
	bool CheckLineMovement(entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1,
	                              pass_class_t passClass, const Grid<NavcellData>& grid);
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
	PathfinderPassability(pass_class_t mask, const CParamNode& node);

	bool IsPassable(fixed waterdepth, fixed steepness, fixed shoredist) const
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
