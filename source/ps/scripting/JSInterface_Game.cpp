/* Copyright (C) 2021 Wildfire Games.
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

#include "JSInterface_Game.h"

#include "graphics/Terrain.h"
#include "network/NetClient.h"
#include "network/NetServer.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/Replay.h"
#include "ps/World.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/system/TurnManager.h"
#include "simulation2/Simulation2.h"
#include "soundmanager/SoundManager.h"

namespace JSI_Game
{
bool IsGameStarted()
{
	return g_Game;
}

void StartGame(ScriptInterface::CmptPrivate* pCmptPrivate, JS::HandleValue attribs, int playerID)
{
	ENSURE(!g_NetServer);
	ENSURE(!g_NetClient);
	ENSURE(!g_Game);

	g_Game = new CGame(true);

	// Convert from GUI script context to sim script context
	CSimulation2* sim = g_Game->GetSimulation2();
	ScriptRequest rqSim(sim->GetScriptInterface());

	JS::RootedValue gameAttribs(rqSim.cx,
		sim->GetScriptInterface().CloneValueFromOtherCompartment(*(pCmptPrivate->pScriptInterface), attribs));

	g_Game->SetPlayerID(playerID);
	g_Game->StartGame(&gameAttribs, "");
}

void Script_EndGame()
{
	EndGame();
}

int GetPlayerID()
{
	if (!g_Game)
		return -1;

	return g_Game->GetPlayerID();
}

void SetPlayerID(int id)
{
	if (!g_Game)
		return;

	g_Game->SetPlayerID(id);
}

void SetViewedPlayer(int id)
{
	if (!g_Game)
		return;

	g_Game->SetViewedPlayerID(id);
}

float GetSimRate()
{
	return g_Game->GetSimRate();
}

void SetSimRate(float rate)
{
	g_Game->SetSimRate(rate);
}

int GetPendingTurns(const ScriptRequest& rq)
{
	if (!g_Game || !g_Game->GetTurnManager())
	{
		ScriptException::Raise(rq, "Game is not started");
		return 0;
	}

	return g_Game->GetTurnManager()->GetPendingTurns();
}

bool IsPaused(const ScriptRequest& rq)
{
	if (!g_Game)
	{
		ScriptException::Raise(rq, "Game is not started");
		return false;
	}

	return g_Game->m_Paused;
}

void SetPaused(const ScriptRequest& rq, bool pause, bool sendMessage)
{
	if (!g_Game)
	{
		ScriptException::Raise(rq, "Game is not started");
		return;
	}

	g_Game->m_Paused = pause;

#if CONFIG2_AUDIO
	if (g_SoundManager)
	{
		g_SoundManager->PauseAmbient(pause);
		g_SoundManager->PauseAction(pause);
	}
#endif

	if (g_NetClient && sendMessage)
		g_NetClient->SendPausedMessage(pause);
}

bool IsVisualReplay()
{
	if (!g_Game)
		return false;

	return g_Game->IsVisualReplay();
}

std::wstring GetCurrentReplayDirectory()
{
	if (!g_Game)
		return std::wstring();

	if (g_Game->IsVisualReplay())
		return g_Game->GetReplayPath().Parent().Filename().string();

	return g_Game->GetReplayLogger().GetDirectory().Filename().string();
}

void EnableTimeWarpRecording(unsigned int numTurns)
{
	g_Game->GetTurnManager()->EnableTimeWarpRecording(numTurns);
}

void RewindTimeWarp()
{
	g_Game->GetTurnManager()->RewindTimeWarp();
}

void DumpTerrainMipmap()
{
	VfsPath filename(L"screenshots/terrainmipmap.png");
	g_Game->GetWorld()->GetTerrain()->GetHeightMipmap().DumpToDisk(filename);
	OsPath realPath;
	g_VFS->GetRealPath(filename, realPath);
	LOGMESSAGERENDER("Terrain mipmap written to '%s'", realPath.string8());
}

void RegisterScriptFunctions(const ScriptRequest& rq)
{
	ScriptFunction::Register<&IsGameStarted>(rq, "IsGameStarted");
	ScriptFunction::Register<&StartGame>(rq, "StartGame");
	ScriptFunction::Register<&Script_EndGame>(rq, "EndGame");
	ScriptFunction::Register<&GetPlayerID>(rq, "GetPlayerID");
	ScriptFunction::Register<&SetPlayerID>(rq, "SetPlayerID");
	ScriptFunction::Register<&SetViewedPlayer>(rq, "SetViewedPlayer");
	ScriptFunction::Register<&GetSimRate>(rq, "GetSimRate");
	ScriptFunction::Register<&SetSimRate>(rq, "SetSimRate");
	ScriptFunction::Register<&GetPendingTurns>(rq, "GetPendingTurns");
	ScriptFunction::Register<&IsPaused>(rq, "IsPaused");
	ScriptFunction::Register<&SetPaused>(rq, "SetPaused");
	ScriptFunction::Register<&IsVisualReplay>(rq, "IsVisualReplay");
	ScriptFunction::Register<&GetCurrentReplayDirectory>(rq, "GetCurrentReplayDirectory");
	ScriptFunction::Register<&EnableTimeWarpRecording>(rq, "EnableTimeWarpRecording");
	ScriptFunction::Register<&RewindTimeWarp>(rq, "RewindTimeWarp");
	ScriptFunction::Register<&DumpTerrainMipmap>(rq, "DumpTerrainMipmap");
}
}
