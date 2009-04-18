/* Copyright (C) 2009 Wildfire Games.
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
 * File        : Game.cpp
 * Project     : engine
 * Description : Contains the CGame Class implementation.
 *
 **/
#include "precompiled.h"

#include "Game.h"

#include "graphics/GameView.h"
#include "graphics/UnitManager.h"
#include "lib/timer.h"
#include "network/NetClient.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/GameAttributes.h"
#include "ps/Loader.h"
#include "ps/Profile.h"
#include "ps/World.h"
#include "simulation/Entity.h"
#include "simulation/EntityManager.h"
#include "simulation/Simulation.h"

#ifndef NO_GUI
#include "gui/CGUI.h"
#endif

class CNetServer;
extern CNetServer *g_NetServer;

extern bool g_GameRestarted;

/**
 * Globally accessible pointer to the CGame object.
 **/
CGame *g_Game=NULL;

// Disable "warning C4355: 'this' : used in base member initializer list".
//   "The base-class constructors and class member constructors are called before
//   this constructor. In effect, you've passed a pointer to an unconstructed
//   object to another constructor. If those other constructors access any
//   members or call member functions on this, the result will be undefined."
// In this case, the pointers are simply stored for later use, so there
// should be no problem.
#if MSC_VERSION
# pragma warning (disable: 4355)
#endif

/**
 * Constructor
 *
 **/
CGame::CGame():
	m_World(new CWorld(this)),
	m_Simulation(new CSimulation(this)),
	m_GameView(new CGameView(this)),
	m_pLocalPlayer(NULL),
	m_GameStarted(false),
	m_Paused(false),
	m_SimRate(1.0f)
{
	// Need to set the CObjectManager references after various objects have
	// been initialised, so do it here rather than via the initialisers above.
	m_World->GetUnitManager().SetObjectManager(m_GameView->GetObjectManager());
}

#if MSC_VERSION
# pragma warning (default: 4355)
#endif

/**
 * Destructor
 *
 **/
CGame::~CGame()
{
	// Again, the in-game call tree is going to be different to the main menu one.
	g_Profiler.StructuralReset();

	delete m_GameView;
	delete m_Simulation;
	delete m_World;
}



/**
 * Initializes the game with the set of attributes provided.
 * Makes calls to initialize the game view, world, and simulation objects.
 * Calls are made to facilitate progress reporting of the initialization.
 *
 * @param CGameAttributes * pAttribs pointer to the game attribute values
 * @return PSRETURN 0
 **/
PSRETURN CGame::RegisterInit(CGameAttributes* pAttribs)
{
	LDR_BeginRegistering();

	// RC, 040804 - GameView needs to be initialized before World, otherwise GameView initialization
	// overwrites anything stored in the map file that gets loaded by CWorld::Initialize with default
	// values.  At the minute, it's just lighting settings, but could be extended to store camera position.  
	// Storing lighting settings in the game view seems a little odd, but it's no big deal; maybe move it at 
	// some point to be stored in the world object?
	m_GameView->RegisterInit(pAttribs);
	m_World->RegisterInit(pAttribs);
	m_Simulation->RegisterInit(pAttribs);
	LDR_EndRegistering();
	return 0;
}
/**
 * Game initialization has been completed. Set game started flag and start the session.
 *
 * @return PSRETURN 0
 **/
PSRETURN CGame::ReallyStartGame()
{
#ifndef NO_GUI

	// Call the reallyStartGame GUI function, but only if it exists
	jsval fval, rval;
	JSBool ok = JS_GetProperty(g_ScriptingHost.getContext(), g_GUI.GetScriptObject(), "reallyStartGame", &fval);
	debug_assert(ok);
	if (ok && !JSVAL_IS_VOID(fval))
	{
		ok = JS_CallFunctionValue(g_ScriptingHost.getContext(), g_GUI.GetScriptObject(), fval, 0, NULL, &rval);
		debug_assert(ok);
	}
#endif

	debug_printf("GAME STARTED, ALL INIT COMPLETE\n");
	m_GameStarted=true;

	// The call tree we've built for pregame probably isn't useful in-game.
	g_Profiler.StructuralReset();

	// Mark terrain as modified so the minimap can repaint (is there a cleaner way of handling this?)
	g_GameRestarted = true;


#ifndef NO_GUI
	g_GUI.SendEventToAll("sessionstart");
#endif

	return 0;
}

/**
 * Prepare to start the game.
 * Set up the players list then call RegisterInit that initializes the game and is used to report progress.
 *
 * @param CGameAttributes * pGameAttributes game attributes for initialization.
 * @return PSRETURN 0 if successful,
 *					error information if not.
 **/
PSRETURN CGame::StartGame(CGameAttributes *pAttribs)
{
	try
	{
		// JW: this loop is taken from ScEd and fixes lack of player color.
		// TODO: determine proper number of players.
		// SB: Only do this for non-network games
		if (!g_NetClient && !g_NetServer)
		{
			for (int i=1; i<8; ++i) 
				pAttribs->GetSlot(i)->AssignLocal();
		}

		pAttribs->FinalizeSlots();
		m_NumPlayers=pAttribs->GetSlotCount();

		// Player 0 = Gaia - allocate one extra
		m_Players.resize(m_NumPlayers + 1);

		for (size_t i=0;i <= m_NumPlayers;i++)
			m_Players[i]=pAttribs->GetPlayer(i);
		
		if (g_NetClient)
		{
			// TODO
			m_pLocalPlayer = g_NetClient->GetLocalPlayer();
			debug_assert(m_pLocalPlayer && "Darn it! We weren't assigned to a slot!");
		}
		else
		{
			m_pLocalPlayer=m_Players[1];
		}

		RegisterInit(pAttribs);
	}
	catch (PSERROR_Game& e)
	{
		return e.getCode();
	}
	return 0;
}


// TODO: doInterpolate is optional because Atlas interpolates explicitly,
// so that it has more control over the update rate. The game might want to
// do the same, and then doInterpolate should be redundant and removed.

/**
 * Periodic heartbeat that controls the process.
 * Simulation update is called and game status update is called.
 *
 * @param double deltaTime elapsed time since last beat in seconds.
 * @param bool doInterpolate perform interpolation if true.
 * @return bool false if it can't keep up with the desired simulation rate
 *	indicating that you might want to render less frequently.
 **/
bool CGame::Update(double deltaTime, bool doInterpolate)
{
	if (m_Paused)
		return true;

	deltaTime *= m_SimRate;
	
	bool ok = m_Simulation->Update(deltaTime);
	if (doInterpolate)
		m_Simulation->Interpolate(deltaTime);
	
	// TODO Detect game over and bring up the summary screen or something
	// ^ Quick game over hack is implemented, no summary screen however
	/*if (m_World->GetEntityManager().GetDeath())
	{
		UpdateGameStatus();
		if (GameStatus != 0)
			EndGame();
	}
	//reset death event flag
	m_World->GetEntityManager().SetDeath(false);*/

	return ok;
}

/**
 * Test player statistics and update game status as required.
 *
 **/
/*
void CGame::UpdateGameStatus()
{
	bool EOG_lose = true;
	bool EOG_win = true;
	CPlayer *local = GetLocalPlayer();

	for (int i=0; i<MAX_HANDLES; i++)
	{	
		CHandle *handle = m_World->GetEntityManager().GetHandle(i);
		if ( !handle )
			continue;
		CPlayer *tmpPlayer = handle->m_entity->GetPlayer();
		
		//Are we still alive?
		if ( local == tmpPlayer && handle->m_entity->m_extant )
		{	
			EOG_lose = false;
			if (EOG_win == false)
				break;
		}
		//Are they still alive?
		else if ( handle->m_entity->m_extant )
		{
			EOG_win = false;
			if (EOG_lose == false)
				break;
		}
	}
	if (EOG_lose && EOG_win)
		GameStatus = EOG_SPECIAL_DRAW;
	else if (EOG_win)
		GameStatus = EOG_WIN;
	else if (EOG_lose)	
		GameStatus = EOG_LOSE;
	else
		GameStatus = EOG_NEUTRAL;
}*/

/**
 * End of game console message creation.
 *
 **/
void CGame::EndGame()
{
	g_Console->InsertMessage( L"It's the end of the game as we know it!");
	switch (GameStatus)
	{
	case EOG_DRAW:
		g_Console->InsertMessage( L"A diplomatic draw ain't so bad, eh?");
		break;
	case EOG_SPECIAL_DRAW:
		g_Console->InsertMessage( L"Amazingly, you managed to draw from dieing at the same time as your opponent...you have my respect.");
		break;
	case EOG_LOSE:
	    g_Console->InsertMessage( L"My condolences on your loss.");
		break;
	case EOG_WIN:
		g_Console->InsertMessage( L"Thou art victorious!");
		break;
	default:
		break;
	}
}
/**
 * Get the player object from the players list at the provided index.
 *
 * @param idx sequential position in the list.
 * @return CPlayer * pointer to player requested.
 **/
CPlayer *CGame::GetPlayer(size_t idx)
{
	if (idx > m_NumPlayers)
	{
//		debug_warn("Invalid player ID");
//		LOG(CLogger::Error, "", "Invalid player ID %d (outside 0..%d)", idx, m_NumPlayers);
		return m_Players[0];
	}
	// Be a bit more paranoid - maybe m_Players hasn't been set large enough
	else if (idx >= m_Players.size())
	{
		debug_warn("Invalid player ID");
		LOG(CLogger::Error, "", "Invalid player ID %d (not <=%d - internal error?)", idx, m_Players.size());

		if (m_Players.size() != 0)
			return m_Players[0];
		else
			return NULL; // the caller will probably crash because of this,
			             // but at least we've reported the error
	}
	else
		return m_Players[idx];
}
