#include "precompiled.h"

#include <timer.h>

#include "Simulation.h"
#include "Game.h"
#include "EntityManager.h"
#include "Scheduler.h"

#include "gui/CGUI.h"

CSimulation::CSimulation(CGame *pGame):
	m_pGame(pGame),
	m_pWorld(pGame->GetWorld()),
	m_DeltaTime(0)
{}

void CSimulation::Initialize(CGameAttributes *pAttribs)
{}

void CSimulation::Update(double frameTime)
{
	m_DeltaTime += frameTime;

	if( m_DeltaTime >= 0.0 )
	{
		// A new simulation frame is required.
		MICROLOG( L"calculate simulation" );
		Simulate();
		m_DeltaTime -= (m_SimUpdateInterval/1000.0);
		if( m_DeltaTime >= 0.0 )
		{
			// The desired sim frame rate can't be achieved. Settle for process & render
			// frames as fast as possible.
			// g_Console->InsertMessage( L"Can't maintain %d FPS simulation rate!", SIM_FRAMERATE );
			m_DeltaTime = 0.0;
		}
	}

	Interpolate(frameTime, ((1000.0*m_DeltaTime) / (float)m_SimUpdateInterval) + 1.0);
}

void CSimulation::Interpolate(double frameTime, double offset)
{
	const std::vector<CUnit*>& units=m_pWorld->GetUnitManager()->GetUnits();
	for (uint i=0;i<units.size();++i)
		units[i]->GetModel()->Update(frameTime);

	g_EntityManager.interpolateAll(offset);
}

void CSimulation::Simulate()
{
	g_GUI.TickObjects();
	g_Scheduler.update(m_SimUpdateInterval);
	g_EntityManager.updateAll( m_SimUpdateInterval );
}
