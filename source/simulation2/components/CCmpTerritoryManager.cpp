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

#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "graphics/TextureManager.h"
#include "maths/MathUtil.h"
#include "maths/Vector2D.h"
#include "ps/Overlay.h"
#include "renderer/Renderer.h"
#include "renderer/Scene.h"
#include "renderer/TerrainOverlay.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpObstruction.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpPathfinder.h"
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpSettlement.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpTerritoryInfluence.h"
#include "simulation2/helpers/Geometry.h"
#include "simulation2/helpers/Grid.h"
#include "simulation2/helpers/PriorityQueue.h"
#include "simulation2/helpers/Render.h"

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
		componentManager.SubscribeToMessageType(MT_RenderSubmit);
	}

	DEFAULT_COMPONENT_ALLOCATOR(TerritoryManager)

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	u8 m_ImpassableCost;
	float m_BorderThickness;
	float m_BorderSeparation;

	Grid<u8>* m_Territories;
	TerritoryOverlay* m_DebugOverlay;
	std::vector<SOverlayTexturedLine> m_BoundaryLines;
	bool m_BoundaryLinesDirty;

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		m_Territories = NULL;
		m_DebugOverlay = NULL;
//		m_DebugOverlay = new TerritoryOverlay(*this);
		m_BoundaryLinesDirty = true;

		m_DirtyID = 1;

		CParamNode externalParamNode;
		CParamNode::LoadXML(externalParamNode, L"simulation/data/territorymanager.xml");

		int impassableCost = externalParamNode.GetChild("TerritoryManager").GetChild("ImpassableCost").ToInt();
		ENSURE(0 <= impassableCost && impassableCost <= 255);
		m_ImpassableCost = (u8)impassableCost;
		m_BorderThickness = externalParamNode.GetChild("TerritoryManager").GetChild("BorderThickness").ToFixed().ToFloat();
		m_BorderSeparation = externalParamNode.GetChild("TerritoryManager").GetChild("BorderSeparation").ToFixed().ToFloat();
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
		case MT_RenderSubmit:
		{
			const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
			RenderSubmit(msgData.collector);
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

	virtual int32_t GetOwner(entity_pos_t x, entity_pos_t z);

	// To support lazy updates of territory render data,
	// we maintain a DirtyID here and increment it whenever territories change;
	// if a caller has a lower DirtyID then it needs to be updated.

	size_t m_DirtyID;

	void MakeDirty()
	{
		SAFE_DELETE(m_Territories);
		++m_DirtyID;
		m_BoundaryLinesDirty = true;
	}

	virtual bool NeedUpdate(size_t* dirtyID)
	{
		if (*dirtyID != m_DirtyID)
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
	void RasteriseInfluences(CComponentManager::InterfaceList& infls, Grid<u8>& grid);

	struct TerritoryBoundary
	{
		player_id_t owner;
		std::vector<CVector2D> points;
	};

	std::vector<TerritoryBoundary> ComputeBoundaries();

	void UpdateBoundaryLines();

	void RenderSubmit(SceneCollector& collector);
};

REGISTER_COMPONENT_TYPE(TerritoryManager)


/*
We compute the territory influence of an entity with a kind of best-first search,
storing an 'open' list of tiles that have not yet been processed,
then taking the highest-weight tile (closest to origin) and updating the weight
of extending to each neighbour (based on radius-determining 'falloff' value,
adjusted by terrain movement cost), and repeating until all tiles are processed.
*/

typedef PriorityQueueHeap<std::pair<u16, u16>, u32, std::greater<u32> > OpenQueue;

static void ProcessNeighbour(u32 falloff, u16 i, u16 j, u32 pg, bool diagonal,
		Grid<u32>& grid, OpenQueue& queue, const Grid<u8>& costGrid)
{
	u32 dg = falloff * costGrid.get(i, j);
	if (diagonal)
		dg = (dg * 362) / 256;

	// Stop if new cost g=pg-dg is not better than previous value for that tile
	// (arranged to avoid underflow if pg < dg)
	if (pg <= grid.get(i, j) + dg)
		return;

	u32 g = pg - dg; // cost to this tile = cost to predecessor - falloff from predecessor

	grid.set(i, j, g);
	OpenQueue::Item tile = { std::make_pair(i, j), g };
	queue.push(tile);
}

static void FloodFill(Grid<u32>& grid, Grid<u8>& costGrid, OpenQueue& openTiles, u32 falloff)
{
	u32 tilesW = grid.m_W;
	u32 tilesH = grid.m_H;

	while (!openTiles.empty())
	{
		OpenQueue::Item tile = openTiles.pop();

		// Process neighbours (if they're not off the edge of the map)
		u16 x = tile.id.first;
		u16 z = tile.id.second;
		if (x > 0)
			ProcessNeighbour(falloff, (u16)(x-1), z, tile.rank, false, grid, openTiles, costGrid);
		if (x < tilesW-1)
			ProcessNeighbour(falloff, (u16)(x+1), z, tile.rank, false, grid, openTiles, costGrid);
		if (z > 0)
			ProcessNeighbour(falloff, x, (u16)(z-1), tile.rank, false, grid, openTiles, costGrid);
		if (z < tilesH-1)
			ProcessNeighbour(falloff, x, (u16)(z+1), tile.rank, false, grid, openTiles, costGrid);
		if (x > 0 && z > 0)
			ProcessNeighbour(falloff, (u16)(x-1), (u16)(z-1), tile.rank, true, grid, openTiles, costGrid);
		if (x > 0 && z < tilesH-1)
			ProcessNeighbour(falloff, (u16)(x-1), (u16)(z+1), tile.rank, true, grid, openTiles, costGrid);
		if (x < tilesW-1 && z > 0)
			ProcessNeighbour(falloff, (u16)(x+1), (u16)(z-1), tile.rank, true, grid, openTiles, costGrid);
		if (x < tilesW-1 && z < tilesH-1)
			ProcessNeighbour(falloff, (u16)(x+1), (u16)(z+1), tile.rank, true, grid, openTiles, costGrid);
	}
}

void CCmpTerritoryManager::CalculateTerritories()
{
	PROFILE("CalculateTerritories");

	if (m_Territories)
		return;

	CmpPtr<ICmpTerrain> cmpTerrain(GetSimContext(), SYSTEM_ENTITY);
	u16 tilesW = cmpTerrain->GetTilesPerSide();
	u16 tilesH = cmpTerrain->GetTilesPerSide();

	SAFE_DELETE(m_Territories);
	m_Territories = new Grid<u8>(tilesW, tilesH);

	// Compute terrain-passability-dependent costs per tile
	Grid<u8> influenceGrid(tilesW, tilesH);

	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSimContext(), SYSTEM_ENTITY);
	ICmpPathfinder::pass_class_t passClassUnrestricted = cmpPathfinder->GetPassabilityClass("unrestricted");
	ICmpPathfinder::pass_class_t passClassDefault = cmpPathfinder->GetPassabilityClass("default");
	const Grid<u16>& passGrid = cmpPathfinder->GetPassabilityGrid();
	for (u16 j = 0; j < tilesH; ++j)
	{
		for (u16 i = 0; i < tilesW; ++i)
		{
			u16 g = passGrid.get(i, j);
			u8 cost;
			if (g & passClassUnrestricted)
				cost = 255; // off the world; use maximum cost
			else if (g & passClassDefault)
				cost = m_ImpassableCost;
			else
				cost = 1;
			influenceGrid.set(i, j, cost);
		}
	}

	// Find all territory influence entities
	CComponentManager::InterfaceList influences = GetSimContext().GetComponentManager().GetEntitiesWithInterface(IID_TerritoryInfluence);

	// Allow influence entities to override the terrain costs
	RasteriseInfluences(influences, influenceGrid);

	// Split influence entities into per-player lists, ignoring any with invalid properties
	std::map<player_id_t, std::vector<entity_id_t> > influenceEntities;
	for (CComponentManager::InterfaceList::iterator it = influences.begin(); it != influences.end(); ++it)
	{
		// Ignore any with no weight or radius (to avoid divide-by-zero later)
		ICmpTerritoryInfluence* cmpTerritoryInfluence = static_cast<ICmpTerritoryInfluence*>(it->second);
		if (cmpTerritoryInfluence->GetWeight() == 0 || cmpTerritoryInfluence->GetRadius() == 0)
			continue;

		CmpPtr<ICmpOwnership> cmpOwnership(GetSimContext(), it->first);
		if (cmpOwnership.null())
			continue;

		// Ignore Gaia and unassigned
		player_id_t owner = cmpOwnership->GetOwner();
		if (owner <= 0)
			continue;

		// Ignore if invalid position
		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), it->first);
		if (cmpPosition.null() || !cmpPosition->IsInWorld())
			continue;

		influenceEntities[owner].push_back(it->first);
	}

	// For each player, store the sum of influences on each tile
	std::vector<std::pair<player_id_t, Grid<u32> > > playerGrids;
	// TODO: this is a large waste of memory; we don't really need to store
	// all the intermediate grids

	for (std::map<player_id_t, std::vector<entity_id_t> >::iterator it = influenceEntities.begin(); it != influenceEntities.end(); ++it)
	{
		Grid<u32> playerGrid(tilesW, tilesH);

		std::vector<entity_id_t>& ents = it->second;
		for (std::vector<entity_id_t>::iterator eit = ents.begin(); eit != ents.end(); ++eit)
		{
			// Compute the influence map of the current entity, then add it to the player grid

			Grid<u32> entityGrid(tilesW, tilesH);

			CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), *eit);
			CFixedVector2D pos = cmpPosition->GetPosition2D();
			u16 i = (u16)clamp((pos.X / (int)CELL_SIZE).ToInt_RoundToNegInfinity(), 0, tilesW-1);
			u16 j = (u16)clamp((pos.Y / (int)CELL_SIZE).ToInt_RoundToNegInfinity(), 0, tilesH-1);

			CmpPtr<ICmpTerritoryInfluence> cmpTerritoryInfluence(GetSimContext(), *eit);
			u32 weight = cmpTerritoryInfluence->GetWeight();
			u32 radius = cmpTerritoryInfluence->GetRadius() / CELL_SIZE;
			u32 falloff = weight / radius; // earlier check for GetRadius() == 0 prevents divide-by-zero

			// TODO: we should have some maximum value on weight, to avoid overflow
			// when doing all the sums

			// Initialise the tile under the entity
			entityGrid.set(i, j, weight);
			OpenQueue openTiles;
			OpenQueue::Item tile = { std::make_pair((u16)i, (i16)j), weight };
			openTiles.push(tile);

			// Expand influences outwards
			FloodFill(entityGrid, influenceGrid, openTiles, falloff);

			// TODO: we should do a sparse grid and only add the non-zero regions, for performance
			for (u16 j = 0; j < entityGrid.m_H; ++j)
				for (u16 i = 0; i < entityGrid.m_W; ++i)
					playerGrid.set(i, j, playerGrid.get(i, j) + entityGrid.get(i, j));
		}

		playerGrids.push_back(std::make_pair(it->first, playerGrid));
	}

	// Set m_Territories to the player ID with the highest influence for each tile
	for (u16 j = 0; j < tilesH; ++j)
	{
		for (u16 i = 0; i < tilesW; ++i)
		{
			u32 bestWeight = 0;
			for (size_t k = 0; k < playerGrids.size(); ++k)
			{
				u32 w = playerGrids[k].second.get(i, j);
				if (w > bestWeight)
				{
					player_id_t id = playerGrids[k].first;
					m_Territories->set(i, j, (u8)id);
					bestWeight = w;
				}
			}
		}
	}
}

/**
 * Compute the tile indexes on the grid nearest to a given point
 */
static void NearestTile(entity_pos_t x, entity_pos_t z, u16& i, u16& j, u16 w, u16 h)
{
	i = (u16)clamp((x / (int)CELL_SIZE).ToInt_RoundToZero(), 0, w-1);
	j = (u16)clamp((z / (int)CELL_SIZE).ToInt_RoundToZero(), 0, h-1);
}

/**
 * Returns the position of the center of the given tile
 */
static void TileCenter(u16 i, u16 j, entity_pos_t& x, entity_pos_t& z)
{
	x = entity_pos_t::FromInt(i*(int)CELL_SIZE + (int)CELL_SIZE/2);
	z = entity_pos_t::FromInt(j*(int)CELL_SIZE + (int)CELL_SIZE/2);
}

// TODO: would be nice not to duplicate those two functions from CCmpObstructionManager.cpp


void CCmpTerritoryManager::RasteriseInfluences(CComponentManager::InterfaceList& infls, Grid<u8>& grid)
{
	for (CComponentManager::InterfaceList::iterator it = infls.begin(); it != infls.end(); ++it)
	{
		ICmpTerritoryInfluence* cmpTerritoryInfluence = static_cast<ICmpTerritoryInfluence*>(it->second);

		i32 cost = cmpTerritoryInfluence->GetCost();
		if (cost == -1)
			continue;

		CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), it->first);
		if (cmpObstruction.null())
			continue;

		ICmpObstructionManager::ObstructionSquare square;
		if (!cmpObstruction->GetObstructionSquare(square))
			continue;

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
					grid.set(i, j, (u8)cost);
			}
		}

	}
}

std::vector<CCmpTerritoryManager::TerritoryBoundary> CCmpTerritoryManager::ComputeBoundaries()
{
	PROFILE("ComputeBoundaries");

	std::vector<CCmpTerritoryManager::TerritoryBoundary> boundaries;

	CalculateTerritories();

	// Copy the territories grid so we can mess with it
	Grid<u8> grid (*m_Territories);

	// Some constants for the border walk
	CVector2D edgeOffsets[] = {
		CVector2D(0.5f, 0.0f),
		CVector2D(1.0f, 0.5f),
		CVector2D(0.5f, 1.0f),
		CVector2D(0.0f, 0.5f)
	};

	// Try to find an assigned tile
	for (u16 j = 0; j < grid.m_H; ++j)
	{
		for (u16 i = 0; i < grid.m_W; ++i)
		{
			u8 owner = grid.get(i, j);
			if (owner)
			{
				// Found the first tile (which must be the lowest j value of any non-zero tile);
				// start at the bottom edge of it and chase anticlockwise around the border until
				// we reach the starting point again

				boundaries.push_back(TerritoryBoundary());
				boundaries.back().owner = owner;
				std::vector<CVector2D>& points = boundaries.back().points;

				u8 dir = 0; // 0 == bottom edge of tile, 1 == right, 2 == top, 3 == left

				u8 cdir = dir;
				u16 ci = i, cj = j;

				u16 maxi = (u16)(grid.m_W-1);
				u16 maxj = (u16)(grid.m_H-1);

				while (true)
				{
					points.push_back((CVector2D(ci, cj) + edgeOffsets[cdir]) * CELL_SIZE);

					// Given that we're on an edge on a continuous boundary and aiming anticlockwise,
					// we can either carry on straight or turn left or turn right, so examine each
					// of the three possible cases (depending on initial direction):
					switch (cdir)
					{
					case 0:
						if (ci < maxi && cj > 0 && grid.get(ci+1, cj-1) == owner)
						{
							++ci;
							--cj;
							cdir = 3;
						}
						else if (ci < maxi && grid.get(ci+1, cj) == owner)
							++ci;
						else
							cdir = 1;
						break;
					case 1:
						if (ci < maxi && cj < maxj && grid.get(ci+1, cj+1) == owner)
						{
							++ci;
							++cj;
							cdir = 0;
						}
						else if (cj < maxj && grid.get(ci, cj+1) == owner)
							++cj;
						else
							cdir = 2;
						break;
					case 2:
						if (ci > 0 && cj < maxj && grid.get(ci-1, cj+1) == owner)
						{
							--ci;
							++cj;
							cdir = 1;
						}
						else if (ci > 0 && grid.get(ci-1, cj) == owner)
							--ci;
						else
							cdir = 3;
						break;
					case 3:
						if (ci > 0 && cj > 0 && grid.get(ci-1, cj-1) == owner)
						{
							--ci;
							--cj;
							cdir = 2;
						}
						else if (cj > 0 && grid.get(ci, cj-1) == owner)
							--cj;
						else
							cdir = 0;
						break;
					}

					// Stop when we've reached the starting point again
					if (ci == i && cj == j && cdir == dir)
						break;
				}

				// Zero out this whole territory with a simple flood fill, so we don't
				// process it a second time
				std::vector<std::pair<u16, u16> > tileStack;

#define ZERO_AND_PUSH(i, j) STMT(grid.set(i, j, 0); tileStack.push_back(std::make_pair(i, j)); )

				ZERO_AND_PUSH(i, j);
				while (!tileStack.empty())
				{
					int ti = tileStack.back().first;
					int tj = tileStack.back().second;
					tileStack.pop_back();

					if (ti > 0 && grid.get(ti-1, tj) == owner)
						ZERO_AND_PUSH(ti-1, tj);
					if (ti < maxi && grid.get(ti+1, tj) == owner)
						ZERO_AND_PUSH(ti+1, tj);
					if (tj > 0 && grid.get(ti, tj-1) == owner)
						ZERO_AND_PUSH(ti, tj-1);
					if (tj < maxj && grid.get(ti, tj+1) == owner)
						ZERO_AND_PUSH(ti, tj+1);

					if (ti > 0 && tj > 0 && grid.get(ti-1, tj-1) == owner)
						ZERO_AND_PUSH(ti-1, tj-1);
					if (ti > 0 && tj < maxj && grid.get(ti-1, tj+1) == owner)
						ZERO_AND_PUSH(ti-1, tj+1);
					if (ti < maxi && tj > 0 && grid.get(ti+1, tj-1) == owner)
						ZERO_AND_PUSH(ti+1, tj-1);
					if (ti < maxi && tj < maxj && grid.get(ti+1, tj+1) == owner)
						ZERO_AND_PUSH(ti+1, tj+1);
				}

#undef ZERO_AND_PUSH
			}
		}
	}

	return boundaries;
}

void CCmpTerritoryManager::UpdateBoundaryLines()
{
	PROFILE("update boundary lines");

	m_BoundaryLines.clear();

	std::vector<CCmpTerritoryManager::TerritoryBoundary> boundaries = ComputeBoundaries();

	CTextureProperties texturePropsBase("art/textures/misc/territory_border.png");
	texturePropsBase.SetWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE);
	texturePropsBase.SetMaxAnisotropy(2.f);
	CTexturePtr textureBase = g_Renderer.GetTextureManager().CreateTexture(texturePropsBase);

	CTextureProperties texturePropsMask("art/textures/misc/territory_border_mask.png");
	texturePropsMask.SetWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE);
	texturePropsMask.SetMaxAnisotropy(2.f);
	CTexturePtr textureMask = g_Renderer.GetTextureManager().CreateTexture(texturePropsMask);

	CmpPtr<ICmpTerrain> cmpTerrain(GetSimContext(), SYSTEM_ENTITY);
	if (cmpTerrain.null())
		return;
	CTerrain* terrain = cmpTerrain->GetCTerrain();

	CmpPtr<ICmpPlayerManager> cmpPlayerManager(GetSimContext(), SYSTEM_ENTITY);
	if (cmpPlayerManager.null())
		return;

	for (size_t i = 0; i < boundaries.size(); ++i)
	{
		if (boundaries[i].points.empty())
			continue;

		CColor color(1, 0, 1, 1);
		CmpPtr<ICmpPlayer> cmpPlayer(GetSimContext(), cmpPlayerManager->GetPlayerByID(boundaries[i].owner));
		if (!cmpPlayer.null())
			color = cmpPlayer->GetColour();

		m_BoundaryLines.push_back(SOverlayTexturedLine());
		m_BoundaryLines.back().m_Terrain = terrain;
		m_BoundaryLines.back().m_TextureBase = textureBase;
		m_BoundaryLines.back().m_TextureMask = textureMask;
		m_BoundaryLines.back().m_Color = color;
		m_BoundaryLines.back().m_Thickness = m_BorderThickness;

		SimRender::SmoothPointsAverage(boundaries[i].points, true);

		SimRender::InterpolatePointsRNS(boundaries[i].points, true, m_BorderSeparation);

		std::vector<float>& points = m_BoundaryLines.back().m_Coords;
		for (size_t j = 0; j < boundaries[i].points.size(); ++j)
		{
			points.push_back(boundaries[i].points[j].X);
			points.push_back(boundaries[i].points[j].Y);
		}
	}
}

void CCmpTerritoryManager::RenderSubmit(SceneCollector& collector)
{
	if (m_BoundaryLinesDirty)
	{
		UpdateBoundaryLines();
		m_BoundaryLinesDirty = false;
	}

	for (size_t i = 0; i < m_BoundaryLines.size(); ++i)
		collector.Submit(&m_BoundaryLines[i]);
}

int32_t CCmpTerritoryManager::GetOwner(entity_pos_t x, entity_pos_t z)
{
	u16 i, j;
	CalculateTerritories();
	NearestTile(x, z, i, j, m_Territories->m_W, m_Territories->m_H);
	return m_Territories->get(i, j);
}

void TerritoryOverlay::StartRender()
{
	m_TerritoryManager.CalculateTerritories();
}

void TerritoryOverlay::ProcessTile(ssize_t i, ssize_t j)
{
	if (!m_TerritoryManager.m_Territories)
		return;

	u8 id = m_TerritoryManager.m_Territories->get((int)i, (int)j);

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
