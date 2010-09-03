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
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpWaterManager.h"

// Default cost to move a single tile is a fairly arbitrary number, which should be big
// enough to be precise when multiplied/divided and small enough to never overflow when
// summing the cost of a whole path.
const int DEFAULT_MOVE_COST = 256;

REGISTER_COMPONENT_TYPE(Pathfinder)

void CCmpPathfinder::Init(const CSimContext& UNUSED(context), const CParamNode& paramNode)
{
	m_MapSize = 0;
	m_Grid = NULL;
	m_ObstructionGrid = NULL;
	m_TerrainDirty = true;
	m_NextAsyncTicket = 1;

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

void CCmpPathfinder::Deinit(const CSimContext& UNUSED(context))
{
	SetDebugOverlay(false); // cleans up memory
	ResetDebugPath();

	delete m_Grid;
	delete m_ObstructionGrid;
}

void CCmpPathfinder::HandleMessage(const CSimContext& context, const CMessage& msg, bool UNUSED(global))
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

void CCmpPathfinder::RenderSubmit(const CSimContext& UNUSED(context), SceneCollector& collector)
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

u8 CCmpPathfinder::GetPassabilityClass(const std::string& name)
{
	if (m_PassClassMasks.find(name) == m_PassClassMasks.end())
	{
		LOGERROR(L"Invalid passability class name '%hs'", name.c_str());
		return 0;
	}

	return m_PassClassMasks[name];
}

std::vector<std::string> CCmpPathfinder::GetPassabilityClasses()
{
	std::vector<std::string> classes;
	for (std::map<std::string, u8>::iterator it = m_PassClassMasks.begin(); it != m_PassClassMasks.end(); ++it)
		classes.push_back(it->first);
	return classes;
}

u8 CCmpPathfinder::GetCostClass(const std::string& name)
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

void CCmpPathfinder::UpdateGrid()
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

				u8 obstruct = m_ObstructionGrid->get(i, j);

				fixed height = terrain.GetVertexGroundLevelFixed(i, j); // TODO: should use tile centre

				fixed water;
				if (!cmpWaterMan.null())
					water = cmpWaterMan->GetWaterLevel(x, z);

				fixed depth = water - height;

				fixed slope = terrain.GetSlopeFixed(i, j);

				if (obstruct & ICmpObstructionManager::TILE_OBSTRUCTED)
					t |= 1;

				if (obstruct & ICmpObstructionManager::TILE_OUTOFBOUNDS)
				{
					// If out of bounds, nobody is allowed to pass
					for (size_t n = 0; n < m_PassClasses.size(); ++n)
						t |= (m_PassClasses[n].m_Mask & ~1);
				}
				else
				{
					for (size_t n = 0; n < m_PassClasses.size(); ++n)
					{
						if (!m_PassClasses[n].IsPassable(depth, slope))
							t |= (m_PassClasses[n].m_Mask & ~1);
					}
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

//////////////////////////////////////////////////////////

// Async path requests:

u32 CCmpPathfinder::ComputePathAsync(entity_pos_t x0, entity_pos_t z0, const Goal& goal, u8 passClass, u8 costClass, entity_id_t notify)
{
	AsyncLongPathRequest req = { m_NextAsyncTicket++, x0, z0, goal, passClass, costClass, notify };
	m_AsyncLongPathRequests.push_back(req);
	return req.ticket;
}

u32 CCmpPathfinder::ComputeShortPathAsync(entity_pos_t x0, entity_pos_t z0, entity_pos_t r, entity_pos_t range, const Goal& goal, u8 passClass, bool avoidMovingUnits, entity_id_t group, entity_id_t notify)
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

	for (size_t i = 0; i < longRequests.size(); ++i)
	{
		const AsyncLongPathRequest& req = longRequests[i];
		Path path;
		ComputePath(req.x0, req.z0, req.goal, req.passClass, req.costClass, path);
		CMessagePathResult msg(req.ticket, path);
		GetSimContext().GetComponentManager().PostMessage(req.notify, msg);
	}

	for (size_t i = 0; i < shortRequests.size(); ++i)
	{
		const AsyncShortPathRequest& req = shortRequests[i];
		Path path;
		ControlGroupObstructionFilter filter(req.avoidMovingUnits, req.group);
		ComputeShortPath(filter, req.x0, req.z0, req.r, req.range, req.goal, req.passClass, path);
		CMessagePathResult msg(req.ticket, path);
		GetSimContext().GetComponentManager().PostMessage(req.notify, msg);
	}
}
