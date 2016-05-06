/* Copyright (C) 2016 Wildfire Games.
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
#include "graphics/TerritoryBoundary.h"
#include "maths/MathUtil.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/Renderer.h"
#include "renderer/Scene.h"
#include "renderer/TerrainOverlay.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpPathfinder.h"
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpTerritoryInfluence.h"
#include "simulation2/helpers/Grid.h"
#include "simulation2/helpers/Render.h"

#include <queue>

class CCmpTerritoryManager;

class TerritoryOverlay : public TerrainTextureOverlay
{
	NONCOPYABLE(TerritoryOverlay);
public:
	CCmpTerritoryManager& m_TerritoryManager;

	TerritoryOverlay(CCmpTerritoryManager& manager);
	virtual void BuildTextureRGBA(u8* data, size_t w, size_t h);
};

class CCmpTerritoryManager : public ICmpTerritoryManager
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeGloballyToMessageType(MT_OwnershipChanged);
		componentManager.SubscribeGloballyToMessageType(MT_PositionChanged);
		componentManager.SubscribeGloballyToMessageType(MT_ValueModification);
		componentManager.SubscribeToMessageType(MT_ObstructionMapShapeChanged);
		componentManager.SubscribeToMessageType(MT_TerrainChanged);
		componentManager.SubscribeToMessageType(MT_WaterChanged);
		componentManager.SubscribeToMessageType(MT_Update);
		componentManager.SubscribeToMessageType(MT_Interpolate);
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

	// Player ID   in bits 0-4 (TERRITORY_PLAYER_MASK)
	// connected flag in bit 4 (TERRITORY_CONNECTED_MASK)
	// blinking flag  in bit 5 (TERRITORY_BLINKING_MASK)
	// processed flag in bit 7 (TERRITORY_PROCESSED_MASK)
	Grid<u8>* m_Territories;

	std::vector<u16> m_TerritoryCellCounts;
	u16 m_TerritoryTotalPassableCellCount;

	// Saves the cost per tile (to stop territory on impassable tiles)
	Grid<u8>* m_CostGrid;

	// Set to true when territories change; will send a TerritoriesChanged message
	// during the Update phase
	bool m_TriggerEvent;

	struct SBoundaryLine
	{
		bool blinking;
		CColor color;
		SOverlayTexturedLine overlay;
	};

	std::vector<SBoundaryLine> m_BoundaryLines;
	bool m_BoundaryLinesDirty;

	double m_AnimTime; // time since start of rendering, in seconds

	TerritoryOverlay* m_DebugOverlay;

	bool m_EnableLineDebugOverlays; ///< Enable node debugging overlays for boundary lines?
	std::vector<SOverlayLine> m_DebugBoundaryLineNodes;

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		m_Territories = NULL;
		m_CostGrid = NULL;
		m_DebugOverlay = NULL;
//		m_DebugOverlay = new TerritoryOverlay(*this);
		m_BoundaryLinesDirty = true;
		m_TriggerEvent = true;
		m_EnableLineDebugOverlays = false;
		m_DirtyID = 1;
		m_Visible = true;

		m_AnimTime = 0.0;

		m_TerritoryTotalPassableCellCount = 0;

		// Register Relax NG validator
		CXeromyces::AddValidator(g_VFS, "territorymanager", "simulation/data/territorymanager.rng");

		CParamNode externalParamNode;
		CParamNode::LoadXML(externalParamNode, L"simulation/data/territorymanager.xml", "territorymanager");

		int impassableCost = externalParamNode.GetChild("TerritoryManager").GetChild("ImpassableCost").ToInt();
		ENSURE(0 <= impassableCost && impassableCost <= 255);
		m_ImpassableCost = (u8)impassableCost;
		m_BorderThickness = externalParamNode.GetChild("TerritoryManager").GetChild("BorderThickness").ToFixed().ToFloat();
		m_BorderSeparation = externalParamNode.GetChild("TerritoryManager").GetChild("BorderSeparation").ToFixed().ToFloat();
	}

	virtual void Deinit()
	{
		SAFE_DELETE(m_Territories);
		SAFE_DELETE(m_CostGrid);
		SAFE_DELETE(m_DebugOverlay);
	}

	virtual void Serialize(ISerializer& serialize)
	{
		// Territory state can be recomputed as required, so we don't need to serialize any of it.
		serialize.Bool("trigger event", m_TriggerEvent);
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(paramNode);
		deserialize.Bool("trigger event", m_TriggerEvent);
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
		case MT_ValueModification:
		{
			const CMessageValueModification& msgData = static_cast<const CMessageValueModification&> (msg);
			if (msgData.component == L"TerritoryInfluence")
				MakeDirty();
			break;
		}
		case MT_ObstructionMapShapeChanged:
		case MT_TerrainChanged:
		case MT_WaterChanged:
		{
			// also recalculate the cost grid to support atlas changes
			SAFE_DELETE(m_CostGrid);
			MakeDirty();
			break;
		}
		case MT_Update:
		{
			if (m_TriggerEvent)
			{
				m_TriggerEvent = false;
				CMessageTerritoriesChanged msg;
				GetSimContext().GetComponentManager().BroadcastMessage(msg);
			}
			break;
		}
		case MT_Interpolate:
		{
			const CMessageInterpolate& msgData = static_cast<const CMessageInterpolate&> (msg);
			Interpolate(msgData.deltaSimTime, msgData.offset);
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
		CmpPtr<ICmpTerritoryInfluence> cmpTerritoryInfluence(GetSimContext(), ent);
		if (cmpTerritoryInfluence)
			MakeDirty();
	}

	virtual const Grid<u8>& GetTerritoryGrid()
	{
		CalculateTerritories();
		ENSURE(m_Territories);
		return *m_Territories;
	}

	virtual player_id_t GetOwner(entity_pos_t x, entity_pos_t z);
	virtual std::vector<u32> GetNeighbours(entity_pos_t x, entity_pos_t z, bool filterConnected);
	virtual bool IsConnected(entity_pos_t x, entity_pos_t z);

	virtual void SetTerritoryBlinking(entity_pos_t x, entity_pos_t z, bool enable);
	virtual bool IsTerritoryBlinking(entity_pos_t x, entity_pos_t z);

	// To support lazy updates of territory render data,
	// we maintain a DirtyID here and increment it whenever territories change;
	// if a caller has a lower DirtyID then it needs to be updated.

	size_t m_DirtyID;

	void MakeDirty()
	{
		SAFE_DELETE(m_Territories);
		++m_DirtyID;
		m_BoundaryLinesDirty = true;
		m_TriggerEvent = true;
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

	void CalculateCostGrid();

	void CalculateTerritories();

	u8 GetTerritoryPercentage(player_id_t player);

	std::vector<STerritoryBoundary> ComputeBoundaries();

	void UpdateBoundaryLines();

	void Interpolate(float frameTime, float frameOffset);

	void RenderSubmit(SceneCollector& collector);

	void SetVisibility(bool visible)
	{
		m_Visible = visible;
	}

private:

	bool m_Visible;
};

REGISTER_COMPONENT_TYPE(TerritoryManager)

// Tile data type, for easier accessing of coordinates
struct Tile
{
	Tile(u16 i, u16 j) : x(i), z(j) { }
	u16 x, z;
};

// Floodfill templates that expand neighbours from a certain source onwards
// (x, z) are the coordinates of the currently expanded tile
// (nx, nz) are the coordinates of the current neighbour handled
// The user of this floodfill should use "continue" on every neighbour that
// shouldn't be expanded on its own. (without continue, an infinite loop will happen)
# define FLOODFILL(i, j, code)\
	do {\
		const int NUM_NEIGHBOURS = 8;\
		const int NEIGHBOURS_X[NUM_NEIGHBOURS] = {1,-1, 0, 0, 1,-1, 1,-1};\
		const int NEIGHBOURS_Z[NUM_NEIGHBOURS] = {0, 0, 1,-1, 1,-1,-1, 1};\
		std::queue<Tile> openTiles;\
		openTiles.emplace(i, j);\
		while (!openTiles.empty())\
		{\
			u16 x = openTiles.front().x;\
			u16 z = openTiles.front().z;\
			openTiles.pop();\
			for (int n = 0; n < NUM_NEIGHBOURS; ++n)\
			{\
				u16 nx = x + NEIGHBOURS_X[n];\
				u16 nz = z + NEIGHBOURS_Z[n];\
				/* Check the bounds, underflow will cause the values to be big again */\
				if (nx >= tilesW || nz >= tilesH)\
					continue;\
				code\
				openTiles.emplace(nx, nz);\
			}\
		}\
	}\
	while (false)

/**
 * Compute the tile indexes on the grid nearest to a given point
 */
static void NearestTerritoryTile(entity_pos_t x, entity_pos_t z, u16& i, u16& j, u16 w, u16 h)
{
	entity_pos_t scale = Pathfinding::NAVCELL_SIZE * ICmpTerritoryManager::NAVCELLS_PER_TERRITORY_TILE;
	i = clamp((x / scale).ToInt_RoundToNegInfinity(), 0, w - 1);
	j = clamp((z / scale).ToInt_RoundToNegInfinity(), 0, h - 1);
}

void CCmpTerritoryManager::CalculateCostGrid()
{
	if (m_CostGrid)
		return;

	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
	if (!cmpPathfinder)
		return;

	pass_class_t passClassTerritory = cmpPathfinder->GetPassabilityClass("default-terrain-only");
	pass_class_t passClassUnrestricted = cmpPathfinder->GetPassabilityClass("unrestricted");

	const Grid<NavcellData>& passGrid = cmpPathfinder->GetPassabilityGrid();

	int tilesW = passGrid.m_W / NAVCELLS_PER_TERRITORY_TILE;
	int tilesH = passGrid.m_H / NAVCELLS_PER_TERRITORY_TILE;

	m_CostGrid = new Grid<u8>(tilesW, tilesH);
	m_TerritoryTotalPassableCellCount = 0;

	for (int i = 0; i < tilesW; ++i)
	{
		for (int j = 0; j < tilesH; ++j)
		{
			NavcellData c = 0;
			for (u16 di = 0; di < NAVCELLS_PER_TERRITORY_TILE; ++di)
				for (u16 dj = 0; dj < NAVCELLS_PER_TERRITORY_TILE; ++dj)
					c |= passGrid.get(
						i * NAVCELLS_PER_TERRITORY_TILE + di,
						j * NAVCELLS_PER_TERRITORY_TILE + dj);
			if (!IS_PASSABLE(c, passClassTerritory))
				m_CostGrid->set(i, j, m_ImpassableCost);
			else if (!IS_PASSABLE(c, passClassUnrestricted))
				m_CostGrid->set(i, j, 255); // off the world; use maximum cost
			else
			{
				m_CostGrid->set(i, j, 1);
				++m_TerritoryTotalPassableCellCount;
			}
		}
	}
}

void CCmpTerritoryManager::CalculateTerritories()
{
	if (m_Territories)
		return;

	PROFILE("CalculateTerritories");

	// If the pathfinder hasn't been loaded (e.g. this is called during map initialisation),
	// abort the computation (and assume callers can cope with m_Territories == NULL)
	CalculateCostGrid();
	if (!m_CostGrid)
		return;

	const u16 tilesW = m_CostGrid->m_W;
	const u16 tilesH = m_CostGrid->m_H;

	m_Territories = new Grid<u8>(tilesW, tilesH);

	// Reset territory counts for all players
	CmpPtr<ICmpPlayerManager> cmpPlayerManager(GetSystemEntity());
	if (cmpPlayerManager && (size_t)cmpPlayerManager->GetNumPlayers() != m_TerritoryCellCounts.size())
		m_TerritoryCellCounts.resize(cmpPlayerManager->GetNumPlayers());
	for (u16& count : m_TerritoryCellCounts)
		count = 0;

	// Find all territory influence entities
	CComponentManager::InterfaceList influences = GetSimContext().GetComponentManager().GetEntitiesWithInterface(IID_TerritoryInfluence);

	// Split influence entities into per-player lists, ignoring any with invalid properties
	std::map<player_id_t, std::vector<entity_id_t> > influenceEntities;
	for (const CComponentManager::InterfacePair& pair : influences)
	{
		entity_id_t ent = pair.first;

		CmpPtr<ICmpOwnership> cmpOwnership(GetSimContext(), ent);
		if (!cmpOwnership)
			continue;

		// Ignore Gaia and unassigned or players we can't represent
		player_id_t owner = cmpOwnership->GetOwner();
		if (owner <= 0 || owner > TERRITORY_PLAYER_MASK)
			continue;

		influenceEntities[owner].push_back(ent);
	}

	// Store the overall best weight for comparison
	Grid<u32> bestWeightGrid(tilesW, tilesH);
	// store the root influences to mark territory as connected
	std::vector<entity_id_t> rootInfluenceEntities;

	for (const std::pair<player_id_t, std::vector<entity_id_t> >& pair : influenceEntities)
	{
		// entityGrid stores the weight for a single entity, and is reset per entity
		Grid<u32> entityGrid(tilesW, tilesH);
		// playerGrid stores the combined weight of all entities for this player
		Grid<u32> playerGrid(tilesW, tilesH);

		u8 owner = (u8)pair.first;
		const std::vector<entity_id_t>& ents = pair.second;
		// With 2^16 entities, we're safe against overflows as the weight is also limited to 2^16
		ENSURE(ents.size() < 1 << 16); 
		// Compute the influence map of the current entity, then add it to the player grid
		for (entity_id_t ent : ents)
		{
			CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), ent);
			if (!cmpPosition || !cmpPosition->IsInWorld())
				continue;

			CmpPtr<ICmpTerritoryInfluence> cmpTerritoryInfluence(GetSimContext(), ent);
			u32 weight = cmpTerritoryInfluence->GetWeight();
			u32 radius = cmpTerritoryInfluence->GetRadius();
			if (weight == 0 || radius == 0)
				continue;
			u32 falloff = weight * (Pathfinding::NAVCELL_SIZE * NAVCELLS_PER_TERRITORY_TILE).ToInt_RoundToNegInfinity() / radius;

			CFixedVector2D pos = cmpPosition->GetPosition2D();
			u16 i, j;
			NearestTerritoryTile(pos.X, pos.Y, i, j, tilesW, tilesH);

			if (cmpTerritoryInfluence->IsRoot())
				rootInfluenceEntities.push_back(ent);

			// Initialise the tile under the entity
			entityGrid.set(i, j, weight);
			if (weight > bestWeightGrid.get(i, j))
			{
				bestWeightGrid.set(i, j, weight);
				m_Territories->set(i, j, owner);
			}

			// Expand influences outwards
			FLOODFILL(i, j,
				u32 dg = falloff * m_CostGrid->get(nx, nz);

				// diagonal neighbour -> multiply with approx sqrt(2)
				if (nx != x && nz != z)
					dg = (dg * 362) / 256;

				// Don't expand if new cost is not better than previous value for that tile
				// (arranged to avoid underflow if entityGrid.get(x, z) < dg)
				if (entityGrid.get(x, z) <= entityGrid.get(nx, nz) + dg)
					continue;

				// weight of this tile = weight of predecessor - falloff from predecessor
				u32 newWeight = entityGrid.get(x, z) - dg;
				u32 totalWeight = playerGrid.get(nx, nz) - entityGrid.get(nx, nz) + newWeight;
				playerGrid.set(nx, nz, totalWeight);
				entityGrid.set(nx, nz, newWeight);
				// if this weight is better than the best thus far, set the owner
				if (totalWeight > bestWeightGrid.get(nx, nz))
				{
					bestWeightGrid.set(nx, nz, totalWeight);
					m_Territories->set(nx, nz, owner);
				}
			);

			entityGrid.reset();
		}
	}

	// Detect territories connected to a 'root' influence (typically a civ center)
	// belonging to their player, and mark them with the connected flag
	for (entity_id_t ent : rootInfluenceEntities)
	{
		// (These components must be valid else the entities wouldn't be added to this list)
		CmpPtr<ICmpOwnership> cmpOwnership(GetSimContext(), ent);
		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), ent);

		CFixedVector2D pos = cmpPosition->GetPosition2D();
		u16 i, j;
		NearestTerritoryTile(pos.X, pos.Y, i, j, tilesW, tilesH);

		u8 owner = (u8)cmpOwnership->GetOwner();

		if (m_Territories->get(i, j) != owner)
			continue;

		m_Territories->set(i, j, owner | TERRITORY_CONNECTED_MASK);

		FLOODFILL(i, j,
			// Don't expand non-owner tiles, or tiles that already have a connected mask
			if (m_Territories->get(nx, nz) != owner)
				continue;
			m_Territories->set(nx, nz, owner | TERRITORY_CONNECTED_MASK);
			if (m_CostGrid->get(nx, nz) < m_ImpassableCost)
				++m_TerritoryCellCounts[owner];
		);
	}
}

std::vector<STerritoryBoundary> CCmpTerritoryManager::ComputeBoundaries()
{
	PROFILE("ComputeBoundaries");

	CalculateTerritories();
	ENSURE(m_Territories);

	return CTerritoryBoundaryCalculator::ComputeBoundaries(m_Territories);
}

u8 CCmpTerritoryManager::GetTerritoryPercentage(player_id_t player)
{
	if (player <= 0 || (size_t)player > m_TerritoryCellCounts.size())
		return 0;

	CalculateTerritories();

	if (m_TerritoryTotalPassableCellCount == 0)
		return 0;

	u8 percentage = (m_TerritoryCellCounts[player] * 100) / m_TerritoryTotalPassableCellCount;
	ENSURE(percentage <= 100);
	return percentage;
}

void CCmpTerritoryManager::UpdateBoundaryLines()
{
	PROFILE("update boundary lines");

	m_BoundaryLines.clear();
	m_DebugBoundaryLineNodes.clear();

	if (!CRenderer::IsInitialised())
		return;

	std::vector<STerritoryBoundary> boundaries = ComputeBoundaries();

	CTextureProperties texturePropsBase("art/textures/misc/territory_border.png");
	texturePropsBase.SetWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE);
	texturePropsBase.SetMaxAnisotropy(2.f);
	CTexturePtr textureBase = g_Renderer.GetTextureManager().CreateTexture(texturePropsBase);

	CTextureProperties texturePropsMask("art/textures/misc/territory_border_mask.png");
	texturePropsMask.SetWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE);
	texturePropsMask.SetMaxAnisotropy(2.f);
	CTexturePtr textureMask = g_Renderer.GetTextureManager().CreateTexture(texturePropsMask);

	CmpPtr<ICmpPlayerManager> cmpPlayerManager(GetSystemEntity());
	if (!cmpPlayerManager)
		return;

	for (size_t i = 0; i < boundaries.size(); ++i)
	{
		if (boundaries[i].points.empty())
			continue;

		CColor color(1, 0, 1, 1);
		CmpPtr<ICmpPlayer> cmpPlayer(GetSimContext(), cmpPlayerManager->GetPlayerByID(boundaries[i].owner));
		if (cmpPlayer)
			color = cmpPlayer->GetColor();

		m_BoundaryLines.push_back(SBoundaryLine());
		m_BoundaryLines.back().blinking = boundaries[i].blinking;
		m_BoundaryLines.back().color = color;
		m_BoundaryLines.back().overlay.m_SimContext = &GetSimContext();
		m_BoundaryLines.back().overlay.m_TextureBase = textureBase;
		m_BoundaryLines.back().overlay.m_TextureMask = textureMask;
		m_BoundaryLines.back().overlay.m_Color = color;
		m_BoundaryLines.back().overlay.m_Thickness = m_BorderThickness;
		m_BoundaryLines.back().overlay.m_Closed = true;

		SimRender::SmoothPointsAverage(boundaries[i].points, m_BoundaryLines.back().overlay.m_Closed);
		SimRender::InterpolatePointsRNS(boundaries[i].points, m_BoundaryLines.back().overlay.m_Closed, m_BorderSeparation);

		std::vector<float>& points = m_BoundaryLines.back().overlay.m_Coords;
		for (size_t j = 0; j < boundaries[i].points.size(); ++j)
		{
			points.push_back(boundaries[i].points[j].X);
			points.push_back(boundaries[i].points[j].Y);

			if (m_EnableLineDebugOverlays)
			{
				const size_t numHighlightNodes = 7; // highlight the X last nodes on either end to see where they meet (if closed)
				SOverlayLine overlayNode;
				if (j > boundaries[i].points.size() - 1 - numHighlightNodes)
					overlayNode.m_Color = CColor(1.f, 0.f, 0.f, 1.f);
				else if (j < numHighlightNodes)
					overlayNode.m_Color = CColor(0.f, 1.f, 0.f, 1.f);
				else
					overlayNode.m_Color = CColor(1.0f, 1.0f, 1.0f, 1.0f);

				overlayNode.m_Thickness = 1;
				SimRender::ConstructCircleOnGround(GetSimContext(), boundaries[i].points[j].X, boundaries[i].points[j].Y, 0.1f, overlayNode, true);
				m_DebugBoundaryLineNodes.push_back(overlayNode);
			}
		}

	}
}

void CCmpTerritoryManager::Interpolate(float frameTime, float UNUSED(frameOffset))
{
	m_AnimTime += frameTime;

	if (m_BoundaryLinesDirty)
	{
		UpdateBoundaryLines();
		m_BoundaryLinesDirty = false;
	}

	for (size_t i = 0; i < m_BoundaryLines.size(); ++i)
	{
		if (m_BoundaryLines[i].blinking)
		{
			CColor c = m_BoundaryLines[i].color;
			c.a *= 0.2f + 0.8f * fabsf((float)cos(m_AnimTime * M_PI)); // TODO: should let artists tweak this
			m_BoundaryLines[i].overlay.m_Color = c;
		}
	}
}

void CCmpTerritoryManager::RenderSubmit(SceneCollector& collector)
{
	if (!m_Visible)
		return;

	for (size_t i = 0; i < m_BoundaryLines.size(); ++i)
		collector.Submit(&m_BoundaryLines[i].overlay);
	
	for (size_t i = 0; i < m_DebugBoundaryLineNodes.size(); ++i)
		collector.Submit(&m_DebugBoundaryLineNodes[i]);

}

player_id_t CCmpTerritoryManager::GetOwner(entity_pos_t x, entity_pos_t z)
{
	u16 i, j;
	if (!m_Territories)
	{
		CalculateTerritories();
		if (!m_Territories)
			return 0;
	}

	NearestTerritoryTile(x, z, i, j, m_Territories->m_W, m_Territories->m_H);
	return m_Territories->get(i, j) & TERRITORY_PLAYER_MASK;
}

std::vector<u32> CCmpTerritoryManager::GetNeighbours(entity_pos_t x, entity_pos_t z, bool filterConnected)
{
	CmpPtr<ICmpPlayerManager> cmpPlayerManager(GetSystemEntity());
	if (!cmpPlayerManager)
		return std::vector<u32>();

	std::vector<u32> ret(cmpPlayerManager->GetNumPlayers(), 0);
	CalculateTerritories();
	if (!m_Territories)
		return ret;

	u16 i, j;
	NearestTerritoryTile(x, z, i, j, m_Territories->m_W, m_Territories->m_H);

	// calculate the neighbours
	player_id_t thisOwner = m_Territories->get(i, j) & TERRITORY_PLAYER_MASK;

	u16 tilesW = m_Territories->m_W;
	u16 tilesH = m_Territories->m_H;

	// use a flood-fill algorithm that fills up to the borders and remembers the owners
	Grid<bool> markerGrid(tilesW, tilesH);
	markerGrid.set(i, j, true);

	FLOODFILL(i, j,
		if (markerGrid.get(nx, nz))
			continue;
		// mark the tile as visited in any case
		markerGrid.set(nx, nz, true);
		int owner = m_Territories->get(nx, nz) & TERRITORY_PLAYER_MASK;
		if (owner != thisOwner)
		{
			if (owner == 0 || !filterConnected || (m_Territories->get(nx, nz) & TERRITORY_CONNECTED_MASK) != 0)
				ret[owner]++; // add player to the neighbour list when requested
			continue; // don't expand non-owner tiles further
		}
	);

	return ret;
}

bool CCmpTerritoryManager::IsConnected(entity_pos_t x, entity_pos_t z)
{
	u16 i, j;
	CalculateTerritories();
	if (!m_Territories)
		return false;

	NearestTerritoryTile(x, z, i, j, m_Territories->m_W, m_Territories->m_H);
	return (m_Territories->get(i, j) & TERRITORY_CONNECTED_MASK) != 0;
}

void CCmpTerritoryManager::SetTerritoryBlinking(entity_pos_t x, entity_pos_t z, bool enable)
{
	CalculateTerritories();
	if (!m_Territories)
		return;

	u16 i, j;
	NearestTerritoryTile(x, z, i, j, m_Territories->m_W, m_Territories->m_H);

	u16 tilesW = m_Territories->m_W;
	u16 tilesH = m_Territories->m_H;

	player_id_t thisOwner = m_Territories->get(i, j) & TERRITORY_PLAYER_MASK;

	FLOODFILL(i, j,
		u8 bitmask = m_Territories->get(nx, nz);
		if ((bitmask & TERRITORY_PLAYER_MASK) != thisOwner)
			continue;
		u8 blinking = bitmask & TERRITORY_BLINKING_MASK;
		if (enable && !blinking)
			m_Territories->set(nx, nz, bitmask | TERRITORY_BLINKING_MASK);
		else if (!enable && blinking)
			m_Territories->set(nx, nz, bitmask & ~TERRITORY_BLINKING_MASK);
		else
			continue;
	);
	m_BoundaryLinesDirty = true;
}

bool CCmpTerritoryManager::IsTerritoryBlinking(entity_pos_t x, entity_pos_t z)
{
	u16 i, j;
	NearestTerritoryTile(x, z, i, j, m_Territories->m_W, m_Territories->m_H);
	return (m_Territories->get(i, j) & TERRITORY_BLINKING_MASK) != 0;
}

TerritoryOverlay::TerritoryOverlay(CCmpTerritoryManager& manager) :
	TerrainTextureOverlay((float)Pathfinding::NAVCELLS_PER_TILE / ICmpTerritoryManager::NAVCELLS_PER_TERRITORY_TILE), 
	m_TerritoryManager(manager)
{ }

void TerritoryOverlay::BuildTextureRGBA(u8* data, size_t w, size_t h)
{
	for (size_t j = 0; j < h; ++j)
	{
		for (size_t i = 0; i < w; ++i)
		{
			SColor4ub color;
			u8 id = (m_TerritoryManager.m_Territories->get((int)i, (int)j) & ICmpTerritoryManager::TERRITORY_PLAYER_MASK);
			color = GetColor(id, 64);
			*data++ = color.R;
			*data++ = color.G;
			*data++ = color.B;
			*data++ = color.A;
		}
	}
}

#undef FLOODFILL
