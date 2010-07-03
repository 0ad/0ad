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
#include "network/NetServer.h"
#include "network/NetTurnManager.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Loader.h"
#include "ps/Overlay.h"
#include "ps/Profile.h"
#include "ps/World.h"
#include "scripting/ScriptingHost.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPlayerManager.h"

#include "gui/GUIManager.h"

extern bool g_GameRestarted;

/**
 * Globally accessible pointer to the CGame object.
 **/
CGame *g_Game=NULL;

/**
 * Constructor
 *
 **/
CGame::CGame(bool disableGraphics):
	m_World(new CWorld(this)),
	m_Simulation2(new CSimulation2(&m_World->GetUnitManager(), m_World->GetTerrain())),
	m_GameView(disableGraphics ? NULL : new CGameView(this)),
	m_GameStarted(false),
	m_Paused(false),
	m_SimRate(1.0f),
	m_PlayerID(-1)
{
	// Need to set the CObjectManager references after various objects have
	// been initialised, so do it here rather than via the initialisers above.
	if (m_GameView)
		m_World->GetUnitManager().SetObjectManager(m_GameView->GetObjectManager());

	m_TurnManager = new CNetLocalTurnManager(*m_Simulation2); // this will get replaced if we're a net server/client

	m_Simulation2->LoadDefaultScripts();
	m_Simulation2->ResetState();

	CScriptVal initData; // TODO: ought to get this from the GUI, somehow
	m_Simulation2->InitGame(initData);
}

/**
 * Destructor
 *
 **/
CGame::~CGame()
{
	// Again, the in-game call tree is going to be different to the main menu one.
	if (CProfileManager::IsInitialised())
		g_Profiler.StructuralReset();

	delete m_TurnManager;
	delete m_GameView;
	delete m_Simulation2;
	delete m_World;
}

void CGame::SetTurnManager(CNetTurnManager* turnManager)
{
	if (m_TurnManager)
		delete m_TurnManager;

	m_TurnManager = turnManager;

	if (m_TurnManager)
		m_TurnManager->SetPlayerID(m_PlayerID);
}


/**
 * Initializes the game with the set of attributes provided.
 * Makes calls to initialize the game view, world, and simulation objects.
 * Calls are made to facilitate progress reporting of the initialization.
 **/
void CGame::RegisterInit(const CScriptValRooted& attribs)
{
	std::wstring mapFile;
	m_Simulation2->GetScriptInterface().GetProperty(attribs.get(), "map", mapFile);
	mapFile += L".pmp";

	LDR_BeginRegistering();

	// RC, 040804 - GameView needs to be initialized before World, otherwise GameView initialization
	// overwrites anything stored in the map file that gets loaded by CWorld::Initialize with default
	// values.  At the minute, it's just lighting settings, but could be extended to store camera position.
	// Storing lighting settings in the game view seems a little odd, but it's no big deal; maybe move it at
	// some point to be stored in the world object?
	if (m_GameView)
		m_GameView->RegisterInit();
	m_World->RegisterInit(mapFile, m_PlayerID);
	LDR_EndRegistering();
}

/**
 * Game initialization has been completed. Set game started flag and start the session.
 *
 * @return PSRETURN 0
 **/
PSRETURN CGame::ReallyStartGame()
{
	// Call the reallyStartGame GUI function, but only if it exists
	if (g_GUI && g_GUI->HasPages())
	{
		jsval fval, rval;
		JSBool ok = JS_GetProperty(g_ScriptingHost.getContext(), g_GUI->GetScriptObject(), "reallyStartGame", &fval);
		debug_assert(ok);
		if (ok && !JSVAL_IS_VOID(fval))
			ok = JS_CallFunctionValue(g_ScriptingHost.getContext(), g_GUI->GetScriptObject(), fval, 0, NULL, &rval);
	}

	if (g_NetClient)
		g_NetClient->LoadFinished();

	debug_printf(L"GAME STARTED, ALL INIT COMPLETE\n");
	m_GameStarted=true;

	// The call tree we've built for pregame probably isn't useful in-game.
	if (CProfileManager::IsInitialised())
		g_Profiler.StructuralReset();

	// Mark terrain as modified so the minimap can repaint (is there a cleaner way of handling this?)
	g_GameRestarted = true;

	return 0;
}

int CGame::GetPlayerID()
{
	return m_PlayerID;
}

void CGame::SetPlayerID(int playerID)
{
	m_PlayerID = playerID;
	if (m_TurnManager)
		m_TurnManager->SetPlayerID(m_PlayerID);
}

void CGame::StartGame(const CScriptValRooted& attribs)
{
	RegisterInit(attribs);
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

	if (!m_TurnManager)
		return true;

	deltaTime *= m_SimRate;
	
	bool ok = true;
	if (deltaTime)
	{
		PROFILE("update");
		if (m_TurnManager->Update(deltaTime))
			g_GUI->SendEventToAll("SimulationUpdate");
	}

	if (doInterpolate)
	{
		PROFILE("interpolate");
		m_TurnManager->Interpolate(deltaTime);
	}
	
	return ok;
}

void CGame::Interpolate(float frameLength)
{
	if (!m_TurnManager)
		return;

	m_TurnManager->Interpolate(frameLength);
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

static CColor BrokenColor(0.3f, 0.3f, 0.3f, 1.0f);
CColor CGame::GetPlayerColour(int player) const
{
	CmpPtr<ICmpPlayerManager> cmpPlayerManager(*m_Simulation2, SYSTEM_ENTITY);
	if (cmpPlayerManager.null())
		return BrokenColor;
	CmpPtr<ICmpPlayer> cmpPlayer(*m_Simulation2, cmpPlayerManager->GetPlayerByID(player));
	if (cmpPlayer.null())
		return BrokenColor;
	return cmpPlayer->GetColour();
}
