#include "precompiled.h"

#include "Game.h"

CGame *g_Game=NULL;

PSRETURN CGame::Initialize(CGameAttributes *pAttribs)
{
	try
	{
		// RC, 040804 - GameView needs to be initialised before World, otherwise GameView initialisation
		// overwrites anything stored in the map file that gets loaded by CWorld::Initialize with default
		// values.  At the minute, it's just lighting settings, but could be extended to store camera position.  
		// Storing lighting settings in the gameview seems a little odd, but it's no big deal; maybe move it at 
		// some point to be stored in the world object?
		m_GameView.Initialize(pAttribs);
		m_World.Initialize(pAttribs);
		m_Simulation.Initialize(pAttribs);
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
