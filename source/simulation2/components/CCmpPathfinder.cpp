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

/**
 * @file
 * Common code and setup code for CCmpPathfinder.
 */

#include "precompiled.h"

#include "CCmpPathfinder_Common.h"

#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/components/ICmpObstruction.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "simulation2/serialization/SerializeTemplates.h"

// Default cost to move a single tile is a fairly arbitrary number, which should be big
// enough to be precise when multiplied/divided and small enough to never overflow when
// summing the cost of a whole path.
const int DEFAULT_MOVE_COST = 256;

REGISTER_COMPONENT_TYPE(Pathfinder)

void CCmpPathfinder::Init(const CParamNode& UNUSED(paramNode))
{
	m_MapSize = 0;
	m_Grid = NULL;
	m_ObstructionGrid = NULL;
	m_TerrainDirty = true;
	m_NextAsyncTicket = 1;

	m_DebugOverlay = NULL;
	m_DebugGrid = NULL;
	m_DebugPath = NULL;

	m_SameTurnMovesCount = 0;

	// Since this is used as a system component (not loaded from an entity template),
	// we can't use the real paramNode (it won't get handled properly when deserializing),
	// so load the data from a special XML file.
	CParamNode externalParamNode;
	CParamNode::LoadXML(externalParamNode, L"simulation/data/pathfinder.xml");

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
		pass_class_t mask = (pass_class_t)(1u << (m_PassClasses.size() + 2));
		m_PassClasses.push_back(PathfinderPassability(mask, it->second));
		m_PassClassMasks[name] = mask;
	}


	const CParamNode::ChildrenMap& moveClasses = externalParamNode.GetChild("Pathfinder").GetChild("MovementClasses").GetChildren();

	// First find the set of unit classes used by any terrain classes,
	// and assign unique tags to terrain classes
	std::set<std::string> unitClassNames;
	unitClassNames.insert("default"); // must always have costs for default

	{
		size_t i = 0;
		for (CParamNode::ChildrenMap::const_iterator it = moveClasses.begin(); it != moveClasses.end(); ++it)
		{
			std::string terrainClassName = it->first;
			m_TerrainCostClassTags[terrainClassName] = (cost_class_t)i;
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
			m_UnitCostClassTags[*nit] = (cost_class_t)i;
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

void CCmpPathfinder::Deinit()
{
	SetDebugOverlay(false); // cleans up memory
	ResetDebugPath();

	delete m_Grid;
	delete m_ObstructionGrid;
}

struct SerializeLongRequest
{
	template<typename S>
	void operator()(S& serialize, const char* UNUSED(name), AsyncLongPathRequest& value)
	{
		serialize.NumberU32_Unbounded("ticket", value.ticket);
		serialize.NumberFixed_Unbounded("x0", value.x0);
		serialize.NumberFixed_Unbounded("z0", value.z0);
		SerializeGoal()(serialize, "goal", value.goal);
		serialize.NumberU16_Unbounded("pass class", value.passClass);
		serialize.NumberU8_Unbounded("cost class", value.costClass);
		serialize.NumberU32_Unbounded("notify", value.notify);
	}
};

struct SerializeShortRequest
{
	template<typename S>
	void operator()(S& serialize, const char* UNUSED(name), AsyncShortPathRequest& value)
	{
		serialize.NumberU32_Unbounded("ticket", value.ticket);
		serialize.NumberFixed_Unbounded("x0", value.x0);
		serialize.NumberFixed_Unbounded("z0", value.z0);
		serialize.NumberFixed_Unbounded("r", value.r);
		serialize.NumberFixed_Unbounded("range", value.range);
		SerializeGoal()(serialize, "goal", value.goal);
		serialize.NumberU16_Unbounded("pass class", value.passClass);
		serialize.Bool("avoid moving units", value.avoidMovingUnits);
		serialize.NumberU32_Unbounded("group", value.group);
		serialize.NumberU32_Unbounded("notify", value.notify);
	}
};

void CCmpPathfinder::Serialize(ISerializer& serialize)
{
	SerializeVector<SerializeLongRequest>()(serialize, "long requests", m_AsyncLongPathRequests);
	SerializeVector<SerializeShortRequest>()(serialize, "short requests", m_AsyncShortPathRequests);
	serialize.NumberU32_Unbounded("next ticket", m_NextAsyncTicket);
	serialize.NumberU16_Unbounded("same turn moves count", m_SameTurnMovesCount);
}

void CCmpPathfinder::Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
{
	Init(paramNode);

	SerializeVector<SerializeLongRequest>()(deserialize, "long requests", m_AsyncLongPathRequests);
	SerializeVector<SerializeShortRequest>()(deserialize, "short requests", m_AsyncShortPathRequests);
	deserialize.NumberU32_Unbounded("next ticket", m_NextAsyncTicket);
	deserialize.NumberU16_Unbounded("same turn moves count", m_SameTurnMovesCount);
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
	case MT_WaterChanged:
	case MT_ObstructionMapShapeChanged:
	{
		// TODO: we ought to only bother updating the dirtied region
		m_TerrainDirty = true;
		break;
	}
	case MT_TurnStart:
	{
		m_SameTurnMovesCount = 0;
		break;
	}
	}
}

void CCmpPathfinder::RenderSubmit(SceneCollector& collector)
{
	for (size_t i = 0; i < m_DebugOverlayShortPathLines.size(); ++i)
		collector.Submit(&m_DebugOverlayShortPathLines[i]);
}


fixed CCmpPathfinder::GetMovementSpeed(entity_pos_t x0, entity_pos_t z0, u8 costClass)
{
	UpdateGrid();

	u16 i, j;
	NearestTile(x0, z0, i, j);
	TerrainTile tileTag = m_Grid->get(i, j);
	return m_MoveSpeeds.at(costClass).at(GET_COST_CLASS(tileTag));
}

ICmpPathfinder::pass_class_t CCmpPathfinder::GetPassabilityClass(const std::string& name)
{
	if (m_PassClassMasks.find(name) == m_PassClassMasks.end())
	{
		LOGERROR(L"Invalid passability class name '%hs'", name.c_str());
		return 0;
	}

	return m_PassClassMasks[name];
}

std::map<std::string, ICmpPathfinder::pass_class_t> CCmpPathfinder::GetPassabilityClasses()
{
	return m_PassClassMasks;
}

ICmpPathfinder::cost_class_t CCmpPathfinder::GetCostClass(const std::string& name)
{
	if (m_UnitCostClassTags.find(name) == m_UnitCostClassTags.end())
	{
		LOGERROR(L"Invalid unit cost class name '%hs'", name.c_str());
		return m_UnitCostClassTags["default"];
	}

	return m_UnitCostClassTags[name];
}

fixed CCmpPathfinder::DistanceToGoal(CFixedVector2D pos, const CCmpPathfinder::Goal& goal)
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

const Grid<u16>& CCmpPathfinder::GetPassabilityGrid()
{
	UpdateGrid();
	return *m_Grid;
}

void CCmpPathfinder::UpdateGrid()
{
	CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
	if (!cmpTerrain)
		return; // error

	// If the terrain was resized then delete the old grid data
	if (m_Grid && m_MapSize != cmpTerrain->GetTilesPerSide())
	{
		SAFE_DELETE(m_Grid);
		SAFE_DELETE(m_ObstructionGrid);
		m_TerrainDirty = true;
	}

	// Initialise the terrain data when first needed
	if (!m_Grid)
	{
		m_MapSize = cmpTerrain->GetTilesPerSide();
		m_Grid = new Grid<TerrainTile>(m_MapSize, m_MapSize);
		m_ObstructionGrid = new Grid<u8>(m_MapSize, m_MapSize);
	}

	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());

	bool obstructionsDirty = cmpObstructionManager->Rasterise(*m_ObstructionGrid);

	if (obstructionsDirty && !m_TerrainDirty)
	{
		PROFILE("UpdateGrid obstructions");

		// Obstructions changed - we need to recompute passability
		// Since terrain hasn't changed we only need to update the obstruction bits
		// and can skip the rest of the data

		for (u16 j = 0; j < m_MapSize; ++j)
		{
			for (u16 i = 0; i < m_MapSize; ++i)
			{
				TerrainTile& t = m_Grid->get(i, j);

				u8 obstruct = m_ObstructionGrid->get(i, j);

				if (obstruct & ICmpObstructionManager::TILE_OBSTRUCTED_PATHFINDING)
					t |= 1;
				else
					t &= (TerrainTile)~1;

				if (obstruct & ICmpObstructionManager::TILE_OBSTRUCTED_FOUNDATION)
					t |= 2;
				else
					t &= (TerrainTile)~2;
			}
		}

		++m_Grid->m_DirtyID;
	}
	else if (obstructionsDirty || m_TerrainDirty)
	{
		PROFILE("UpdateGrid full");

		// Obstructions or terrain changed - we need to recompute passability
		// TODO: only bother recomputing the region that has actually changed

		CmpPtr<ICmpWaterManager> cmpWaterManager(GetSystemEntity());

		// TOOD: these bits should come from ICmpTerrain
		CTerrain& terrain = GetSimContext().GetTerrain();

		// avoid integer overflow in intermediate calculation
		const u16 shoreMax = 32767;
		
		// First pass - find underwater tiles
		Grid<bool> waterGrid(m_MapSize, m_MapSize);
		for (u16 j = 0; j < m_MapSize; ++j)
		{
			for (u16 i = 0; i < m_MapSize; ++i)
			{
				fixed x, z;
				TileCenter(i, j, x, z);
				
				bool underWater = cmpWaterManager && (cmpWaterManager->GetWaterLevel(x, z) > terrain.GetExactGroundLevelFixed(x, z));
				waterGrid.set(i, j, underWater);
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
					if ((i > 0 && waterGrid.get(i-1, j)) || (i > 0 && j < m_MapSize-1 && waterGrid.get(i-1, j+1)) || (i > 0 && j > 0 && waterGrid.get(i-1, j-1))
						|| (i < m_MapSize-1 && waterGrid.get(i+1, j)) || (i < m_MapSize-1 && j < m_MapSize-1 && waterGrid.get(i+1, j+1)) || (i < m_MapSize-1 && j > 0 && waterGrid.get(i+1, j-1))
						|| (j > 0 && waterGrid.get(i, j-1)) || (j < m_MapSize-1 && waterGrid.get(i, j+1))
						)
					{	// If it's bordered by water, it's a shore tile
						shoreGrid.set(i, j, 0);
					}
					else
					{
						shoreGrid.set(i, j, shoreMax);
					}
				}
			}
		}

		// Expand influences on land to find shore distance
		for (u16 y = 0; y < m_MapSize; ++y)
		{
			u16 min = shoreMax;
			for (u16 x = 0; x < m_MapSize; ++x)
			{
				if (!waterGrid.get(x, y))
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
				if (!waterGrid.get(x-1, y))
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
				if (!waterGrid.get(x, y))
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
				if (!waterGrid.get(x, y-1))
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

		// Apply passability classes to terrain
		for (u16 j = 0; j < m_MapSize; ++j)
		{
			for (u16 i = 0; i < m_MapSize; ++i)
			{
				fixed x, z;
				TileCenter(i, j, x, z);

				TerrainTile t = 0;

				u8 obstruct = m_ObstructionGrid->get(i, j);

				fixed height = terrain.GetExactGroundLevelFixed(x, z);

				fixed water;
				if (cmpWaterManager)
					water = cmpWaterManager->GetWaterLevel(x, z);

				fixed depth = water - height;

				fixed slope = terrain.GetSlopeFixed(i, j);

				fixed shoredist = fixed::FromInt(shoreGrid.get(i, j));

				if (obstruct & ICmpObstructionManager::TILE_OBSTRUCTED_PATHFINDING)
					t |= 1;

				if (obstruct & ICmpObstructionManager::TILE_OBSTRUCTED_FOUNDATION)
					t |= 2;

				if (obstruct & ICmpObstructionManager::TILE_OUTOFBOUNDS)
				{
					// If out of bounds, nobody is allowed to pass
					for (size_t n = 0; n < m_PassClasses.size(); ++n)
						t |= m_PassClasses[n].m_Mask;
				}
				else
				{
					for (size_t n = 0; n < m_PassClasses.size(); ++n)
					{
						if (!m_PassClasses[n].IsPassable(depth, slope, shoredist))
							t |= m_PassClasses[n].m_Mask;
					}
				}

				std::string moveClass = terrain.GetMovementClass(i, j);
				if (m_TerrainCostClassTags.find(moveClass) != m_TerrainCostClassTags.end())
					t |= COST_CLASS_MASK(m_TerrainCostClassTags[moveClass]);

				m_Grid->set(i, j, t);
			}
		}

		m_TerrainDirty = false;

		++m_Grid->m_DirtyID;
	}
}

//////////////////////////////////////////////////////////

// Async path requests:

u32 CCmpPathfinder::ComputePathAsync(entity_pos_t x0, entity_pos_t z0, const Goal& goal, pass_class_t passClass, cost_class_t costClass, entity_id_t notify)
{
	AsyncLongPathRequest req = { m_NextAsyncTicket++, x0, z0, goal, passClass, costClass, notify };
	m_AsyncLongPathRequests.push_back(req);
	return req.ticket;
}

u32 CCmpPathfinder::ComputeShortPathAsync(entity_pos_t x0, entity_pos_t z0, entity_pos_t r, entity_pos_t range, const Goal& goal, pass_class_t passClass, bool avoidMovingUnits, entity_id_t group, entity_id_t notify)
{
	AsyncShortPathRequest req = { m_NextAsyncTicket++, x0, z0, r, range, goal, passClass, avoidMovingUnits, group, notify };
	m_AsyncShortPathRequests.push_back(req);
	return req.ticket;
}

void CCmpPathfinder::FinishAsyncRequests()
{
	// Save the request queue in case it gets modified while iterating
	std::vector<AsyncLongPathRequest> longRequests;
	m_AsyncLongPathRequests.swap(longRequests);

	std::vector<AsyncShortPathRequest> shortRequests;
	m_AsyncShortPathRequests.swap(shortRequests);

	// TODO: we should only compute one path per entity per turn

	// TODO: this computation should be done incrementally, spread
	// across multiple frames (or even multiple turns)

	ProcessLongRequests(longRequests);
	ProcessShortRequests(shortRequests);
}

void CCmpPathfinder::ProcessLongRequests(const std::vector<AsyncLongPathRequest>& longRequests)
{
	for (size_t i = 0; i < longRequests.size(); ++i)
	{
		const AsyncLongPathRequest& req = longRequests[i];
		Path path;
		ComputePath(req.x0, req.z0, req.goal, req.passClass, req.costClass, path);
		CMessagePathResult msg(req.ticket, path);
		GetSimContext().GetComponentManager().PostMessage(req.notify, msg);
	}
}

void CCmpPathfinder::ProcessShortRequests(const std::vector<AsyncShortPathRequest>& shortRequests)
{
	for (size_t i = 0; i < shortRequests.size(); ++i)
	{
		const AsyncShortPathRequest& req = shortRequests[i];
		Path path;
		ControlGroupMovementObstructionFilter filter(req.avoidMovingUnits, req.group);
		ComputeShortPath(filter, req.x0, req.z0, req.r, req.range, req.goal, req.passClass, path);
		CMessagePathResult msg(req.ticket, path);
		GetSimContext().GetComponentManager().PostMessage(req.notify, msg);
	}
}

void CCmpPathfinder::ProcessSameTurnMoves()
{
	if (!m_AsyncLongPathRequests.empty())
	{
		// Figure out how many moves we can do this time
		i32 moveCount = m_MaxSameTurnMoves - m_SameTurnMovesCount;

		if (moveCount <= 0)
			return;

		// Copy the long request elements we are going to process into a new array
		std::vector<AsyncLongPathRequest> longRequests;
		if ((i32)m_AsyncLongPathRequests.size() <= moveCount)
		{
			m_AsyncLongPathRequests.swap(longRequests);
			moveCount = (i32)longRequests.size();
		}
		else
		{
			longRequests.resize(moveCount);
			copy(m_AsyncLongPathRequests.begin(), m_AsyncLongPathRequests.begin() + moveCount, longRequests.begin());
			m_AsyncLongPathRequests.erase(m_AsyncLongPathRequests.begin(), m_AsyncLongPathRequests.begin() + moveCount);
		}

		ProcessLongRequests(longRequests);

		m_SameTurnMovesCount = (u16)(m_SameTurnMovesCount + moveCount);
	}
	
	if (!m_AsyncShortPathRequests.empty())
	{
		// Figure out how many moves we can do now
		i32 moveCount = m_MaxSameTurnMoves - m_SameTurnMovesCount;

		if (moveCount <= 0)
			return;

		// Copy the short request elements we are going to process into a new array
		std::vector<AsyncShortPathRequest> shortRequests;
		if ((i32)m_AsyncShortPathRequests.size() <= moveCount)
		{
			m_AsyncShortPathRequests.swap(shortRequests);
			moveCount = (i32)shortRequests.size();
		}
		else
		{
			shortRequests.resize(moveCount);
			copy(m_AsyncShortPathRequests.begin(), m_AsyncShortPathRequests.begin() + moveCount, shortRequests.begin());
			m_AsyncShortPathRequests.erase(m_AsyncShortPathRequests.begin(), m_AsyncShortPathRequests.begin() + moveCount);
		}

		ProcessShortRequests(shortRequests);

		m_SameTurnMovesCount = (u16)(m_SameTurnMovesCount + moveCount);
	}
}

ICmpObstruction::EFoundationCheck CCmpPathfinder::CheckUnitPlacement(const IObstructionTestFilter& filter,
	entity_pos_t x, entity_pos_t z, entity_pos_t r,	pass_class_t passClass)
{
	return CCmpPathfinder::CheckUnitPlacement(filter, x, z, r, passClass, false);
}

ICmpObstruction::EFoundationCheck CCmpPathfinder::CheckUnitPlacement(const IObstructionTestFilter& filter,
	entity_pos_t x, entity_pos_t z, entity_pos_t r,	pass_class_t passClass, bool onlyCenterPoint)
{
	// Check unit obstruction
	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
	if (!cmpObstructionManager)
		return ICmpObstruction::FOUNDATION_CHECK_FAIL_ERROR;

	if (cmpObstructionManager->TestUnitShape(filter, x, z, r, NULL))
		return ICmpObstruction::FOUNDATION_CHECK_FAIL_OBSTRUCTS_FOUNDATION;

	// Test against terrain:

	UpdateGrid();
	
	if (onlyCenterPoint)
	{
		u16 i, j;
		NearestTile(x , z, i, j);

		if (IS_TERRAIN_PASSABLE(m_Grid->get(i,j), passClass))
			return ICmpObstruction::FOUNDATION_CHECK_SUCCESS;

		return ICmpObstruction::FOUNDATION_CHECK_FAIL_TERRAIN_CLASS;
	}

	u16 i0, j0, i1, j1;
	NearestTile(x - r, z - r, i0, j0);
	NearestTile(x + r, z + r, i1, j1);
	for (u16 j = j0; j <= j1; ++j)
	{
		for (u16 i = i0; i <= i1; ++i)
		{
			if (!IS_TERRAIN_PASSABLE(m_Grid->get(i,j), passClass))
			{
				return ICmpObstruction::FOUNDATION_CHECK_FAIL_TERRAIN_CLASS;
			}
		}
	}
	return ICmpObstruction::FOUNDATION_CHECK_SUCCESS;
}

ICmpObstruction::EFoundationCheck CCmpPathfinder::CheckBuildingPlacement(const IObstructionTestFilter& filter,
	entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w,
	entity_pos_t h, entity_id_t id, pass_class_t passClass)
{
	return CCmpPathfinder::CheckBuildingPlacement(filter, x, z, a, w, h, id, passClass, false);
}


ICmpObstruction::EFoundationCheck CCmpPathfinder::CheckBuildingPlacement(const IObstructionTestFilter& filter,
	entity_pos_t x, entity_pos_t z, entity_pos_t a, entity_pos_t w,
	entity_pos_t h, entity_id_t id, pass_class_t passClass, bool onlyCenterPoint)
{
	// Check unit obstruction
	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
	if (!cmpObstructionManager)
		return ICmpObstruction::FOUNDATION_CHECK_FAIL_ERROR;

	if (cmpObstructionManager->TestStaticShape(filter, x, z, a, w, h, NULL))
		return ICmpObstruction::FOUNDATION_CHECK_FAIL_OBSTRUCTS_FOUNDATION;

	// Test against terrain:

	UpdateGrid();

	ICmpObstructionManager::ObstructionSquare square;
	CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), id);
	if (!cmpObstruction || !cmpObstruction->GetObstructionSquare(square))
		return ICmpObstruction::FOUNDATION_CHECK_FAIL_NO_OBSTRUCTION;

	if (onlyCenterPoint)
	{
		u16 i, j;
		NearestTile(x, z, i, j);

		if (IS_TERRAIN_PASSABLE(m_Grid->get(i,j), passClass))
			return ICmpObstruction::FOUNDATION_CHECK_SUCCESS;

		return ICmpObstruction::FOUNDATION_CHECK_FAIL_TERRAIN_CLASS;
	}

	// Expand bounds by 1/sqrt(2) tile (multiply by TERRAIN_TILE_SIZE since we want world coordinates)
	entity_pos_t expand = entity_pos_t::FromInt(2).Sqrt().Multiply(entity_pos_t::FromInt(TERRAIN_TILE_SIZE / 2));
	CFixedVector2D halfSize(square.hw + expand, square.hh + expand);
	CFixedVector2D halfBound = Geometry::GetHalfBoundingBox(square.u, square.v, halfSize);

	u16 i0, j0, i1, j1;
	NearestTile(square.x - halfBound.X, square.z - halfBound.Y, i0, j0);
	NearestTile(square.x + halfBound.X, square.z + halfBound.Y, i1, j1);
	for (u16 j = j0; j <= j1; ++j)
	{
		for (u16 i = i0; i <= i1; ++i)
		{
			entity_pos_t x, z;
			TileCenter(i, j, x, z);
			if (Geometry::PointIsInSquare(CFixedVector2D(x - square.x, z - square.z), square.u, square.v, halfSize)
				&& !IS_TERRAIN_PASSABLE(m_Grid->get(i,j), passClass))
			{
				return ICmpObstruction::FOUNDATION_CHECK_FAIL_TERRAIN_CLASS;
			}
		}
	}

	return ICmpObstruction::FOUNDATION_CHECK_SUCCESS;
}
