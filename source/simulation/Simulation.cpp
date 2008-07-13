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
#include "simulation/Projectile.h"
#include "simulation/Scheduler.h"
#include "simulation/Simulation.h"
#include "simulation/TerritoryManager.h"
#include "simulation/TurnManager.h"
#include "simulation/TriggerManager.h"

CSimulation::CSimulation(CGame *pGame):
	m_pGame(pGame),
	m_pWorld(pGame->GetWorld()),
	m_pTurnManager((g_SinglePlayerTurnManager=new CSinglePlayerTurnManager())),
	m_DeltaTime(0),
	m_Time(0)
{
}

CSimulation::~CSimulation()
{
delete g_SinglePlayerTurnManager;
	g_SinglePlayerTurnManager=NULL;
}

int CSimulation::Initialize(CGameAttributes* pAttribs)
{
	m_Random.seed(0);		// TODO: Store a random seed in CGameAttributes and synchronize it accross the network

	m_pTurnManager->Initialize(m_pGame->GetNumPlayers());

	// Call the game startup script 
	// TODO: Maybe don't do this if we're in Atlas
	// [2006-06-26 20ms]
	g_ScriptingHost.RunScript( "scripts/game_startup.js" );

	// [2006-06-26 3647ms]
	g_EntityManager.m_screenshotMode = pAttribs->m_ScreenshotMode;
	g_EntityManager.InitializeAll();

	// [2006-06-26: 61ms]
	m_pWorld->GetLOSManager()->Initialize(pAttribs->m_LOSSetting, pAttribs->m_FogOfWar);

	m_pWorld->GetTerritoryManager()->Initialize();

	return 0;
}


void CSimulation::RegisterInit(CGameAttributes *pAttribs)
{
	RegMemFun1(this, &CSimulation::Initialize, pAttribs, L"CSimulation", 3900);
}



bool CSimulation::Update(double frameTime)
{
	bool ok = true;

	m_DeltaTime += frameTime;
	
	if (m_DeltaTime >= 0.0)
	{
		// A new simulation frame is required.
		if (m_pTurnManager->NewTurnReady())
		{
			PROFILE( "simulation turn" );
			Simulate();
			double turnLength = m_pTurnManager->GetTurnLength() / 1000.0;
			m_DeltaTime -= turnLength;
			if (m_DeltaTime >= 0.0)
			{
				// The desired sim frame rate can't be achieved - we're being called
				// with average(frameTime) > turnLength.
				// Let the caller know we can't go fast enough - they should try
			// cutting down on Interpolate and rendering, and call us a few times
				// with frameTime == 0 to give us a chance to catch up.
				ok = false;
				debug_printf("WARNING: missing a simulation turn due to low FPS\n");
			}
		}
		else
		{
			// The network is lagging behind the simulation rate. 
			// Set delta time back to zero so we don't jump into the middle
			// of the next simulation frame when we get the next turn.
			// This creates "lag" on the client rather than just jumpiness.
			m_DeltaTime = 0;
		}
	}

	return ok;
}

void CSimulation::DiscardMissedUpdates()
{
	if (m_DeltaTime > 0.0)
		m_DeltaTime = 0.0;
}

void CSimulation::Interpolate(double frameTime)
{
	double turnLength = m_pTurnManager->GetTurnLength()/1000.0;

	// 'offset' should be how far we are between the previous and next
	// simulation frames.
	// m_DeltaTime/turnLength will usually be between -1 and 0, indicating
	// the time until the next frame, so we can use that easily.
	// If the simulation is going too slowly and hasn't been giving a chance
	// to catch up before Interpolate is called, then m_DeltaTime > 0, so we'll
	// just clamp it to offset=1, which is alright.
	Interpolate(frameTime, clamp(m_DeltaTime / turnLength + 1.0, 0.0, 1.0));
}

void CSimulation::Interpolate(double frameTime, double offset)
{
	PROFILE( "simulation interpolation" );

	const std::vector<CUnit*>& units = m_pWorld->GetUnitManager().GetUnits();
	for (size_t i = 0; i < units.size(); ++i)
		units[i]->UpdateModel((float)frameTime);

	g_EntityManager.InterpolateAll(offset);
	m_pWorld->GetProjectileManager().InterpolateAll(offset);
	g_Renderer.GetWaterManager()->m_WaterTexTimer += frameTime;
}

void CSimulation::Simulate()
{
	int time = m_pTurnManager->GetTurnLength();
	
	m_Time += time / 1000.0f;
#if defined(DEBUG_SYNCHRONIZATION)
	debug_printf("Simulation turn: %.3lf\n", m_Time);
#endif

	PROFILE_START( "scheduler tick" );
	g_Scheduler.Update(time);
	PROFILE_END( "scheduler tick" );
	PROFILE_START( "entity updates" );
	g_EntityManager.UpdateAll(time);
	PROFILE_END( "entity updates" );

	PROFILE_START( "projectile updates" );
	m_pWorld->GetProjectileManager().UpdateAll(time);
	PROFILE_END( "projectile updates" );

	PROFILE_START( "los update" );
	m_pWorld->GetLOSManager()->Update();
	PROFILE_END( "los update" );

	PROFILE_START("trigger update");
	g_TriggerManager.Update(time);
	PROFILE_END("trigger udpate");

	PROFILE_START( "turn manager update" );
	m_pTurnManager->NewTurn();
	m_pTurnManager->IterateBatch(0, TranslateMessage, this);
	PROFILE_END( "turn manager update" );
	
#if defined(DEBUG_SYNCHRONIZATION)
	debug_printf("End turn\n", m_Time);
#endif
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
	std::vector<HEntity>::const_iterator it;
	float radius = 2.0f * sqrt( (float)entities.size() - 1 ); 

	for (it = entities.begin(); it < entities.end(); it++)
	{
		float _x, _y;
		CEntityOrder randomizedOrder = order;
		
		CSimulation* sim = g_Game->GetSimulation();
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
	CVector2D upvec(0.0f, 1.0f);
	std::vector<HEntity>::const_iterator it = entities.begin();
	CEntityFormation* formation = (*it)->GetFormation();


	for (; it != entities.end(); it++)
	{
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
	std::vector<HEntity>::const_iterator it;

	for (it = entities.begin(); it < entities.end(); it++)
	{
		if( !isQueued )
			(*it)->ClearOrders();

		(*it)->PushOrder( order );
	}
}

size_t CSimulation::TranslateMessage(CNetMessage* pMsg, size_t clientMask, void* UNUSED(userdata))
{
	CEntityOrder order;
	bool isQueued = true;
	
#define ENTITY_POSITION(_msg, _order) \
	do { \
		_msg *msg=(_msg *)pMsg; \
		isQueued = msg->m_IsQueued != 0; \
		order.m_type=CEntityOrder::_order; \
		order.m_target_location.x=(float)msg->m_TargetX; \
		order.m_target_location.y=(float)msg->m_TargetY; \
		RandomizeLocations(order, msg->m_Entities, isQueued); \
	} while(0)
#define ENTITY_POSITION_FORM(_msg, _order) \
	do { \
		_msg *msg=(_msg *)pMsg; \
		isQueued = msg->m_IsQueued != 0; \
		order.m_type=CEntityOrder::_order; \
		order.m_target_location.x=(float)msg->m_TargetX; \
		order.m_target_location.y=(float)msg->m_TargetY; \
		FormationLocations(order, msg->m_Entities, isQueued); \
	} while(0)
#define ENTITY_ENTITY_INT(_msg, _order) \
	do { \
		_msg *msg=(_msg *)pMsg; \
		isQueued = msg->m_IsQueued != 0; \
		order.m_type=CEntityOrder::_order; \
		order.m_target_entity=msg->m_Target; \
		order.m_action=msg->m_Action; \
		QueueOrder(order, msg->m_Entities, isQueued); \
	} while(0)
#define ENTITY_ENTITY_INT_BOOL(_msg, _order) \
	do { \
		_msg *msg=(_msg *)pMsg; \
		isQueued = msg->m_IsQueued != 0; \
		order.m_type=CEntityOrder::_order; \
		order.m_target_entity=msg->m_Target; \
		order.m_action=msg->m_Action; \
		order.m_run=msg->m_Run != 0; \
		QueueOrder(order, msg->m_Entities, isQueued); \
	} while(0)
#define ENTITY_INT_STRING(_msg, _order) \
	do { \
		_msg *msg=(_msg *)pMsg; \
		isQueued = msg->m_IsQueued != 0; \
		order.m_type=CEntityOrder::_order; \
		order.m_produce_name=msg->m_Name; \
		order.m_produce_type=msg->m_Type; \
		QueueOrder(order, msg->m_Entities, isQueued); \
	} while(0)
	
	switch (pMsg->GetType())
	{
		case NMT_ADD_WAYPOINT:
		{
			CAddWaypointMessage *msg=(CAddWaypointMessage *)pMsg;
			isQueued = msg->m_IsQueued != 0;
			order.m_type=CEntityOrder::ORDER_LAST;
			order.m_target_location.x=(float)msg->m_TargetX;
			order.m_target_location.y=(float)msg->m_TargetY;
			for(CEntityIt it = msg->m_Entities.begin(); it != msg->m_Entities.end(); ++it)
			{
				HEntity& hentity = *it;

				const CEntityOrders& order_queue = hentity->m_orderQueue;
				for(CEntityOrderCRIt ord_it = order_queue.rbegin(); ord_it != order_queue.rend(); ++ord_it)
				{
					if (ord_it->m_type == CEntityOrder::ORDER_PATH_END_MARKER)
					{
						order.m_type = CEntityOrder::ORDER_GOTO;
						hentity->PushOrder(order);
						break;
					}
					if (ord_it->m_type == CEntityOrder::ORDER_PATROL)
					{
						order.m_type = ord_it->m_type;
						hentity->PushOrder(order);
						break;
					}
				}
				if (order.m_type == CEntityOrder::ORDER_LAST)
				{
					LOG(CLogger::Error, "simulation", "Got an AddWaypoint message for an entity that isn't moving.");
				}
			}
			break;
		}
		case NMT_GOTO:
			ENTITY_POSITION(CGotoMessage, ORDER_GOTO);
			break;
		case NMT_RUN:
			ENTITY_POSITION(CRunMessage, ORDER_RUN);
			break;
		case NMT_PATROL:
			ENTITY_POSITION(CPatrolMessage, ORDER_PATROL);
			break;
		case NMT_FORMATION_GOTO:
			ENTITY_POSITION_FORM(CFormationGotoMessage, ORDER_GOTO);
			break;

		//TODO: make formation move to within range of target and then attack normally
		case NMT_GENERIC:
			ENTITY_ENTITY_INT_BOOL(CGenericMessage, ORDER_GENERIC);
			break;
		case NMT_FORMATION_GENERIC:
			ENTITY_ENTITY_INT(CFormationGenericMessage, ORDER_GENERIC);
			break;
		case NMT_NOTIFY_REQUEST:
			ENTITY_ENTITY_INT(CNotifyRequestMessage, ORDER_NOTIFY_REQUEST);
			break;
		case NMT_PRODUCE:
			ENTITY_INT_STRING(CProduceMessage, ORDER_PRODUCE);
			break;
		case NMT_PLACE_OBJECT:
			{
				CPlaceObjectMessage *msg = (CPlaceObjectMessage *) pMsg;
				isQueued = msg->m_IsQueued != 0;
				
				// Figure out the player
				CPlayer* player = 0;
				if(msg->m_Entities.size() > 0) 
					player = msg->m_Entities[0]->GetPlayer();
				else
					player = g_Game->GetLocalPlayer();

				// Create the object
				CVector3D pos(msg->m_X/1000.0f, msg->m_Y/1000.0f, msg->m_Z/1000.0f);
				HEntity newObj = g_EntityManager.CreateFoundation( msg->m_Template, player, pos, msg->m_Angle/1000.0f );
				newObj->m_actor->SetPlayerID(player->GetPlayerID());
				if( newObj->Initialize() )
				{
					// Order all the selected units to work on the new object using the given action
					order.m_type = CEntityOrder::ORDER_START_CONSTRUCTION;
					order.m_new_obj = newObj;
					QueueOrder(order, msg->m_Entities, isQueued);
				}
			}
			break;
		
	}

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
	m_pTurnManager->QueueLocalCommand(pMsg);
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
