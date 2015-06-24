/* Copyright (C) 2015 Wildfire Games.
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
 * Declares CCmpPathfinder. Its implementation is mainly done in CCmpPathfinder.cpp,
 * but the short-range (vertex) pathfinding is done in CCmpPathfinder_Vertex.cpp.
 * This file provides common code needed for both files.
 *
 * The long-range pathfinding is done by a LongPathfinder object.
 */

#include "simulation2/system/Component.h"

#include "ICmpPathfinder.h"

#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/helpers/Geometry.h"
#include "simulation2/helpers/LongPathfinder.h"

class SceneCollector;

#ifdef NDEBUG
#define PATHFIND_DEBUG 0
#else
#define PATHFIND_DEBUG 1
#endif


struct AsyncLongPathRequest
{
	u32 ticket;
	entity_pos_t x0;
	entity_pos_t z0;
	PathGoal goal;
	pass_class_t passClass;
	entity_id_t notify;
};

struct AsyncShortPathRequest
{
	u32 ticket;
	entity_pos_t x0;
	entity_pos_t z0;
	entity_pos_t r;
	entity_pos_t range;
	PathGoal goal;
	pass_class_t passClass;
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

	// Dynamic state:

	std::vector<AsyncLongPathRequest> m_AsyncLongPathRequests;
	std::vector<AsyncShortPathRequest> m_AsyncShortPathRequests;
	u32 m_NextAsyncTicket; // unique IDs for asynchronous path requests
	u16 m_SameTurnMovesCount; // current number of same turn moves we have processed this turn

	// Lazily-constructed dynamic state (not serialized):

	u16 m_MapSize; // tiles per side
	Grid<NavcellData>* m_Grid; // terrain/passability information
	Grid<NavcellData>* m_TerrainOnlyGrid; // same as m_Grid, but only with terrain, to avoid some recomputations

	// Update data, used for clever updates and then stored for the AI manager
	GridUpdateInformation m_ObstructionsDirty;
	bool m_TerrainDirty;
	// When other components request the passability grid and trigger an update, 
	// the following regular update should not clean the dirtiness state.
	bool m_PreserveUpdateInformations;

	// Interface to the long-range pathfinder.
	LongPathfinder m_LongPathfinder;

	// For responsiveness we will process some moves in the same turn they were generated in
	
	u16 m_MaxSameTurnMoves; // max number of moves that can be created and processed in the same turn

	bool m_DebugOverlay;
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

	virtual std::map<std::string, pass_class_t> GetPathfindingPassabilityClasses();

	const PathfinderPassability* GetPassabilityFromMask(pass_class_t passClass) const;

	virtual entity_pos_t GetClearance(pass_class_t passClass) const
	{
		const PathfinderPassability* passability = GetPassabilityFromMask(passClass);
		if (!passability)
			return fixed::Zero();

		return passability->m_Clearance;
	}

	virtual entity_pos_t GetMaximumClearance() const
	{
		entity_pos_t max = fixed::Zero();

		for (const PathfinderPassability& passability : m_PassClasses)
			if (passability.m_Clearance > max)
				max = passability.m_Clearance;

		return max;
	}

	virtual const Grid<u16>& GetPassabilityGrid();

	virtual const GridUpdateInformation& GetDirtinessData() const;

	virtual Grid<u16> ComputeShoreGrid(bool expandOnWater = false);

	virtual void ComputePath(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass, WaypointPath& ret)
	{
		m_LongPathfinder.ComputePath(x0, z0, goal, passClass, ret);
	}

	virtual u32 ComputePathAsync(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass, entity_id_t notify);

	virtual void ComputeShortPath(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t r, entity_pos_t range, const PathGoal& goal, pass_class_t passClass, WaypointPath& ret);

	virtual u32 ComputeShortPathAsync(entity_pos_t x0, entity_pos_t z0, entity_pos_t r, entity_pos_t range, const PathGoal& goal, pass_class_t passClass, bool avoidMovingUnits, entity_id_t controller, entity_id_t notify);

	virtual void SetDebugPath(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass)
	{
		m_LongPathfinder.SetDebugPath(x0, z0, goal, passClass);
	}

	virtual void SetDebugOverlay(bool enabled)
	{
		m_DebugOverlay = enabled;
		m_LongPathfinder.SetDebugOverlay(enabled);
	}

	virtual void SetHierDebugOverlay(bool enabled)
	{
		m_LongPathfinder.SetHierDebugOverlay(enabled, &GetSimContext());
	}

	virtual void GetDebugData(u32& steps, double& time, Grid<u8>& grid)
	{
		m_LongPathfinder.GetDebugData(steps, time, grid);
	}

	virtual bool CheckMovement(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r, pass_class_t passClass);

	virtual ICmpObstruction::EFoundationCheck CheckUnitPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t r, pass_class_t passClass, bool onlyCenterPoint);

	virtual ICmpObstruction::EFoundationCheck CheckBuildingPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h, entity_id_t id, pass_class_t passClass);

	virtual ICmpObstruction::EFoundationCheck CheckBuildingPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h, entity_id_t id, pass_class_t passClass, bool onlyCenterPoint);

	virtual void FinishAsyncRequests();

	void ProcessLongRequests(const std::vector<AsyncLongPathRequest>& longRequests);
	
	void ProcessShortRequests(const std::vector<AsyncShortPathRequest>& shortRequests);

	virtual void ProcessSameTurnMoves();

	/**
	 * Regenerates the grid based on the current obstruction list, if necessary
	 */
	virtual void UpdateGrid();

	void ComputeTerrainPassabilityGrid(const Grid<u16>& shoreGrid);

	void RenderSubmit(SceneCollector& collector);
};

#endif // INCLUDED_CCMPPATHFINDER_COMMON
