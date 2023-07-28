/* Copyright (C) 2022 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
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
#include "ps/TaskManager.h"
#include "renderer/TerrainOverlay.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/helpers/Grid.h"

#include <vector>

class HierarchicalPathfinder;
class LongPathfinder;
class VertexPathfinder;

class SceneCollector;
class AtlasOverlay;

#ifdef NDEBUG
#define PATHFIND_DEBUG 0
#else
#define PATHFIND_DEBUG 1
#endif

/**
 * Implementation of ICmpPathfinder
 */
class CCmpPathfinder final : public ICmpPathfinder
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Deserialized);
		componentManager.SubscribeToMessageType(MT_RenderSubmit); // for debug overlays
		componentManager.SubscribeToMessageType(MT_TerrainChanged);
		componentManager.SubscribeToMessageType(MT_WaterChanged);
		componentManager.SubscribeToMessageType(MT_ObstructionMapShapeChanged);
	}

	~CCmpPathfinder();

	DEFAULT_COMPONENT_ALLOCATOR(Pathfinder)

	// Template state:

	std::map<std::string, pass_class_t> m_PassClassMasks;
	std::vector<PathfinderPassability> m_PassClasses;
	u16 m_MaxSameTurnMoves; // Compute only this many paths when useMax is true in StartProcessingMoves.

	// Dynamic state:

	// Lazily-constructed dynamic state (not serialized):

	u16 m_GridSize; // Navcells per side of the map.
	Grid<NavcellData>* m_Grid; // terrain/passability information
	Grid<NavcellData>* m_TerrainOnlyGrid; // same as m_Grid, but only with terrain, to avoid some recomputations

	// Keep clever updates in memory to avoid memory fragmentation from the grid.
	// This should be used only in UpdateGrid(), there is no guarantee the data is properly initialized anywhere else.
	GridUpdateInformation m_DirtinessInformation;
	// The data from clever updates is stored for the AI manager
	GridUpdateInformation m_AIPathfinderDirtinessInformation;
	bool m_TerrainDirty;

	std::vector<VertexPathfinder> m_VertexPathfinders;
	std::unique_ptr<HierarchicalPathfinder> m_PathfinderHier;
	std::unique_ptr<LongPathfinder> m_LongPathfinder;

	// One per live asynchronous path computing task.
	std::vector<Future<void>> m_Futures;

	template<typename T>
	class PathRequests {
	public:
		std::vector<T> m_Requests;
		std::vector<PathResult> m_Results;
		// This is the array index of the next path to compute.
		std::atomic<size_t> m_NextPathToCompute = 0;
		// This is false until all scheduled paths have been computed.
		std::atomic<bool> m_ComputeDone = true;

		void ClearComputed()
		{
			if (m_Results.size() == m_Requests.size())
				m_Requests.clear();
			else
				m_Requests.erase(m_Requests.end() - m_Results.size(), m_Requests.end());
			m_Results.clear();
		}

		/**
		 * @param max - if non-zero, how many paths to process.
		 */
		void PrepareForComputation(u16 max)
		{
			size_t n = m_Requests.size();
			if (max && n > max)
				n = max;
			m_NextPathToCompute = 0;
			m_Results.resize(n);
			m_ComputeDone = n == 0;
		}

		template<typename U>
		void Compute(const CCmpPathfinder& cmpPathfinder, const U& pathfinder);
	};
	PathRequests<LongPathRequest> m_LongPathRequests;
	PathRequests<ShortPathRequest> m_ShortPathRequests;

	u32 m_NextAsyncTicket; // Unique IDs for asynchronous path requests.

	AtlasOverlay* m_AtlasOverlay;

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	void Init(const CParamNode& paramNode) override;

	void Deinit() override;

	template<typename S>
	void SerializeCommon(S& serialize);

	void Serialize(ISerializer& serialize) override;

	void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize) override;

	void HandleMessage(const CMessage& msg, bool global) override;

	pass_class_t GetPassabilityClass(const std::string& name) const override;

	void GetPassabilityClasses(std::map<std::string, pass_class_t>& passClasses) const override;
	void GetPassabilityClasses(
		std::map<std::string, pass_class_t>& nonPathfindingPassClasses,
		std::map<std::string, pass_class_t>& pathfindingPassClasses) const override;

	const PathfinderPassability* GetPassabilityFromMask(pass_class_t passClass) const;

	entity_pos_t GetClearance(pass_class_t passClass) const override
	{
		const PathfinderPassability* passability = GetPassabilityFromMask(passClass);
		if (!passability)
			return fixed::Zero();

		return passability->m_Clearance;
	}

	entity_pos_t GetMaximumClearance() const override
	{
		entity_pos_t max = fixed::Zero();

		for (const PathfinderPassability& passability : m_PassClasses)
			if (passability.m_Clearance > max)
				max = passability.m_Clearance;

		return max + Pathfinding::CLEARANCE_EXTENSION_RADIUS;
	}

	const Grid<NavcellData>& GetPassabilityGrid() override;

	const GridUpdateInformation& GetAIPathfinderDirtinessInformation() const override
	{
		return m_AIPathfinderDirtinessInformation;
	}

	void FlushAIPathfinderDirtinessInformation() override
	{
		m_AIPathfinderDirtinessInformation.Clean();
	}

	Grid<u16> ComputeShoreGrid(bool expandOnWater = false) override;

	void ComputePathImmediate(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass, WaypointPath& ret) const override;

	u32 ComputePathAsync(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass, entity_id_t notify) override;

	WaypointPath ComputeShortPathImmediate(const ShortPathRequest& request) const override;

	u32 ComputeShortPathAsync(entity_pos_t x0, entity_pos_t z0, entity_pos_t clearance, entity_pos_t range, const PathGoal& goal, pass_class_t passClass, bool avoidMovingUnits, entity_id_t controller, entity_id_t notify) override;

	bool IsGoalReachable(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass) override;

	void SetDebugPath(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass) override;

	void SetDebugOverlay(bool enabled) override;

	void SetHierDebugOverlay(bool enabled) override;

	void GetDebugData(u32& steps, double& time, Grid<u8>& grid) const override;

	void SetAtlasOverlay(bool enable, pass_class_t passClass = 0) override;

	bool CheckMovement(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r, pass_class_t passClass) const override;

	ICmpObstruction::EFoundationCheck CheckUnitPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t r, pass_class_t passClass, bool onlyCenterPoint) const override;

	ICmpObstruction::EFoundationCheck CheckBuildingPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h, entity_id_t id, pass_class_t passClass) const override;

	ICmpObstruction::EFoundationCheck CheckBuildingPlacement(const IObstructionTestFilter& filter, entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w, entity_pos_t h, entity_id_t id, pass_class_t passClass, bool onlyCenterPoint) const override;

	void SendRequestedPaths() override;

	void StartProcessingMoves(bool useMax) override;

	template <typename T>
	std::vector<T> GetMovesToProcess(std::vector<T>& requests, bool useMax = false, size_t maxMoves = 0);

	template <typename T>
	void PushRequestsToWorkers(std::vector<T>& from);

	/**
	 * Regenerates the grid based on the current obstruction list, if necessary
	 */
	void UpdateGrid() override;

	/**
	 * Updates the terrain-only grid without updating the dirtiness informations.
	 * Useful for fast passability updates in Atlas.
	 */
	void MinimalTerrainUpdate(int itile0, int jtile0, int itile1, int jtile1);

	/**
	 * Regenerates the terrain-only grid.
	 * Atlas doesn't need to have passability cells expanded.
	 */
	void TerrainUpdateHelper(bool expandPassability = true, int itile0 = -1, int jtile0 = -1, int itile1 = -1, int jtile1 = -1);

	void RenderSubmit(SceneCollector& collector);
};

class AtlasOverlay : public TerrainTextureOverlay
{
public:
	const CCmpPathfinder* m_Pathfinder;
	pass_class_t m_PassClass;

	AtlasOverlay(const CCmpPathfinder* pathfinder, pass_class_t passClass) :
		TerrainTextureOverlay(Pathfinding::NAVCELLS_PER_TERRAIN_TILE), m_Pathfinder(pathfinder), m_PassClass(passClass)
	{
	}

	void BuildTextureRGBA(u8* data, size_t w, size_t h) override
	{
		// Render navcell passability, based on the terrain-only grid
		u8* p = data;
		for (size_t j = 0; j < h; ++j)
		{
			for (size_t i = 0; i < w; ++i)
			{
				SColor4ub color(0, 0, 0, 0);
				if (!IS_PASSABLE(m_Pathfinder->m_TerrainOnlyGrid->get((int)i, (int)j), m_PassClass))
					color = SColor4ub(255, 0, 0, 127);

				*p++ = color.R;
				*p++ = color.G;
				*p++ = color.B;
				*p++ = color.A;
			}
		}
	}
};

#endif // INCLUDED_CCMPPATHFINDER_COMMON
