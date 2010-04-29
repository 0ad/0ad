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

#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "maths/FixedVector2D.h"
#include "maths/MathUtil.h"
#include "ps/Overlay.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"
#include "renderer/TerrainOverlay.h"
#include "simulation2/helpers/Render.h"
#include "simulation2/helpers/Geometry.h"

/*
 * Note this file contains two separate pathfinding implementations, the 'normal' tile-based
 * one and the precise vertex-based 'short' pathfinder.
 * They share a priority queue implementation but have independent A* implementations
 * (with slightly different characteristics).
 */

#ifdef NDEBUG
#define PATHFIND_DEBUG 0
#else
#define PATHFIND_DEBUG 1
#endif

#define PATHFIND_STATS 0

class CCmpPathfinder;
struct PathfindTile;

typedef CFixed_23_8 fixed;

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
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_RenderSubmit); // for debug overlays
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

	std::vector<SOverlayLine> m_DebugOverlayShortPathLines;

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CSimContext& context, const CParamNode& UNUSED(paramNode))
	{
		m_Context = &context;

		m_MapSize = 0;
		m_Grid = NULL;

		m_DebugOverlay = NULL;
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

	virtual void HandleMessage(const CSimContext& context, const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_RenderSubmit:
		{
			const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
			RenderSubmit(context, msgData.collector);
			break;
		}
		}
	}

	virtual void ComputePath(entity_pos_t x0, entity_pos_t z0, const Goal& goal, Path& ret);

	virtual void ComputeShortPath(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t r, entity_pos_t range, const Goal& goal, Path& ret);

	virtual void SetDebugPath(entity_pos_t x0, entity_pos_t z0, const Goal& goal)
	{
		if (!m_DebugOverlay)
			return;

		delete m_DebugGrid;
		m_DebugGrid = NULL;
		delete m_DebugPath;
		m_DebugPath = new Path();
		ComputePath(x0, z0, goal, *m_DebugPath);
	}

	virtual void SetDebugOverlay(bool enabled)
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

	/**
	 * Returns the tile containing the given position
	 */
	void NearestTile(entity_pos_t x, entity_pos_t z, u16& i, u16& j)
	{
		i = clamp((x / (int)CELL_SIZE).ToInt_RoundToZero(), 0, m_MapSize-1);
		j = clamp((z / (int)CELL_SIZE).ToInt_RoundToZero(), 0, m_MapSize-1);
	}

	/**
	 * Returns the position of the center of the given tile
	 */
	static void TileCenter(u16 i, u16 j, entity_pos_t& x, entity_pos_t& z)
	{
		x = entity_pos_t::FromInt(i*(int)CELL_SIZE + CELL_SIZE/2);
		z = entity_pos_t::FromInt(j*(int)CELL_SIZE + CELL_SIZE/2);
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

	void RenderSubmit(const CSimContext& context, SceneCollector& collector);
};

REGISTER_COMPONENT_TYPE(Pathfinder)


const u32 g_CostPerTile = 256; // base cost to move between adjacent tiles

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

template <typename Item>
struct QueueItemPriority
{
	bool operator()(const Item& a, const Item& b)
	{
		if (a.rank > b.rank) // higher costs are lower priority
			return true;
		if (a.rank < b.rank)
			return false;
		// Need to tie-break to get a consistent ordering
		// TODO: Should probably tie-break on g or h or something, but don't bother for now
		if (a.id < b.id)
			return true;
		if (b.id < a.id)
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
template <typename ID, typename R>
class PriorityQueueHeap
{
public:
	struct Item
	{
		ID id;
		R rank; // f = g+h (estimated total cost of path through here)
	};

	void push(const Item& item)
	{
		m_Heap.push_back(item);
		push_heap(m_Heap.begin(), m_Heap.end(), QueueItemPriority<Item>());
	}

	Item* find(ID id)
	{
		for (size_t n = 0; n < m_Heap.size(); ++n)
		{
			if (m_Heap[n].id == id)
				return &m_Heap[n];
		}
		return NULL;
	}

	void promote(ID id, u32 newrank)
	{
		for (size_t n = 0; n < m_Heap.size(); ++n)
		{
			if (m_Heap[n].id == id)
			{
#if PATHFIND_DEBUG
				debug_assert(m_Heap[n].rank > newrank);
#endif
				m_Heap[n].rank = newrank;
				push_heap(m_Heap.begin(), m_Heap.begin()+n+1, QueueItemPriority<Item>());
				return;
			}
		}
	}

	Item pop()
	{
#if PATHFIND_DEBUG
		debug_assert(m_Heap.size());
#endif
		Item r = m_Heap.front();
		pop_heap(m_Heap.begin(), m_Heap.end(), QueueItemPriority<Item>());
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

	std::vector<Item> m_Heap;
};

/**
 * Priority queue implemented as an unsorted array.
 * This means pop() is O(n), but push and promote are O(1), and n is typically small
 * (average around 50-100 in some rough tests).
 * It seems fractionally slower than a binary heap in optimised builds, but is
 * much simpler and less susceptible to MSVC's painfully slow debug STL.
 */
template <typename ID, typename R>
class PriorityQueueList
{
public:
	struct Item
	{
		ID id;
		R rank; // f = g+h (estimated total cost of path through here)
	};

	void push(const Item& item)
	{
		m_List.push_back(item);
	}

	Item* find(ID id)
	{
		for (size_t n = 0; n < m_List.size(); ++n)
		{
			if (m_List[n].id == id)
				return &m_List[n];
		}
		return NULL;
	}

	void promote(ID id, R newrank)
	{
		find(id)->rank = newrank;
	}

	Item pop()
	{
#if PATHFIND_DEBUG
		debug_assert(m_List.size());
#endif
		// Loop backwards looking for the best (it's most likely to be one
		// we've recently pushed, so going backwards saves a bit of copying)
		Item best = m_List.back();
		size_t bestidx = m_List.size()-1;
		for (ssize_t i = (ssize_t)bestidx-1; i >= 0; --i)
		{
			if (QueueItemPriority<Item>()(best, m_List[i]))
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

	std::vector<Item> m_List;
};

typedef PriorityQueueList<std::pair<u16, u16>, u32> PriorityQueue;


#define USE_DIAGONAL_MOVEMENT

// Calculate heuristic cost from tile i,j to destination
// (This ought to be an underestimate for correctness)
static u32 CalculateHeuristic(u16 i, u16 j, u16 iGoal, u16 jGoal, u16 rGoal)
{
#ifdef USE_DIAGONAL_MOVEMENT
	CFixedVector2D pos (CFixed_23_8::FromInt(i), CFixed_23_8::FromInt(j));
	CFixedVector2D goal (CFixed_23_8::FromInt(iGoal), CFixed_23_8::FromInt(jGoal));
	CFixed_23_8 dist = (pos - goal).Length();
	// TODO: the heuristic could match the costs better - it's not really Euclidean movement

	CFixed_23_8 rdist = dist - CFixed_23_8::FromInt(rGoal);
	rdist = rdist.Absolute();

	return (rdist * (int)g_CostPerTile).ToInt_RoundToZero();

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
		if (n.status == PathfindTile::STATUS_OPEN)
		{
			// This is a better path, so replace the old one with the new cost/parent
			n.cost = g;
			n.pi = pi;
			n.pj = pj;
			n.step = state.steps;
			state.open.promote(std::make_pair(i, j), g + n.h);
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
	PriorityQueue::Item t = { std::make_pair(i, j), g + n.h };
	state.open.push(t);
#if PATHFIND_STATS
	state.numAddToOpen++;
#endif
}

static fixed DistanceToGoal(CFixedVector2D pos, const CCmpPathfinder::Goal& goal)
{
	switch (goal.type)
	{
	case CCmpPathfinder::Goal::POINT:
		return (pos - CFixedVector2D(goal.x, goal.z)).Length();

	case CCmpPathfinder::Goal::CIRCLE:
		return ((pos - CFixedVector2D(goal.x, goal.z)).Length() - goal.hw).Absolute();

	case CCmpPathfinder::Goal::SQUARE:
	{
		CFixedVector2D halfSize(goal.hw, goal.hh);
		CFixedVector2D d(pos.X - goal.x, pos.Y - goal.z);
		return Geometry::DistanceToSquare(d, goal.u, goal.v, halfSize);
	}

	default:
		debug_warn(L"invalid type");
		return fixed::Zero();
	}
}

static bool AtGoal(u16 i, u16 j, const ICmpPathfinder::Goal& goal)
{
	// Allow tiles slightly more than sqrt(2) from the actual goal,
	// i.e. adjacent diagonally to the target tile
	fixed tolerance = entity_pos_t::FromInt(CELL_SIZE*3/2);

	entity_pos_t x, z;
	CCmpPathfinder::TileCenter(i, j, x, z);
	fixed dist = DistanceToGoal(CFixedVector2D(x, z), goal);
	return (dist < tolerance);
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

	state.steps = 0;

	state.tiles = new Grid<PathfindTile>(m_MapSize, m_MapSize);
	state.terrain = m_Grid;

	state.iBest = i0;
	state.jBest = j0;
	state.hBest = CalculateHeuristic(i0, j0, state.iGoal, state.jGoal, state.rGoal);

	PriorityQueue::Item start = { std::make_pair(i0, j0), 0 };
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
		PriorityQueue::Item curr = state.open.pop();
		u16 i = curr.id.first;
		u16 j = curr.id.second;
		state.tiles->get(i, j).status = PathfindTile::STATUS_CLOSED;

		// If we've reached the destination, stop
		if (AtGoal(i, j, goal))
		{
			state.iBest = i;
			state.jBest = j;
			state.hBest = 0;
			break;
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


//////////////////////////////////////////////////////////

struct Vertex
{
	CFixedVector2D p;
	fixed g, h;
	u16 pred;
	enum
	{
		UNEXPLORED,
		OPEN,
		CLOSED,
	} status;
};

struct Edge
{
	CFixedVector2D p0, p1;
};

/**
 * Check whether a ray from 'a' to 'b' crosses any of the edges.
 * (Edges are one-sided so it's only considered a cross if going from front to back.)
 */
static bool CheckVisibility(CFixedVector2D a, CFixedVector2D b, const std::vector<Edge>& edges)
{
	CFixedVector2D abn = (b - a).Perpendicular();

	for (size_t i = 0; i < edges.size(); ++i)
	{
		CFixedVector2D d = (edges[i].p1 - edges[i].p0).Perpendicular();

		// If 'a' is behind the edge, we can't cross
		fixed q = (a - edges[i].p0).Dot(d);
		if (q < fixed::Zero())
			continue;

		// If 'b' is in front of the edge, we can't cross
		fixed r = (b - edges[i].p0).Dot(d);
		if (r > fixed::Zero())
			continue;

		// The ray is crossing the infinitely-extended edge from in front to behind.
		// If the edge's points are the same side of the infinitely-extended ray
		// then the finite lines can't intersect, otherwise they're crossing
		fixed s = (edges[i].p0 - a).Dot(abn);
		fixed t = (edges[i].p1 - a).Dot(abn);
		if ((s <= fixed::Zero() && t >= fixed::Zero()) || (s >= fixed::Zero() && t <= fixed::Zero()))
			return false;
	}

	return true;
}

static CFixedVector2D NearestPointOnGoal(CFixedVector2D pos, const CCmpPathfinder::Goal& goal)
{
	CFixedVector2D g(goal.x, goal.z);

	switch (goal.type)
	{
	case CCmpPathfinder::Goal::POINT:
	{
		return g;
	}

	case CCmpPathfinder::Goal::CIRCLE:
	{
		CFixedVector2D d = pos - g;
		if (d.IsZero())
			d = CFixedVector2D(fixed::FromInt(1), fixed::Zero()); // some arbitrary direction
		d.Normalize(goal.hw);
		return g + d;
	}

	case CCmpPathfinder::Goal::SQUARE:
	{
		CFixedVector2D halfSize(goal.hw, goal.hh);
		CFixedVector2D d = pos - g;
		return g + Geometry::NearestPointOnSquare(d, goal.u, goal.v, halfSize);
	}

	default:
		debug_warn(L"invalid type");
		return CFixedVector2D();
	}
}

typedef PriorityQueueList<u16, fixed> ShortPathPriorityQueue;

void CCmpPathfinder::ComputeShortPath(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t r, entity_pos_t range, const Goal& goal, Path& path)
{
	PROFILE("ComputeShortPath");

	m_DebugOverlayShortPathLines.clear();

	if (m_DebugOverlay)
	{
		// Render the goal shape
		m_DebugOverlayShortPathLines.push_back(SOverlayLine());
		m_DebugOverlayShortPathLines.back().m_Color = CColor(1, 0, 0, 1);
		switch (goal.type)
		{
		case CCmpPathfinder::Goal::POINT:
		{
			SimRender::ConstructCircleOnGround(*m_Context, goal.x.ToFloat(), goal.z.ToFloat(), 0.2f, m_DebugOverlayShortPathLines.back());
			break;
		}
		case CCmpPathfinder::Goal::CIRCLE:
		{
			SimRender::ConstructCircleOnGround(*m_Context, goal.x.ToFloat(), goal.z.ToFloat(), goal.hw.ToFloat(), m_DebugOverlayShortPathLines.back());
			break;
		}
		case CCmpPathfinder::Goal::SQUARE:
		{
			float a = atan2(goal.v.X.ToFloat(), goal.v.Y.ToFloat());
			SimRender::ConstructSquareOnGround(*m_Context, goal.x.ToFloat(), goal.z.ToFloat(), goal.hw.ToFloat()*2, goal.hh.ToFloat()*2, a, m_DebugOverlayShortPathLines.back());
			break;
		}
		}
	}

	// List of collision edges - paths must never cross these.
	// (Edges are one-sided so intersections are fine in one direction, but not the other direction.)
	std::vector<Edge> edges;

	// Create impassable edges at the max-range boundary, so we can't escape the region
	// where we're meant to be searching
	fixed rangeXMin = x0 - range;
	fixed rangeXMax = x0 + range;
	fixed rangeZMin = z0 - range;
	fixed rangeZMax = z0 + range;
	{
		// (The edges are the opposite direction to usual, so it's an inside-out square)
		Edge e0 = { CFixedVector2D(rangeXMin, rangeZMin), CFixedVector2D(rangeXMin, rangeZMax) };
		Edge e1 = { CFixedVector2D(rangeXMin, rangeZMax), CFixedVector2D(rangeXMax, rangeZMax) };
		Edge e2 = { CFixedVector2D(rangeXMax, rangeZMax), CFixedVector2D(rangeXMax, rangeZMin) };
		Edge e3 = { CFixedVector2D(rangeXMax, rangeZMin), CFixedVector2D(rangeXMin, rangeZMin) };
		edges.push_back(e0);
		edges.push_back(e1);
		edges.push_back(e2);
		edges.push_back(e3);
	}


	CFixedVector2D goalVec(goal.x, goal.z);

	// List of obstruction vertexes (plus start/end points); we'll try to find paths through
	// the graph defined by these vertexes
	std::vector<Vertex> vertexes;

	// Add the start point to the graph
	Vertex start = { CFixedVector2D(x0, z0), fixed::Zero(), (CFixedVector2D(x0, z0) - goalVec).Length(), 0, Vertex::OPEN };
	vertexes.push_back(start);
	const size_t START_VERTEX_ID = 0;

	// Add the goal vertex to the graph.
	// Since the goal isn't always a point, this a special magic virtual vertex which moves around - whenever
	// we look at it from another vertex, it is moved to be the closest point on the goal shape to that vertex.
	Vertex end = { CFixedVector2D(goal.x, goal.z), fixed::Zero(), fixed::Zero(), 0, Vertex::UNEXPLORED };
	vertexes.push_back(end);
	const size_t GOAL_VERTEX_ID = 1;


	// Find all the obstruction squares that might affect us
	CmpPtr<ICmpObstructionManager> cmpObstructionManager(*m_Context, SYSTEM_ENTITY);
	std::vector<ICmpObstructionManager::ObstructionSquare> squares;
	cmpObstructionManager->GetObstructionsInRange(filter, rangeXMin - r, rangeZMin - r, rangeXMax + r, rangeZMax + r, squares);

	// Resize arrays to reduce reallocations
	vertexes.reserve(vertexes.size() + squares.size()*4);
	edges.reserve(edges.size() + squares.size()*4);

	// Convert each obstruction square into collision edges and search graph vertexes
	for (size_t i = 0; i < squares.size(); ++i)
	{
		CFixedVector2D center(squares[i].x, squares[i].z);
		CFixedVector2D u = squares[i].u;
		CFixedVector2D v = squares[i].v;

		// Expand the vertexes by the moving unit's collision radius, to find the
		// closest we can get to it

		entity_pos_t delta = entity_pos_t::FromInt(1)/4;
			// add a small delta so that the vertexes of an edge don't get interpreted
			// as crossing the edge (given minor numerical inaccuracies)
		CFixedVector2D hd0(squares[i].hw + r + delta, squares[i].hh + r + delta);
		CFixedVector2D hd1(squares[i].hw + r + delta, -(squares[i].hh + r + delta));

		Vertex vert;
		vert.status = Vertex::UNEXPLORED;
		vert.p.X = center.X - hd0.Dot(u); vert.p.Y = center.Y + hd0.Dot(v); vertexes.push_back(vert);
		vert.p.X = center.X - hd1.Dot(u); vert.p.Y = center.Y + hd1.Dot(v); vertexes.push_back(vert);
		vert.p.X = center.X + hd0.Dot(u); vert.p.Y = center.Y - hd0.Dot(v); vertexes.push_back(vert);
		vert.p.X = center.X + hd1.Dot(u); vert.p.Y = center.Y - hd1.Dot(v); vertexes.push_back(vert);

		// Add the four edges

		CFixedVector2D h0(squares[i].hw + r, squares[i].hh + r);
		CFixedVector2D h1(squares[i].hw + r, -(squares[i].hh + r));

		CFixedVector2D ev0(center.X - h0.Dot(u), center.Y + h0.Dot(v));
		CFixedVector2D ev1(center.X - h1.Dot(u), center.Y + h1.Dot(v));
		CFixedVector2D ev2(center.X + h0.Dot(u), center.Y - h0.Dot(v));
		CFixedVector2D ev3(center.X + h1.Dot(u), center.Y - h1.Dot(v));
		Edge e0 = { ev0, ev1 };
		Edge e1 = { ev1, ev2 };
		Edge e2 = { ev2, ev3 };
		Edge e3 = { ev3, ev0 };
		edges.push_back(e0);
		edges.push_back(e1);
		edges.push_back(e2);
		edges.push_back(e3);

		// TODO: should clip out vertexes and edges that are outside the range,
		// to reduce the search space
	}

	debug_assert(vertexes.size() < 65536); // we store array indexes as u16

	if (m_DebugOverlay)
	{
		// Render the obstruction edges
		for (size_t i = 0; i < edges.size(); ++i)
		{
			m_DebugOverlayShortPathLines.push_back(SOverlayLine());
			m_DebugOverlayShortPathLines.back().m_Color = CColor(0, 1, 1, 1);
			std::vector<float> xz;
			xz.push_back(edges[i].p0.X.ToFloat());
			xz.push_back(edges[i].p0.Y.ToFloat());
			xz.push_back(edges[i].p1.X.ToFloat());
			xz.push_back(edges[i].p1.Y.ToFloat());
			SimRender::ConstructLineOnGround(*m_Context, xz, m_DebugOverlayShortPathLines.back());
		}
	}

	// Do an A* search over the vertex/visibility graph:

	// Since we are just measuring Euclidean distance the heuristic is admissible,
	// so we never have to re-examine a node once it's been moved to the closed set.

	// To save time in common cases, we don't precompute a graph of valid edges between vertexes;
	// we do it lazily instead. When the search algorithm reaches a vertex, we examine every other
	// vertex and see if we can reach it without hitting any collision edges, and ignore the ones
	// we can't reach. Since the algorithm can only reach a vertex once (and then it'll be marked
	// as closed), we won't be doing any redundant visibility computations.

	PROFILE_START("A*");

	ShortPathPriorityQueue open;
	ShortPathPriorityQueue::Item qiStart = { START_VERTEX_ID, start.h };
	open.push(qiStart);

	u16 idBest = START_VERTEX_ID;
	fixed hBest = start.h;

	while (!open.empty())
	{
		// Move best tile from open to closed
		ShortPathPriorityQueue::Item curr = open.pop();
		vertexes[curr.id].status = Vertex::CLOSED;

		// If we've reached the destination, stop
		if (curr.id == GOAL_VERTEX_ID)
		{
			idBest = curr.id;
			break;
		}

		for (size_t n = 0; n < vertexes.size(); ++n)
		{
			if (vertexes[n].status == Vertex::CLOSED)
				continue;

			// If this is the magical goal vertex, move it to near the current vertex
			CFixedVector2D npos;
			if (n == GOAL_VERTEX_ID)
				npos = NearestPointOnGoal(vertexes[curr.id].p, goal);
			else
				npos = vertexes[n].p;

			bool visible = CheckVisibility(vertexes[curr.id].p, npos, edges);

			/*
			// Render the edges that we examine
			m_DebugOverlayShortPathLines.push_back(SOverlayLine());
			m_DebugOverlayShortPathLines.back().m_Color = visible ? CColor(0, 1, 0, 1) : CColor(0, 0, 0, 1);
			std::vector<float> xz;
			xz.push_back(vertexes[curr.id].p.X.ToFloat());
			xz.push_back(vertexes[curr.id].p.Y.ToFloat());
			xz.push_back(npos.X.ToFloat());
			xz.push_back(npos.Y.ToFloat());
			SimRender::ConstructLineOnGround(*m_Context, xz, m_DebugOverlayShortPathLines.back());
			//*/

			if (visible)
			{
				fixed g = vertexes[curr.id].g + (vertexes[curr.id].p - npos).Length();

				// If this is a new tile, compute the heuristic distance
				if (vertexes[n].status == Vertex::UNEXPLORED)
				{
					// Add it to the open list:
					vertexes[n].status = Vertex::OPEN;
					vertexes[n].g = g;
					vertexes[n].h = DistanceToGoal(npos, goal);
					vertexes[n].pred = curr.id;
					if (n == GOAL_VERTEX_ID)
						vertexes[n].p = npos; // remember the new best goal position
					ShortPathPriorityQueue::Item t = { n, g + vertexes[n].h };
					open.push(t);

					// Remember the heuristically best vertex we've seen so far, in case we never actually reach the target
					if (vertexes[n].h < hBest)
					{
						idBest = n;
						hBest = vertexes[n].h;
					}
				}
				else // must be OPEN
				{
					// If we've already seen this tile, and the new path to this tile does not have a
					// better cost, then stop now
					if (g >= vertexes[n].g)
						continue;

					// Otherwise, we have a better path, so replace the old one with the new cost/parent
					vertexes[n].g = g;
					vertexes[n].pred = curr.id;
					if (n == GOAL_VERTEX_ID)
						vertexes[n].p = npos; // remember the new best goal position
					open.promote((u16)n, g + vertexes[n].h);
					continue;
				}
			}
		}
	}

	// Reconstruct the path (in reverse)
	for (u16 id = idBest; id != START_VERTEX_ID; id = vertexes[id].pred)
	{
		Waypoint w = { vertexes[id].p.X, vertexes[id].p.Y };
		path.m_Waypoints.push_back(w);
	}

	PROFILE_END("A*");
}

//////////////////////////////////////////////////////////

void CCmpPathfinder::RenderSubmit(const CSimContext& context, SceneCollector& collector)
{
	for (size_t i = 0; i < m_DebugOverlayShortPathLines.size(); ++i)
		collector.Submit(&m_DebugOverlayShortPathLines[i]);
}
