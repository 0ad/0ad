/* Copyright (C) 2011 Wildfire Games.
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
#include "ICmpTerritoryManager.h"

#include "graphics/Terrain.h"
#include "maths/MathUtil.h"
#include "ps/Overlay.h"
#include "renderer/TerrainOverlay.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpObstruction.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpPathfinder.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpSettlement.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpTerritoryInfluence.h"
#include "simulation2/helpers/Geometry.h"
#include "simulation2/helpers/Grid.h"
#include "simulation2/helpers/PriorityQueue.h"

class CCmpTerritoryManager;

class TerritoryOverlay : public TerrainOverlay
{
	NONCOPYABLE(TerritoryOverlay);
public:
	CCmpTerritoryManager& m_TerritoryManager;

	TerritoryOverlay(CCmpTerritoryManager& manager) : m_TerritoryManager(manager) { }
	virtual void StartRender();
	virtual void ProcessTile(ssize_t i, ssize_t j);
};

class CCmpTerritoryManager : public ICmpTerritoryManager
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeGloballyToMessageType(MT_OwnershipChanged);
		componentManager.SubscribeGloballyToMessageType(MT_PositionChanged);
		componentManager.SubscribeToMessageType(MT_TerrainChanged);
	}

	DEFAULT_COMPONENT_ALLOCATOR(TerritoryManager)

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	Grid<u8>* m_Territories;
	TerritoryOverlay* m_DebugOverlay;

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		m_Territories = NULL;
		m_DebugOverlay = NULL;
//		m_DebugOverlay = new TerritoryOverlay(*this);

		m_DirtyID = 1;
	}

	virtual void Deinit()
	{
		SAFE_DELETE(m_Territories);
		SAFE_DELETE(m_DebugOverlay);
	}

	virtual void Serialize(ISerializer& serialize)
	{
		// TODO
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(paramNode);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_OwnershipChanged:
		{
			const CMessageOwnershipChanged& msgData = static_cast<const CMessageOwnershipChanged&> (msg);
			MakeDirtyIfRelevantEntity(msgData.entity);
			break;
		}
		case MT_PositionChanged:
		{
			const CMessagePositionChanged& msgData = static_cast<const CMessagePositionChanged&> (msg);
			MakeDirtyIfRelevantEntity(msgData.entity);
			break;
		}
		case MT_TerrainChanged:
		{
			MakeDirty();
			break;
		}
		}
	}

	// Check whether the entity is either a settlement or territory influence;
	// ignore any others
	void MakeDirtyIfRelevantEntity(entity_id_t ent)
	{
		CmpPtr<ICmpSettlement> cmpSettlement(GetSimContext(), ent);
		if (!cmpSettlement.null())
			MakeDirty();

		CmpPtr<ICmpTerritoryInfluence> cmpTerritoryInfluence(GetSimContext(), ent);
		if (!cmpTerritoryInfluence.null())
			MakeDirty();
	}

	virtual const Grid<u8>& GetTerritoryGrid()
	{
		CalculateTerritories();
		return *m_Territories;
	}

	// To support lazy updates of territory render data,
	// we maintain a DirtyID here and increment it whenever territories change;
	// if a caller has a lower DirtyID then it needs to be updated.

	size_t m_DirtyID;

	void MakeDirty()
	{
		SAFE_DELETE(m_Territories);
		++m_DirtyID;
	}

	virtual bool NeedUpdate(size_t* dirtyID)
	{
		ENSURE(*dirtyID <= m_DirtyID);
		if (*dirtyID < m_DirtyID)
		{
			*dirtyID = m_DirtyID;
			return true;
		}
		return false;
	}

	void CalculateTerritories();

	/**
	 * Updates @p grid based on the obstruction shapes of all entities with
	 * a TerritoryInfluence component. Grid cells are 0 if no influence,
	 * or 1+c if the influence have cost c (assumed between 0 and 254).
	 */
	void RasteriseInfluences(Grid<u8>& grid);
};

REGISTER_COMPONENT_TYPE(TerritoryManager)


/*

We compute territory influences with a kind of best-first search:
 1) Initialise an 'open' list with tiles that contain settlements (annotated with
    territory ID) with initial cost 0
 2) Pick the lowest cost tile from 'item'
 3) For each neighbour which has not already been assigned to a territory,
    assign it to this territory and compute its new cost (effectively the
    distance from the associated settlement) and add to 'open'
 4) Go to 2 until 'open' is empty

*/

typedef PriorityQueueHeap<std::pair<u16, u16>, u32> OpenQueue;

static void ProcessNeighbour(u8 pid, u16 i, u16 j, u32 pg, bool diagonal,
		Grid<u8>& grid, OpenQueue& queue, const Grid<u8>& influenceGrid)
{
	// Ignore tiles that are already claimed
	u8 id = grid.get(i, j);
	if (id)
		return;

	// Base cost for moving onto this tile
	u32 dg = diagonal ? 362 : 256;

	// Adjust cost based on this tile's influences
	dg *= influenceGrid.get(i, j);

	u32 g = pg + dg; // cost to this tile = cost to predecessor + delta from predecessor

	grid.set(i, j, pid);
	OpenQueue::Item tile = { std::make_pair(i, j), g };
	queue.push(tile);
}

void CCmpTerritoryManager::CalculateTerritories()
{
	PROFILE("CalculateTerritories");

	if (m_Territories)
		return;

	CmpPtr<ICmpTerrain> cmpTerrain(GetSimContext(), SYSTEM_ENTITY);
	uint32_t tilesW = cmpTerrain->GetVerticesPerSide() - 1;
	uint32_t tilesH = cmpTerrain->GetVerticesPerSide() - 1;

	Grid<u8> influenceGrid(tilesW, tilesH);

	RasteriseInfluences(influenceGrid);

	SAFE_DELETE(m_Territories);
	m_Territories = new Grid<u8>(tilesW, tilesH);

	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSimContext(), SYSTEM_ENTITY);
	ICmpPathfinder::pass_class_t passClass = cmpPathfinder->GetPassabilityClass("default");
	const Grid<u16>& passGrid = cmpPathfinder->GetPassabilityGrid();

	// Adjust influenceGrid so it contains terrain-passability-dependent costs,
	// unless overridden by existing values in influenceGrid
	for (u32 j = 0; j < tilesH; ++j)
	{
		for (u32 i = 0; i < tilesW; ++i)
		{
			u8 cost;
			u8 inflCost = influenceGrid.get(i, j);
			if (inflCost)
			{
				cost = inflCost-1; // undo RasteriseInfluences's offset
			}
			else
			{
				if (passGrid.get(i, j) & passClass)
					cost = 100;
				else
					cost = 1;
			}
			influenceGrid.set(i, j, cost);
		}
	}

	OpenQueue openTiles;

	// Initialise open list with all settlements

	CComponentManager::InterfaceList settlements = GetSimContext().GetComponentManager().GetEntitiesWithInterface(IID_Settlement);
	u8 id = 1;
	for (CComponentManager::InterfaceList::iterator it = settlements.begin(); it != settlements.end(); ++it)
	{
		entity_id_t settlement = it->first;
		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), settlement);
		if (cmpPosition.null() || !cmpPosition->IsInWorld())
			continue;

		// TODO: maybe we need to ignore settlements with owner -1,
		// since they're probably destroyed

		CFixedVector2D pos = cmpPosition->GetPosition2D();
		int i = clamp((pos.X / (int)CELL_SIZE).ToInt_RoundToNegInfinity(), 0, (int)tilesW-1);
		int j = clamp((pos.Y / (int)CELL_SIZE).ToInt_RoundToNegInfinity(), 0, (int)tilesH-1);

		// Must avoid duplicates in the priority queue; ignore the settlement
		// if there's already one on that tile
		if (!m_Territories->get(i, j))
		{
			m_Territories->set(i, j, id);
			OpenQueue::Item tile = { std::make_pair((u16)i, (i16)j), 0 };
			openTiles.push(tile);
		}

		id += 1;
	}

	while (!openTiles.empty())
	{
		OpenQueue::Item tile = openTiles.pop();

		// Get current tile's territory ID
		u8 tid = m_Territories->get(tile.id.first, tile.id.second);

		// Process neighbours (if they're not off the edge of the map)
		u16 x = tile.id.first;
		u16 z = tile.id.second;
		if (x > 0)
			ProcessNeighbour(tid, x-1, z, tile.rank, false, *m_Territories, openTiles, influenceGrid);
		if (x < tilesW-1)
			ProcessNeighbour(tid, x+1, z, tile.rank, false, *m_Territories, openTiles, influenceGrid);
		if (z > 0)
			ProcessNeighbour(tid, x, z-1, tile.rank, false, *m_Territories, openTiles, influenceGrid);
		if (z < tilesH-1)
			ProcessNeighbour(tid, x, z+1, tile.rank, false, *m_Territories, openTiles, influenceGrid);
		if (x > 0 && z > 0)
			ProcessNeighbour(tid, x-1, z-1, tile.rank, true, *m_Territories, openTiles, influenceGrid);
		if (x > 0 && z < tilesH-1)
			ProcessNeighbour(tid, x-1, z+1, tile.rank, true, *m_Territories, openTiles, influenceGrid);
		if (x < tilesW-1 && z > 0)
			ProcessNeighbour(tid, x+1, z-1, tile.rank, true, *m_Territories, openTiles, influenceGrid);
		if (x < tilesW-1 && z < tilesH-1)
			ProcessNeighbour(tid, x+1, z+1, tile.rank, true, *m_Territories, openTiles, influenceGrid);
	}
}

/**
 * Compute the tile indexes on the grid nearest to a given point
 */
static void NearestTile(entity_pos_t x, entity_pos_t z, u16& i, u16& j, u16 w, u16 h)
{
	i = clamp((x / (int)CELL_SIZE).ToInt_RoundToZero(), 0, w-1);
	j = clamp((z / (int)CELL_SIZE).ToInt_RoundToZero(), 0, h-1);
}

/**
 * Returns the position of the center of the given tile
 */
static void TileCenter(u16 i, u16 j, entity_pos_t& x, entity_pos_t& z)
{
	x = entity_pos_t::FromInt(i*(int)CELL_SIZE + CELL_SIZE/2);
	z = entity_pos_t::FromInt(j*(int)CELL_SIZE + CELL_SIZE/2);
}

// TODO: would be nice not to duplicate those two functions from CCmpObstructionManager.cpp


void CCmpTerritoryManager::RasteriseInfluences(Grid<u8>& grid)
{
	CComponentManager::InterfaceList infls = GetSimContext().GetComponentManager().GetEntitiesWithInterface(IID_TerritoryInfluence);
	for (CComponentManager::InterfaceList::iterator it = infls.begin(); it != infls.end(); ++it)
	{
		ICmpTerritoryInfluence* cmpTerritoryInfluence = static_cast<ICmpTerritoryInfluence*>(it->second);

		CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), it->first);
		if (cmpObstruction.null())
			continue;

		ICmpObstructionManager::ObstructionSquare square;
		if (!cmpObstruction->GetObstructionSquare(square))
			continue;

		u8 cost = cmpTerritoryInfluence->GetCost();

		CFixedVector2D halfSize(square.hw, square.hh);
		CFixedVector2D halfBound = Geometry::GetHalfBoundingBox(square.u, square.v, halfSize);

		u16 i0, j0, i1, j1;
		NearestTile(square.x - halfBound.X, square.z - halfBound.Y, i0, j0, grid.m_W, grid.m_H);
		NearestTile(square.x + halfBound.X, square.z + halfBound.Y, i1, j1, grid.m_W, grid.m_H);
		for (u16 j = j0; j <= j1; ++j)
		{
			for (u16 i = i0; i <= i1; ++i)
			{
				entity_pos_t x, z;
				TileCenter(i, j, x, z);
				if (Geometry::PointIsInSquare(CFixedVector2D(x - square.x, z - square.z), square.u, square.v, halfSize))
					grid.set(i, j, cost+1);
			}
		}

	}
}


void TerritoryOverlay::StartRender()
{
	m_TerritoryManager.CalculateTerritories();
}

void TerritoryOverlay::ProcessTile(ssize_t i, ssize_t j)
{
	if (!m_TerritoryManager.m_Territories)
		return;

	u8 id = m_TerritoryManager.m_Territories->get(i, j);

	float a = 0.2f;
	switch (id)
	{
	case 0: break;
	case 1: RenderTile(CColor(1, 0, 0, a), false); break;
	case 2: RenderTile(CColor(0, 1, 0, a), false); break;
	case 3: RenderTile(CColor(0, 0, 1, a), false); break;
	case 4: RenderTile(CColor(1, 1, 0, a), false); break;
	case 5: RenderTile(CColor(0, 1, 1, a), false); break;
	case 6: RenderTile(CColor(1, 0, 1, a), false); break;
	default: RenderTile(CColor(1, 1, 1, a), false); break;
	}
}
