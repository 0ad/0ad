#include "precompiled.h"

#include <vector>

#include "EntityFormation.h"
#include "EntityManager.h"
#include "LOSManager.h"
#include "Projectile.h"
#include "Scheduler.h"
#include "Simulation.h"
#include "TurnManager.h"
#include "graphics/Model.h"
#include "graphics/Terrain.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "lib/timer.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/GameAttributes.h"
#include "ps/Loader.h"
#include "ps/LoaderThunks.h"
#include "ps/Network/NetMessage.h"
#include "ps/Profile.h"
#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"
#include "simulation/Entity.h"

#include "gui/CGUI.h"

using namespace std;

extern CConsole *g_Console;

CSimulation::CSimulation(CGame *pGame):
	m_pGame(pGame),
	m_pWorld(pGame->GetWorld()),
	m_pTurnManager((g_SinglePlayerTurnManager=new CSinglePlayerTurnManager())),
	m_DeltaTime(0)
{}

CSimulation::~CSimulation()
{
	delete g_SinglePlayerTurnManager;
	g_SinglePlayerTurnManager=NULL;
}

int CSimulation::Initialize(CGameAttributes* pAttribs)
{
	m_pTurnManager->Initialize(m_pGame->GetNumPlayers());

	// Call the game startup script 
	// TODO: Maybe don't do this if we're in Atlas
	// [2006-06-26 20ms]
	g_ScriptingHost.RunScript( "scripts/game_startup.js" );

	// [2006-06-26 3647ms]
	g_EntityManager.InitializeAll();

	// [2006-06-26: 61ms]
	m_pWorld->GetLOSManager()->Initialize(pAttribs->m_LOSSetting);

	return 0;
}


void CSimulation::RegisterInit(CGameAttributes *pAttribs)
{
	RegMemFun1(this, &CSimulation::Initialize, pAttribs, L"CSimulation", 3900);
}



void CSimulation::Update(double frameTime)
{
	m_DeltaTime += frameTime;
	
	if( m_DeltaTime >= 0.0 && frameTime )
	{
		PROFILE( "simulation turn" );
		// A new simulation frame is required.
		MICROLOG( L"calculate simulation" );
		Simulate();
		m_DeltaTime -= (m_pTurnManager->GetTurnLength()/1000.0);
		if( m_DeltaTime >= 0.0 )
		{
			// The desired sim frame rate can't be achieved. Settle for process & render
			// frames as fast as possible.
			frameTime -= m_DeltaTime; // so the animation stays in sync with the sim
			m_DeltaTime = 0.0;
		}
	}

	PROFILE_START( "simulation interpolation" );
	Interpolate(frameTime, ((1000.0*m_DeltaTime) / (float)m_pTurnManager->GetTurnLength()) + 1.0);
	PROFILE_END( "simulation interpolation" );
}

void CSimulation::Interpolate(double frameTime, double offset)
{
	const std::vector<CUnit*>& units=m_pWorld->GetUnitManager()->GetUnits();
	for (uint i=0;i<units.size();++i)
		units[i]->GetModel()->Update((float)frameTime);

	g_EntityManager.interpolateAll( (float)offset );
	g_ProjectileManager.InterpolateAll( (float)offset );
	g_Renderer.GetWaterManager()->m_WaterTexTimer += frameTime;
}

void CSimulation::Simulate()
{
	PROFILE_START( "scheduler tick" );
	g_Scheduler.update(m_pTurnManager->GetTurnLength());
	PROFILE_END( "scheduler tick" );

	PROFILE_START( "entity updates" );
	g_EntityManager.updateAll( m_pTurnManager->GetTurnLength() );
	PROFILE_END( "entity updates" );

	PROFILE_START( "projectile updates" );
	g_ProjectileManager.UpdateAll( m_pTurnManager->GetTurnLength() );
	PROFILE_END( "projectile updates" );

	PROFILE_START( "los update" );
	m_pWorld->GetLOSManager()->Update();
	PROFILE_END( "los update" );

	PROFILE_START( "turn manager update" );
	m_pTurnManager->NewTurn();
	m_pTurnManager->IterateBatch(0, TranslateMessage, this);
	PROFILE_END( "turn manager update" );
}

// Location randomizer, for group orders...
// Having the group turn up at the destination with /some/ sort of cohesion is
// good but tasking them all to the exact same point will leave them brawling
// for it at the other end (it shouldn't, but the PASAP pathfinder is too
// simplistic)

// Task them all to a point within a radius of the target, radius depends upon
// the number of units in the group.

void RandomizeLocations(CEntityOrder order, const vector <HEntity> &entities, bool clearQueue)
{
	vector<HEntity>::const_iterator it;
	float radius = 2.0f * sqrt( (float)entities.size() - 1 ); 

	for (it = entities.begin(); it < entities.end(); it++)
	{
		float _x, _y;
		CEntityOrder randomizedOrder = order;
		
		do
		{
			_x = (float)( rand() % 20000 ) / 10000.0f - 1.0f;
			_y = (float)( rand() % 20000 ) / 10000.0f - 1.0f;
		}
		while( ( _x * _x ) + ( _y * _y ) > 1.0f );

		randomizedOrder.m_data[0].location.x += _x * radius;
		randomizedOrder.m_data[0].location.y += _y * radius;

		// Clamp it to within the map, just in case.
		float mapsize = (float)g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide() * CELL_SIZE;

		if( randomizedOrder.m_data[0].location.x < 0.0f )
			randomizedOrder.m_data[0].location.x = 0.0f;
		if( randomizedOrder.m_data[0].location.x >= mapsize )
			randomizedOrder.m_data[0].location.x = mapsize;
		if( randomizedOrder.m_data[0].location.y < 0.0f )
			randomizedOrder.m_data[0].location.y = 0.0f;
		if( randomizedOrder.m_data[0].location.y >= mapsize )
			randomizedOrder.m_data[0].location.y = mapsize;

		if( clearQueue )
			(*it)->clearOrders();

		(*it)->pushOrder( randomizedOrder );
	}
}

void FormationLocations(CEntityOrder order, const vector <HEntity> &entities, bool clearQueue)
{
	CVector2D upvec(0.0f, 1.0f);
	vector<HEntity>::const_iterator it = entities.begin();
	CEntityFormation* formation = (*it)->GetFormation();


	for (; it != entities.end(); it++)
	{
		CEntityOrder orderCopy = order;
		CVector2D posDelta = orderCopy.m_data[0].location - formation->GetPosition();
		CVector2D formDelta = formation->GetSlotPosition( (*it)->m_formationSlot );
		
		posDelta = posDelta.normalize();
		//Rotate the slot position's offset vector according to the rotation of posDelta.
		CVector2D rotDelta;
		float deltaCos = posDelta.dot(upvec);
		float deltaSin = sinf( acosf(deltaCos) );
		rotDelta.x = formDelta.x * deltaCos - formDelta.y * deltaSin;
		rotDelta.y = formDelta.x * deltaSin + formDelta.y * deltaCos; 

		orderCopy.m_data[0].location += rotDelta;

		// Clamp it to within the map, just in case.
		float mapsize = (float)g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide() * CELL_SIZE;

		if( orderCopy.m_data[0].location.x < 0.0f )
			orderCopy.m_data[0].location.x = 0.0f;
		if( orderCopy.m_data[0].location.x >= mapsize )
			orderCopy.m_data[0].location.x = mapsize;
		if( orderCopy.m_data[0].location.y < 0.0f )
			orderCopy.m_data[0].location.y = 0.0f;
		if( orderCopy.m_data[0].location.y >= mapsize )
			orderCopy.m_data[0].location.y = mapsize;

		if( clearQueue )
			(*it)->clearOrders();

		(*it)->pushOrder( orderCopy );
	}
}

void QueueOrder(CEntityOrder order, const vector <HEntity> &entities, bool clearQueue)
{
	vector<HEntity>::const_iterator it;

	for (it = entities.begin(); it < entities.end(); it++)
	{
		if( clearQueue )
			(*it)->clearOrders();

		(*it)->pushOrder( order );
	}
}

uint CSimulation::TranslateMessage(CNetMessage* pMsg, uint clientMask, void* UNUSED(userdata))
{
	CEntityOrder order;
	bool clearQueue = true;
	
#define ENTITY_POSITION(_msg, _order) do\
	{ \
		_msg *msg=(_msg *)pMsg; \
		order.m_type=CEntityOrder::_order; \
		order.m_data[0].location.x=(float)msg->m_TargetX; \
		order.m_data[0].location.y=(float)msg->m_TargetY; \
		RandomizeLocations(order, msg->m_Entities, clearQueue); \
	} while(0)
#define ENTITY_POSITION_FORM(_msg, _order) do\
	{ \
		_msg *msg=(_msg *)pMsg; \
		order.m_type=CEntityOrder::_order; \
		order.m_data[0].location.x=(float)msg->m_TargetX; \
		order.m_data[0].location.y=(float)msg->m_TargetY; \
		FormationLocations(order, msg->m_Entities, clearQueue); \
	} while(0)
#define ENTITY_ENTITY(_msg, _order) do\
	{ \
		_msg *msg=(_msg *)pMsg; \
		order.m_type=CEntityOrder::_order; \
		order.m_data[0].entity=msg->m_Target; \
		QueueOrder(order, msg->m_Entities, clearQueue); \
	} while(0)
#define ENTITY_ENTITY_INT(_msg, _order) do\
	{ \
		_msg *msg=(_msg *)pMsg; \
		order.m_type=CEntityOrder::_order; \
		order.m_data[0].entity=msg->m_Target; \
		order.m_data[1].data=msg->m_Action; \
		QueueOrder(order, msg->m_Entities, clearQueue); \
	} while(0)
#define ENTITY_INT_STRING(_msg, _order) do\
	{ \
		_msg *msg=(_msg *)pMsg; \
		order.m_type=CEntityOrder::_order; \
		order.m_data[0].string=msg->m_Name; \
		order.m_data[1].data=msg->m_Type; \
		QueueOrder(order, msg->m_Entities, clearQueue); \
	} while(0)
	
	switch (pMsg->GetType())
	{
		case NMT_AddWaypoint:
		{
			CAddWaypoint *msg=(CAddWaypoint *)pMsg;
			order.m_type=CEntityOrder::ORDER_LAST;
			order.m_data[0].location.x=(float)msg->m_TargetX;
			order.m_data[0].location.y=(float)msg->m_TargetY;
			vector<HEntity>::iterator it = msg->m_Entities.begin(); 
			for (;it != msg->m_Entities.end(); ++it)
			{
				deque<CEntityOrder>::const_iterator ord_it;
				ord_it=(*it)->m_orderQueue.end() - 1;
				for (;ord_it >= (*it)->m_orderQueue.begin();--ord_it)
				{
					if (ord_it->m_type == CEntityOrder::ORDER_PATH_END_MARKER)
					{
						order.m_type = CEntityOrder::ORDER_GOTO;
						(*it)->pushOrder(order);
						break;
					}
					if (ord_it->m_type == CEntityOrder::ORDER_PATROL)
					{
						order.m_type = ord_it->m_type;
						(*it)->pushOrder(order);
						break;
					}
				}
				if (order.m_type == CEntityOrder::ORDER_LAST)
				{
					LOG(ERROR, "simulation", "Got an AddWaypoint message for an entity that isn't moving.");
				}
			}
			break;
		}
		case NMT_Goto:
			ENTITY_POSITION(CGoto, ORDER_GOTO);
			break;
		case NMT_FormationGoto:
			ENTITY_POSITION_FORM(CFormationGoto, ORDER_GOTO);
			break;
		//TODO: make formation move to within range of target and then attack normally
		case NMT_FormationGeneric:
			ENTITY_ENTITY_INT(CFormationGeneric, ORDER_GENERIC);
			break;
		case NMT_Run:
			ENTITY_POSITION(CRun, ORDER_RUN);
			break;
		case NMT_Patrol:
			ENTITY_POSITION(CPatrol, ORDER_PATROL);
			break;
		case NMT_Generic:
			ENTITY_ENTITY_INT(CGeneric, ORDER_GENERIC);
			break;
		case NMT_Produce:
			ENTITY_INT_STRING(CProduce, ORDER_PRODUCE);
			break;
		case NMT_NotifyRequest:
			ENTITY_ENTITY_INT(CNotifyRequest, ORDER_NOTIFY_REQUEST);
			break;
		case NMT_PlaceObject:
			{
				CPlaceObject *msg = (CPlaceObject *) pMsg;
				
				// Figure out the player
				CPlayer* player = 0;
				if(msg->m_Entities.size() > 0) 
					player = msg->m_Entities[0]->GetPlayer();
				else
					player = g_Game->GetLocalPlayer();

				// Create the object
				CVector3D pos(msg->m_X/1000.0f, msg->m_Y/1000.0f, msg->m_Z/1000.0f);
				HEntity newObj = g_EntityManager.createFoundation( msg->m_Template, player, pos, msg->m_Angle/1000.0f );
				newObj->SetPlayer(player);
				if( newObj->Initialize() )
				{
					// Order all the selected units to work on the new object using the given action
					order.m_type = CEntityOrder::ORDER_START_CONSTRUCTION;
					order.m_data[0].entity = newObj;
					QueueOrder(order, msg->m_Entities, false);
				}
			} while(0)
			break;
		
	}

	return clientMask;
}

uint CSimulation::GetMessageMask(CNetMessage* UNUSED(pMsg), uint UNUSED(oldMask), void* UNUSED(userdata))
{
	//CSimulation *pSimulation=(CSimulation *)userdata;

	// Pending a complete visibility/minimal-update implementation, we'll
	// simply select the first 32 connected clients ;-)
	return 0xffffffff;
}

void CSimulation::QueueLocalCommand(CNetMessage *pMsg)
{
	m_pTurnManager->QueueLocalCommand(pMsg);
}
