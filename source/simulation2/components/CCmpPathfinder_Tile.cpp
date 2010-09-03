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

/**
 * @file
 * Tile-based algorithm for CCmpPathfinder.
 * This is a fairly naive algorithm and could probably be improved substantially
 * (hopefully without needing to change the interface much).
 */

#include "precompiled.h"

#include "CCmpPathfinder_Common.h"

#include "ps/Profile.h"
#include "renderer/TerrainOverlay.h"
#include "simulation2/helpers/PriorityQueue.h"

typedef PriorityQueueList<std::pair<u16, u16>, u32> PriorityQueue;

#define PATHFIND_STATS 0

#define USE_DIAGONAL_MOVEMENT 1

// Heuristic cost to move between adjacent tiles.
// This should be similar to DEFAULT_MOVE_COST.
const u32 g_CostPerTile = 256;

/**
 * Tile data for A* computation.
 * (We store an array of one of these per terrain tile, so it ought to be optimised for size)
 */
struct PathfindTile
{
public:
	enum {
		STATUS_UNEXPLORED = 0,
		STATUS_OPEN = 1,
		STATUS_CLOSED = 2
	};

	bool IsUnexplored() { return status == STATUS_UNEXPLORED; }
	bool IsOpen() { return status == STATUS_OPEN; }
	bool IsClosed() { return status == STATUS_CLOSED; }
	void SetStatusOpen() { status = STATUS_OPEN; }
	void SetStatusClosed() { status = STATUS_CLOSED; }

	// Get pi,pj coords of predecessor to this tile on best path, given i,j coords of this tile
	u16 GetPredI(u16 i) { return i+dpi; }
	u16 GetPredJ(u16 j) { return j+dpj; }
	// Set the pi,pj coords of predecessor, given i,j coords of this tile
	void SetPred(u16 pi_, u16 pj_, u16 i, u16 j)
	{
		dpi = pi_-i;
		dpj = pj_-j;
#if PATHFIND_DEBUG
		// predecessor must be adjacent
		debug_assert(pi_-i == -1 || pi_-i == 0 || pi_-i == 1);
		debug_assert(pj_-j == -1 || pj_-j == 0 || pj_-j == 1);
#endif
	}

private:
	u8 status; // this only needs 2 bits
	i8 dpi, dpj; // these only really need 2 bits in total
public:
	u32 cost; // g (cost to this tile)
	u32 h; // h (heuristic cost to goal) (TODO: is it really better for performance to store this instead of recomputing?)

#if PATHFIND_DEBUG
	u32 GetStep() { return step; }
	void SetStep(u32 s) { step = s; }
private:
	u32 step; // step at which this tile was last processed (for debug rendering)
#else
	u32 GetStep() { return 0; }
	void SetStep(u32) { }
#endif

};

/**
 * Terrain overlay for pathfinder debugging.
 * Renders a representation of the most recent pathfinding operation.
 */
class PathfinderOverlay : public TerrainOverlay
{
	NONCOPYABLE(PathfinderOverlay);
public:
	CCmpPathfinder& m_Pathfinder;

	PathfinderOverlay(CCmpPathfinder& pathfinder) : m_Pathfinder(pathfinder)
	{
	}

	virtual void ProcessTile(ssize_t i, ssize_t j)
	{
		if (m_Pathfinder.m_Grid && !IS_PASSABLE(m_Pathfinder.m_Grid->get(i, j), m_Pathfinder.m_DebugPassClass))
			RenderTile(CColor(1, 0, 0, 0.6f), false);

		if (m_Pathfinder.m_DebugGrid)
		{
			PathfindTile& n = m_Pathfinder.m_DebugGrid->get(i, j);

			float c = clamp(n.GetStep() / (float)m_Pathfinder.m_DebugSteps, 0.f, 1.f);

			if (n.IsOpen())
				RenderTile(CColor(1, 1, c, 0.6f), false);
			else if (n.IsClosed())
				RenderTile(CColor(0, 1, c, 0.6f), false);
		}
	}

	virtual void EndRender()
	{
		if (m_Pathfinder.m_DebugPath)
		{
			std::vector<ICmpPathfinder::Waypoint>& wp = m_Pathfinder.m_DebugPath->m_Waypoints;
			for (size_t n = 0; n < wp.size(); ++n)
			{
				u16 i, j;
				m_Pathfinder.NearestTile(wp[n].x, wp[n].z, i, j);
				RenderTileOutline(CColor(1, 1, 1, 1), 2, false, i, j);
			}
		}
	}
};

void CCmpPathfinder::SetDebugOverlay(bool enabled)
{
	if (enabled && !m_DebugOverlay)
	{
		m_DebugOverlay = new PathfinderOverlay(*this);
	}
	else if (!enabled && m_DebugOverlay)
	{
		delete m_DebugOverlay;
		m_DebugOverlay = NULL;
	}
}

void CCmpPathfinder::SetDebugPath(entity_pos_t x0, entity_pos_t z0, const Goal& goal, u8 passClass, u8 costClass)
{
	if (!m_DebugOverlay)
		return;

	delete m_DebugGrid;
	m_DebugGrid = NULL;
	delete m_DebugPath;
	m_DebugPath = new Path();
	ComputePath(x0, z0, goal, passClass, costClass, *m_DebugPath);
	m_DebugPassClass = passClass;
}

void CCmpPathfinder::ResetDebugPath()
{
	delete m_DebugGrid;
	m_DebugGrid = NULL;
	delete m_DebugPath;
	m_DebugPath = NULL;
}


//////////////////////////////////////////////////////////


struct PathfinderState
{
	u32 steps; // number of algorithm iterations

	u16 iGoal, jGoal; // goal tile
	u16 rGoal; // radius of goal (around tile center)

	u8 passClass;
	std::vector<u32> moveCosts;

	PriorityQueue open;
	// (there's no explicit closed list; it's encoded in PathfindTile)

	Grid<PathfindTile>* tiles;
	Grid<TerrainTile>* terrain;

	bool ignoreImpassable; // allows us to escape if stuck in patches of impassability

	u32 hBest; // heuristic of closest discovered tile to goal
	u16 iBest, jBest; // closest tile

#if PATHFIND_STATS
	// Performance debug counters
	size_t numProcessed;
	size_t numImproveOpen;
	size_t numImproveClosed;
	size_t numAddToOpen;
	size_t sumOpenSize;
#endif
};

static bool AtGoal(u16 i, u16 j, const ICmpPathfinder::Goal& goal)
{
	// Allow tiles slightly more than sqrt(2) from the actual goal,
	// i.e. adjacent diagonally to the target tile
	fixed tolerance = entity_pos_t::FromInt(CELL_SIZE*3/2);

	entity_pos_t x, z;
	CCmpPathfinder::TileCenter(i, j, x, z);
	fixed dist = CCmpPathfinder::DistanceToGoal(CFixedVector2D(x, z), goal);
	return (dist < tolerance);
}

// Calculate heuristic cost from tile i,j to destination
// (This ought to be an underestimate for correctness)
static u32 CalculateHeuristic(u16 i, u16 j, u16 iGoal, u16 jGoal, u16 rGoal)
{
#if USE_DIAGONAL_MOVEMENT
	CFixedVector2D pos (fixed::FromInt(i), fixed::FromInt(j));
	CFixedVector2D goal (fixed::FromInt(iGoal), fixed::FromInt(jGoal));
	fixed dist = (pos - goal).Length();
	// TODO: the heuristic could match the costs better - it's not really Euclidean movement

	fixed rdist = dist - fixed::FromInt(rGoal);
	rdist = rdist.Absolute();

	// To avoid overflows on large distances we have to convert to int before multiplying
	// by the full tile cost, which means we lose some accuracy over short distances,
	// so do a partial multiplication here.
	// (This will overflow if sqrt(2)*tilesPerSide*premul >= 32768, so
	// premul=32 means max tilesPerSide=724)
	const int premul = 32;
	cassert(g_CostPerTile % premul == 0);
	return (rdist * premul).ToInt_RoundToZero() * (g_CostPerTile / premul);

#else
	return (abs((int)i - (int)iGoal) + abs((int)j - (int)jGoal)) * g_CostPerTile;
#endif
}

// Calculate movement cost from predecessor tile pi,pj to tile i,j
static u32 CalculateCostDelta(u16 pi, u16 pj, u16 i, u16 j, Grid<PathfindTile>* tempGrid, u32 tileCost)
{
	u32 dg = tileCost;

#if USE_DIAGONAL_MOVEMENT
	// XXX: Probably a terrible hack:
	// For simplicity, we only consider horizontally/vertically adjacent neighbours, but
	// units can move along arbitrary lines. That results in ugly square paths, so we want
	// to prefer diagonal paths.
	// Instead of solving this nicely, I'll just special-case 45-degree and 30-degree lines
	// by checking the three predecessor tiles (which'll be in the closed set and therefore
	// likely to be reasonably stable) and reducing the cost, and use a Euclidean heuristic.
	// At least this makes paths look a bit nicer for now...

	PathfindTile& p = tempGrid->get(pi, pj);
	u16 ppi = p.GetPredI(pi);
	u16 ppj = p.GetPredJ(pj);
	if (ppi != i && ppj != j)
		dg = (dg << 16) / 92682; // dg*sqrt(2)/2
	else
	{
		PathfindTile& pp = tempGrid->get(ppi, ppj);
		int di = abs(i - pp.GetPredI(ppi));
		int dj = abs(j - pp.GetPredJ(ppj));
		if ((di == 1 && dj == 2) || (di == 2 && dj == 1))
			dg = (dg << 16) / 79742; // dg*(sqrt(5)-sqrt(2))
	}
#endif

	return dg;
}

// Do the A* processing for a neighbour tile i,j.
static void ProcessNeighbour(u16 pi, u16 pj, u16 i, u16 j, u32 pg, PathfinderState& state)
{
#if PATHFIND_STATS
	state.numProcessed++;
#endif

	// Reject impassable tiles
	TerrainTile tileTag = state.terrain->get(i, j);
	if (!IS_PASSABLE(tileTag, state.passClass) && !state.ignoreImpassable)
		return;

	u32 dg = CalculateCostDelta(pi, pj, i, j, state.tiles, state.moveCosts.at(GET_COST_CLASS(tileTag)));

	u32 g = pg + dg; // cost to this tile = cost to predecessor + delta from predecessor

	PathfindTile& n = state.tiles->get(i, j);

	// If this is a new tile, compute the heuristic distance
	if (n.IsUnexplored())
	{
		n.h = CalculateHeuristic(i, j, state.iGoal, state.jGoal, state.rGoal);
		// Remember the best tile we've seen so far, in case we never actually reach the target
		if (n.h < state.hBest)
		{
			state.hBest = n.h;
			state.iBest = i;
			state.jBest = j;
		}
	}
	else
	{
		// If we've already seen this tile, and the new path to this tile does not have a
		// better cost, then stop now
		if (g >= n.cost)
			return;

		// Otherwise, we have a better path.

		// If we've already added this tile to the open list:
		if (n.IsOpen())
		{
			// This is a better path, so replace the old one with the new cost/parent
			n.cost = g;
			n.SetPred(pi, pj, i, j);
			n.SetStep(state.steps);
			state.open.promote(std::make_pair(i, j), g + n.h);
#if PATHFIND_STATS
			state.numImproveOpen++;
#endif
			return;
		}

		// If we've already found the 'best' path to this tile:
		if (n.IsClosed())
		{
			// This is a better path (possible when we use inadmissible heuristics), so reopen it
#if PATHFIND_STATS
			state.numImproveClosed++;
#endif
			// (fall through)
		}
	}

	// Add it to the open list:
	n.SetStatusOpen();
	n.cost = g;
	n.SetPred(pi, pj, i, j);
	n.SetStep(state.steps);
	PriorityQueue::Item t = { std::make_pair(i, j), g + n.h };
	state.open.push(t);
#if PATHFIND_STATS
	state.numAddToOpen++;
#endif
}

void CCmpPathfinder::ComputePath(entity_pos_t x0, entity_pos_t z0, const Goal& goal, u8 passClass, u8 costClass, Path& path)
{
	UpdateGrid();

	PROFILE("ComputePath");

	PathfinderState state = { 0 };

	// Convert the start/end coordinates to tile indexes
	u16 i0, j0;
	NearestTile(x0, z0, i0, j0);
	NearestTile(goal.x, goal.z, state.iGoal, state.jGoal);

	// If we're already at the goal tile, then move directly to the exact goal coordinates
	if (AtGoal(i0, j0, goal))
	{
		Waypoint w = { goal.x, goal.z };
		path.m_Waypoints.push_back(w);
		return;
	}

	// If the target is a circle, we want to aim for the edge of it (so e.g. if we're inside
	// a large circle then the heuristics will aim us directly outwards);
	// otherwise just aim at the center point. (We'll never try moving outwards to a square shape.)
	if (goal.type == Goal::CIRCLE)
		state.rGoal = (goal.hw / (int)CELL_SIZE).ToInt_RoundToZero();
	else
		state.rGoal = 0;

	state.passClass = passClass;
	state.moveCosts = m_MoveCosts.at(costClass);

	state.steps = 0;

	state.tiles = new Grid<PathfindTile>(m_MapSize, m_MapSize);
	state.terrain = m_Grid;

	state.iBest = i0;
	state.jBest = j0;
	state.hBest = CalculateHeuristic(i0, j0, state.iGoal, state.jGoal, state.rGoal);

	PriorityQueue::Item start = { std::make_pair(i0, j0), 0 };
	state.open.push(start);
	state.tiles->get(i0, j0).SetStatusOpen();
	state.tiles->get(i0, j0).SetPred(i0, j0, i0, j0);
	state.tiles->get(i0, j0).cost = 0;

	// To prevent units getting very stuck, if they start on an impassable tile
	// surrounded entirely by impassable tiles, we ignore the impassability
	state.ignoreImpassable = !IS_PASSABLE(state.terrain->get(i0, j0), state.passClass);

	while (1)
	{
		++state.steps;

		// Hack to avoid spending ages computing giant paths, particularly when
		// the destination is unreachable
		if (state.steps > 10000)
			break;

		// If we ran out of tiles to examine, give up
		if (state.open.empty())
			break;

#if PATHFIND_STATS
		state.sumOpenSize += state.open.size();
#endif

		// Move best tile from open to closed
		PriorityQueue::Item curr = state.open.pop();
		u16 i = curr.id.first;
		u16 j = curr.id.second;
		state.tiles->get(i, j).SetStatusClosed();

		// If we've reached the destination, stop
		if (AtGoal(i, j, goal))
		{
			state.iBest = i;
			state.jBest = j;
			state.hBest = 0;
			break;
		}

		// As soon as we find an escape route from the impassable terrain,
		// take it and forbid any further use of impassable tiles
		if (state.ignoreImpassable)
		{
			if (i > 0 && IS_PASSABLE(state.terrain->get(i-1, j), state.passClass))
				state.ignoreImpassable = false;
			else if (i < m_MapSize-1 && IS_PASSABLE(state.terrain->get(i+1, j), state.passClass))
				state.ignoreImpassable = false;
			else if (j > 0 && IS_PASSABLE(state.terrain->get(i, j-1), state.passClass))
				state.ignoreImpassable = false;
			else if (j < m_MapSize-1 && IS_PASSABLE(state.terrain->get(i, j+1), state.passClass))
				state.ignoreImpassable = false;
		}

		u32 g = state.tiles->get(i, j).cost;
		if (i > 0)
			ProcessNeighbour(i, j, i-1, j, g, state);
		if (i < m_MapSize-1)
			ProcessNeighbour(i, j, i+1, j, g, state);
		if (j > 0)
			ProcessNeighbour(i, j, i, j-1, g, state);
		if (j < m_MapSize-1)
			ProcessNeighbour(i, j, i, j+1, g, state);
	}

	// Reconstruct the path (in reverse)
	u16 ip = state.iBest, jp = state.jBest;
	while (ip != i0 || jp != j0)
	{
		PathfindTile& n = state.tiles->get(ip, jp);
		entity_pos_t x, z;
		TileCenter(ip, jp, x, z);
		Waypoint w = { x, z };
		path.m_Waypoints.push_back(w);

		// Follow the predecessor link
		ip = n.GetPredI(ip);
		jp = n.GetPredJ(jp);
	}

	// Save this grid for debug display
	delete m_DebugGrid;
	m_DebugGrid = state.tiles;
	m_DebugSteps = state.steps;

#if PATHFIND_STATS
	printf("PATHFINDER: steps=%d avgo=%d proc=%d impc=%d impo=%d addo=%d\n", state.steps, state.sumOpenSize/state.steps, state.numProcessed, state.numImproveClosed, state.numImproveOpen, state.numAddToOpen);
#endif
}
