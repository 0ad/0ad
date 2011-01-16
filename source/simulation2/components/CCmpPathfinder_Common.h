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

#ifndef INCLUDED_CCMPPATHFINDER_COMMON
#define INCLUDED_CCMPPATHFINDER_COMMON

/**
 * @file
 * Declares CCmpPathfinder, whose implementation is split into multiple source files,
 * and provides common code needed for more than one of those files.
 * CCmpPathfinder includes two pathfinding algorithms (one tile-based, one vertex-based)
 * with some shared state and functionality, so the code is split into
 * CCmpPathfinder_Vertex.cpp, CCmpPathfinder_Tile.cpp and CCmpPathfinder.cpp
 */

#include "simulation2/system/Component.h"

#include "ICmpPathfinder.h"

#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "maths/MathUtil.h"
#include "simulation2/helpers/Geometry.h"
#include "simulation2/helpers/Grid.h"

class PathfinderOverlay;
class SceneCollector;
struct PathfindTile;

#ifdef NDEBUG
#define PATHFIND_DEBUG 0
#else
#define PATHFIND_DEBUG 1
#endif

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
#define COST_CLASS_TAG(id) ( (u8) ((id) << (PASS_CLASS_BITS + 1)) )

struct AsyncLongPathRequest
{
	u32 ticket;
	entity_pos_t x0;
	entity_pos_t z0;
	ICmpPathfinder::Goal goal;
	u8 passClass;
	u8 costClass;
	entity_id_t notify;
};

struct AsyncShortPathRequest
{
	u32 ticket;
	entity_pos_t x0;
	entity_pos_t z0;
	entity_pos_t r;
	entity_pos_t range;
	ICmpPathfinder::Goal goal;
	u8 passClass;
	bool avoidMovingUnits;
	entity_id_t group;
	entity_id_t notify;
};

/**
 * Implementation of ICmpPathfinder
 */
class CCmpPathfinder : public ICmpPathfinder
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Update);
		componentManager.SubscribeToMessageType(MT_RenderSubmit); // for debug overlays
		componentManager.SubscribeToMessageType(MT_TerrainChanged);
	}

	DEFAULT_COMPONENT_ALLOCATOR(Pathfinder)

	// Template state:

	std::map<std::string, u8> m_PassClassMasks;
	std::vector<PathfinderPassability> m_PassClasses;

	std::map<std::string, u8> m_TerrainCostClassTags;
	std::map<std::string, u8> m_UnitCostClassTags;
	std::vector<std::vector<u32> > m_MoveCosts; // costs[unitClass][terrainClass]
	std::vector<std::vector<fixed> > m_MoveSpeeds; // speeds[unitClass][terrainClass]

	// Dynamic state:

	std::vector<AsyncLongPathRequest> m_AsyncLongPathRequests;
	std::vector<AsyncShortPathRequest> m_AsyncShortPathRequests;
	u32 m_NextAsyncTicket; // unique IDs for asynchronous path requests

	// Lazily-constructed dynamic state (not serialized):

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

	virtual void Init(const CParamNode& paramNode);

	virtual void Deinit();

	virtual void Serialize(ISerializer& serialize);

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize);

	virtual void HandleMessage(const CMessage& msg, bool global);

	virtual u8 GetPassabilityClass(const std::string& name);

	virtual std::vector<std::string> GetPassabilityClasses();

	virtual u8 GetCostClass(const std::string& name);

	virtual void ComputePath(entity_pos_t x0, entity_pos_t z0, const Goal& goal, u8 passClass, u8 costClass, Path& ret);

	virtual u32 ComputePathAsync(entity_pos_t x0, entity_pos_t z0, const Goal& goal, u8 passClass, u8 costClass, entity_id_t notify);

	virtual void ComputeShortPath(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t r, entity_pos_t range, const Goal& goal, u8 passClass, Path& ret);

	virtual u32 ComputeShortPathAsync(entity_pos_t x0, entity_pos_t z0, entity_pos_t r, entity_pos_t range, const Goal& goal, u8 passClass, bool avoidMovingUnits, entity_id_t controller, entity_id_t notify);

	virtual void SetDebugPath(entity_pos_t x0, entity_pos_t z0, const Goal& goal, u8 passClass, u8 costClass);

	virtual void ResetDebugPath();

	virtual void SetDebugOverlay(bool enabled);

	virtual fixed GetMovementSpeed(entity_pos_t x0, entity_pos_t z0, u8 costClass);

	virtual CFixedVector2D GetNearestPointOnGoal(CFixedVector2D pos, const Goal& goal);

	virtual bool CheckMovement(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r, u8 passClass);

	virtual void FinishAsyncRequests();

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

	static fixed DistanceToGoal(CFixedVector2D pos, const CCmpPathfinder::Goal& goal);

	/**
	 * Regenerates the grid based on the current obstruction list, if necessary
	 */
	void UpdateGrid();

	void RenderSubmit(SceneCollector& collector);
};

#endif // INCLUDED_CCMPPATHFINDER_COMMON
