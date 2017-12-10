/* Copyright (C) 2017 Wildfire Games.
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

#include "LongPathfinder.h"

#include "lib/bits.h"
#include "ps/Profile.h"

#include "Geometry.h"

/**
 * Jump point cache.
 *
 * The JPS algorithm wants to efficiently either find the first jump point
 * in some direction from some cell (not counting the cell itself),
 * if it is reachable without crossing any impassable cells;
 * or know that there is no such reachable jump point.
 * The jump point is always on a passable cell.
 * We cache that data to allow fast lookups, which helps performance
 * significantly (especially on sparse maps).
 * Recalculation might be expensive but the underlying passability data
 * changes relatively rarely.
 *
 * To allow the algorithm to detect goal cells, we want to treat them as
 * jump points too. (That means the algorithm will push those cells onto
 * its open queue, and will eventually pop a goal cell and realise it's done.)
 * (Goals might be circles/squares/etc, not just a single cell.)
 * But the goal generally changes for every path request, so we can't cache
 * it like the normal jump points.
 * Instead, if there's no jump point from some cell then we'll cache the
 * first impassable cell as an 'obstruction jump point'
 * (with a flag to distinguish from a real jump point), and then the caller
 * can test whether the goal includes a cell that's closer than the first
 * (obstruction or real) jump point,
 * and treat the goal cell as a jump point in that case.
 *
 * We only ever need to find the jump point relative to a passable cell;
 * the cache is allowed to return bogus values for impassable cells.
 */
class JumpPointCache
{
	/**
	 * Simple space-inefficient row storage.
	 */
	struct RowRaw
	{
		std::vector<u16> data;

		size_t GetMemoryUsage() const
		{
			return data.capacity() * sizeof(u16);
		}

		RowRaw(int length)
		{
			data.resize(length);
		}

		/**
		 * Set cells x0 <= x < x1 to have jump point x1.
		 */
		void SetRange(int x0, int x1, bool obstruction)
		{
			ENSURE(0 <= x0 && x0 <= x1 && x1 < (int)data.size());
			for (int x = x0; x < x1; ++x)
				data[x] = (x1 << 1) | (obstruction ? 1 : 0);
		}

		/**
		 * Returns the coordinate of the next jump point xp (where x < xp),
		 * and whether it's an obstruction point or jump point.
		 */
		void Get(int x, int& xp, bool& obstruction)
		{
			ENSURE(0 <= x && x < (int)data.size());
			xp = data[x] >> 1;
			obstruction = data[x] & 1;
		}

		void Finish() { }
	};

	struct RowTree
	{
		/**
		 * Represents an interval [u15 x0, u16 x1)
		 * with a boolean obstruction flag,
		 * packed into a single u32.
		 */
		struct Interval
		{
			Interval() : data(0) { }

			Interval(int x0, int x1, bool obstruction)
			{
				ENSURE(0 <= x0 && x0 < 0x8000);
				ENSURE(0 <= x1 && x1 < 0x10000);
				data = ((u32)x0 << 17) | (u32)(obstruction ? 0x10000 : 0) | (u32)x1;
			}

			int x0() { return data >> 17; }
			int x1() { return data & 0xFFFF; }
			bool obstruction() { return (data & 0x10000) != 0; }

			u32 data;
		};

		std::vector<Interval> data;

		size_t GetMemoryUsage() const
		{
			return data.capacity() * sizeof(Interval);
		}

		RowTree(int UNUSED(length))
		{
		}

		void SetRange(int x0, int x1, bool obstruction)
		{
			ENSURE(0 <= x0 && x0 <= x1);
			data.emplace_back(x0, x1, obstruction);
		}

		/**
		 * Recursive helper function for Finish().
		 * Given two ranges [x0, pivot) and [pivot, x1) in the sorted array 'data',
		 * the pivot element is added onto the binary tree (stored flattened in an
		 * array), and then each range is split into two sub-ranges with a pivot in
		 * the middle (to ensure the tree remains balanced) and ConstructTree recurses.
		 */
		void ConstructTree(std::vector<Interval>& tree, size_t x0, size_t pivot, size_t x1, size_t idx_tree)
		{
			ENSURE(x0 < data.size());
			ENSURE(x1 <= data.size());
			ENSURE(x0 <= pivot);
			ENSURE(pivot < x1);
			ENSURE(idx_tree < tree.size());

			tree[idx_tree] = data[pivot];

			if (x0 < pivot)
				ConstructTree(tree, x0, (x0 + pivot) / 2, pivot, (idx_tree << 1) + 1);
			if (pivot + 1 < x1)
				ConstructTree(tree, pivot + 1, (pivot + x1) / 2, x1, (idx_tree << 1) + 2);
		}

		void Finish()
		{
			// Convert the sorted interval list into a balanced binary tree

			std::vector<Interval> tree;

			if (!data.empty())
			{
				size_t depth = ceil_log2(data.size() + 1);
				tree.resize((1 << depth) - 1);
				ConstructTree(tree, 0, data.size() / 2, data.size(), 0);
			}

			data.swap(tree);
		}

		void Get(int x, int& xp, bool& obstruction)
		{
			// Search the binary tree for an interval which contains x
			int i = 0;
			while (true)
			{
				ENSURE(i < (int)data.size());
				Interval interval = data[i];
				if (x < interval.x0())
					i = (i << 1) + 1;
				else if (x >= interval.x1())
					i = (i << 1) + 2;
				else
				{
					ENSURE(interval.x0() <= x && x < interval.x1());
					xp = interval.x1();
					obstruction = interval.obstruction();
					return;
				}
			}
		}
	};

	// Pick one of the row implementations
	typedef RowRaw Row;

public:
	int m_Width;
	int m_Height;
	std::vector<Row> m_JumpPointsRight;
	std::vector<Row> m_JumpPointsLeft;
	std::vector<Row> m_JumpPointsUp;
	std::vector<Row> m_JumpPointsDown;

	/**
	 * Compute the cached obstruction/jump points for each cell,
	 * in a single direction. By default the code assumes the rightwards
	 * (+i) direction; set 'transpose' to switch to upwards (+j),
	 * and/or set 'mirror' to reverse the direction.
	 */
	void ComputeRows(std::vector<Row>& rows,
		const Grid<NavcellData>& terrain, pass_class_t passClass,
		bool transpose, bool mirror)
	{
		int w = terrain.m_W;
		int h = terrain.m_H;

		if (transpose)
			std::swap(w, h);

		// Check the terrain passability, adjusted for transpose/mirror
#define TERRAIN_IS_PASSABLE(i, j) \
	IS_PASSABLE( \
		mirror \
		? (transpose ? terrain.get((j), w-1-(i)) : terrain.get(w-1-(i), (j))) \
		: (transpose ? terrain.get((j), (i)) : terrain.get((i), (j))) \
	, passClass)

		rows.reserve(h);
		for (int j = 0; j < h; ++j)
			rows.emplace_back(w);

		for (int j = 1; j < h - 1; ++j)
		{
			// Find the first passable cell.
			// Then, find the next jump/obstruction point after that cell,
			// and store that point for the passable range up to that cell,
			// then repeat.

			int i = 0;
			while (i < w)
			{
				// Restart the 'while' loop until we reach a passable cell
				if (!TERRAIN_IS_PASSABLE(i, j))
				{
					++i;
					continue;
				}

				// i is now a passable cell; find the next jump/obstruction point.
				// (We assume the map is surrounded by impassable cells, so we don't
				// need to explicitly check for world bounds here.)

				int i0 = i;
				while (true)
				{
					++i;

					// Check if we hit an obstructed tile
					if (!TERRAIN_IS_PASSABLE(i, j))
					{
						rows[j].SetRange(i0, i, true);
						break;
					}

					// Check if we reached a jump point
					if ((!TERRAIN_IS_PASSABLE(i - 1, j - 1) && TERRAIN_IS_PASSABLE(i, j - 1)) ||
						(!TERRAIN_IS_PASSABLE(i - 1, j + 1) && TERRAIN_IS_PASSABLE(i, j + 1)))
					{
						rows[j].SetRange(i0, i, false);
						break;
					}
				}
			}

			rows[j].Finish();
		}
#undef TERRAIN_IS_PASSABLE
	}

	void reset(const Grid<NavcellData>* terrain, pass_class_t passClass)
	{
		PROFILE3("JumpPointCache reset");
		TIMER(L"JumpPointCache reset");

		m_Width = terrain->m_W;
		m_Height = terrain->m_H;

		ComputeRows(m_JumpPointsRight, *terrain, passClass, false, false);
		ComputeRows(m_JumpPointsLeft, *terrain, passClass, false, true);
		ComputeRows(m_JumpPointsUp, *terrain, passClass, true, false);
		ComputeRows(m_JumpPointsDown, *terrain, passClass, true, true);
	}

	size_t GetMemoryUsage() const
	{
		size_t bytes = 0;
		for (int i = 0; i < m_Width; ++i)
		{
			bytes += m_JumpPointsUp[i].GetMemoryUsage();
			bytes += m_JumpPointsDown[i].GetMemoryUsage();
		}
		for (int j = 0; j < m_Height; ++j)
		{
			bytes += m_JumpPointsRight[j].GetMemoryUsage();
			bytes += m_JumpPointsLeft[j].GetMemoryUsage();
		}
		return bytes;
	}

	/**
	 * Returns the next jump point (or goal point) to explore,
	 * at (ip, j) where i < ip.
	 * Returns i if there is no such point.
	 */
	int GetJumpPointRight(int i, int j, const PathGoal& goal)
	{
		int ip;
		bool obstruction;
		m_JumpPointsRight[j].Get(i, ip, obstruction);
		// Adjust ip to be a goal cell, if there is one closer than the jump point;
		// and then return the new ip if there is a goal,
		// or the old ip if there is a (non-obstruction) jump point
		if (goal.NavcellRectContainsGoal(i + 1, j, ip - 1, j, &ip, NULL) || !obstruction)
			return ip;
		return i;
	}

	int GetJumpPointLeft(int i, int j, const PathGoal& goal)
	{
		int mip; // mirrored value, because m_JumpPointsLeft is generated from a mirrored map
		bool obstruction;
		m_JumpPointsLeft[j].Get(m_Width - 1 - i, mip, obstruction);
		int ip = m_Width - 1 - mip;
		if (goal.NavcellRectContainsGoal(i - 1, j, ip + 1, j, &ip, NULL) || !obstruction)
			return ip;
		return i;
	}

	int GetJumpPointUp(int i, int j, const PathGoal& goal)
	{
		int jp;
		bool obstruction;
		m_JumpPointsUp[i].Get(j, jp, obstruction);
		if (goal.NavcellRectContainsGoal(i, j + 1, i, jp - 1, NULL, &jp) || !obstruction)
			return jp;
		return j;
	}

	int GetJumpPointDown(int i, int j, const PathGoal& goal)
	{
		int mjp; // mirrored value
		bool obstruction;
		m_JumpPointsDown[i].Get(m_Height - 1 - j, mjp, obstruction);
		int jp = m_Height - 1 - mjp;
		if (goal.NavcellRectContainsGoal(i, j - 1, i, jp + 1, NULL, &jp) || !obstruction)
			return jp;
		return j;
	}
};

//////////////////////////////////////////////////////////

LongPathfinder::LongPathfinder() :
	m_UseJPSCache(false),
	m_Grid(NULL), m_GridSize(0),
	m_DebugOverlay(NULL), m_DebugGrid(NULL), m_DebugPath(NULL)
{
}

LongPathfinder::~LongPathfinder()
{
	SAFE_DELETE(m_DebugOverlay);
	SAFE_DELETE(m_DebugGrid);
	SAFE_DELETE(m_DebugPath);
}

#define PASSABLE(i, j) IS_PASSABLE(state.terrain->get(i, j), state.passClass)

// Calculate heuristic cost from tile i,j to goal
// (This ought to be an underestimate for correctness)
PathCost LongPathfinder::CalculateHeuristic(int i, int j, int iGoal, int jGoal)
{
	int di = abs(i - iGoal);
	int dj = abs(j - jGoal);
	int diag = std::min(di, dj);
	return PathCost(di - diag + dj - diag, diag);
}

// Do the A* processing for a neighbour tile i,j.
void LongPathfinder::ProcessNeighbour(int pi, int pj, int i, int j, PathCost pg, PathfinderState& state)
{
	// Reject impassable tiles
	if (!PASSABLE(i, j))
		return;

	PathfindTile& n = state.tiles->get(i, j);

	if (n.IsClosed())
		return;

	PathCost dg;
	if (pi == i)
		dg = PathCost::horizvert(abs(pj - j));
	else if (pj == j)
		dg = PathCost::horizvert(abs(pi - i));
	else
	{
		ASSERT(abs((int)pi - (int)i) == abs((int)pj - (int)j)); // must be 45 degrees
		dg = PathCost::diag(abs((int)pi - (int)i));
	}

	PathCost g = pg + dg; // cost to this tile = cost to predecessor + delta from predecessor

	PathCost h = CalculateHeuristic(i, j, state.iGoal, state.jGoal);

	// If this is a new tile, compute the heuristic distance
	if (n.IsUnexplored())
	{
		// Remember the best tile we've seen so far, in case we never actually reach the target
		if (h < state.hBest)
		{
			state.hBest = h;
			state.iBest = i;
			state.jBest = j;
		}
	}
	else
	{
		// If we've already seen this tile, and the new path to this tile does not have a
		// better cost, then stop now
		if (g >= n.GetCost())
			return;

		// Otherwise, we have a better path.

		// If we've already added this tile to the open list:
		if (n.IsOpen())
		{
			// This is a better path, so replace the old one with the new cost/parent
			PathCost gprev = n.GetCost();
			n.SetCost(g);
			n.SetPred(pi, pj, i, j);
			state.open.promote(TileID(i, j), gprev + h, g + h, h);
			return;
		}
	}

	// Add it to the open list:
	n.SetStatusOpen();
	n.SetCost(g);
	n.SetPred(pi, pj, i, j);
	PriorityQueue::Item t = { TileID(i, j), g + h, h };
	state.open.push(t);
}

/*
 * In the JPS algorithm, after a tile is taken off the open queue,
 * we don't process every adjacent neighbour (as in standard A*).
 * Instead we only move in a subset of directions (depending on the
 * direction from the predecessor); and instead of moving by a single
 * cell, we move up to the next jump point in that direction.
 * The AddJumped... functions do this by calling ProcessNeighbour
 * on the jump point (if any) in a certain direction.
 * The HasJumped... functions return whether there is any jump point
 * in that direction.
 */

// JPS functions scan navcells towards one direction
// OnTheWay tests whether we are scanning towards the right direction, to avoid useless scans
inline bool OnTheWay(int i, int j, int di, int dj, int iGoal, int jGoal)
{
	if (dj != 0)
	{
		// We're not moving towards the goal
		if ((jGoal - j) * dj < 0)
			return false;
	}
	else if (j != jGoal)
		return false;

	if (di != 0)
	{
		// We're not moving towards the goal
		if ((iGoal - i) * di < 0)
			return false;
	}
	else if (i != iGoal)
		return false;

	return true;
}


void LongPathfinder::AddJumpedHoriz(int i, int j, int di, PathCost g, PathfinderState& state, bool detectGoal)
{
	if (m_UseJPSCache)
	{
		int jump;
		if (di > 0)
			jump = state.jpc->GetJumpPointRight(i, j, state.goal);
		else
			jump = state.jpc->GetJumpPointLeft(i, j, state.goal);

		if (jump != i)
			ProcessNeighbour(i, j, jump, j, g, state);
	}
	else
	{
		ASSERT(di == 1 || di == -1);
		int ni = i + di;
		while (true)
		{
			if (!PASSABLE(ni, j))
				break;

			if (detectGoal && state.goal.NavcellContainsGoal(ni, j))
			{
				state.open.clear();
				ProcessNeighbour(i, j, ni, j, g, state);
				break;
			}

			if	((!PASSABLE(ni - di, j - 1) && PASSABLE(ni, j - 1)) ||
				(!PASSABLE(ni - di, j + 1) && PASSABLE(ni, j + 1)))
			{
				ProcessNeighbour(i, j, ni, j, g, state);
				break;
			}

			ni += di;
		}
	}
}

// Returns the i-coordinate of the jump point if it exists, else returns i
int LongPathfinder::HasJumpedHoriz(int i, int j, int di, PathfinderState& state, bool detectGoal)
{
	if (m_UseJPSCache)
	{
		int jump;
		if (di > 0)
			jump = state.jpc->GetJumpPointRight(i, j, state.goal);
		else
			jump = state.jpc->GetJumpPointLeft(i, j, state.goal);

		return jump;
	}
	else
	{
		ASSERT(di == 1 || di == -1);
		int ni = i + di;
		while (true)
		{
			if (!PASSABLE(ni, j))
				return i;

			if (detectGoal && state.goal.NavcellContainsGoal(ni, j))
			{
				state.open.clear();
				return ni;
			}

			if	((!PASSABLE(ni - di, j - 1) && PASSABLE(ni, j - 1)) ||
				(!PASSABLE(ni - di, j + 1) && PASSABLE(ni, j + 1)))
				return ni;

			ni += di;
		}
	}
}

void LongPathfinder::AddJumpedVert(int i, int j, int dj, PathCost g, PathfinderState& state, bool detectGoal)
{
	if (m_UseJPSCache)
	{
		int jump;
		if (dj > 0)
			jump = state.jpc->GetJumpPointUp(i, j, state.goal);
		else
			jump = state.jpc->GetJumpPointDown(i, j, state.goal);

		if (jump != j)
			ProcessNeighbour(i, j, i, jump, g, state);
	}
	else
	{
		ASSERT(dj == 1 || dj == -1);
		int nj = j + dj;
		while (true)
		{
			if (!PASSABLE(i, nj))
				break;

			if (detectGoal && state.goal.NavcellContainsGoal(i, nj))
			{
				state.open.clear();
				ProcessNeighbour(i, j, i, nj, g, state);
				break;
			}

			if	((!PASSABLE(i - 1, nj - dj) && PASSABLE(i - 1, nj)) ||
				(!PASSABLE(i + 1, nj - dj) && PASSABLE(i + 1, nj)))
			{
				ProcessNeighbour(i, j, i, nj, g, state);
				break;
			}

			nj += dj;
		}
	}
}

// Returns the j-coordinate of the jump point if it exists, else returns j
int LongPathfinder::HasJumpedVert(int i, int j, int dj, PathfinderState& state, bool detectGoal)
{
	if (m_UseJPSCache)
	{
		int jump;
		if (dj > 0)
			jump = state.jpc->GetJumpPointUp(i, j, state.goal);
		else
			jump = state.jpc->GetJumpPointDown(i, j, state.goal);

		return jump;
	}
	else
	{
		ASSERT(dj == 1 || dj == -1);
		int nj = j + dj;
		while (true)
		{
			if (!PASSABLE(i, nj))
				return j;

			if (detectGoal && state.goal.NavcellContainsGoal(i, nj))
			{
				state.open.clear();
				return nj;
			}

			if	((!PASSABLE(i - 1, nj - dj) && PASSABLE(i - 1, nj)) ||
				(!PASSABLE(i + 1, nj - dj) && PASSABLE(i + 1, nj)))
				return nj;

			nj += dj;
		}
	}
}

/*
 * We never cache diagonal jump points - they're usually so frequent that
 * a linear search is about as cheap and avoids the setup cost and memory cost.
 */
void LongPathfinder::AddJumpedDiag(int i, int j, int di, int dj, PathCost g, PathfinderState& state)
{
	// 	ProcessNeighbour(i, j, i + di, j + dj, g, state);
	// 	return;

	ASSERT(di == 1 || di == -1);
	ASSERT(dj == 1 || dj == -1);

	int ni = i + di;
	int nj = j + dj;
	bool detectGoal = OnTheWay(i, j, di, dj, state.iGoal, state.jGoal);
	while (true)
	{
		// Stop if we hit an obstructed cell
		if (!PASSABLE(ni, nj))
			return;

		// Stop if moving onto this cell caused us to
		// touch the corner of an obstructed cell
		if (!PASSABLE(ni - di, nj) || !PASSABLE(ni, nj - dj))
			return;

		// Process this cell if it's at the goal
		if (detectGoal && state.goal.NavcellContainsGoal(ni, nj))
		{
			state.open.clear();
			ProcessNeighbour(i, j, ni, nj, g, state);
			return;
		}

		int fi = HasJumpedHoriz(ni, nj, di, state, detectGoal && OnTheWay(ni, nj, di, 0, state.iGoal, state.jGoal));
		int fj = HasJumpedVert(ni, nj, dj, state, detectGoal && OnTheWay(ni, nj, 0, dj, state.iGoal, state.jGoal));
		if (fi != ni || fj != nj)
		{
			ProcessNeighbour(i, j, ni, nj, g, state);
			g += PathCost::diag(abs(ni - i));

			if (fi != ni)
				ProcessNeighbour(ni, nj, fi, nj, g, state);

			if (fj != nj)
				ProcessNeighbour(ni, nj, ni, fj, g, state);

			return;
		}

		ni += di;
		nj += dj;
	}
}

void LongPathfinder::ComputeJPSPath(entity_pos_t x0, entity_pos_t z0, const PathGoal& origGoal, pass_class_t passClass, WaypointPath& path)
{
	PROFILE("ComputePathJPS");
	PROFILE2_IFSPIKE("ComputePathJPS", 0.0002);
	PathfinderState state = { 0 };

	state.jpc = m_JumpPointCache[passClass].get();
	if (m_UseJPSCache && !state.jpc)
	{
		state.jpc = new JumpPointCache;
		state.jpc->reset(m_Grid, passClass);
		debug_printf("PATHFINDER: JPC memory: %d kB\n", (int)state.jpc->GetMemoryUsage() / 1024);
		m_JumpPointCache[passClass] = shared_ptr<JumpPointCache>(state.jpc);
	}

	// Convert the start coordinates to tile indexes
	u16 i0, j0;
	Pathfinding::NearestNavcell(x0, z0, i0, j0, m_GridSize, m_GridSize);

	if (!IS_PASSABLE(m_Grid->get(i0, j0), passClass))
	{
		// The JPS pathfinder requires units to be on passable tiles
		// (otherwise it might crash), so handle the supposedly-invalid
		// state specially
		m_PathfinderHier.FindNearestPassableNavcell(i0, j0, passClass);
	}

	state.goal = origGoal;

	// Make the goal reachable. This includes shortening the path if the goal is in a non-passable
	// region, transforming non-point goals to reachable point goals, etc.
	m_PathfinderHier.MakeGoalReachable(i0, j0, state.goal, passClass);

	ENSURE(state.goal.type == PathGoal::POINT);

	// If we're already at the goal tile, then move directly to the exact goal coordinates
	if (state.goal.NavcellContainsGoal(i0, j0))
	{
		path.m_Waypoints.emplace_back(Waypoint{ state.goal.x, state.goal.z });
		return;
	}

	Pathfinding::NearestNavcell(state.goal.x, state.goal.z, state.iGoal, state.jGoal, m_GridSize, m_GridSize);

	ENSURE((state.goal.x / Pathfinding::NAVCELL_SIZE).ToInt_RoundToNegInfinity() == state.iGoal);
	ENSURE((state.goal.z / Pathfinding::NAVCELL_SIZE).ToInt_RoundToNegInfinity() == state.jGoal);

	state.passClass = passClass;

	state.steps = 0;

	state.tiles = new PathfindTileGrid(m_Grid->m_W, m_Grid->m_H);
	state.terrain = m_Grid;

	state.iBest = i0;
	state.jBest = j0;
	state.hBest = CalculateHeuristic(i0, j0, state.iGoal, state.jGoal);

	PriorityQueue::Item start = { TileID(i0, j0), PathCost() };
	state.open.push(start);
	state.tiles->get(i0, j0).SetStatusOpen();
	state.tiles->get(i0, j0).SetPred(i0, j0, i0, j0);
	state.tiles->get(i0, j0).SetCost(PathCost());

	while (true)
	{
		++state.steps;

		// If we ran out of tiles to examine, give up
		if (state.open.empty())
			break;

		// Move best tile from open to closed
		PriorityQueue::Item curr = state.open.pop();
		u16 i = curr.id.i();
		u16 j = curr.id.j();
		state.tiles->get(i, j).SetStatusClosed();

		// If we've reached the destination, stop
		if (state.goal.NavcellContainsGoal(i, j))
		{
			state.iBest = i;
			state.jBest = j;
			state.hBest = PathCost();
			break;
		}

		PathfindTile tile = state.tiles->get(i, j);
		PathCost g = tile.GetCost();

		// Get the direction of the predecessor tile from this tile
		int dpi = tile.GetPredDI();
		int dpj = tile.GetPredDJ();
		dpi = (dpi < 0 ? -1 : dpi > 0 ? 1 : 0);
		dpj = (dpj < 0 ? -1 : dpj > 0 ? 1 : 0);

		if (dpi != 0 && dpj == 0)
		{
			// Moving horizontally from predecessor
			if (!PASSABLE(i + dpi, j - 1))
			{
				AddJumpedDiag(i, j, -dpi, -1, g, state);
				AddJumpedVert(i, j, -1, g, state, OnTheWay(i, j, 0, -1, state.iGoal, state.jGoal));
			}
			if (!PASSABLE(i + dpi, j + 1))
			{
				AddJumpedDiag(i, j, -dpi, +1, g, state);
				AddJumpedVert(i, j, +1, g, state, OnTheWay(i, j, 0, +1, state.iGoal, state.jGoal));
			}
			AddJumpedHoriz(i, j, -dpi, g, state, OnTheWay(i, j, -dpi, 0, state.iGoal, state.jGoal));
		}
		else if (dpi == 0 && dpj != 0)
		{
			// Moving vertically from predecessor
			if (!PASSABLE(i - 1, j + dpj))
			{
				AddJumpedDiag(i, j, -1, -dpj, g, state);
				AddJumpedHoriz(i, j, -1, g, state,OnTheWay(i, j, -1, 0, state.iGoal, state.jGoal));
			}
			if (!PASSABLE(i + 1, j + dpj))
			{
				AddJumpedDiag(i, j, +1, -dpj, g, state);
				AddJumpedHoriz(i, j, +1, g, state,OnTheWay(i, j, +1, 0, state.iGoal, state.jGoal));
			}
			AddJumpedVert(i, j, -dpj, g, state, OnTheWay(i, j, 0, -dpj, state.iGoal, state.jGoal));
		}
		else if (dpi != 0 && dpj != 0)
		{
			// Moving diagonally from predecessor
			AddJumpedHoriz(i, j, -dpi, g, state, OnTheWay(i, j, -dpi, 0, state.iGoal, state.jGoal));
			AddJumpedVert(i, j, -dpj, g, state, OnTheWay(i, j, 0, -dpj, state.iGoal, state.jGoal));
			AddJumpedDiag(i, j, -dpi, -dpj, g, state);
		}
		else
		{
			// No predecessor, i.e. the start tile
			// Start searching in every direction

			// XXX - check passability?

			bool passl = PASSABLE(i - 1, j);
			bool passr = PASSABLE(i + 1, j);
			bool passd = PASSABLE(i, j - 1);
			bool passu = PASSABLE(i, j + 1);

			if (passl && passd)
				ProcessNeighbour(i, j, i-1, j-1, g, state);
			if (passr && passd)
				ProcessNeighbour(i, j, i+1, j-1, g, state);
			if (passl && passu)
				ProcessNeighbour(i, j, i-1, j+1, g, state);
			if (passr && passu)
				ProcessNeighbour(i, j, i+1, j+1, g, state);
			if (passl)
				ProcessNeighbour(i, j, i-1, j, g, state);
			if (passr)
				ProcessNeighbour(i, j, i+1, j, g, state);
			if (passd)
				ProcessNeighbour(i, j, i, j-1, g, state);
			if (passu)
				ProcessNeighbour(i, j, i, j+1, g, state);
		}
	}

	// Reconstruct the path (in reverse)
	u16 ip = state.iBest, jp = state.jBest;
	while (ip != i0 || jp != j0)
	{
		PathfindTile& n = state.tiles->get(ip, jp);
		entity_pos_t x, z;
		Pathfinding::NavcellCenter(ip, jp, x, z);
		path.m_Waypoints.emplace_back(Waypoint{ x, z });

		// Follow the predecessor link
		ip = n.GetPredI(ip);
		jp = n.GetPredJ(jp);
	}
	// The last waypoint is slightly incorrect (it's not the goal but the center
	// of the navcell of the goal), so replace it
	if (!path.m_Waypoints.empty())
		path.m_Waypoints.front() = { state.goal.x, state.goal.z };

	ImprovePathWaypoints(path, passClass, origGoal.maxdist, x0, z0);

	// Save this grid for debug display
	delete m_DebugGrid;
	m_DebugGrid = state.tiles;
	m_DebugSteps = state.steps;
	m_DebugGoal = state.goal;
}

#undef PASSABLE

void LongPathfinder::ImprovePathWaypoints(WaypointPath& path, pass_class_t passClass, entity_pos_t maxDist, entity_pos_t x0, entity_pos_t z0)
{
	if (path.m_Waypoints.empty())
		return;

	if (maxDist > fixed::Zero())
	{
		CFixedVector2D start(x0, z0);
		CFixedVector2D first(path.m_Waypoints.back().x, path.m_Waypoints.back().z);
		CFixedVector2D offset = first - start;
		if (offset.CompareLength(maxDist) > 0)
		{
			offset.Normalize(maxDist);
			path.m_Waypoints.emplace_back(Waypoint{ (start + offset).X, (start + offset).Y });
		}
	}

	if (path.m_Waypoints.size() < 2)
		return;

	std::vector<Waypoint>& waypoints = path.m_Waypoints;
	std::vector<Waypoint> newWaypoints;

	CFixedVector2D prev(waypoints.front().x, waypoints.front().z);
	newWaypoints.push_back(waypoints.front());
	for (size_t k = 1; k < waypoints.size() - 1; ++k)
	{
		CFixedVector2D ahead(waypoints[k + 1].x, waypoints[k + 1].z);
		CFixedVector2D curr(waypoints[k].x, waypoints[k].z);

		if (maxDist > fixed::Zero() && (curr - prev).CompareLength(maxDist) > 0)
		{
			// We are too far away from the previous waypoint, so create one in
			// between and continue with the improvement of the path
			prev = prev + (curr - prev) / 2;
			newWaypoints.emplace_back(Waypoint{ prev.X, prev.Y });
		}

		// If we're mostly straight, don't even bother.
		if ((ahead - curr).Perpendicular().Dot(curr - prev).Absolute() <= fixed::Epsilon() * 100)
			continue;

		if (!Pathfinding::CheckLineMovement(prev.X, prev.Y, ahead.X, ahead.Y, passClass, *m_Grid))
		{
			prev = CFixedVector2D(waypoints[k].x, waypoints[k].z);
			newWaypoints.push_back(waypoints[k]);
		}
	}
	newWaypoints.push_back(waypoints.back());
	path.m_Waypoints.swap(newWaypoints);
}

void LongPathfinder::GetDebugDataJPS(u32& steps, double& time, Grid<u8>& grid) const
{
	steps = m_DebugSteps;
	time = m_DebugTime;

	if (!m_DebugGrid)
		return;

	u16 iGoal, jGoal;
	Pathfinding::NearestNavcell(m_DebugGoal.x, m_DebugGoal.z, iGoal, jGoal, m_GridSize, m_GridSize);

	grid = Grid<u8>(m_DebugGrid->m_W, m_DebugGrid->m_H);
	for (u16 j = 0; j < grid.m_H; ++j)
	{
		for (u16 i = 0; i < grid.m_W; ++i)
		{
			if (i == iGoal && j == jGoal)
				continue;
			PathfindTile t = m_DebugGrid->get(i, j);
			grid.set(i, j, (t.IsOpen() ? 1 : 0) | (t.IsClosed() ? 2 : 0));
		}
	}
}

void LongPathfinder::SetDebugOverlay(bool enabled)
{
	if (enabled && !m_DebugOverlay)
		m_DebugOverlay = new LongOverlay(*this);
	else if (!enabled && m_DebugOverlay)
		SAFE_DELETE(m_DebugOverlay);
}

void LongPathfinder::ComputePath(entity_pos_t x0, entity_pos_t z0, const PathGoal& origGoal,
	pass_class_t passClass, std::vector<CircularRegion> excludedRegions, WaypointPath& path)
{
	GenerateSpecialMap(passClass, excludedRegions);
	ComputePath(x0, z0, origGoal, SPECIAL_PASS_CLASS, path);
}

inline bool InRegion(u16 i, u16 j, CircularRegion region)
{
	fixed cellX = Pathfinding::NAVCELL_SIZE * i;
	fixed cellZ = Pathfinding::NAVCELL_SIZE * j;

	return CFixedVector2D(cellX - region.x, cellZ - region.z).CompareLength(region.r) <= 0;
}

void LongPathfinder::GenerateSpecialMap(pass_class_t passClass, std::vector<CircularRegion> excludedRegions)
{
	for (u16 j = 0; j < m_Grid->m_H; ++j)
	{
		for (u16 i = 0; i < m_Grid->m_W; ++i)
		{
			NavcellData n = m_Grid->get(i, j);
			if (!IS_PASSABLE(n, passClass))
			{
				n |= SPECIAL_PASS_CLASS;
				m_Grid->set(i, j, n);
				continue;
			}

			for (CircularRegion& region : excludedRegions)
			{
				if (!InRegion(i, j, region))
					continue;

				n |= SPECIAL_PASS_CLASS;
				break;
			}
			m_Grid->set(i, j, n);
		}
	}
}
