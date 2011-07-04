/* Copyright (C) 2011 Wildfire Games.
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

#include "Game.h"

#include "graphics/GameView.h"
#include "graphics/LOSTexture.h"
#include "graphics/ParticleManager.h"
#include "graphics/UnitManager.h"
#include "lib/timer.h"
#include "network/NetClient.h"
#include "network/NetServer.h"
#include "network/NetTurnManager.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Loader.h"
#include "ps/LoaderThunks.h"
#include "ps/Overlay.h"
#include "ps/Profile.h"
#include "ps/Replay.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
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
	m_ReplayLogger = new CReplayLogger(m_Simulation2->GetScriptInterface());
	// TODO: should use CDummyReplayLogger unless activated by cmd-line arg, perhaps?

	// Need to set the CObjectManager references after various objects have
	// been initialised, so do it here rather than via the initialisers above.
	if (m_GameView)
		m_World->GetUnitManager().SetObjectManager(m_GameView->GetObjectManager());

	m_TurnManager = new CNetLocalTurnManager(*m_Simulation2, GetReplayLogger()); // this will get replaced if we're a net server/client

	m_Simulation2->LoadDefaultScripts();
}

/**
 * Destructor
 *
 **/
CGame::~CGame()
{
	// Clear rooted value before destroying its context
	m_RegisteredAttribs = CScriptValRooted();

	// Again, the in-game call tree is going to be different to the main menu one.
	if (CProfileManager::IsInitialised())
		g_Profiler.StructuralReset();

	delete m_TurnManager;
	delete m_GameView;
	delete m_Simulation2;
	delete m_World;
	delete m_ReplayLogger;
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
	m_RegisteredAttribs = attribs; // save the attributes for ReallyStartGame

	std::string mapType;
	m_Simulation2->GetScriptInterface().GetProperty(attribs.get(), "mapType", mapType);

	LDR_BeginRegistering();

	RegMemFun(m_Simulation2, &CSimulation2::ProgressiveLoad, L"Simulation init", 1000);

	// RC, 040804 - GameView needs to be initialized before World, otherwise GameView initialization
	// overwrites anything stored in the map file that gets loaded by CWorld::Initialize with default
	// values.  At the minute, it's just lighting settings, but could be extended to store camera position.
	// Storing lighting settings in the game view seems a little odd, but it's no big deal; maybe move it at
	// some point to be stored in the world object?
	if (m_GameView)
		m_GameView->RegisterInit();

	if (mapType == "scenario")
	{
		// Load scenario attributes
		std::wstring mapFile;
		m_Simulation2->GetScriptInterface().GetProperty(attribs.get(), "map", mapFile);

		m_World->RegisterInit(mapFile, m_PlayerID);
	}
	else if (mapType == "random")
	{
		// Load random map attributes
		std::wstring scriptFile;
		CScriptValRooted settings;

		m_Simulation2->GetScriptInterface().GetProperty(attribs.get(), "script", scriptFile);
		m_Simulation2->GetScriptInterface().GetProperty(attribs.get(), "settings", settings);

		m_World->RegisterInitRMS(scriptFile, settings, m_PlayerID);
	}

	LDR_EndRegistering();
}

/**
 * Game initialization has been completed. Set game started flag and start the session.
 *
 * @return PSRETURN 0
 **/
PSRETURN CGame::ReallyStartGame()
{
	CScriptVal settings;
	m_Simulation2->GetScriptInterface().GetProperty(m_RegisteredAttribs.get(), "settings", settings);
	m_Simulation2->InitGame(settings);

	// Call the reallyStartGame GUI function, but only if it exists
	if (g_GUI && g_GUI->HasPages())
	{
		jsval fval, rval;
		JSBool ok = JS_GetProperty(g_ScriptingHost.getContext(), g_GUI->GetScriptObject(), "reallyStartGame", &fval);
		ENSURE(ok);
		if (ok && !JSVAL_IS_VOID(fval))
			ok = JS_CallFunctionValue(g_ScriptingHost.getContext(), g_GUI->GetScriptObject(), fval, 0, NULL, &rval);
	}

	if (g_NetClient)
		g_NetClient->LoadFinished();

	// We need to do an initial Interpolate call to set up all the models etc,
	// because Update might never interpolate (e.g. if the game starts paused)
	// and we could end up rendering before having set up any models (so they'd
	// all be invisible)
	Interpolate(0);

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
	m_ReplayLogger->StartGame(attribs);

	RegisterInit(attribs);
}

// TODO: doInterpolate is optional because Atlas interpolates explicitly,
// so that it has more control over the update rate. The game might want to
// do the same, and then doInterpolate should be redundant and removed.

/**
 * Periodic heartbeat that controls the process.
 * Simulation update is called and game status update is called.
 *
 * @param deltaTime Double. Elapsed time since last beat in seconds.
 * @param doInterpolate Bool. Perform interpolation if true.
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
		// To avoid confusing the profiler, we need to trigger the new turn
		// while we're not nested inside any PROFILE blocks
		if (m_TurnManager->WillUpdate(deltaTime))
			g_Profiler.Turn();

		// At the normal sim rate, we currently want to render at least one
		// frame per simulation turn, so let maxTurns be 1. But for fast-forward
		// sim rates we want to allow more, so it's not bounded by framerate,
		// so just use the sim rate itself as the number of turns per frame.
		size_t maxTurns = (size_t)m_SimRate;

		PROFILE("simulation update");
		if (m_TurnManager->Update(deltaTime, maxTurns))
		{
			g_GUI->SendEventToAll("SimulationUpdate");
			GetView()->GetLOSTexture().MakeDirty();
		}
	}

	if (doInterpolate)
	{
		PROFILE("interpolate");
		m_TurnManager->Interpolate(deltaTime);
	}
	
	// TODO: maybe we should add a CCmpParticleInterface that passes the interpolation commands
	// etc to CParticleManager. But in the meantime just handle it explicitly here.
	if (doInterpolate && CRenderer::IsInitialised())
		g_Renderer.GetParticleManager().Interpolate(deltaTime);

	return ok;
}

void CGame::Interpolate(float frameLength)
{
	if (!m_TurnManager)
		return;

	m_TurnManager->Interpolate(frameLength);

	if (CRenderer::IsInitialised())
		g_Renderer.GetParticleManager().Interpolate(frameLength);
}


static CColor BrokenColor(0.3f, 0.3f, 0.3f, 1.0f);

void CGame::CachePlayerColours()
{
	m_PlayerColours.clear();

	CmpPtr<ICmpPlayerManager> cmpPlayerManager(*m_Simulation2, SYSTEM_ENTITY);
	if (cmpPlayerManager.null())
		return;

	int numPlayers = cmpPlayerManager->GetNumPlayers();
	m_PlayerColours.resize(numPlayers);

	for (int i = 0; i < numPlayers; ++i)
	{
		CmpPtr<ICmpPlayer> cmpPlayer(*m_Simulation2, cmpPlayerManager->GetPlayerByID(i));
		if (cmpPlayer.null())
			m_PlayerColours[i] = BrokenColor;
		else
			m_PlayerColours[i] = cmpPlayer->GetColour();
	}
}


CColor CGame::GetPlayerColour(int player) const
{
	if (player < 0 || player >= (int)m_PlayerColours.size())
		return BrokenColor;

	return m_PlayerColours[player];
}
