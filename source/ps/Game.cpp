#include "precompiled.h"

#include "Game.h"

CGame *g_Game=NULL;

PSRETURN CGame::Initialize(CGameAttributes *pAttribs)
{
	try
	{
		m_World.Initialize(pAttribs);
		m_Simulation.Initialize(pAttribs);
		m_GameView.Initialize(pAttribs);
	}
	catch (PSERROR_Game e)
	{
		return e.code;
	}
	return 0;
}

void CGame::Update(double deltaTime)
{
	m_Simulation.Update(deltaTime);
	
	// TODO Detect game over and bring up the summary screen or something
}
