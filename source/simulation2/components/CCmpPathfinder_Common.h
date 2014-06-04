/* Copyright (C) 2013 Wildfire Games.
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
 * We use another bit to indicate tiles near obstructions that block construction,
 * for the AI to plan safe building spots.
 *
 * To handle movement costs, we have an arbitrary number of unit cost classes (e.g. "infantry", "camel"),
 * and a small number of terrain cost classes (e.g. "grass", "steep grass", "road", "sand"),
 * and a cost mapping table between the classes (e.g. camels are fast on sand).
 * We need log2(|terrain cost classes|) bits per tile to represent costs.
 *
 * We could have one passability bitmap per class, and another array for cost classes,
 * but instead (for no particular reason) we'll pack them all into a single u16 array.
 *
 * We handle dynamic updates currently by recomputing the entire array, which is stupid;
 * it should only bother updating the region that has changed.
 */
class PathfinderPassability
{
public:
	PathfinderPassability(ICmpPathfinder::pass_class_t mask, const CParamNode& node) :
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

	}

	bool IsPassable(fixed waterdepth, fixed steepness, fixed shoredist)
	{
		return ((m_MinDepth <= waterdepth && waterdepth <= m_MaxDepth) && (steepness < m_MaxSlope) && (m_MinShore <= shoredist && shoredist <= m_MaxShore));
	}

	ICmpPathfinder::pass_class_t m_Mask;
private:
	fixed m_MinDepth;
	fixed m_MaxDepth;
	fixed m_MaxSlope;
	fixed m_MinShore;
	fixed m_MaxShore;
};

typedef u16 TerrainTile;
// 1 bit for pathfinding obstructions,
// 1 bit for construction obstructions (used by AI),
// PASS_CLASS_BITS for terrain passability (allowing PASS_CLASS_BITS classes),
// COST_CLASS_BITS for movement costs (allowing 2^COST_CLASS_BITS classes)

const int PASS_CLASS_BITS = 10;
const int COST_CLASS_BITS = 16 - (PASS_CLASS_BITS + 2);
#define IS_TERRAIN_PASSABLE(item, classmask) (((item) & (classmask)) == 0)
#define IS_PASSABLE(item, classmask) (((item) & ((classmask) | 1)) == 0)
#define GET_COST_CLASS(item) ((item) >> (PASS_CLASS_BITS + 2))
#define COST_CLASS_MASK(id) ( (TerrainTile) ((id) << (PASS_CLASS_BITS + 2)) )

typedef SparseGrid<PathfindTile> PathfindTileGrid;

struct AsyncLongPathRequest
{
	u32 ticket;
	entity_pos_t x0;
	entity_pos_t z0;
	ICmpPathfinder::Goal goal;
	ICmpPathfinder::pass_class_t passClass;
	ICmpPathfinder::cost_class_t costClass;
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
	ICmpPathfinder::pass_class_t passClass;
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
		componentManager.SubscribeToMessageType(MT_WaterChanged);
		componentManager.SubscribeToMessageType(MT_ObstructionMapShapeChanged);
		componentManager.SubscribeToMessageType(MT_TurnStart);
	}

	DEFAULT_COMPONENT_ALLOCATOR(Pathfinder)

	// Template state:

	std::map<std::string, pass_class_t> m_PassClassMasks;
	std::vector<PathfinderPassability> m_PassClasses;

	std::map<std::string, cost_class_t> m_TerrainCostClassTags;
	std::map<std::string, cost_class_t> m_UnitCostClassTags;
	std::vector<std::vector<u32> > m_MoveCosts; // costs[unitClass][terrainClass]
	std::vector<std::vector<fixed> > m_MoveSpeeds; // speeds[unitClass][terrainClass]

	// Dynamic state:

	std::vector<AsyncLongPathRequest> m_AsyncLongPathRequests;
	std::vector<AsyncShortPathRequest> m_AsyncShortPathRequests;
	u32 m_NextAsyncTicket; // unique IDs for asynchronous path requests
	u16 m_SameTurnMovesCount; // current number of same turn moves we have processed this turn

	// Lazily-constructed dynamic state (not serialized):

	u16 m_MapSize; // tiles per side
	Grid<TerrainTile>* m_Grid; // terrain/passability information
	Grid<u8>* m_ObstructionGrid; // cached obstruction information (TODO: we shouldn't bother storing this, it's redundant with LSBs of m_Grid)
	bool m_TerrainDirty; // indicates if m_Grid has been updated since terrain changed
	
	// For responsiveness we will process some moves in the same turn they were generated in
	
	u16 m_MaxSameTurnMoves; // max number of moves that can be created and processed in the same turn

	// Debugging - output from last pathfind operation:

	PathfindTileGrid* m_DebugGrid;
	u32 m_DebugSteps;
	Path* m_DebugPath;
	PathfinderOverlay* m_DebugOverlay;
	pass_class_t m_DebugPassClass;

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

	virtual pass_class_t GetPassabilityClass(const std::string& name);

	virtual std::map<std::string, pass_class_t> GetPassabilityClasses();

	virtual cost_class_t GetCostClass(const std::string& name);

	virtual const Grid<u16>& GetPassabilityGrid();

	virtual void ComputePath(entity_pos_t x0, entity_pos_t z0, const Goal& goal, pass_class_t passClass, cost_class_t costClass, Path& ret);

	virtual u32 ComputePathAsync(entity_pos_t x0, entity_pos_t z0, const Goal& goal, pass_class_t passClass, cost_class_t costClass, entity_id_t notify);

	virtual void ComputeShortPath(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t r, entity_pos_t range, const Goal& goal, pass_class_t passClass, Path& ret);

	virtual u32 ComputeShortPathAsync(entity_pos_t x0, entity_pos_t z0, entity_pos_t r, entity_pos_t range, const Goal& goal, pass_class_t passClass, bool avoidMovingUnits, entity_id_t controller, entity_id_t notify);

	virtual void SetDebugPath(entity_pos_t x0, entity_pos_t z0, const Goal& goal, pass_class_t passClass, cost_class_t costClass);

	virtual void ResetDebugPath();

	virtual void SetDebugOverlay(bool enabled);

	virtual fixed GetMovementSpeed(entity_pos_t x0, entity_pos_t z0, cost_class_t costClass);

	virtual CFixedVector2D GetNearestPointOnGoal(CFixedVector2D pos, const Goal& goal);

	virtual bool CheckMovement(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r, pass_class_t passClass);

	virtual ICmpObstruction::EFoundationCheck CheckUnitPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t r, pass_class_t passClass);

virtual ICmpObstruction::EFoundationCheck CheckUnitPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t r, pass_class_t passClass, bool onlyCenterPoint);

	virtual ICmpObstruction::EFoundationCheck CheckBuildingPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h, entity_id_t id, pass_class_t passClass);

	virtual ICmpObstruction::EFoundationCheck CheckBuildingPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h, entity_id_t id, pass_class_t passClass, bool onlyCenterPoint);

	virtual void FinishAsyncRequests();

	void ProcessLongRequests(const std::vector<AsyncLongPathRequest>& longRequests);
	
	void ProcessShortRequests(const std::vector<AsyncShortPathRequest>& shortRequests);

	virtual void ProcessSameTurnMoves();

	/**
	 * Returns the tile containing the given position
	 */
	void NearestTile(entity_pos_t x, entity_pos_t z, u16& i, u16& j)
	{
		i = (u16)clamp((x / (int)TERRAIN_TILE_SIZE).ToInt_RoundToZero(), 0, m_MapSize-1);
		j = (u16)clamp((z / (int)TERRAIN_TILE_SIZE).ToInt_RoundToZero(), 0, m_MapSize-1);
	}

	/**
	 * Returns the position of the center of the given tile
	 */
	static void TileCenter(u16 i, u16 j, entity_pos_t& x, entity_pos_t& z)
	{
		x = entity_pos_t::FromInt(i*(int)TERRAIN_TILE_SIZE + (int)TERRAIN_TILE_SIZE/2);
		z = entity_pos_t::FromInt(j*(int)TERRAIN_TILE_SIZE + (int)TERRAIN_TILE_SIZE/2);
	}

	static fixed DistanceToGoal(CFixedVector2D pos, const CCmpPathfinder::Goal& goal);

	/**
	 * Regenerates the grid based on the current obstruction list, if necessary
	 */
	void UpdateGrid();

	void RenderSubmit(SceneCollector& collector);
};

#endif // INCLUDED_CCMPPATHFINDER_COMMON
