#include "precompiled.h"

#include <timer.h>

#include "Simulation.h"
#include "TurnManager.h"
#include "Game.h"
#include "EntityManager.h"
#include "Scheduler.h"
#include "Network/NetMessage.h"
#include "CLogger.h"
#include "CConsole.h"
#include "Unit.h"
#include "Model.h"
#include "Loader.h"

#include "gui/CGUI.h"

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

void CSimulation::Initialize(CGameAttributes *pAttribs)
{
	m_pTurnManager->Initialize(m_pGame->GetNumPlayers());

	g_EntityManager.InitializeAll();
}


struct ThunkParams
{
	CSimulation* const this_;
	CGameAttributes* const pAttribs;
	ThunkParams(CSimulation* this__, CGameAttributes* pAttribs_)
		: this_(this__), pAttribs(pAttribs_) {}
};

static int LoadThunk(void* param, double time_left)
{
	const ThunkParams* p = (const ThunkParams*)param;
	CSimulation* const this_        = p->this_;
	CGameAttributes* const pAttribs = p->pAttribs;

	this_->Initialize(pAttribs);
	delete p;
	return 0;
}

void CSimulation::RegisterInit(CGameAttributes *pAttribs)
{
	void* param = new ThunkParams(this, pAttribs);
	THROW_ERR(LDR_Register(LoadThunk, param, L"CSimulation", 1000));
}



void CSimulation::Update(double frameTime)
{
	m_DeltaTime += frameTime;
	
	if( m_DeltaTime >= 0.0 )
	{
		// A new simulation frame is required.
		MICROLOG( L"calculate simulation" );
		Simulate();
		m_DeltaTime -= (m_pTurnManager->GetTurnLength()/1000.0);
		if( m_DeltaTime >= 0.0 )
		{
			// The desired sim frame rate can't be achieved. Settle for process & render
			// frames as fast as possible.
			m_DeltaTime = 0.0;
		}
	}

	Interpolate(frameTime, ((1000.0*m_DeltaTime) / (float)m_pTurnManager->GetTurnLength()) + 1.0);
}

void CSimulation::Interpolate(double frameTime, double offset)
{
	const std::vector<CUnit*>& units=m_pWorld->GetUnitManager()->GetUnits();
	for (uint i=0;i<units.size();++i)
		units[i]->GetModel()->Update((float)frameTime);

	g_EntityManager.interpolateAll((float)offset);
}

void CSimulation::Simulate()
{
	g_Scheduler.update(m_pTurnManager->GetTurnLength());
	g_EntityManager.updateAll( m_pTurnManager->GetTurnLength() );

	m_pTurnManager->NewTurn();
	m_pTurnManager->IterateBatch(0, TranslateMessage, this);
}

uint CSimulation::TranslateMessage(CNetMessage *pMsg, uint clientMask, void *userdata)
{
	CSimulation *pSimulation=(CSimulation *)userdata;

	CEntityOrder entOrder;
	switch (pMsg->GetType())
	{
	case NMT_GotoCommand:
		CGotoCommand *msg=(CGotoCommand *)pMsg;
		entOrder.m_type=CEntityOrder::ORDER_GOTO;
		entOrder.m_data[0].location.x=(float)msg->m_TargetX;
		entOrder.m_data[0].location.y=(float)msg->m_TargetY;
		CEntity *ent=msg->m_Entity;
		ent->pushOrder( entOrder );
		break;
	}

	return clientMask;
}

uint CSimulation::GetMessageMask(CNetMessage *pMsg, uint oldMask, void *userdata)
{
	CSimulation *pSimulation=(CSimulation *)userdata;
	// Pending a complete visibility/minimal-update implementation, we'll
	// simply select the first 32 connected clients ;-)
	return 0xffffffff;
}

void CSimulation::QueueLocalCommand(CNetMessage *pMsg)
{
	m_pTurnManager->QueueLocalCommand(pMsg);
}
