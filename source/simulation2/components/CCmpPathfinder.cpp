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
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Overlay.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"
#include "renderer/TerrainOverlay.h"
#include "simulation2/helpers/Render.h"
#include "simulation2/helpers/Geometry.h"
#include "simulation2/components/ICmpWaterManager.h"

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
 * We use a separate bit to indicate building obstructions (instead of folding it into
 * the class passabilities) so that it can be ignored when doing the accurate short paths.
 *
 * To handle movement costs, we have an arbitrary number of unit cost classes (e.g. "infantry", "camel"),
 * and a small number of terrain cost classes (e.g. "grass", "steep grass", "road", "sand"),
 * and a cost mapping table between the classes (e.g. camels are fast on sand).
 * We need log2(|terrain cost classes|) bits per tile to represent costs.
 *
 * We could have one passability bitmap per class, and another array for cost classes,
 * but instead (for no particular reason) we'll pack them all into a single u8 array.
 * Space is a bit tight so maybe this should be changed to a u16 in the future.
 *
 * We handle dynamic updates currently by recomputing the entire array, which is stupid;
 * it should only bother updating the region that has changed.
 */

class PathfinderPassability
{
public:
	PathfinderPassability(u8 mask, const CParamNode& node) :
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
	}

	bool IsPassable(fixed waterdepth, fixed steepness)
	{
		return ((m_MinDepth <= waterdepth && waterdepth <= m_MaxDepth) && (steepness < m_MaxSlope));
	}

	u8 m_Mask;
private:
	fixed m_MinDepth;
	fixed m_MaxDepth;
	fixed m_MaxSlope;
};

typedef u8 TerrainTile; // 1 bit for obstructions, PASS_CLASS_BITS for terrain passability, COST_CLASS_BITS for movement costs
const int PASS_CLASS_BITS = 4;
const int COST_CLASS_BITS = 8 - (PASS_CLASS_BITS + 1);
#define IS_TERRAIN_PASSABLE(item, classmask) (((item) & (classmask)) == 0)
#define IS_PASSABLE(item, classmask) (((item) & ((classmask) | 1)) == 0)
#define GET_COST_CLASS(item) ((item) >> (PASS_CLASS_BITS + 1))
#define COST_CLASS_TAG(id) ((id) << (PASS_CLASS_BITS + 1))

// Default cost to move a single tile is a fairly arbitrary number, which should be big
// enough to be precise when multiplied/divided and small enough to never overflow when
// summing the cost of a whole path.
const int DEFAULT_MOVE_COST = 256;

/**
 * Implementation of ICmpPathfinder
 */
class CCmpPathfinder : public ICmpPathfinder
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_RenderSubmit); // for debug overlays
		componentManager.SubscribeToMessageType(MT_TerrainChanged);
	}

	DEFAULT_COMPONENT_ALLOCATOR(Pathfinder)

	std::map<std::string, u8> m_PassClassMasks;
	std::vector<PathfinderPassability> m_PassClasses;

	std::map<std::string, u8> m_TerrainCostClassTags;
	std::map<std::string, u8> m_UnitCostClassTags;
	std::vector<std::vector<u32> > m_MoveCosts; // costs[unitClass][terrainClass]
	std::vector<std::vector<fixed> > m_MoveSpeeds; // speeds[unitClass][terrainClass]

	u16 m_MapSize; // tiles per side
	Grid<TerrainTile>* m_Grid; // terrain/passability information
	Grid<u8>* m_ObstructionGrid; // cached obstruction information (TODO: we shouldn't bother storing this, it's redundant with LSBs of m_Grid)
	bool m_TerrainDirty; // indicates if m_Grid has been updated since terrain changed

	// Debugging - output from last pathfind operation:
	Grid<PathfindTile>* m_DebugGrid;
	u32 m_DebugSteps;
	Path* m_DebugPath;
	PathfinderOverlay* m_DebugOverlay;
	u8 m_DebugPassClass;

	std::vector<SOverlayLine> m_DebugOverlayShortPathLines;

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CSimContext& UNUSED(context), const CParamNode& paramNode)
	{
		m_MapSize = 0;
		m_Grid = NULL;
		m_ObstructionGrid = NULL;
		m_TerrainDirty = true;

		m_DebugOverlay = NULL;
		m_DebugGrid = NULL;
		m_DebugPath = NULL;

		const CParamNode::ChildrenMap& passClasses = paramNode.GetChild("PassabilityClasses").GetChildren();
		for (CParamNode::ChildrenMap::const_iterator it = passClasses.begin(); it != passClasses.end(); ++it)
		{
			std::string name = it->first;
			debug_assert((int)m_PassClasses.size() <= PASS_CLASS_BITS);
			u8 mask = (1 << (m_PassClasses.size() + 1));
			m_PassClasses.push_back(PathfinderPassability(mask, it->second));
			m_PassClassMasks[name] = mask;
		}


		const CParamNode::ChildrenMap& moveClasses = paramNode.GetChild("MovementClasses").GetChildren();

		// First find the set of unit classes used by any terrain classes,
		// and assign unique tags to terrain classes
		std::set<std::string> unitClassNames;
		unitClassNames.insert("default"); // must always have costs for default

		{
			size_t i = 0;
			for (CParamNode::ChildrenMap::const_iterator it = moveClasses.begin(); it != moveClasses.end(); ++it)
			{
				std::string terrainClassName = it->first;
				m_TerrainCostClassTags[terrainClassName] = COST_CLASS_TAG(i);
				++i;

				const CParamNode::ChildrenMap& unitClasses = it->second.GetChild("UnitClasses").GetChildren();
				for (CParamNode::ChildrenMap::const_iterator uit = unitClasses.begin(); uit != unitClasses.end(); ++uit)
					unitClassNames.insert(uit->first);
			}
		}

		// For each terrain class, set the costs for every unit class,
		// and assign unique tags to unit classes
		{
			size_t i = 0;
			for (std::set<std::string>::const_iterator nit = unitClassNames.begin(); nit != unitClassNames.end(); ++nit)
			{
				m_UnitCostClassTags[*nit] = i;
				++i;

				std::vector<u32> costs;
				std::vector<fixed> speeds;

				for (CParamNode::ChildrenMap::const_iterator it = moveClasses.begin(); it != moveClasses.end(); ++it)
				{
					// Default to the general costs for this terrain class
					fixed cost = it->second.GetChild("@Cost").ToFixed();
					fixed speed = it->second.GetChild("@Speed").ToFixed();
					// Check for specific cost overrides for this unit class
					const CParamNode& unitClass = it->second.GetChild("UnitClasses").GetChild(nit->c_str());
					if (unitClass.IsOk())
					{
						cost = unitClass.GetChild("@Cost").ToFixed();
						speed = unitClass.GetChild("@Speed").ToFixed();
					}
					costs.push_back((cost * DEFAULT_MOVE_COST).ToInt_RoundToZero());
					speeds.push_back(speed);
				}

				m_MoveCosts.push_back(costs);
				m_MoveSpeeds.push_back(speeds);
			}
		}
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
		delete m_Grid;
		delete m_ObstructionGrid;
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
		case MT_TerrainChanged:
		{
			// TODO: we ought to only bother updating the dirtied region
			m_TerrainDirty = true;
			break;
		}
		}
	}

	virtual u8 GetPassabilityClass(const std::string& name)
	{
		if (m_PassClassMasks.find(name) == m_PassClassMasks.end())
		{
			LOGERROR(L"Invalid passability class name '%hs'", name.c_str());
			return 0;
		}

		return m_PassClassMasks[name];
	}

	virtual std::vector<std::string> GetPassabilityClasses()
	{
		std::vector<std::string> classes;
		for (std::map<std::string, u8>::iterator it = m_PassClassMasks.begin(); it != m_PassClassMasks.end(); ++it)
			classes.push_back(it->first);
		return classes;
	}

	virtual u8 GetCostClass(const std::string& name)
	{
		if (m_UnitCostClassTags.find(name) == m_UnitCostClassTags.end())
		{
			LOGERROR(L"Invalid unit cost class name '%hs'", name.c_str());
			return m_UnitCostClassTags["default"];
		}

		return m_UnitCostClassTags[name];
	}

	virtual void ComputePath(entity_pos_t x0, entity_pos_t z0, const Goal& goal, u8 passClass, u8 costClass, Path& ret);

	virtual void ComputeShortPath(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t r, entity_pos_t range, const Goal& goal, u8 passClass, Path& ret);

	virtual void SetDebugPath(entity_pos_t x0, entity_pos_t z0, const Goal& goal, u8 passClass, u8 costClass)
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

	virtual fixed GetMovementSpeed(entity_pos_t x0, entity_pos_t z0, u8 costClass)
	{
		u16 i, j;
		NearestTile(x0, z0, i, j);
		TerrainTile tileTag = m_Grid->get(i, j);
		return m_MoveSpeeds.at(costClass).at(GET_COST_CLASS(tileTag));
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
			ssize_t size = GetSimContext().GetTerrain().GetTilesPerSide();

			debug_assert(size >= 1 && size <= 0xffff); // must fit in 16 bits
			m_MapSize = size;
			m_Grid = new Grid<TerrainTile>(m_MapSize, m_MapSize);
			m_ObstructionGrid = new Grid<u8>(m_MapSize, m_MapSize);
		}

		CmpPtr<ICmpWaterManager> cmpWaterMan(GetSimContext(), SYSTEM_ENTITY);

		CTerrain& terrain = GetSimContext().GetTerrain();

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
		if (cmpObstructionManager->Rasterise(*m_ObstructionGrid) || m_TerrainDirty)
		{
			// Obstructions or terrain changed - we need to recompute passability
			// TODO: only bother recomputing the region that has actually changed

			for (size_t j = 0; j < m_MapSize; ++j)
			{
				for (size_t i = 0; i < m_MapSize; ++i)
				{
					fixed x, z;
					TileCenter(i, j, x, z);

					TerrainTile t = 0;

					bool obstruct = (m_ObstructionGrid->get(i, j) != 0);

					fixed height = terrain.GetVertexGroundLevelFixed(i, j); // TODO: should use tile centre

					fixed water;
					if (!cmpWaterMan.null())
						water = cmpWaterMan->GetWaterLevel(x, z);

					fixed depth = water - height;

					fixed slope = terrain.GetSlopeFixed(i, j);

					if (obstruct)
						t |= 1;

					for (size_t n = 0; n < m_PassClasses.size(); ++n)
					{
						if (!m_PassClasses[n].IsPassable(depth, slope))
							t |= (m_PassClasses[n].m_Mask & ~1);
					}

					std::string moveClass = terrain.GetMovementClass(i, j);
					if (m_TerrainCostClassTags.find(moveClass) != m_TerrainCostClassTags.end())
						t |= m_TerrainCostClassTags[moveClass];

					m_Grid->set(i, j, t);
				}
			}

			m_TerrainDirty = false;
		}
	}

	void RenderSubmit(const CSimContext& context, SceneCollector& collector);
};

REGISTER_COMPONENT_TYPE(Pathfinder)

// Base cost to move between adjacent tiles
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
	TerrainTile tileTag = state.terrain->get(i, j);
	if (!IS_PASSABLE(tileTag, state.passClass))
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


//////////////////////////////////////////////////////////

struct Vertex
{
	enum
	{
		UNEXPLORED,
		OPEN,
		CLOSED,
	};

	CFixedVector2D p;
	fixed g, h;
	u16 pred;
	u8 status;
};

struct Edge
{
	CFixedVector2D p0, p1;
};

// When computing vertexes to insert into the search graph,
// add a small delta so that the vertexes of an edge don't get interpreted
// as crossing the edge (given minor numerical inaccuracies)
static const entity_pos_t EDGE_EXPAND_DELTA = entity_pos_t::FromInt(1)/4;

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

struct TileEdge
{
	u16 i, j;
	enum { TOP, BOTTOM, LEFT, RIGHT } dir;
};

static void AddTerrainEdges(std::vector<Edge>& edges, std::vector<Vertex>& vertexes, u16 i0, u16 j0, u16 i1, u16 j1, fixed r, u8 passClass, const Grid<TerrainTile>& terrain)
{
	PROFILE("AddTerrainEdges");

	std::vector<TileEdge> tileEdges;

	// Find all edges between tiles of differently passability statuses
	for (u16 j = j0; j <= j1; ++j)
	{
		for (u16 i = i0; i <= i1; ++i)
		{
			if (!IS_TERRAIN_PASSABLE(terrain.get(i, j), passClass))
			{
				if (j > 0 && IS_TERRAIN_PASSABLE(terrain.get(i, j-1), passClass))
				{
					TileEdge e = { i, j, TileEdge::BOTTOM };
					tileEdges.push_back(e);
				}

				if (j < terrain.m_H-1 && IS_TERRAIN_PASSABLE(terrain.get(i, j+1), passClass))
				{
					TileEdge e = { i, j, TileEdge::TOP };
					tileEdges.push_back(e);
				}

				if (i > 0 && IS_TERRAIN_PASSABLE(terrain.get(i-1, j), passClass))
				{
					TileEdge e = { i, j, TileEdge::LEFT };
					tileEdges.push_back(e);
				}

				if (i < terrain.m_W-1 && IS_TERRAIN_PASSABLE(terrain.get(i+1, j), passClass))
				{
					TileEdge e = { i, j, TileEdge::RIGHT };
					tileEdges.push_back(e);
				}
			}
		}
	}

	// TODO: maybe we should precompute these terrain edges since they'll rarely change?

	// TODO: for efficiency (minimising the A* search space), we should coalesce adjoining edges

	// Add all the tile edges to the search edge/vertex lists
	for (size_t n = 0; n < tileEdges.size(); ++n)
	{
		u16 i = tileEdges[n].i;
		u16 j = tileEdges[n].j;
		CFixedVector2D v0, v1;
		Vertex vert;
		vert.status = Vertex::UNEXPLORED;

		switch (tileEdges[n].dir)
		{
		case TileEdge::BOTTOM:
		{
			v0 = CFixedVector2D(fixed::FromInt(i * CELL_SIZE) - r, fixed::FromInt(j * CELL_SIZE) - r);
			v1 = CFixedVector2D(fixed::FromInt((i+1) * CELL_SIZE) + r, fixed::FromInt(j * CELL_SIZE) - r);
			Edge e = { v0, v1 };
			edges.push_back(e);
			vert.p.X = v0.X - EDGE_EXPAND_DELTA; vert.p.Y = v0.Y - EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			vert.p.X = v1.X + EDGE_EXPAND_DELTA; vert.p.Y = v1.Y - EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			break;
		}
		case TileEdge::TOP:
		{
			v0 = CFixedVector2D(fixed::FromInt((i+1) * CELL_SIZE) + r, fixed::FromInt((j+1) * CELL_SIZE) + r);
			v1 = CFixedVector2D(fixed::FromInt(i * CELL_SIZE) - r, fixed::FromInt((j+1) * CELL_SIZE) + r);
			Edge e = { v0, v1 };
			edges.push_back(e);
			vert.p.X = v0.X + EDGE_EXPAND_DELTA; vert.p.Y = v0.Y + EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			vert.p.X = v1.X - EDGE_EXPAND_DELTA; vert.p.Y = v1.Y + EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			break;
		}
		case TileEdge::LEFT:
		{
			v0 = CFixedVector2D(fixed::FromInt(i * CELL_SIZE) - r, fixed::FromInt((j+1) * CELL_SIZE) + r);
			v1 = CFixedVector2D(fixed::FromInt(i * CELL_SIZE) - r, fixed::FromInt(j * CELL_SIZE) - r);
			Edge e = { v0, v1 };
			edges.push_back(e);
			vert.p.X = v0.X - EDGE_EXPAND_DELTA; vert.p.Y = v0.Y + EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			vert.p.X = v1.X - EDGE_EXPAND_DELTA; vert.p.Y = v1.Y - EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			break;
		}
		case TileEdge::RIGHT:
		{
			v0 = CFixedVector2D(fixed::FromInt((i+1) * CELL_SIZE) + r, fixed::FromInt(j * CELL_SIZE) - r);
			v1 = CFixedVector2D(fixed::FromInt((i+1) * CELL_SIZE) + r, fixed::FromInt((j+1) * CELL_SIZE) + r);
			Edge e = { v0, v1 };
			edges.push_back(e);
			vert.p.X = v0.X + EDGE_EXPAND_DELTA; vert.p.Y = v0.Y - EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			vert.p.X = v1.X + EDGE_EXPAND_DELTA; vert.p.Y = v1.Y + EDGE_EXPAND_DELTA; vertexes.push_back(vert);
			break;
		}
		}
	}
}

void CCmpPathfinder::ComputeShortPath(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t r, entity_pos_t range, const Goal& goal, u8 passClass, Path& path)
{
	UpdateGrid(); // TODO: only need to bother updating if the terrain changed

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
			SimRender::ConstructCircleOnGround(GetSimContext(), goal.x.ToFloat(), goal.z.ToFloat(), 0.2f, m_DebugOverlayShortPathLines.back(), true);
			break;
		}
		case CCmpPathfinder::Goal::CIRCLE:
		{
			SimRender::ConstructCircleOnGround(GetSimContext(), goal.x.ToFloat(), goal.z.ToFloat(), goal.hw.ToFloat(), m_DebugOverlayShortPathLines.back(), true);
			break;
		}
		case CCmpPathfinder::Goal::SQUARE:
		{
			float a = atan2(goal.v.X.ToFloat(), goal.v.Y.ToFloat());
			SimRender::ConstructSquareOnGround(GetSimContext(), goal.x.ToFloat(), goal.z.ToFloat(), goal.hw.ToFloat()*2, goal.hh.ToFloat()*2, a, m_DebugOverlayShortPathLines.back(), true);
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

	// Add terrain obstructions
	{
		u16 i0, j0, i1, j1;
		NearestTile(rangeXMin, rangeZMin, i0, j0);
		NearestTile(rangeXMax, rangeZMax, i1, j1);
		AddTerrainEdges(edges, vertexes, i0, j0, i1, j1, r, passClass, *m_Grid);
	}

	// Find all the obstruction squares that might affect us
	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
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

		CFixedVector2D hd0(squares[i].hw + r + EDGE_EXPAND_DELTA, squares[i].hh + r + EDGE_EXPAND_DELTA);
		CFixedVector2D hd1(squares[i].hw + r + EDGE_EXPAND_DELTA, -(squares[i].hh + r + EDGE_EXPAND_DELTA));

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
			SimRender::ConstructLineOnGround(GetSimContext(), xz, m_DebugOverlayShortPathLines.back(), true);
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
			SimRender::ConstructLineOnGround(GetSimContext(), xz, m_DebugOverlayShortPathLines.back());
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
					ShortPathPriorityQueue::Item t = { (u16)n, g + vertexes[n].h };
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

void CCmpPathfinder::RenderSubmit(const CSimContext& UNUSED(context), SceneCollector& collector)
{
	for (size_t i = 0; i < m_DebugOverlayShortPathLines.size(); ++i)
		collector.Submit(&m_DebugOverlayShortPathLines[i]);
}
