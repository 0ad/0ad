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

#include "precompiled.h"

#include <vector>

#include "graphics/Model.h"
#include "graphics/Terrain.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "maths/MathUtil.h"
#include "network/NetMessage.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/GameAttributes.h"
#include "ps/Loader.h"
#include "ps/LoaderThunks.h"
#include "ps/Profile.h"
#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"
#include "simulation/Entity.h"
#include "simulation/EntityFormation.h"
#include "simulation/EntityManager.h"
#include "simulation/EntityTemplateCollection.h"
#include "simulation/LOSManager.h"
#include "simulation/Scheduler.h"
#include "simulation/Simulation.h"
#include "simulation/TerritoryManager.h"
#include "simulation/TriggerManager.h"

#define LOG_CATEGORY L"simulation"

CSimulation::CSimulation(CGame *pGame):
	m_pGame(pGame),
	m_pWorld(pGame->GetWorld()),
	m_DeltaTime(0),
	m_Time(0)
{
}

CSimulation::~CSimulation()
{
}

int CSimulation::Initialize(CGameAttributes* pAttribs)
{
	m_Random.seed(0);		// TODO: Store a random seed in CGameAttributes and synchronize it accross the network

	// Call the game startup script 
	// TODO: Maybe don't do this if we're in Atlas
	// [2006-06-26 20ms]
	g_ScriptingHost.RunScript( L"scripts/game_startup.js" );

	// [2006-06-26 3647ms]
//	g_EntityManager.m_screenshotMode = pAttribs->m_ScreenshotMode;
//	g_EntityManager.InitializeAll();

	// [2006-06-26: 61ms]
	m_pWorld->GetLOSManager()->Initialize((ELOSSetting)pAttribs->m_LOSSetting, pAttribs->m_FogOfWar);

	m_pWorld->GetTerritoryManager()->Initialize();

	return 0;
}


void CSimulation::RegisterInit(CGameAttributes *pAttribs)
{
	RegMemFun1(this, &CSimulation::Initialize, pAttribs, L"CSimulation", 3900);
}



bool CSimulation::Update(double UNUSED(frameTime))
{
	bool ok = true;
	return ok;
}

void CSimulation::DiscardMissedUpdates()
{
	if (m_DeltaTime > 0.0)
		m_DeltaTime = 0.0;
}

void CSimulation::Interpolate(double UNUSED(frameTime))
{
}

void CSimulation::Interpolate(double frameTime, double offset)
{
	PROFILE( "simulation interpolation" );

	const std::vector<CUnit*>& units = m_pWorld->GetUnitManager().GetUnits();
	for (size_t i = 0; i < units.size(); ++i)
		units[i]->UpdateModel((float)frameTime);

//	g_EntityManager.InterpolateAll(offset);
}

void CSimulation::Simulate()
{
}

// Location randomizer, for group orders...
// Having the group turn up at the destination with /some/ sort of cohesion is
// good but tasking them all to the exact same point will leave them brawling
// for it at the other end (it shouldn't, but the PASAP pathfinder is too
// simplistic)

// Task them all to a point within a radius of the target, radius depends upon
// the number of units in the group.

void RandomizeLocations(const CEntityOrder& order, const std::vector<HEntity> &entities, bool isQueued)
{
	float radius = 2.0f * sqrt( (float)entities.size() - 1 ); 

	CSimulation* sim = g_Game->GetSimulation();

	for (std::vector<HEntity>::const_iterator it = entities.begin(); it < entities.end(); it++)
	{
		if(!*it)
			continue;

		float _x, _y;
		CEntityOrder randomizedOrder = order;
		
		do
		{
			_x = sim->RandFloat() * 2.0f - 1.0f;
			_y = sim->RandFloat() * 2.0f - 1.0f;
		}
		while( ( _x * _x ) + ( _y * _y ) > 1.0f );

		randomizedOrder.m_target_location.x += _x * radius;
		randomizedOrder.m_target_location.y += _y * radius;

		// Clamp it to within the map, just in case.
		float mapsize = (float)g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide() * CELL_SIZE;
		randomizedOrder.m_target_location.x = clamp(randomizedOrder.m_target_location.x, 0.0f, mapsize);
		randomizedOrder.m_target_location.y = clamp(randomizedOrder.m_target_location.y, 0.0f, mapsize);

		if( !isQueued )
			(*it)->ClearOrders();

		(*it)->PushOrder( randomizedOrder );
	}
}

void FormationLocations(const CEntityOrder& order, const std::vector<HEntity> &entities, bool isQueued)
{
	const CVector2D upvec(0.0f, 1.0f);
	const CEntityFormation* formation = entities.front()->GetFormation();

	for (std::vector<HEntity>::const_iterator it = entities.begin(); it != entities.end(); it++)
	{
		if(!*it)
			continue;

		CEntityOrder orderCopy = order;
		CVector2D posDelta = orderCopy.m_target_location - formation->GetPosition();
		CVector2D formDelta = formation->GetSlotPosition( (*it)->m_formationSlot );
		
		posDelta = posDelta.Normalize();
		//Rotate the slot position's offset vector according to the rotation of posDelta.
		CVector2D rotDelta;
		float deltaCos = posDelta.Dot(upvec);
		float deltaSin = sinf( acosf(deltaCos) );
		rotDelta.x = formDelta.x * deltaCos - formDelta.y * deltaSin;
		rotDelta.y = formDelta.x * deltaSin + formDelta.y * deltaCos; 

		orderCopy.m_target_location += rotDelta;

		// Clamp it to within the map, just in case.
		float mapsize = (float)g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide() * CELL_SIZE;
		orderCopy.m_target_location.x = clamp(orderCopy.m_target_location.x, 0.0f, mapsize);
		orderCopy.m_target_location.y = clamp(orderCopy.m_target_location.y, 0.0f, mapsize);

		if( !isQueued )
			(*it)->ClearOrders();

		(*it)->PushOrder( orderCopy );
	}
}

void QueueOrder(const CEntityOrder& order, const std::vector<HEntity> &entities, bool isQueued)
{
	for (std::vector<HEntity>::const_iterator it = entities.begin(); it < entities.end(); it++)
	{
		if (!*it)
			continue;

		if( !isQueued )
			(*it)->ClearOrders();

		(*it)->PushOrder( order );
	}
}

size_t CSimulation::TranslateMessage(CNetMessage* pMsg, size_t clientMask, void* UNUSED(userdata))
{
	return clientMask;
}

size_t CSimulation::GetMessageMask(CNetMessage* UNUSED(pMsg), size_t UNUSED(oldMask), void* UNUSED(userdata))
{
	//CSimulation *pSimulation=(CSimulation *)userdata;

	// Pending a complete visibility/minimal-update implementation, we'll
	// simply select the first 32 connected clients ;-)
	return 0xFFFFFFFF;
}

void CSimulation::QueueLocalCommand(CNetMessage *pMsg)
{
}


// Get a random integer between 0 and maxVal-1 from the simulation's random number generator
int CSimulation::RandInt(int maxVal) 
{
	boost::uniform_smallint<int> distr(0, maxVal-1);
	return distr(m_Random);
}

// Get a random float in [0, 1) from the simulation's random number generator
float CSimulation::RandFloat() 
{
	// Cannot use uniform_01 here because it is not a real distribution, but rather an
	// utility class that makes a copy of the generator, and therefore it would repeatedly
	// return the same values because it never modifies our copy of the generator.
	boost::uniform_real<float> distr(0.0f, 1.0f);
	return distr(m_Random);
}
