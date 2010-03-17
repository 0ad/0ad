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

#include "precompiled.h"

#include "simulation2/system/Component.h"
#include "ICmpPathfinder.h"

#include "simulation2/MessageTypes.h"

#include "ICmpObstructionManager.h"

#include "graphics/Terrain.h"
#include "maths/FixedVector2D.h"
#include "maths/MathUtil.h"
#include "ps/Overlay.h"
#include "ps/Profile.h"
#include "renderer/TerrainOverlay.h"

#ifdef NDEBUG
#define PATHFIND_DEBUG 0
#else
#define PATHFIND_DEBUG 1
#endif

#define PATHFIND_STATS 0

class CCmpPathfinder;
struct PathfindTile;

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

	virtual void EndRender();

	virtual void ProcessTile(ssize_t i, ssize_t j);
};

/**
 * Implementation of ICmpPathfinder
 */
class CCmpPathfinder : public ICmpPathfinder
{
public:
	static void ClassInit(CComponentManager& UNUSED(componentManager))
	{
	}

	DEFAULT_COMPONENT_ALLOCATOR(Pathfinder)

	const CSimContext* m_Context;

	u16 m_MapSize; // tiles per side
	Grid<u8>* m_Grid; // terrain/passability information

	// Debugging - output from last pathfind operation:
	Grid<PathfindTile>* m_DebugGrid;
	u32 m_DebugSteps;
	Path* m_DebugPath;
	PathfinderOverlay* m_DebugOverlay;

	virtual void Init(const CSimContext& context, const CParamNode& UNUSED(paramNode))
	{
		m_Context = &context;

		m_MapSize = 0;
		m_Grid = NULL;

		m_DebugOverlay = new PathfinderOverlay(*this);
		m_DebugGrid = NULL;
		m_DebugPath = NULL;
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
		delete m_Grid;
		delete m_DebugOverlay;
		delete m_DebugGrid;
		delete m_DebugPath;
	}

	virtual void Serialize(ISerializer& serialize)
	{
		// TODO: do something here
		// (Do we need to serialise the pathfinder state, or is it fine to regenerate it from
		// the original entities after deserialisation?)
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(context, paramNode);

		// TODO
	}

	virtual bool CanMoveStraight(entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r, u32& cost);

	virtual void ComputePath(entity_pos_t x0, entity_pos_t z0, const Goal& goal, Path& ret);

	virtual void SetDebugPath(entity_pos_t x0, entity_pos_t z0, const Goal& goal)
	{
		delete m_DebugGrid;
		m_DebugGrid = NULL;
		delete m_DebugPath;
		m_DebugPath = new Path();
		ComputePath(x0, z0, goal, *m_DebugPath);
	}

	/**
	 * Returns the tile containing the given position
	 */
	void NearestTile(entity_pos_t x, entity_pos_t z, u16& i, u16& j)
	{
		i = clamp((x / CELL_SIZE).ToInt_RoundToZero(), 0, m_MapSize-1);
		j = clamp((z / CELL_SIZE).ToInt_RoundToZero(), 0, m_MapSize-1);
	}

	/**
	 * Returns the position of the center of the given tile
	 */
	void TileCenter(u16 i, u16 j, entity_pos_t& x, entity_pos_t& z)
	{
		x = entity_pos_t::FromInt(i*CELL_SIZE + CELL_SIZE/2);
		z = entity_pos_t::FromInt(j*CELL_SIZE + CELL_SIZE/2);
	}

	/**
	 * Regenerates the grid based on the current obstruction list, if necessary
	 */
	void UpdateGrid()
	{
		PROFILE("UpdateGrid");

		// Initialise the terrain data when first needed
		if (!m_Grid)
		{
			// TOOD: these bits should come from ICmpTerrain
			ssize_t size = m_Context->GetTerrain().GetTilesPerSide();

			debug_assert(size >= 1 && size <= 0xffff); // must fit in 16 bits
			m_MapSize = size;
			m_Grid = new Grid<u8>(m_MapSize, m_MapSize);
		}

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(*m_Context, SYSTEM_ENTITY);
		cmpObstructionManager->Rasterise(*m_Grid);
	}
};

REGISTER_COMPONENT_TYPE(Pathfinder)


const u32 g_CostPerTile = 256; // base cost to move between adjacent tiles

bool CCmpPathfinder::CanMoveStraight(entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r, u32& cost)
{
	// Test whether there's a straight path
	CmpPtr<ICmpObstructionManager> cmpObstructionManager(*m_Context, SYSTEM_ENTITY);
	NullObstructionFilter filter;
	if (!cmpObstructionManager->TestLine(filter, x0, z0, x1, z1, r))
		return false;

	// Calculate the exact movement cost
	// (TODO: this needs to care about terrain costs etc)
	cost = (CFixedVector2D(x1 - x0, z1 - z0).Length() * g_CostPerTile).ToInt_RoundToZero();

	return true;
}

/**
 * Tile data for A* computation
 */
struct PathfindTile
{
	enum {
		STATUS_UNEXPLORED = 0,
		STATUS_OPEN = 1,
		STATUS_CLOSED = 2
	};
	u8 status; // (TODO: this only needs 2 bits)
	u16 pi, pj; // predecessor on best path (TODO: this only needs 2 bits)
	u32 cost; // g (cost to this tile)
	u32 h; // h (TODO: is it really better for performance to store this instead of recomputing?)

	u32 step; // step at which this tile was last processed (TODO: this should only be present for debugging)
};

void PathfinderOverlay::EndRender()
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

void PathfinderOverlay::ProcessTile(ssize_t i, ssize_t j)
{
	if (m_Pathfinder.m_Grid && m_Pathfinder.m_Grid->get(i, j))
		RenderTile(CColor(1, 0, 0, 0.6f), false);

	if (m_Pathfinder.m_DebugGrid)
	{
		PathfindTile& n = m_Pathfinder.m_DebugGrid->get(i, j);

		float c = clamp(n.step / (float)m_Pathfinder.m_DebugSteps, 0.f, 1.f);

		if (n.status == PathfindTile::STATUS_OPEN)
			RenderTile(CColor(1, 1, c, 0.6f), false);
		else if (n.status == PathfindTile::STATUS_CLOSED)
			RenderTile(CColor(0, 1, c, 0.6f), false);
	}
}

/*
 * A* pathfinding implementation
 *
 * This is currently all a bit rubbish and hasn't been tested for correctness or efficiency;
 * the intention is to demonstrate the interface that the pathfinder can use, and improvements
 * to the implementation shouldn't affect that interface much.
 */

struct QueueItem
{
	u16 i, j;
	u32 rank; // g+h (estimated total cost of path through here)
};

struct QueueItemPriority
{
	bool operator()(const QueueItem& a, const QueueItem& b)
	{
		if (a.rank > b.rank) // higher costs are lower priority
			return true;
		if (a.rank < b.rank)
			return false;
		// Need to tie-break to get a consistent ordering
		// TODO: Should probably tie-break on g or h or something, but don't bother for now
		if (a.i < b.i)
			return true;
		if (a.i > b.i)
			return false;
		if (a.j < b.j)
			return true;
		if (a.j > b.j)
			return false;
#if PATHFIND_DEBUG
		debug_warn(L"duplicate tiles in queue");
#endif
		return false;
	}
};

/**
 * Priority queue implemented as a binary heap.
 * This is quite dreadfully slow in MSVC's debug STL implementation,
 * so we shouldn't use it unless we reimplement the heap functions more efficiently.
 */
class PriorityQueueHeap
{
public:
	void push(const QueueItem& item)
	{
		m_Heap.push_back(item);
		push_heap(m_Heap.begin(), m_Heap.end(), QueueItemPriority());
	}

	QueueItem* find(u16 i, u16 j)
	{
		for (size_t n = 0; n < m_Heap.size(); ++n)
		{
			if (m_Heap[n].i == i && m_Heap[n].j == j)
				return &m_Heap[n];
		}
		return NULL;
	}

	void promote(u16 i, u16 j, u32 newrank)
	{
		for (size_t n = 0; n < m_Heap.size(); ++n)
		{
			if (m_Heap[n].i == i && m_Heap[n].j == j)
			{
#if PATHFIND_DEBUG
				debug_assert(m_Heap[n].rank > newrank);
#endif
				m_Heap[n].rank = newrank;
				push_heap(m_Heap.begin(), m_Heap.begin()+n+1, QueueItemPriority());
				return;
			}
		}
	}

	QueueItem pop()
	{
#if PATHFIND_DEBUG
		debug_assert(m_Heap.size());
#endif
		QueueItem r = m_Heap.front();
		pop_heap(m_Heap.begin(), m_Heap.end(), QueueItemPriority());
		m_Heap.pop_back();
		return r;
	}

	bool empty()
	{
		return m_Heap.empty();
	}

	size_t size()
	{
		return m_Heap.size();
	}

	std::vector<QueueItem> m_Heap;
};

/**
 * Priority queue implemented as an unsorted array.
 * This means pop() is O(n), but push and promote are O(1), and n is typically small
 * (average around 50-100 in some rough tests).
 * It seems fractionally slower than a binary heap in optimised builds, but is
 * much simpler and less susceptible to MSVC's painfully slow debug STL.
 */
class PriorityQueueList
{
public:
	void push(const QueueItem& item)
	{
		m_List.push_back(item);
	}

	QueueItem* find(u16 i, u16 j)
	{
		for (size_t n = 0; n < m_List.size(); ++n)
		{
			if (m_List[n].i == i && m_List[n].j == j)
				return &m_List[n];
		}
		return NULL;
	}

	void promote(u16 i, u16 j, u32 newrank)
	{
		find(i, j)->rank = newrank;
	}

	QueueItem pop()
	{
#if PATHFIND_DEBUG
		debug_assert(m_List.size());
#endif
		// Loop backwards looking for the best (it's most likely to be one
		// we've recently pushed, so going backwards saves a bit of copying)
		QueueItem best = m_List.back();
		size_t bestidx = m_List.size()-1;
		for (ssize_t i = (ssize_t)bestidx-1; i >= 0; --i)
		{
			if (QueueItemPriority()(best, m_List[i]))
			{
				bestidx = i;
				best = m_List[i];
			}
		}
		// Swap the matched element with the last in the list, then pop the new last
		m_List[bestidx] = m_List[m_List.size()-1];
		m_List.pop_back();
		return best;
	}

	bool empty()
	{
		return m_List.empty();
	}

	size_t size()
	{
		return m_List.size();
	}

	std::vector<QueueItem> m_List;
};

typedef PriorityQueueList PriorityQueue;


#define USE_DIAGONAL_MOVEMENT

// Calculate heuristic cost from tile i,j to destination
// (This ought to be an underestimate for correctness)
static u32 CalculateHeuristic(u16 i, u16 j, u16 iGoal, u16 jGoal, u16 rGoal, bool aimingInwards)
{
#ifdef USE_DIAGONAL_MOVEMENT
	CFixedVector2D pos (CFixed_23_8::FromInt(i), CFixed_23_8::FromInt(j));
	CFixedVector2D goal (CFixed_23_8::FromInt(iGoal), CFixed_23_8::FromInt(jGoal));
	CFixed_23_8 dist = (pos - goal).Length();
	// TODO: the heuristic could match the costs better - it's not really Euclidean movement

	CFixed_23_8 rdist = dist - CFixed_23_8::FromInt(rGoal);
	if (!aimingInwards)
		rdist = -rdist;

	if (rdist < CFixed_23_8::FromInt(0))
		return 0;
	return (rdist * g_CostPerTile).ToInt_RoundToZero();

#else
	return (abs((int)i - (int)iGoal) + abs((int)j - (int)jGoal)) * g_CostPerTile;
#endif
}

// Calculate movement cost from predecessor tile pi,pj to tile i,j
static u32 CalculateCostDelta(u16 pi, u16 pj, u16 i, u16 j, Grid<PathfindTile>* tempGrid)
{
	u32 dg = g_CostPerTile;

#ifdef USE_DIAGONAL_MOVEMENT
	// XXX: Probably a terrible hack:
	// For simplicity, we only consider horizontally/vertically adjacent neighbours, but
	// units can move along arbitrary lines. That results in ugly square paths, so we want
	// to prefer diagonal paths.
	// Instead of solving this nicely, I'll just special-case 45-degree and 30-degree lines
	// by checking the three predecessor tiles (which'll be in the closed set and therefore
	// likely to be reasonably stable) and reducing the cost, and use a Euclidean heuristic.
	// At least this makes paths look a bit nicer for now...

	PathfindTile& p = tempGrid->get(pi, pj);
	if (p.pi != i && p.pj != j)
		dg = (dg << 16) / 92682; // dg*sqrt(2)/2
	else
	{
		PathfindTile& pp = tempGrid->get(p.pi, p.pj);
		int di = abs(i - pp.pi);
		int dj = abs(j - pp.pj);
		if ((di == 1 && dj == 2) || (di == 2 && dj == 1))
			dg = (dg << 16) / 79742; // dg*(sqrt(5)-sqrt(2))
	}
#endif

	return dg;
}

struct PathfinderState
{
	u32 steps; // number of algorithm iterations

	u16 iGoal, jGoal; // goal tile
	u16 rGoal; // radius of goal (around tile center)
	bool aimingInwards; // whether we're moving towards the goal or away

	PriorityQueue open;
	// (there's no explicit closed list; it's encoded in PathfindTile::status)

	Grid<PathfindTile>* tiles;
	Grid<u8>* terrain;

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

// Do the A* processing for a neighbour tile i,j.
static void ProcessNeighbour(u16 pi, u16 pj, u16 i, u16 j, u32 pg, PathfinderState& state)
{
#if PATHFIND_STATS
	state.numProcessed++;
#endif

	// Reject impassable tiles
	if (state.terrain->get(i, j))
		return;

	u32 dg = CalculateCostDelta(pi, pj, i, j, state.tiles);

	u32 g = pg + dg; // cost to this tile = cost to predecessor + delta from predecessor

	PathfindTile& n = state.tiles->get(i, j);

	// If this is a new tile, compute the heuristic distance
	if (n.status == PathfindTile::STATUS_UNEXPLORED)
	{
		n.h = CalculateHeuristic(i, j, state.iGoal, state.jGoal, state.rGoal, state.aimingInwards);
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
		if (n.status == PathfindTile::STATUS_OPEN)
		{
			// This is a better path, so replace the old one with the new cost/parent
			n.cost = g;
			n.pi = pi;
			n.pj = pj;
			n.step = state.steps;
			state.open.promote(i, j, g + n.h);
#if PATHFIND_STATS
			state.numImproveOpen++;
#endif
			return;
		}

		// If we've already found the 'best' path to this tile:
		if (n.status == PathfindTile::STATUS_CLOSED)
		{
			// This is a better path (possible when we use inadmissible heuristics), so reopen it
#if PATHFIND_STATS
			state.numImproveClosed++;
#endif
			// (fall through)
		}
	}

	// Add it to the open list:
	n.status = PathfindTile::STATUS_OPEN;
	n.cost = g;
	n.pi = pi;
	n.pj = pj;
	n.step = state.steps;
	QueueItem t = { i, j, g + n.h };
	state.open.push(t);
#if PATHFIND_STATS
	state.numAddToOpen++;
#endif
}

static bool AtGoal(u16 i, u16 j, u16 iGoal, u16 jGoal, u16 rGoal, bool aimingInwards)
{
	// If we're aiming towards a point, stop when we get there
	if (aimingInwards && rGoal == 0)
		return (i == iGoal && j == jGoal);

	// Otherwise compute the distance and compare to desired radius
	i32 dist2 = ((i32)i-iGoal)*((i32)i-iGoal) + ((i32)j-jGoal)*((i32)j-jGoal);
	if (aimingInwards && (dist2 <= rGoal*rGoal))
		return true;
	if (!aimingInwards && (dist2 >= rGoal*rGoal))
		return true;

	return false;
}

void CCmpPathfinder::ComputePath(entity_pos_t x0, entity_pos_t z0, const Goal& goal, Path& path)
{
	UpdateGrid();

	PROFILE("ComputePath");

	PathfinderState state = { 0 };

	// Convert the start/end coordinates to tile indexes
	u16 i0, j0;
	NearestTile(x0, z0, i0, j0);
	NearestTile(goal.x, goal.z, state.iGoal, state.jGoal);

	// If we start closer than min radius, aim for the min radius
	// If we start further than max radius, aim for the max radius
	// Otherwise we're there already
	CFixed_23_8 initialDist = (CFixedVector2D(x0, z0) - CFixedVector2D(goal.x, goal.z)).Length();
	if (initialDist < goal.minRadius)
	{
		state.aimingInwards = false;
		state.rGoal = (goal.minRadius / CELL_SIZE).ToInt_RoundToZero(); // TODO: what rounding mode is appropriate?
	}
	else if (initialDist > goal.maxRadius)
	{
		state.aimingInwards = true;
		state.rGoal = (goal.maxRadius / CELL_SIZE).ToInt_RoundToZero(); // TODO: what rounding mode is appropriate?
	}
	else
	{
		return;
	}

	// If we're already at the goal tile, then move directly to the exact goal coordinates
	if (AtGoal(i0, j0, state.iGoal, state.jGoal, state.rGoal, state.aimingInwards))
	{
		Waypoint w = { goal.x, goal.z, 0 };
		path.m_Waypoints.push_back(w);
		return;
	}

	state.steps = 0;

	state.tiles = new Grid<PathfindTile>(m_MapSize, m_MapSize);
	state.terrain = m_Grid;

	state.iBest = i0;
	state.jBest = j0;
	state.hBest = CalculateHeuristic(i0, j0, state.iGoal, state.jGoal, state.rGoal, state.aimingInwards);

	QueueItem start = { i0, j0, 0 };
	state.open.push(start);
	state.tiles->get(i0, j0).status = PathfindTile::STATUS_OPEN;
	state.tiles->get(i0, j0).pi = i0;
	state.tiles->get(i0, j0).pj = j0;
	state.tiles->get(i0, j0).cost = 0;

	while (1)
	{
		++state.steps;

		// Hack to avoid spending ages computing giant paths, particularly when
		// the destination is unreachable
		if (state.steps > 5000)
			break;

		// If we ran out of tiles to examine, give up
		if (state.open.empty())
			break;

#if PATHFIND_STATS
		state.sumOpenSize += state.open.size();
#endif

		// Move best tile from open to closed
		QueueItem curr = state.open.pop();
		state.tiles->get(curr.i, curr.j).status = PathfindTile::STATUS_CLOSED;

		// If we've reached the destination, stop
		if (AtGoal(curr.i, curr.j, state.iGoal, state.jGoal, state.rGoal, state.aimingInwards))
		{
			state.iBest = curr.i;
			state.jBest = curr.j;
			state.hBest = 0;
			break;
		}

		u32 g = state.tiles->get(curr.i, curr.j).cost;
		if (curr.i > 0)
			ProcessNeighbour(curr.i, curr.j, curr.i-1, curr.j, g, state);
		if (curr.i < m_MapSize-1)
			ProcessNeighbour(curr.i, curr.j, curr.i+1, curr.j, g, state);
		if (curr.j > 0)
			ProcessNeighbour(curr.i, curr.j, curr.i, curr.j-1, g, state);
		if (curr.j < m_MapSize-1)
			ProcessNeighbour(curr.i, curr.j, curr.i, curr.j+1, g, state);
	}

	// Reconstruct the path (in reverse)
	u16 ip = state.iBest, jp = state.jBest;
	while (ip != i0 || jp != j0)
	{
		PathfindTile& n = state.tiles->get(ip, jp);
		// Pick the exact point if it's the goal tile, else the tile's centre
		entity_pos_t x, z;
		if (ip == state.iGoal && jp == state.jGoal)
		{
			x = goal.x;
			z = goal.z;
		}
		else
		{
			TileCenter(ip, jp, x, z);
		}
		Waypoint w = { x, z, n.cost };
		path.m_Waypoints.push_back(w);

		// Follow the predecessor link
		ip = n.pi;
		jp = n.pj;
	}

	// Save this grid for debug display
	delete m_DebugGrid;
	m_DebugGrid = state.tiles;
	m_DebugSteps = state.steps;

#if PATHFIND_STATS
	printf("PATHFINDER: steps=%d avgo=%d proc=%d impc=%d impo=%d addo=%d\n", state.steps, state.sumOpenSize/state.steps, state.numProcessed, state.numImproveClosed, state.numImproveOpen, state.numAddToOpen);
#endif
}
