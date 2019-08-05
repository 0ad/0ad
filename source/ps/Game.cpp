/* Copyright (C) 2019 Wildfire Games.
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
#include "scriptinterface/ScriptConversions.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/system/ReplayTurnManager.h"
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
	m_ViewedPlayerID(-1),
	m_IsSavedGame(false),
	m_IsVisualReplay(false),
	m_ReplayStream(NULL),
    m_IsSavingReplay(replayLog)
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

	m_TurnManager = new CLocalTurnManager(*m_Simulation2, GetReplayLogger()); // this will get replaced if we're a net server/client

	m_Simulation2->LoadDefaultScripts();
}

CGame* CGame::fromConfig(GameConfig config, bool disableGraphics, bool replayLog)
{
    CGame* game = new CGame(disableGraphics, replayLog);
	ScriptInterface& scriptInterface = game->GetSimulation2()->GetScriptInterface();
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue attrs(cx);
	scriptInterface.Eval("({})", &attrs);
	JS::RootedValue settings(cx);
	scriptInterface.Eval("({})", &settings);
	JS::RootedValue playerData(cx);
	scriptInterface.Eval("([])", &playerData);

	// The directory in front of the actual map name indicates which type
	// of map is being loaded. Drawback of this approach is the association
	// of map types and folders is hard-coded, but benefits are:
	// - No need to pass the map type via command line separately
	// - Prevents mixing up of scenarios and skirmish maps to some degree
    CStr fullName = utf8_from_wstring(config.getFullName());
	std::wstring mapDirectory = config.getMapDirectory();

	if (config.type == L"random")
	{
		// Random map definition will be loaded from JSON file, so we need to parse it
		std::wstring scriptPath = L"maps/" + fullName.FromUTF8() + L".json";
		JS::RootedValue scriptData(cx);
		scriptInterface.ReadJSONFile(scriptPath, &scriptData);
		if (!scriptData.isUndefined() && scriptInterface.GetProperty(scriptData, "settings", &settings))
		{
			// JSON loaded ok - copy script name over to game attributes
			std::wstring scriptFile;
			scriptInterface.GetProperty(settings, "Script", scriptFile);
			scriptInterface.SetProperty(attrs, "script", scriptFile);				// RMS filename
		}
		else
		{
			// Problem with JSON file
			LOGERROR("Autostart: Error reading random map script '%s'", utf8_from_wstring(scriptPath));
			throw PSERROR_Game_World_MapLoadFailed("Error reading random map script.\nCheck application log for details.");
		}

		scriptInterface.SetProperty(settings, "Size", config.size);		// Random map size (in patches)

		// Set up player data
		for (size_t i = 0; i < config.numPlayers; ++i)
		{
			JS::RootedValue player(cx);
			scriptInterface.Eval("({})", &player);

			// We could load player_defaults.json here, but that would complicate the logic
			// even more and autostart is only intended for developers anyway
            // FIXME: enable setting the civilizations
			scriptInterface.SetProperty(player, "Civ", std::string("athen"));
			scriptInterface.SetPropertyInt(playerData, i, player);
		}
	}
	else if (config.type == L"scenario" || config.type == L"skirmish")
	{
		// Initialize general settings from the map data so some values
		// (e.g. name of map) are always present, even when autostart is
		// partially configured
		CStr8 mapSettingsJSON = LoadSettingsOfScenarioMap("maps/" + fullName + ".xml");
		scriptInterface.ParseJSON(mapSettingsJSON, &settings);

		// Initialize the playerData array being modified by autostart
		// with the real map data, so sensible values are present:
		scriptInterface.GetProperty(settings, "PlayerData", &playerData);
	}
	else
	{
		LOGERROR("Autostart: Unrecognized map type '%s'", utf8_from_wstring(mapDirectory));
		throw PSERROR_Game_World_MapLoadFailed("Unrecognized map type.\nConsult readme.txt for the currently supported types.");
	}

	scriptInterface.SetProperty(attrs, "mapType", config.type);
	scriptInterface.SetProperty(attrs, "map", std::string("maps/" + fullName));
	scriptInterface.SetProperty(settings, "mapType", config.type);
	scriptInterface.SetProperty(settings, "CheatsEnabled", true);

	// The seed is used for both random map generation and simulation
	scriptInterface.SetProperty(settings, "Seed", config.seed);

	// Set seed for AIs
	scriptInterface.SetProperty(settings, "AISeed", config.aiseed);

	// Set player data for AIs
	//		attrs.settings = { PlayerData: [ { AI: ... }, ... ] }
	//		            or = { PlayerData: [ null, { AI: ... }, ... ] } when gaia set
	int offset = 1;
	JS::RootedValue player(cx);
	if (scriptInterface.GetPropertyInt(playerData, 0, &player) && player.isNull())
		offset = 0;

	// Set teams
    std::vector<std::tuple<int, int>> civArgs = config.teams;
    for (size_t i = 0; i < config.teams.size(); ++i)
    {
        int playerID = std::get<0>(config.teams[i]);
        int teamID = std::get<1>(config.teams[i]);

        // Instead of overwriting existing player data, modify the array
        JS::RootedValue player(cx);
        if (!scriptInterface.GetPropertyInt(playerData, playerID-offset, &player) || player.isUndefined())
        {
            if (mapDirectory == L"skirmishes")
            {
                // playerID is certainly bigger than this map player number
                LOGWARNING("Autostart: Invalid player %d in autostart-team option", playerID);
                continue;
            }
            scriptInterface.Eval("({})", &player);
        }

        scriptInterface.SetProperty(player, "Team", teamID);
        scriptInterface.SetPropertyInt(playerData, playerID-offset, player);
	}

	scriptInterface.SetProperty(settings, "Ceasefire", config.ceasefire);

    for (size_t i = 0; i < config.ai.size(); ++i)
    {
        int playerID = std::get<0>(config.ai[i]);
        CStr name = std::get<1>(config.ai[i]);

        // Instead of overwriting existing player data, modify the array
        JS::RootedValue player(cx);
        if (!scriptInterface.GetPropertyInt(playerData, playerID-offset, &player) || player.isUndefined())
        {
            if (mapDirectory == L"scenarios" || mapDirectory == L"skirmishes")
            {
                // playerID is certainly bigger than this map player number
                LOGWARNING("Autostart: Invalid player %d in autostart-ai option", playerID);
                continue;
            }
            scriptInterface.Eval("({})", &player);
        }

        scriptInterface.SetProperty(player, "AI", std::string(name));
        scriptInterface.SetProperty(player, "AIDiff", 3);
        scriptInterface.SetProperty(player, "AIBehavior", std::string("balanced"));
        scriptInterface.SetPropertyInt(playerData, playerID-offset, player);
    }
	// Set AI difficulty
    for (size_t i = 0; i < config.difficulties.size(); ++i)
    {
        int playerID = std::get<0>(config.difficulties[i]);
        int difficulty = std::get<1>(config.difficulties[i]);

        // Instead of overwriting existing player data, modify the array
        JS::RootedValue player(cx);
        if (!scriptInterface.GetPropertyInt(playerData, playerID-offset, &player) || player.isUndefined())
        {
            if (mapDirectory == L"scenarios" || mapDirectory == L"skirmishes")
            {
                // playerID is certainly bigger than this map player number
                LOGWARNING("Autostart: Invalid player %d in autostart-aidiff option", playerID);
                continue;
            }
            scriptInterface.Eval("({})", &player);
        }

        scriptInterface.SetProperty(player, "AIDiff", difficulty);
        scriptInterface.SetPropertyInt(playerData, playerID-offset, player);
    }
	// Set player data for Civs
    if (config.type != L"scenario")
    {
        for (size_t i = 0; i < config.civs.size(); ++i)
        {
            int playerID = std::get<0>(config.civs[i]);
            CStr name = std::get<1>(config.civs[i]);

            // Instead of overwriting existing player data, modify the array
            JS::RootedValue player(cx);
            if (!scriptInterface.GetPropertyInt(playerData, playerID-offset, &player) || player.isUndefined())
            {
                if (mapDirectory == L"skirmishes")
                {
                    // playerID is certainly bigger than this map player number
                    LOGWARNING("Autostart: Invalid player %d in autostart-civ option", playerID);
                    continue;
                }
                scriptInterface.Eval("({})", &player);
            }

            scriptInterface.SetProperty(player, "Civ", std::string(name));
            scriptInterface.SetPropertyInt(playerData, playerID-offset, player);
        }
    }
    else if (config.civs.size() > 0)
        LOGWARNING("Autostart: Option 'autostart-civ' is invalid for scenarios");

	// Add player data to map settings
	scriptInterface.SetProperty(settings, "PlayerData", playerData);

	// Add map settings to game attributes
	scriptInterface.SetProperty(attrs, "settings", settings);

	// Get optional playername
	CStrW userName = config.username;

	// Add additional scripts to the TriggerScripts property
	std::vector<CStrW> triggerScriptsVector;
	JS::RootedValue triggerScripts(cx);

	if (scriptInterface.HasProperty(settings, "TriggerScripts"))
	{
		scriptInterface.GetProperty(settings, "TriggerScripts", &triggerScripts);
		FromJSVal_vector(cx, triggerScripts, triggerScriptsVector);
	}

	if (config.nonVisual)
	{
		CStr nonVisualScript = "scripts/NonVisualTrigger.js";
		triggerScriptsVector.push_back(nonVisualScript.FromUTF8());
	}

	std::vector<CStr> victoryConditions;
    for (size_t i = 0; i < config.victoryConditions.size(); ++i)
    {
        victoryConditions.push_back(config.victoryConditions[i]);
        std::cout << "victory condition: " << config.victoryConditions[i] << std::endl;
    }

	if (victoryConditions.size() == 1 && victoryConditions[0] == "endless")
		victoryConditions.clear();

    // FIXME: Remove one of these
	//scriptInterface.SetProperty(settings, "victoryConditions", victoryConditions);
	scriptInterface.SetProperty(settings, "VictoryConditions", victoryConditions);

	for (const CStr& victory : victoryConditions)
	{
		JS::RootedValue scriptData(cx);
		JS::RootedValue data(cx);
		JS::RootedValue victoryScripts(cx);

		CStrW scriptPath = L"simulation/data/settings/victory_conditions/" + victory.FromUTF8() + L".json";
		scriptInterface.ReadJSONFile(scriptPath, &scriptData);
		if (!scriptData.isUndefined() && scriptInterface.GetProperty(scriptData, "Data", &data) && !data.isUndefined()
			&& scriptInterface.GetProperty(data, "Scripts", &victoryScripts) && !victoryScripts.isUndefined())
		{
			std::vector<CStrW> victoryScriptsVector;
			FromJSVal_vector(cx, victoryScripts, victoryScriptsVector);
			triggerScriptsVector.insert(triggerScriptsVector.end(), victoryScriptsVector.begin(), victoryScriptsVector.end());
		}
		else
		{
			LOGERROR("Autostart: Error reading victory script '%s'", utf8_from_wstring(scriptPath));
			throw PSERROR_Game_World_MapLoadFailed("Error reading victory script.\nCheck application log for details.");
		}
	}

	ToJSVal_vector(cx, &triggerScripts, triggerScriptsVector);
	scriptInterface.SetProperty(settings, "TriggerScripts", triggerScripts);

	scriptInterface.SetProperty(settings, "WonderDuration", config.wonderDuration);
	scriptInterface.SetProperty(settings, "RelicDuration", config.relicDuration);
	scriptInterface.SetProperty(settings, "RelicCount", config.relicCount);

	if (!config.isNetworkHost() && !config.isNetworkClient())
	{
		game->SetPlayerID(config.playerID);
		//game->GetSimulation2()->SetMapSettings(settings);
		//game->GetSimulation2()->LoadMapSettings();
		game->StartGame(&attrs, "");
	}

    return game;
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

void CGame::SetTurnManager(CTurnManager* turnManager)
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

	CReplayTurnManager* replayTurnMgr = static_cast<CReplayTurnManager*>(GetTurnManager());

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

bool CGame::StartVisualReplay(const OsPath& replayPath)
{
	debug_printf("Starting to replay %s\n", replayPath.string8().c_str());

	m_IsVisualReplay = true;

	SetTurnManager(new CReplayTurnManager(*m_Simulation2, GetReplayLogger()));

	m_ReplayPath = replayPath;
	m_ReplayStream = new std::ifstream(OsString(replayPath).c_str());

	std::string type;
	ENSURE((*m_ReplayStream >> type).good() && type == "start");

	std::string line;
	std::getline(*m_ReplayStream, line);

	const ScriptInterface& scriptInterface = m_Simulation2->GetScriptInterface();
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue attribs(cx);
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
	const ScriptInterface& scriptInterface = m_Simulation2->GetScriptInterface();
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

		m_Simulation2->InitGame();
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
	m_ViewedPlayerID = playerID;

	if (m_TurnManager)
		m_TurnManager->SetPlayerID(m_PlayerID);
}

int CGame::GetViewedPlayerID()
{
	return m_ViewedPlayerID;
}

void CGame::SetViewedPlayerID(player_id_t playerID)
{
	m_ViewedPlayerID = playerID;
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
		m_TurnManager->Interpolate(deltaSimTime, deltaRealTime);
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
			m_PlayerColors[i] = cmpPlayer->GetDisplayedColor();
	}
}


CColor CGame::GetPlayerColor(player_id_t player) const
{
	if (player < 0 || player >= (int)m_PlayerColors.size())
		return BrokenColor;

	return m_PlayerColors[player];
}

bool CGame::IsGameFinished() const
{
	for (const std::pair<entity_id_t, IComponent*>& p : m_Simulation2->GetEntitiesWithInterface(IID_Player))
	{
		CmpPtr<ICmpPlayer> cmpPlayer(*m_Simulation2, p.first);
		if (cmpPlayer && cmpPlayer->GetState() == "won")
			return true;
	}

	return false;
}
