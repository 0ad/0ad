/* Copyright (C) 2019 Wildfire Games.
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
 * Common code and setup code for CCmpPathfinder.
 */

#include "precompiled.h"

#include "CCmpPathfinder_Common.h"

#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Profile.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/Scene.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpObstruction.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "simulation2/helpers/HierarchicalPathfinder.h"
#include "simulation2/helpers/LongPathfinder.h"
#include "simulation2/helpers/MapEdgeTiles.h"
#include "simulation2/helpers/Rasterize.h"
#include "simulation2/helpers/VertexPathfinder.h"
#include "simulation2/serialization/SerializeTemplates.h"

REGISTER_COMPONENT_TYPE(Pathfinder)

void CCmpPathfinder::Init(const CParamNode& UNUSED(paramNode))
{
	m_MapSize = 0;
	m_Grid = NULL;
	m_TerrainOnlyGrid = NULL;

	FlushAIPathfinderDirtinessInformation();

	m_NextAsyncTicket = 1;

	m_AtlasOverlay = NULL;

	m_VertexPathfinder = std::unique_ptr<VertexPathfinder>(new VertexPathfinder(m_MapSize, m_TerrainOnlyGrid));
	m_LongPathfinder = std::unique_ptr<LongPathfinder>(new LongPathfinder());
	m_PathfinderHier = std::unique_ptr<HierarchicalPathfinder>(new HierarchicalPathfinder());

	// Register Relax NG validator
	CXeromyces::AddValidator(g_VFS, "pathfinder", "simulation/data/pathfinder.rng");

	// Since this is used as a system component (not loaded from an entity template),
	// we can't use the real paramNode (it won't get handled properly when deserializing),
	// so load the data from a special XML file.
	CParamNode externalParamNode;
	CParamNode::LoadXML(externalParamNode, L"simulation/data/pathfinder.xml", "pathfinder");

    // Previously all move commands during a turn were
    // queued up and processed asynchronously at the start
    // of the next turn.  Now we are processing queued up
    // events several times duing the turn.  This improves
    // responsiveness and units move more smoothly especially.
    // when in formation.  There is still a call at the
    // beginning of a turn to process all outstanding moves -
    // this will handle any moves above the MaxSameTurnMoves
    // threshold.
    //
    // TODO - The moves processed at the beginning of the
    // turn do not count against the maximum moves per turn
    // currently.  The thinking is that this will eventually
    // happen in another thread.  Either way this probably
    // will require some adjustment and rethinking.
	const CParamNode pathingSettings = externalParamNode.GetChild("Pathfinder");
	m_MaxSameTurnMoves = (u16)pathingSettings.GetChild("MaxSameTurnMoves").ToInt();


	const CParamNode::ChildrenMap& passClasses = externalParamNode.GetChild("Pathfinder").GetChild("PassabilityClasses").GetChildren();
	for (CParamNode::ChildrenMap::const_iterator it = passClasses.begin(); it != passClasses.end(); ++it)
	{
		std::string name = it->first;
		ENSURE((int)m_PassClasses.size() <= PASS_CLASS_BITS);
		pass_class_t mask = PASS_CLASS_MASK_FROM_INDEX(m_PassClasses.size());
		m_PassClasses.push_back(PathfinderPassability(mask, it->second));
		m_PassClassMasks[name] = mask;
	}

	m_Workers.emplace_back(PathfinderWorker{});
}

CCmpPathfinder::~CCmpPathfinder() {};

void CCmpPathfinder::Deinit()
{
	m_Workers.clear();

	SetDebugOverlay(false); // cleans up memory
	SAFE_DELETE(m_AtlasOverlay);

	SAFE_DELETE(m_Grid);
	SAFE_DELETE(m_TerrainOnlyGrid);
}

struct SerializeLongRequest
{
	template<typename S>
	void operator()(S& serialize, const char* UNUSED(name), LongPathRequest& value)
	{
		serialize.NumberU32_Unbounded("ticket", value.ticket);
		serialize.NumberFixed_Unbounded("x0", value.x0);
		serialize.NumberFixed_Unbounded("z0", value.z0);
		SerializeGoal()(serialize, "goal", value.goal);
		serialize.NumberU16_Unbounded("pass class", value.passClass);
		serialize.NumberU32_Unbounded("notify", value.notify);
	}
};

struct SerializeShortRequest
{
	template<typename S>
	void operator()(S& serialize, const char* UNUSED(name), ShortPathRequest& value)
	{
		serialize.NumberU32_Unbounded("ticket", value.ticket);
		serialize.NumberFixed_Unbounded("x0", value.x0);
		serialize.NumberFixed_Unbounded("z0", value.z0);
		serialize.NumberFixed_Unbounded("clearance", value.clearance);
		serialize.NumberFixed_Unbounded("range", value.range);
		SerializeGoal()(serialize, "goal", value.goal);
		serialize.NumberU16_Unbounded("pass class", value.passClass);
		serialize.Bool("avoid moving units", value.avoidMovingUnits);
		serialize.NumberU32_Unbounded("group", value.group);
		serialize.NumberU32_Unbounded("notify", value.notify);
	}
};

template<typename S>
void CCmpPathfinder::SerializeCommon(S& serialize)
{
	SerializeVector<SerializeLongRequest>()(serialize, "long requests", m_LongPathRequests);
	SerializeVector<SerializeShortRequest>()(serialize, "short requests", m_ShortPathRequests);
	serialize.NumberU32_Unbounded("next ticket", m_NextAsyncTicket);
	serialize.NumberU16_Unbounded("map size", m_MapSize);
}

void CCmpPathfinder::Serialize(ISerializer& serialize)
{
	SerializeCommon(serialize);
}

void CCmpPathfinder::Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
{
	Init(paramNode);

	SerializeCommon(deserialize);
}

void CCmpPathfinder::HandleMessage(const CMessage& msg, bool UNUSED(global))
{
	switch (msg.GetType())
	{
	case MT_RenderSubmit:
	{
		const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
		RenderSubmit(msgData.collector);
		break;
	}
	case MT_TerrainChanged:
		m_TerrainDirty = true;
		MinimalTerrainUpdate();
		break;
	case MT_WaterChanged:
	case MT_ObstructionMapShapeChanged:
		m_TerrainDirty = true;
		UpdateGrid();
		break;
	case MT_Deserialized:
		UpdateGrid();
		// In case we were serialised with requests pending, we need to process them.
		if (!m_ShortPathRequests.empty() || !m_LongPathRequests.empty())
		{
			ENSURE(CmpPtr<ICmpObstructionManager>(GetSystemEntity()));
			StartProcessingMoves(false);
		}
		break;
	}
}

void CCmpPathfinder::RenderSubmit(SceneCollector& collector)
{
	m_VertexPathfinder->RenderSubmit(collector);
	m_PathfinderHier->RenderSubmit(collector);
}

void CCmpPathfinder::SetDebugPath(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass)
{
	m_LongPathfinder->SetDebugPath(*m_PathfinderHier, x0, z0, goal, passClass);
}

void CCmpPathfinder::SetDebugOverlay(bool enabled)
{
	m_VertexPathfinder->SetDebugOverlay(enabled);
	m_LongPathfinder->SetDebugOverlay(enabled);
}

void CCmpPathfinder::SetHierDebugOverlay(bool enabled)
{
	m_PathfinderHier->SetDebugOverlay(enabled, &GetSimContext());
}

void CCmpPathfinder::GetDebugData(u32& steps, double& time, Grid<u8>& grid) const
{
	m_LongPathfinder->GetDebugData(steps, time, grid);
}

void CCmpPathfinder::SetAtlasOverlay(bool enable, pass_class_t passClass)
{
	if (enable)
	{
		if (!m_AtlasOverlay)
			m_AtlasOverlay = new AtlasOverlay(this, passClass);
		m_AtlasOverlay->m_PassClass = passClass;
	}
	else
		SAFE_DELETE(m_AtlasOverlay);
}

pass_class_t CCmpPathfinder::GetPassabilityClass(const std::string& name) const
{
	std::map<std::string, pass_class_t>::const_iterator it = m_PassClassMasks.find(name);
	if (it == m_PassClassMasks.end())
	{
		LOGERROR("Invalid passability class name '%s'", name.c_str());
		return 0;
	}

	return it->second;
}

void CCmpPathfinder::GetPassabilityClasses(std::map<std::string, pass_class_t>& passClasses) const
{
	passClasses = m_PassClassMasks;
}

void CCmpPathfinder::GetPassabilityClasses(std::map<std::string, pass_class_t>& nonPathfindingPassClasses, std::map<std::string, pass_class_t>& pathfindingPassClasses) const
{
	for (const std::pair<std::string, pass_class_t>& pair : m_PassClassMasks)
	{
		if ((GetPassabilityFromMask(pair.second)->m_Obstructions == PathfinderPassability::PATHFINDING))
			pathfindingPassClasses[pair.first] = pair.second;
		else
			nonPathfindingPassClasses[pair.first] = pair.second;
	}
}

const PathfinderPassability* CCmpPathfinder::GetPassabilityFromMask(pass_class_t passClass) const
{
	for (const PathfinderPassability& passability : m_PassClasses)
	{
		if (passability.m_Mask == passClass)
			return &passability;
	}

	return NULL;
}

const Grid<NavcellData>& CCmpPathfinder::GetPassabilityGrid()
{
	if (!m_Grid)
		UpdateGrid();

	return *m_Grid;
}

/**
 * Given a grid of passable/impassable navcells (based on some passability mask),
 * computes a new grid where a navcell is impassable (per that mask) if
 * it is <=clearance navcells away from an impassable navcell in the original grid.
 * The results are ORed onto the original grid.
 *
 * This is used for adding clearance onto terrain-based navcell passability.
 *
 * TODO PATHFINDER: might be nicer to get rounded corners by measuring clearances as
 * Euclidean distances; currently it effectively does dist=max(dx,dy) instead.
 * This would only really be a problem for big clearances.
 */
static void ExpandImpassableCells(Grid<NavcellData>& grid, u16 clearance, pass_class_t mask)
{
	PROFILE3("ExpandImpassableCells");

	u16 w = grid.m_W;
	u16 h = grid.m_H;

	// First expand impassable cells horizontally into a temporary 1-bit grid
	Grid<u8> tempGrid(w, h);
	for (u16 j = 0; j < h; ++j)
	{
		// New cell (i,j) is blocked if (i',j) blocked for any i-clearance <= i' <= i+clearance

		// Count the number of blocked cells around i=0
		u16 numBlocked = 0;
		for (u16 i = 0; i <= clearance && i < w; ++i)
			if (!IS_PASSABLE(grid.get(i, j), mask))
				++numBlocked;

		for (u16 i = 0; i < w; ++i)
		{
			// Store a flag if blocked by at least one nearby cell
			if (numBlocked)
				tempGrid.set(i, j, 1);

			// Slide the numBlocked window along:
			// remove the old i-clearance value, add the new (i+1)+clearance
			// (avoiding overflowing the grid)
			if (i >= clearance && !IS_PASSABLE(grid.get(i-clearance, j), mask))
				--numBlocked;
			if (i+1+clearance < w && !IS_PASSABLE(grid.get(i+1+clearance, j), mask))
				++numBlocked;
		}
	}

	for (u16 i = 0; i < w; ++i)
	{
		// New cell (i,j) is blocked if (i,j') blocked for any j-clearance <= j' <= j+clearance
		// Count the number of blocked cells around j=0
		u16 numBlocked = 0;
		for (u16 j = 0; j <= clearance && j < h; ++j)
			if (tempGrid.get(i, j))
				++numBlocked;

		for (u16 j = 0; j < h; ++j)
		{
			// Add the mask if blocked by at least one nearby cell
			if (numBlocked)
				grid.set(i, j, grid.get(i, j) | mask);

			// Slide the numBlocked window along:
			// remove the old j-clearance value, add the new (j+1)+clearance
			// (avoiding overflowing the grid)
			if (j >= clearance && tempGrid.get(i, j-clearance))
				--numBlocked;
			if (j+1+clearance < h && tempGrid.get(i, j+1+clearance))
				++numBlocked;
		}
	}
}

Grid<u16> CCmpPathfinder::ComputeShoreGrid(bool expandOnWater)
{
	PROFILE3("ComputeShoreGrid");

	CmpPtr<ICmpWaterManager> cmpWaterManager(GetSystemEntity());

	// TODO: these bits should come from ICmpTerrain
	CTerrain& terrain = GetSimContext().GetTerrain();

	// avoid integer overflow in intermediate calculation
	const u16 shoreMax = 32767;

	// First pass - find underwater tiles
	Grid<u8> waterGrid(m_MapSize, m_MapSize);
	for (u16 j = 0; j < m_MapSize; ++j)
	{
		for (u16 i = 0; i < m_MapSize; ++i)
		{
			fixed x, z;
			Pathfinding::TileCenter(i, j, x, z);

			bool underWater = cmpWaterManager && (cmpWaterManager->GetWaterLevel(x, z) > terrain.GetExactGroundLevelFixed(x, z));
			waterGrid.set(i, j, underWater ? 1 : 0);
		}
	}

	// Second pass - find shore tiles
	Grid<u16> shoreGrid(m_MapSize, m_MapSize);
	for (u16 j = 0; j < m_MapSize; ++j)
	{
		for (u16 i = 0; i < m_MapSize; ++i)
		{
			// Find a land tile
			if (!waterGrid.get(i, j))
			{
				// If it's bordered by water, it's a shore tile
				if ((i > 0 && waterGrid.get(i-1, j)) || (i > 0 && j < m_MapSize-1 && waterGrid.get(i-1, j+1)) || (i > 0 && j > 0 && waterGrid.get(i-1, j-1))
					|| (i < m_MapSize-1 && waterGrid.get(i+1, j)) || (i < m_MapSize-1 && j < m_MapSize-1 && waterGrid.get(i+1, j+1)) || (i < m_MapSize-1 && j > 0 && waterGrid.get(i+1, j-1))
					|| (j > 0 && waterGrid.get(i, j-1)) || (j < m_MapSize-1 && waterGrid.get(i, j+1))
					)
					shoreGrid.set(i, j, 0);
				else
					shoreGrid.set(i, j, shoreMax);
			}
			// If we want to expand on water, we want water tiles not to be shore tiles
			else if (expandOnWater)
				shoreGrid.set(i, j, shoreMax);
		}
	}

	// Expand influences on land to find shore distance
	for (u16 y = 0; y < m_MapSize; ++y)
	{
		u16 min = shoreMax;
		for (u16 x = 0; x < m_MapSize; ++x)
		{
			if (!waterGrid.get(x, y) || expandOnWater)
			{
				u16 g = shoreGrid.get(x, y);
				if (g > min)
					shoreGrid.set(x, y, min);
				else if (g < min)
					min = g;

				++min;
			}
		}
		for (u16 x = m_MapSize; x > 0; --x)
		{
			if (!waterGrid.get(x-1, y) || expandOnWater)
			{
				u16 g = shoreGrid.get(x-1, y);
				if (g > min)
					shoreGrid.set(x-1, y, min);
				else if (g < min)
					min = g;

				++min;
			}
		}
	}
	for (u16 x = 0; x < m_MapSize; ++x)
	{
		u16 min = shoreMax;
		for (u16 y = 0; y < m_MapSize; ++y)
		{
			if (!waterGrid.get(x, y) || expandOnWater)
			{
				u16 g = shoreGrid.get(x, y);
				if (g > min)
					shoreGrid.set(x, y, min);
				else if (g < min)
					min = g;

				++min;
			}
		}
		for (u16 y = m_MapSize; y > 0; --y)
		{
			if (!waterGrid.get(x, y-1) || expandOnWater)
			{
				u16 g = shoreGrid.get(x, y-1);
				if (g > min)
					shoreGrid.set(x, y-1, min);
				else if (g < min)
					min = g;

				++min;
			}
		}
	}

	return shoreGrid;
}

void CCmpPathfinder::UpdateGrid()
{
	PROFILE3("UpdateGrid");

	CmpPtr<ICmpTerrain> cmpTerrain(GetSimContext(), SYSTEM_ENTITY);
	if (!cmpTerrain)
		return; // error

	u16 terrainSize = cmpTerrain->GetTilesPerSide();
	if (terrainSize == 0)
		return;

	// If the terrain was resized then delete the old grid data
	if (m_Grid && m_MapSize != terrainSize)
	{
		SAFE_DELETE(m_Grid);
		SAFE_DELETE(m_TerrainOnlyGrid);
	}

	// Initialise the terrain data when first needed
	if (!m_Grid)
	{
		m_MapSize = terrainSize;
		m_Grid = new Grid<NavcellData>(m_MapSize * Pathfinding::NAVCELLS_PER_TILE, m_MapSize * Pathfinding::NAVCELLS_PER_TILE);
		SAFE_DELETE(m_TerrainOnlyGrid);
		m_TerrainOnlyGrid = new Grid<NavcellData>(m_MapSize * Pathfinding::NAVCELLS_PER_TILE, m_MapSize * Pathfinding::NAVCELLS_PER_TILE);

		m_DirtinessInformation = { true, true, Grid<u8>(m_MapSize * Pathfinding::NAVCELLS_PER_TILE, m_MapSize * Pathfinding::NAVCELLS_PER_TILE) };
		m_AIPathfinderDirtinessInformation = m_DirtinessInformation;

		m_TerrainDirty = true;
	}

	// The grid should be properly initialized and clean. Checking the latter is expensive so do it only for debugging.
#ifdef NDEBUG
	ENSURE(m_DirtinessInformation.dirtinessGrid.compare_sizes(m_Grid));
#else
	ENSURE(m_DirtinessInformation.dirtinessGrid == Grid<u8>(m_MapSize * Pathfinding::NAVCELLS_PER_TILE, m_MapSize * Pathfinding::NAVCELLS_PER_TILE));
#endif

	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
	cmpObstructionManager->UpdateInformations(m_DirtinessInformation);

	if (!m_DirtinessInformation.dirty && !m_TerrainDirty)
		return;

	// If the terrain has changed, recompute m_Grid
	// Else, use data from m_TerrainOnlyGrid and add obstructions
	if (m_TerrainDirty)
	{
		TerrainUpdateHelper();

		*m_Grid = *m_TerrainOnlyGrid;

		m_TerrainDirty = false;
		m_DirtinessInformation.globallyDirty = true;
	}
	else if (m_DirtinessInformation.globallyDirty)
	{
		ENSURE(m_Grid->compare_sizes(m_TerrainOnlyGrid));
		memcpy(m_Grid->m_Data, m_TerrainOnlyGrid->m_Data, (m_Grid->m_W)*(m_Grid->m_H)*sizeof(NavcellData));
	}
	else
	{
		ENSURE(m_Grid->compare_sizes(m_TerrainOnlyGrid));

		for (u16 j = 0; j < m_DirtinessInformation.dirtinessGrid.m_H; ++j)
			for (u16 i = 0; i < m_DirtinessInformation.dirtinessGrid.m_W; ++i)
				if (m_DirtinessInformation.dirtinessGrid.get(i, j) == 1)
					m_Grid->set(i, j, m_TerrainOnlyGrid->get(i, j));
	}

	// Add obstructions onto the grid
	cmpObstructionManager->Rasterize(*m_Grid, m_PassClasses, m_DirtinessInformation.globallyDirty);

	// Update the long-range and hierarchical pathfinders.
	if (m_DirtinessInformation.globallyDirty)
	{
		std::map<std::string, pass_class_t> nonPathfindingPassClasses, pathfindingPassClasses;
		GetPassabilityClasses(nonPathfindingPassClasses, pathfindingPassClasses);
		m_LongPathfinder->Reload(m_Grid);
		m_PathfinderHier->Recompute(m_Grid, nonPathfindingPassClasses, pathfindingPassClasses);
	}
	else
	{
		m_LongPathfinder->Update(m_Grid);
		m_PathfinderHier->Update(m_Grid, m_DirtinessInformation.dirtinessGrid);
	}

	// Remember the necessary updates that the AI pathfinder will have to perform as well
	m_AIPathfinderDirtinessInformation.MergeAndClear(m_DirtinessInformation);
}

void CCmpPathfinder::MinimalTerrainUpdate()
{
	TerrainUpdateHelper(false);
}

void CCmpPathfinder::TerrainUpdateHelper(bool expandPassability/* = true */)
{
	PROFILE3("TerrainUpdateHelper");

	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
	CmpPtr<ICmpWaterManager> cmpWaterManager(GetSimContext(), SYSTEM_ENTITY);
	CmpPtr<ICmpTerrain> cmpTerrain(GetSimContext(), SYSTEM_ENTITY);
	CTerrain& terrain = GetSimContext().GetTerrain();

	if (!cmpTerrain || !cmpObstructionManager)
		return;

	u16 terrainSize = cmpTerrain->GetTilesPerSide();
	if (terrainSize == 0)
		return;

	if (!m_TerrainOnlyGrid || m_MapSize != terrainSize)
	{
		m_MapSize = terrainSize;

		SAFE_DELETE(m_TerrainOnlyGrid);
		m_TerrainOnlyGrid = new Grid<NavcellData>(m_MapSize * Pathfinding::NAVCELLS_PER_TILE, m_MapSize * Pathfinding::NAVCELLS_PER_TILE);

		// If this update comes from a map resizing, we must reinitialize the other grids as well
		if (!m_TerrainOnlyGrid->compare_sizes(m_Grid))
		{
			SAFE_DELETE(m_Grid);
			m_Grid = new Grid<NavcellData>(m_MapSize * Pathfinding::NAVCELLS_PER_TILE, m_MapSize * Pathfinding::NAVCELLS_PER_TILE);

			m_DirtinessInformation = { true, true, Grid<u8>(m_MapSize * Pathfinding::NAVCELLS_PER_TILE, m_MapSize * Pathfinding::NAVCELLS_PER_TILE) };
			m_AIPathfinderDirtinessInformation = m_DirtinessInformation;
		}
	}

	Grid<u16> shoreGrid = ComputeShoreGrid();

	// Compute initial terrain-dependent passability
	for (int j = 0; j < m_MapSize * Pathfinding::NAVCELLS_PER_TILE; ++j)
	{
		for (int i = 0; i < m_MapSize * Pathfinding::NAVCELLS_PER_TILE; ++i)
		{
			// World-space coordinates for this navcell
			fixed x, z;
			Pathfinding::NavcellCenter(i, j, x, z);

			// Terrain-tile coordinates for this navcell
			int itile = i / Pathfinding::NAVCELLS_PER_TILE;
			int jtile = j / Pathfinding::NAVCELLS_PER_TILE;

			// Gather all the data potentially needed to determine passability:

			fixed height = terrain.GetExactGroundLevelFixed(x, z);

			fixed water;
			if (cmpWaterManager)
				water = cmpWaterManager->GetWaterLevel(x, z);

			fixed depth = water - height;

			// Exact slopes give kind of weird output, so just use rough tile-based slopes
			fixed slope = terrain.GetSlopeFixed(itile, jtile);

			// Get world-space coordinates from shoreGrid (which uses terrain tiles)
			fixed shoredist = fixed::FromInt(shoreGrid.get(itile, jtile)).MultiplyClamp(TERRAIN_TILE_SIZE);

			// Compute the passability for every class for this cell
			NavcellData t = 0;
			for (PathfinderPassability& passability : m_PassClasses)
				if (!passability.IsPassable(depth, slope, shoredist))
					t |= passability.m_Mask;

			m_TerrainOnlyGrid->set(i, j, t);
		}
	}

	// Compute off-world passability
	const int edgeSize = MAP_EDGE_TILES * Pathfinding::NAVCELLS_PER_TILE;

	NavcellData edgeMask = 0;
	for (PathfinderPassability& passability : m_PassClasses)
		edgeMask |= passability.m_Mask;

	int w = m_TerrainOnlyGrid->m_W;
	int h = m_TerrainOnlyGrid->m_H;

	if (cmpObstructionManager->GetPassabilityCircular())
	{
		for (int j = 0; j < h; ++j)
		{
			for (int i = 0; i < w; ++i)
			{
				// Based on CCmpRangeManager::LosIsOffWorld
				// but tweaked since it's tile-based instead.
				// (We double all the values so we can handle half-tile coordinates.)
				// This needs to be slightly tighter than the LOS circle,
				// else units might get themselves lost in the SoD around the edge.

				int dist2 = (i*2 + 1 - w)*(i*2 + 1 - w)
					+ (j*2 + 1 - h)*(j*2 + 1 - h);

				if (dist2 >= (w - 2*edgeSize) * (h - 2*edgeSize))
					m_TerrainOnlyGrid->set(i, j, m_TerrainOnlyGrid->get(i, j) | edgeMask);
			}
		}
	}
	else
	{
		for (u16 j = 0; j < h; ++j)
			for (u16 i = 0; i < edgeSize; ++i)
				m_TerrainOnlyGrid->set(i, j, m_TerrainOnlyGrid->get(i, j) | edgeMask);
		for (u16 j = 0; j < h; ++j)
			for (u16 i = w-edgeSize+1; i < w; ++i)
				m_TerrainOnlyGrid->set(i, j, m_TerrainOnlyGrid->get(i, j) | edgeMask);
		for (u16 j = 0; j < edgeSize; ++j)
			for (u16 i = edgeSize; i < w-edgeSize+1; ++i)
				m_TerrainOnlyGrid->set(i, j, m_TerrainOnlyGrid->get(i, j) | edgeMask);
		for (u16 j = h-edgeSize+1; j < h; ++j)
			for (u16 i = edgeSize; i < w-edgeSize+1; ++i)
				m_TerrainOnlyGrid->set(i, j, m_TerrainOnlyGrid->get(i, j) | edgeMask);
	}

	if (!expandPassability)
		return;

	// Expand the impassability grid, for any class with non-zero clearance,
	// so that we can stop units getting too close to impassable navcells.
	// Note: It's not possible to perform this expansion once for all passabilities
	// with the same clearance, because the impassable cells are not necessarily the
	// same for all these passabilities.
	for (PathfinderPassability& passability : m_PassClasses)
	{
		if (passability.m_Clearance == fixed::Zero())
			continue;

		int clearance = (passability.m_Clearance / Pathfinding::NAVCELL_SIZE).ToInt_RoundToInfinity();
		ExpandImpassableCells(*m_TerrainOnlyGrid, clearance, passability.m_Mask);
	}
}

//////////////////////////////////////////////////////////

// Async pathfinder workers

CCmpPathfinder::PathfinderWorker::PathfinderWorker() {}

template<typename T>
void CCmpPathfinder::PathfinderWorker::PushRequests(std::vector<T>&, ssize_t)
{
	static_assert(sizeof(T) == 0, "Only specializations can be used");
}

template<> void CCmpPathfinder::PathfinderWorker::PushRequests(std::vector<LongPathRequest>& from, ssize_t amount)
{
	m_LongRequests.insert(m_LongRequests.end(), std::make_move_iterator(from.end() - amount), std::make_move_iterator(from.end()));
}

template<> void CCmpPathfinder::PathfinderWorker::PushRequests(std::vector<ShortPathRequest>& from, ssize_t amount)
{
	m_ShortRequests.insert(m_ShortRequests.end(), std::make_move_iterator(from.end() - amount), std::make_move_iterator(from.end()));
}

void CCmpPathfinder::PathfinderWorker::Work(const CCmpPathfinder& pathfinder)
{
	while (!m_LongRequests.empty())
	{
		const LongPathRequest& req = m_LongRequests.back();
		WaypointPath path;
		pathfinder.m_LongPathfinder->ComputePath(*pathfinder.m_PathfinderHier, req.x0, req.z0, req.goal, req.passClass, path);
		m_Results.emplace_back(req.ticket, req.notify, path);

		m_LongRequests.pop_back();
	}

	while (!m_ShortRequests.empty())
	{
		const ShortPathRequest& req = m_ShortRequests.back();
		WaypointPath path = pathfinder.m_VertexPathfinder->ComputeShortPath(req, CmpPtr<ICmpObstructionManager>(pathfinder.GetSystemEntity()));
		m_Results.emplace_back(req.ticket, req.notify, path);

		m_ShortRequests.pop_back();
	}
}

u32 CCmpPathfinder::ComputePathAsync(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass, entity_id_t notify)
{
	LongPathRequest req = { m_NextAsyncTicket++, x0, z0, goal, passClass, notify };
	m_LongPathRequests.push_back(req);
	return req.ticket;
}

u32 CCmpPathfinder::ComputeShortPathAsync(entity_pos_t x0, entity_pos_t z0, entity_pos_t clearance, entity_pos_t range,
                                          const PathGoal& goal, pass_class_t passClass, bool avoidMovingUnits,
                                          entity_id_t group, entity_id_t notify)
{
	ShortPathRequest req = { m_NextAsyncTicket++, x0, z0, clearance, range, goal, passClass, avoidMovingUnits, group, notify };
	m_ShortPathRequests.push_back(req);
	return req.ticket;
}

void CCmpPathfinder::ComputePathImmediate(entity_pos_t x0, entity_pos_t z0, const PathGoal& goal, pass_class_t passClass, WaypointPath& ret) const
{
	m_LongPathfinder->ComputePath(*m_PathfinderHier, x0, z0, goal, passClass, ret);
}

WaypointPath CCmpPathfinder::ComputeShortPathImmediate(const ShortPathRequest& request) const
{
	return m_VertexPathfinder->ComputeShortPath(request, CmpPtr<ICmpObstructionManager>(GetSystemEntity()));
}

void CCmpPathfinder::FetchAsyncResultsAndSendMessages()
{
	PROFILE2("FetchAsyncResults");

	// We may now clear existing requests.
	m_ShortPathRequests.clear();
	m_LongPathRequests.clear();

	// WARNING: the order in which moves are pulled must be consistent when using 1 or n workers.
	// We fetch in the same order we inserted in, but we push moves backwards, so this works.
	std::vector<PathResult> results;
	for (PathfinderWorker& worker : m_Workers)
	{
		results.insert(results.end(), std::make_move_iterator(worker.m_Results.begin()), std::make_move_iterator(worker.m_Results.end()));
		worker.m_Results.clear();
	}

	{
		PROFILE2("PostMessages");
		for (PathResult& path : results)
		{
			CMessagePathResult msg(path.ticket, path.path);
			GetSimContext().GetComponentManager().PostMessage(path.notify, msg);
		}
	}
}

void CCmpPathfinder::StartProcessingMoves(bool useMax)
{
	std::vector<LongPathRequest> longRequests = GetMovesToProcess(m_LongPathRequests, useMax, m_MaxSameTurnMoves);
	std::vector<ShortPathRequest> shortRequests = GetMovesToProcess(m_ShortPathRequests, useMax, m_MaxSameTurnMoves - longRequests.size());

	PushRequestsToWorkers(longRequests);
	PushRequestsToWorkers(shortRequests);

	for (PathfinderWorker& worker : m_Workers)
		worker.Work(*this);
}

template <typename T>
std::vector<T> CCmpPathfinder::GetMovesToProcess(std::vector<T>& requests, bool useMax, size_t maxMoves)
{
	// Keep the original requests in which we need to serialize.
	std::vector<T> copiedRequests;
	if (useMax)
	{
		size_t amount = std::min(requests.size(), maxMoves);
		if (amount > 0)
			copiedRequests.insert(copiedRequests.begin(), requests.end() - amount, requests.end());
	}
	else
		copiedRequests = requests;

	return copiedRequests;
}

template <typename T>
void CCmpPathfinder::PushRequestsToWorkers(std::vector<T>& from)
{
	if (from.empty())
		return;

	// Trivial load-balancing, / rounds towards zero so add 1 to ensure we do push all requests.
	size_t amount = from.size() / m_Workers.size() + 1;

	// WARNING: the order in which moves are pushed must be consistent when using 1 or n workers.
	// In this instance, work is distributed in a strict LIFO order, effectively reversing tickets.
	for (PathfinderWorker& worker : m_Workers)
	{
		amount = std::min(amount, from.size()); // Since we are rounding up before, ensure we aren't pushing beyond the end.
		worker.PushRequests(from, amount);
		from.erase(from.end() - amount, from.end());
	}
}

//////////////////////////////////////////////////////////

bool CCmpPathfinder::CheckMovement(const IObstructionTestFilter& filter,
	entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1, entity_pos_t r,
	pass_class_t passClass) const
{
	PROFILE2_IFSPIKE("Check Movement", 0.001);

	// Test against obstructions first. filter may discard pathfinding-blocking obstructions.
	// Use more permissive version of TestLine to allow unit-unit collisions to overlap slightly.
	// This makes movement smoother and more natural for units, overall.
	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
	if (!cmpObstructionManager || cmpObstructionManager->TestLine(filter, x0, z0, x1, z1, r, true))
		return false;

	// Then test against the terrain grid. This should not be necessary
	// But in case we allow terrain to change it will become so.
	return Pathfinding::CheckLineMovement(x0, z0, x1, z1, passClass, *m_TerrainOnlyGrid);
}

ICmpObstruction::EFoundationCheck CCmpPathfinder::CheckUnitPlacement(const IObstructionTestFilter& filter,
	entity_pos_t x, entity_pos_t z, entity_pos_t r,	pass_class_t passClass, bool UNUSED(onlyCenterPoint)) const
{
	// Check unit obstruction
	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
	if (!cmpObstructionManager)
		return ICmpObstruction::FOUNDATION_CHECK_FAIL_ERROR;

	if (cmpObstructionManager->TestUnitShape(filter, x, z, r, NULL))
		return ICmpObstruction::FOUNDATION_CHECK_FAIL_OBSTRUCTS_FOUNDATION;

	// Test against terrain and static obstructions:

	u16 i, j;
	Pathfinding::NearestNavcell(x, z, i, j, m_MapSize*Pathfinding::NAVCELLS_PER_TILE, m_MapSize*Pathfinding::NAVCELLS_PER_TILE);
	if (!IS_PASSABLE(m_Grid->get(i, j), passClass))
		return ICmpObstruction::FOUNDATION_CHECK_FAIL_TERRAIN_CLASS;

	// (Static obstructions will be redundantly tested against in both the
	// obstruction-shape test and navcell-passability test, which is slightly
	// inefficient but shouldn't affect behaviour)

	return ICmpObstruction::FOUNDATION_CHECK_SUCCESS;
}

ICmpObstruction::EFoundationCheck CCmpPathfinder::CheckBuildingPlacement(const IObstructionTestFilter& filter,
	entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w,
	entity_pos_t h, entity_id_t id, pass_class_t passClass) const
{
	return CCmpPathfinder::CheckBuildingPlacement(filter, x, z, a, w, h, id, passClass, false);
}


ICmpObstruction::EFoundationCheck CCmpPathfinder::CheckBuildingPlacement(const IObstructionTestFilter& filter,
	entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w,
	entity_pos_t h, entity_id_t id, pass_class_t passClass, bool UNUSED(onlyCenterPoint)) const
{
	// Check unit obstruction
	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
	if (!cmpObstructionManager)
		return ICmpObstruction::FOUNDATION_CHECK_FAIL_ERROR;

	if (cmpObstructionManager->TestStaticShape(filter, x, z, a, w, h, NULL))
		return ICmpObstruction::FOUNDATION_CHECK_FAIL_OBSTRUCTS_FOUNDATION;

	// Test against terrain:

	ICmpObstructionManager::ObstructionSquare square;
	CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), id);
	if (!cmpObstruction || !cmpObstruction->GetObstructionSquare(square))
		return ICmpObstruction::FOUNDATION_CHECK_FAIL_NO_OBSTRUCTION;

	entity_pos_t expand;
	const PathfinderPassability* passability = GetPassabilityFromMask(passClass);
	if (passability)
		expand = passability->m_Clearance;

	SimRasterize::Spans spans;
	SimRasterize::RasterizeRectWithClearance(spans, square, expand, Pathfinding::NAVCELL_SIZE);
	for (const SimRasterize::Span& span : spans)
	{
		i16 i0 = span.i0;
		i16 i1 = span.i1;
		i16 j = span.j;

		// Fail if any span extends outside the grid
		if (i0 < 0 || i1 > m_TerrainOnlyGrid->m_W || j < 0 || j > m_TerrainOnlyGrid->m_H)
			return ICmpObstruction::FOUNDATION_CHECK_FAIL_TERRAIN_CLASS;

		// Fail if any span includes an impassable tile
		for (i16 i = i0; i < i1; ++i)
			if (!IS_PASSABLE(m_TerrainOnlyGrid->get(i, j), passClass))
				return ICmpObstruction::FOUNDATION_CHECK_FAIL_TERRAIN_CLASS;
	}

	return ICmpObstruction::FOUNDATION_CHECK_SUCCESS;
}
