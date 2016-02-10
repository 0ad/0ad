/* Copyright (C) 2016 Wildfire Games.
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
#include "gui/GUIManager.h"
#include "gui/CGUI.h"
#include "lib/config2.h"
#include "lib/timer.h"
#include "network/NetClient.h"
#include "network/NetServer.h"
#include "network/NetTurnManager.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Loader.h"
#include "ps/LoaderThunks.h"
#include "ps/Profile.h"
#include "ps/Replay.h"
#include "ps/Shapes.h"
#include "ps/World.h"
#include "ps/GameSetup/GameSetup.h"
#include "renderer/Renderer.h"
#include "renderer/TimeManager.h"
#include "renderer/WaterManager.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "soundmanager/ISoundManager.h"

#include "tools/atlas/GameInterface/GameLoop.h"

extern bool g_GameRestarted;
extern GameLoopState* g_AtlasGameLoop;

/**
 * Globally accessible pointer to the CGame object.
 **/
CGame *g_Game=NULL;

/**
 * Constructor
 *
 **/
CGame::CGame(bool disableGraphics, bool replayLog):
	m_World(new CWorld(this)),
	m_Simulation2(new CSimulation2(&m_World->GetUnitManager(), g_ScriptRuntime, m_World->GetTerrain())),
	m_GameView(disableGraphics ? NULL : new CGameView(this)),
	m_GameStarted(false),
	m_Paused(false),
	m_SimRate(1.0f),
	m_PlayerID(-1),
	m_IsSavedGame(false),
	m_IsVisualReplay(false),
	m_ReplayStream(NULL)
{
	// TODO: should use CDummyReplayLogger unless activated by cmd-line arg, perhaps?
	if (replayLog)
		m_ReplayLogger = new CReplayLogger(m_Simulation2->GetScriptInterface());
	else
		m_ReplayLogger = new CDummyReplayLogger();

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
	// Again, the in-game call tree is going to be different to the main menu one.
	if (CProfileManager::IsInitialised())
		g_Profiler.StructuralReset();

	delete m_TurnManager;
	delete m_GameView;
	delete m_Simulation2;
	delete m_World;
	delete m_ReplayLogger;
	delete m_ReplayStream;
}

void CGame::SetTurnManager(CNetTurnManager* turnManager)
{
	if (m_TurnManager)
		delete m_TurnManager;

	m_TurnManager = turnManager;

	if (m_TurnManager)
		m_TurnManager->SetPlayerID(m_PlayerID);
}

int CGame::LoadVisualReplayData()
{
	ENSURE(m_IsVisualReplay);
	ENSURE(!m_ReplayPath.empty());
	ENSURE(m_ReplayStream);

	CNetReplayTurnManager* replayTurnMgr = static_cast<CNetReplayTurnManager*>(GetTurnManager());

	u32 currentTurn = 0;
	std::string type;
	while ((*m_ReplayStream >> type).good())
	{
		if (type == "turn")
		{
			u32 turn = 0;
			u32 turnLength = 0;
			*m_ReplayStream >> turn >> turnLength;
			ENSURE(turn == currentTurn && "You tried to replay a commands.txt file of a rejoined client. Please use the host's file.");
			replayTurnMgr->StoreReplayTurnLength(currentTurn, turnLength);
		}
		else if (type == "cmd")
		{
			player_id_t player;
			*m_ReplayStream >> player;

			std::string line;
			std::getline(*m_ReplayStream, line);
			replayTurnMgr->StoreReplayCommand(currentTurn, player, line);
		}
		else if (type == "hash" || type == "hash-quick")
		{
			bool quick = (type == "hash-quick");
			std::string replayHash;
			*m_ReplayStream >> replayHash;
			replayTurnMgr->StoreReplayHash(currentTurn, replayHash, quick);
		}
		else if (type == "end")
			++currentTurn;
		else
			CancelLoad(L"Failed to load replay data (unrecognized content)");
	}
	SAFE_DELETE(m_ReplayStream);
	m_FinalReplayTurn = currentTurn > 0 ? currentTurn - 1 : 0;
	replayTurnMgr->StoreFinalReplayTurn(m_FinalReplayTurn);
	return 0;
}

bool CGame::StartVisualReplay(const std::string& replayPath)
{
	m_IsVisualReplay = true;
	ScriptInterface& scriptInterface = m_Simulation2->GetScriptInterface();

	SetTurnManager(new CNetReplayTurnManager(*m_Simulation2, GetReplayLogger()));

	m_ReplayPath = replayPath;
	m_ReplayStream = new std::ifstream(m_ReplayPath.c_str());

	std::string type;
	ENSURE((*m_ReplayStream >> type).good() && type == "start");

	std::string line;
	std::getline(*m_ReplayStream, line);
	JS::RootedValue attribs(scriptInterface.GetContext());
	scriptInterface.ParseJSON(line, &attribs);
	StartGame(&attribs, "");

	return true;
}

/**
 * Initializes the game with the set of attributes provided.
 * Makes calls to initialize the game view, world, and simulation objects.
 * Calls are made to facilitate progress reporting of the initialization.
 **/
void CGame::RegisterInit(const JS::HandleValue attribs, const std::string& savedState)
{
	ScriptInterface& scriptInterface = m_Simulation2->GetScriptInterface();
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);
	
	m_InitialSavedState = savedState;
	m_IsSavedGame = !savedState.empty();

	m_Simulation2->SetInitAttributes(attribs);

	std::string mapType;
	scriptInterface.GetProperty(attribs, "mapType", mapType);

	float speed;
	if (scriptInterface.HasProperty(attribs, "gameSpeed") && scriptInterface.GetProperty(attribs, "gameSpeed", speed))
		SetSimRate(speed);

	LDR_BeginRegistering();

	RegMemFun(m_Simulation2, &CSimulation2::ProgressiveLoad, L"Simulation init", 1000);

	// RC, 040804 - GameView needs to be initialized before World, otherwise GameView initialization
	// overwrites anything stored in the map file that gets loaded by CWorld::Initialize with default
	// values.  At the minute, it's just lighting settings, but could be extended to store camera position.
	// Storing lighting settings in the game view seems a little odd, but it's no big deal; maybe move it at
	// some point to be stored in the world object?
	if (m_GameView)
		m_GameView->RegisterInit();

	if (mapType == "random")
	{
		// Load random map attributes
		std::wstring scriptFile;
		JS::RootedValue settings(cx);

		scriptInterface.GetProperty(attribs, "script", scriptFile);
		scriptInterface.GetProperty(attribs, "settings", &settings);

		m_World->RegisterInitRMS(scriptFile, scriptInterface.GetJSRuntime(), settings, m_PlayerID);
	}
	else
	{
		std::wstring mapFile;
		JS::RootedValue settings(cx);
		scriptInterface.GetProperty(attribs, "map", mapFile);
		scriptInterface.GetProperty(attribs, "settings", &settings);

		m_World->RegisterInit(mapFile, scriptInterface.GetJSRuntime(), settings, m_PlayerID);
	}
	if (m_GameView)
		RegMemFun(g_Renderer.GetSingletonPtr()->GetWaterManager(), &WaterManager::LoadWaterTextures, L"LoadWaterTextures", 80);

	if (m_IsSavedGame)
		RegMemFun(this, &CGame::LoadInitialState, L"Loading game", 1000);

	if (m_IsVisualReplay)
		RegMemFun(this, &CGame::LoadVisualReplayData, L"Loading visual replay data", 1000);

	LDR_EndRegistering();
}

int CGame::LoadInitialState()
{
	ENSURE(m_IsSavedGame);
	ENSURE(!m_InitialSavedState.empty());

	std::string state;
	m_InitialSavedState.swap(state); // deletes the original to save a bit of memory

	std::stringstream stream(state);

	bool ok = m_Simulation2->DeserializeState(stream);
	if (!ok)
	{
		CancelLoad(L"Failed to load saved game state. It might have been\nsaved with an incompatible version of the game.");
		return 0;
	}

	return 0;
}

/**
 * Game initialization has been completed. Set game started flag and start the session.
 *
 * @return PSRETURN 0
 **/
PSRETURN CGame::ReallyStartGame()
{
	JSContext* cx = m_Simulation2->GetScriptInterface().GetContext();
	JSAutoRequest rq(cx);
	
	// Call the script function InitGame only for new games, not saved games
	if (!m_IsSavedGame)
	{
		// Perform some simulation initializations (replace skirmish entities, explore territories, etc.) 
		// that needs to be done before setting up the AI and shouldn't be done in Atlas
		if (!g_AtlasGameLoop->running)
			m_Simulation2->PreInitGame();

		JS::RootedValue settings(cx);
		JS::RootedValue tmpInitAttributes(cx, m_Simulation2->GetInitAttributes());
		m_Simulation2->GetScriptInterface().GetProperty(tmpInitAttributes, "settings", &settings);
		m_Simulation2->InitGame(settings);
	}

	// We need to do an initial Interpolate call to set up all the models etc,
	// because Update might never interpolate (e.g. if the game starts paused)
	// and we could end up rendering before having set up any models (so they'd
	// all be invisible)
	Interpolate(0, 0);

	m_GameStarted=true;
	
	// Render a frame to begin loading assets
	if (CRenderer::IsInitialised())
		Render();

	if (g_NetClient)
		g_NetClient->LoadFinished();

	// Call the reallyStartGame GUI function, but only if it exists
	if (g_GUI && g_GUI->HasPages())
	{
		JS::RootedValue global(cx, g_GUI->GetActiveGUI()->GetGlobalObject());
		if (g_GUI->GetActiveGUI()->GetScriptInterface()->HasProperty(global, "reallyStartGame"))
			g_GUI->GetActiveGUI()->GetScriptInterface()->CallFunctionVoid(global, "reallyStartGame");
	}

	debug_printf("GAME STARTED, ALL INIT COMPLETE\n");

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

void CGame::SetPlayerID(player_id_t playerID)
{
	m_PlayerID = playerID;
	if (m_TurnManager)
		m_TurnManager->SetPlayerID(m_PlayerID);
}

void CGame::StartGame(JS::MutableHandleValue attribs, const std::string& savedState)
{
	if (m_ReplayLogger)
		m_ReplayLogger->StartGame(attribs);

	RegisterInit(attribs, savedState);
}

// TODO: doInterpolate is optional because Atlas interpolates explicitly,
// so that it has more control over the update rate. The game might want to
// do the same, and then doInterpolate should be redundant and removed.

void CGame::Update(const double deltaRealTime, bool doInterpolate)
{
	if (m_Paused || !m_TurnManager)
		return;

	const double deltaSimTime = deltaRealTime * m_SimRate;
	
	if (deltaSimTime)
	{
		// To avoid confusing the profiler, we need to trigger the new turn
		// while we're not nested inside any PROFILE blocks
		if (m_TurnManager->WillUpdate(deltaSimTime))
			g_Profiler.Turn();

		// At the normal sim rate, we currently want to render at least one
		// frame per simulation turn, so let maxTurns be 1. But for fast-forward
		// sim rates we want to allow more, so it's not bounded by framerate,
		// so just use the sim rate itself as the number of turns per frame.
		size_t maxTurns = (size_t)m_SimRate;

		if (m_TurnManager->Update(deltaSimTime, maxTurns))
		{
			{
				PROFILE3("gui sim update");
				g_GUI->SendEventToAll("SimulationUpdate");
			}

			GetView()->GetLOSTexture().MakeDirty();
		}

		if (CRenderer::IsInitialised())
			g_Renderer.GetTimeManager().Update(deltaSimTime);
	}

	if (doInterpolate)
	{
		m_TurnManager->Interpolate(deltaSimTime, deltaRealTime);

		if ( g_SoundManager )
			g_SoundManager->IdleTask();
	}
}

void CGame::Interpolate(float simFrameLength, float realFrameLength)
{
	if (!m_TurnManager)
		return;

	m_TurnManager->Interpolate(simFrameLength, realFrameLength);
}


static CColor BrokenColor(0.3f, 0.3f, 0.3f, 1.0f);

void CGame::CachePlayerColors()
{
	m_PlayerColors.clear();

	CmpPtr<ICmpPlayerManager> cmpPlayerManager(*m_Simulation2, SYSTEM_ENTITY);
	if (!cmpPlayerManager)
		return;

	int numPlayers = cmpPlayerManager->GetNumPlayers();
	m_PlayerColors.resize(numPlayers);

	for (int i = 0; i < numPlayers; ++i)
	{
		CmpPtr<ICmpPlayer> cmpPlayer(*m_Simulation2, cmpPlayerManager->GetPlayerByID(i));
		if (!cmpPlayer)
			m_PlayerColors[i] = BrokenColor;
		else
			m_PlayerColors[i] = cmpPlayer->GetColor();
	}
}


CColor CGame::GetPlayerColor(player_id_t player) const
{
	if (player < 0 || player >= (int)m_PlayerColors.size())
		return BrokenColor;

	return m_PlayerColors[player];
}
