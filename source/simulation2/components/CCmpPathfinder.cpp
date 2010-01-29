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

#include "graphics/Terrain.h"
#include "maths/MathUtil.h"
#include "ps/Overlay.h"
#include "ps/Profile.h"
#include "renderer/TerrainOverlay.h"

class CCmpPathfinder;
struct PathfindTile;

// Basic 2D array, for storing tile data
// (TODO: Maybe this could use a more cache-friendly data layout or something?)
template<typename T>
class Grid
{
public:
	Grid(u16 w, u16 h) : m_W(w), m_H(h)
	{
		m_Data = new T[m_W * m_H];
		reset();
	}

	~Grid()
	{
		delete[] m_Data;
	}

	void reset()
	{
		memset(m_Data, 0, m_W*m_H*sizeof(T));
	}

	void set(size_t i, size_t j, const T& value)
	{
		debug_assert(i < m_W && j < m_H);
		m_Data[j*m_W + i] = value;
	}

	T& get(size_t i, size_t j)
	{
		debug_assert(i < m_W && j < m_H);
		return m_Data[j*m_W + i];
	}

	u16 m_W, m_H;
	T* m_Data;
};

// Externally, tags are opaque non-zero positive integers
// Internally, they are tagged (by shape) indexes into shape lists
#define TAG_IS_CIRCLE(tag) (((tag) & 1) == 0)
#define TAG_IS_SQUARE(tag) (((tag) & 1) == 1)
#define CIRCLE_INDEX_TO_TAG(idx) ((((idx)+1) << 1) | 0)
#define SQUARE_INDEX_TO_TAG(idx) ((((idx)+1) << 1) | 1)
#define TAG_TO_INDEX(tag) (((tag) >> 1)-1)

/**
 * Internal representation of circle shapes
 */
struct Circle
{
	entity_pos_t x, z, r;
};

/**
 * Internal representation of square shapes
 */
struct Square
{
	entity_pos_t x, z;
	entity_angle_t a;
	entity_pos_t w, h;
};

/**
 * Terrain overlay for pathfinder debugging.
 * Renders a representation of the most recent pathfinding operation.
 */
class PathfinderOverlay : public TerrainOverlay
{
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

	bool m_GridDirty; // whether m_Grid is invalid
	u16 m_MapSize; // tiles per side
	Grid<u8>* m_Grid; // terrain/passability information

	// Debugging - output from last pathfind operation:
	Grid<PathfindTile>* m_DebugGrid;
	Path* m_DebugPath;
	PathfinderOverlay* m_DebugOverlay;

	// TODO: using std::map is stupid and inefficient
	std::map<tag_t, Circle> m_Circles;
	std::map<tag_t, Square> m_Squares;

	virtual void Init(const CSimContext& context, const CParamNode& UNUSED(paramNode))
	{
		m_Context = &context;

		m_GridDirty = true;
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

	virtual tag_t AddCircle(entity_pos_t x, entity_pos_t z, entity_pos_t r)
	{
		Circle c = { x, z, r };
		size_t id = m_Circles.size();
		m_Circles[id] = c;
		m_GridDirty = true;
		return CIRCLE_INDEX_TO_TAG(id);
	}

	virtual tag_t AddSquare(entity_pos_t x, entity_pos_t z, entity_angle_t a, entity_pos_t w, entity_pos_t h)
	{
		Square s = { x, z, a, w, h };
		size_t id = m_Squares.size();
		m_Squares[id] = s;
		m_GridDirty = true;
		return SQUARE_INDEX_TO_TAG(id);
	}

	virtual void MoveShape(tag_t tag, entity_pos_t x, entity_pos_t z, entity_angle_t a)
	{
		if (TAG_IS_CIRCLE(tag))
		{
			Circle& c = m_Circles[TAG_TO_INDEX(tag)];
			c.x = x;
			c.z = z;
		}
		else
		{
			Square& s = m_Squares[TAG_TO_INDEX(tag)];
			s.x = x;
			s.z = z;
			s.a = a;
		}

		m_GridDirty = true;
	}

	virtual void RemoveShape(tag_t tag)
	{
		if (TAG_IS_CIRCLE(tag))
			m_Circles.erase(TAG_TO_INDEX(tag));
		else
			m_Squares.erase(TAG_TO_INDEX(tag));

		m_GridDirty = true;
	}

	virtual void SetDebugPath(entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1)
	{
		delete m_DebugGrid;
		m_DebugGrid = NULL;
		delete m_DebugPath;
		m_DebugPath = new Path();
		ComputePath(x0, z0, x1, z1, *m_DebugPath);
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
	 * Regenerates the grid based on the shape lists, if necessary
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

		if (m_GridDirty)
		{
			// TODO: this is all hopelessly inefficient
			// What we should perhaps do is have some kind of quadtree storing Shapes so it's
			// quick to invalidate and update small numbers of tiles

			m_Grid->reset();

			for (std::map<tag_t, Circle>::iterator it = m_Circles.begin(); it != m_Circles.end(); ++it)
			{
				// TODO: need to handle larger circles (r != 0)
				u16 i, j;
				NearestTile(it->second.x, it->second.z, i, j);
				m_Grid->set(i, j, 1);
			}

			for (std::map<tag_t, Square>::iterator it = m_Squares.begin(); it != m_Squares.end(); ++it)
			{
				// TODO: need to handle rotations (a != 0)
				entity_pos_t x0 = it->second.x - it->second.w/2;
				entity_pos_t z0 = it->second.z - it->second.h/2;
				entity_pos_t x1 = it->second.x + it->second.w/2;
				entity_pos_t z1 = it->second.z + it->second.h/2;
				u16 i0, j0, i1, j1;
				NearestTile(x0, z0, i0, j0); // TODO: should be careful about rounding on edges
				NearestTile(x1, z1, i1, j1);
				for (u16 j = j0; j <= j1; ++j)
					for (u16 i = i0; i <= i1; ++i)
						m_Grid->set(i, j, 1);
			}

			m_GridDirty = false;
		}
	}

	void ComputePath(entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, Path& path);
};

REGISTER_COMPONENT_TYPE(Pathfinder)


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
		RenderTile(CColor(1, 0, 0, 0.6), false);

	if (m_Pathfinder.m_DebugGrid)
	{
		PathfindTile& n = m_Pathfinder.m_DebugGrid->get(i, j);

		float c = clamp((n.cost/256.f) / 32.f, 0.f, 1.f);

		if (n.status == PathfindTile::STATUS_OPEN)
			RenderTile(CColor(1, 1, c, 0.6), false);
		else if (n.status == PathfindTile::STATUS_CLOSED)
			RenderTile(CColor(0, 1, c, 0.6), false);
	}
}

/*
 * A* pathfinding implementation
 *
 * This is currently all a bit rubbish and hasn't been tested for correctness or efficiency;
 * the intention is to demonstrate the interface that the pathfinder can use, and improvements
 * to the implementation shouldn't affect that interface much.
 */

u32 g_CostPerTile = 256; // base cost to move between adjacent tiles

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
		debug_warn(L"duplicate tiles in queue");
		return false;
	}
};

// Priority queue implementation, based on std::priority_queue but with O(n) find/update functions
// TODO: this is all a bit rubbish and slow
class PriorityQueue
{
public:
	void push(const QueueItem& item)
	{
		m_Heap.push_back(item);
		push_heap(m_Heap.begin(), m_Heap.end(), QueueItemPriority());
	}

	void fixheap()
	{
		make_heap(m_Heap.begin(), m_Heap.end(), QueueItemPriority());
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

	void remove(u16 i, u16 j)
	{
		for (size_t n = 0; n < m_Heap.size(); ++n)
		{
			if (m_Heap[n].i == i && m_Heap[n].j == j)
			{
				m_Heap.erase(m_Heap.begin() + n);
				fixheap(); // XXX: this is slow
				return;
			}
		}
	}

	const QueueItem& top()
	{
		debug_assert(m_Heap.size());
		return m_Heap.front();
	}

	void pop()
	{
		debug_assert(m_Heap.size());
		pop_heap(m_Heap.begin(), m_Heap.end(), QueueItemPriority());
		m_Heap.pop_back();
	}

	bool empty()
	{
		return m_Heap.empty();
	}

	std::vector<QueueItem> m_Heap;
};

static void ProcessNeighbour(u16 pi, u16 pj, u16 i, u16 j, u32 pg, PriorityQueue& open, PriorityQueue& closed, Grid<PathfindTile>* tempGrid, Grid<u8>* terrainGrid, u16 i1, u16 j1)
{
	if (terrainGrid->get(i, j))
		return;

	u32 cost = pg + g_CostPerTile;
	// TODO: should use terrain costs etc

	u32 h = (abs((int)i - (int)i1) + abs((int)j - (int)j1)) * g_CostPerTile;

	PathfindTile& n = tempGrid->get(i, j);

	// If we've already added this tile to the open list:
	if (n.status == PathfindTile::STATUS_OPEN)
	{
		// If this a better path, replace the old one with the new cost/parent
		if (cost < n.cost)
		{
			n.cost = cost;
			n.pi = pi;
			n.pj = pj;
			open.find(i, j)->rank = cost + h;
			open.fixheap(); // XXX: this is slow
		}
		return;
	}

	// If we've already found the 'best' path to this tile:
	if (n.status == PathfindTile::STATUS_CLOSED)
	{
		// If this is a better path (possible when we use inadmissible heuristics), reopen it
		if (cost < n.cost)
		{
			closed.remove(i, j);
			// (don't return yet)
		}
		else
		{
			return;
		}
	}

	// Add it to the open list:
	n.status = PathfindTile::STATUS_OPEN;
	n.cost = cost;
	n.pi = pi;
	n.pj = pj;
	QueueItem t = { i, j, cost + h };
	open.push(t);
}

void CCmpPathfinder::ComputePath(entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, Path& path)
{
	UpdateGrid();

	PROFILE("ComputePath");

	Grid<PathfindTile>* tempGrid = new Grid<PathfindTile>(m_MapSize, m_MapSize);

	u16 i0, j0, i1, j1;
	NearestTile(x0, z0, i0, j0);
	NearestTile(x1, z1, i1, j1);

	PriorityQueue open;
	PriorityQueue closed;

	QueueItem start = { i0, j0, 0 };
	open.push(start);
	tempGrid->get(i0, j0).status = PathfindTile::STATUS_OPEN;

	u16 ip = i0, jp = j0; // the last tile on the path to the destination

	size_t steps = 0;
	while (1)
	{
		++steps;

		// Hack to avoid spending ages computing giant paths
		// (TODO: ought to return a path that's at least heading in the right direction)
		if (steps > 10000)
			break;

		// If we ran out of tiles to examine, give up
		if (open.empty())
			break;

		// Move best tile from open to closed
		QueueItem curr = open.top();
		open.pop();
		closed.push(curr);
		tempGrid->get(curr.i, curr.j).status = PathfindTile::STATUS_CLOSED;
		ip = curr.i;
		jp = curr.j;

		// If we've reached the destination, stop
		if (curr.i == i1 && curr.j == j1)
			break;

		u32 g = tempGrid->get(curr.i, curr.j).cost;
		if (curr.i > 0)
			ProcessNeighbour(curr.i, curr.j, curr.i-1, curr.j, g, open, closed, tempGrid, m_Grid, i1, j1);
		if (curr.i < m_MapSize-1)
			ProcessNeighbour(curr.i, curr.j, curr.i+1, curr.j, g, open, closed, tempGrid, m_Grid, i1, j1);
		if (curr.j > 0)
			ProcessNeighbour(curr.i, curr.j, curr.i, curr.j-1, g, open, closed, tempGrid, m_Grid, i1, j1);
		if (curr.j < m_MapSize-1)
			ProcessNeighbour(curr.i, curr.j, curr.i, curr.j+1, g, open, closed, tempGrid, m_Grid, i1, j1);
	}

	// Reconstruct the path (in reverse)
	while (ip != i0 || jp != j0)
	{
		PathfindTile& n = tempGrid->get(ip, jp);
		entity_pos_t x, z;
		TileCenter(ip, jp, x, z);
		Waypoint w = { x, z, n.cost };
		path.m_Waypoints.push_back(w);

		// Follow the predecessor link
		ip = n.pi;
		jp = n.pj;
	}

	// Save this grid for debug display
	delete m_DebugGrid;
	m_DebugGrid = tempGrid;
}
