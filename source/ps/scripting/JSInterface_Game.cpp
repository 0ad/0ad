/* Copyright (C) 2020 Wildfire Games.
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
#include "ps/Replay.h"
#include "ps/World.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/system/TurnManager.h"
#include "simulation2/Simulation2.h"
#include "soundmanager/SoundManager.h"

extern void EndGame();

bool JSI_Game::IsGameStarted(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	return g_Game;
}

void JSI_Game::StartGame(ScriptInterface::CmptPrivate* pCmptPrivate, JS::HandleValue attribs, int playerID)
{
	ENSURE(!g_NetServer);
	ENSURE(!g_NetClient);
	ENSURE(!g_Game);

	g_Game = new CGame(true);

	// Convert from GUI script context to sim script context
	CSimulation2* sim = g_Game->GetSimulation2();
	ScriptInterface::Request rqSim(sim->GetScriptInterface());

	JS::RootedValue gameAttribs(rqSim.cx,
		sim->GetScriptInterface().CloneValueFromOtherCompartment(*(pCmptPrivate->pScriptInterface), attribs));

	g_Game->SetPlayerID(playerID);
	g_Game->StartGame(&gameAttribs, "");
}

void JSI_Game::Script_EndGame(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	EndGame();
}

int JSI_Game::GetPlayerID(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	if (!g_Game)
		return -1;

	return g_Game->GetPlayerID();
}

void JSI_Game::SetPlayerID(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), int id)
{
	if (!g_Game)
		return;

	g_Game->SetPlayerID(id);
}

void JSI_Game::SetViewedPlayer(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), int id)
{
	if (!g_Game)
		return;

	g_Game->SetViewedPlayerID(id);
}

float JSI_Game::GetSimRate(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	return g_Game->GetSimRate();
}

void JSI_Game::SetSimRate(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), float rate)
{
	g_Game->SetSimRate(rate);
}

bool JSI_Game::IsPaused(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	if (!g_Game)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Game is not started");
		return false;
	}

	return g_Game->m_Paused;
}

void JSI_Game::SetPaused(ScriptInterface::CmptPrivate* pCmptPrivate, bool pause, bool sendMessage)
{
	if (!g_Game)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Game is not started");
		return;
	}

	g_Game->m_Paused = pause;

#if CONFIG2_AUDIO
	if (g_SoundManager)
		g_SoundManager->Pause(pause);
#endif

	if (g_NetClient && sendMessage)
		g_NetClient->SendPausedMessage(pause);
}

bool JSI_Game::IsVisualReplay(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	if (!g_Game)
		return false;

	return g_Game->IsVisualReplay();
}

std::wstring JSI_Game::GetCurrentReplayDirectory(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	if (!g_Game)
		return std::wstring();

	if (g_Game->IsVisualReplay())
		return g_Game->GetReplayPath().Parent().Filename().string();

	return g_Game->GetReplayLogger().GetDirectory().Filename().string();
}

void JSI_Game::EnableTimeWarpRecording(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), unsigned int numTurns)
{
	g_Game->GetTurnManager()->EnableTimeWarpRecording(numTurns);
}

void JSI_Game::RewindTimeWarp(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	g_Game->GetTurnManager()->RewindTimeWarp();
}

void JSI_Game::DumpTerrainMipmap(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	VfsPath filename(L"screenshots/terrainmipmap.png");
	g_Game->GetWorld()->GetTerrain()->GetHeightMipmap().DumpToDisk(filename);
	OsPath realPath;
	g_VFS->GetRealPath(filename, realPath);
	LOGMESSAGERENDER("Terrain mipmap written to '%s'", realPath.string8());
}

void JSI_Game::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<bool, &IsGameStarted>("IsGameStarted");
	scriptInterface.RegisterFunction<void, JS::HandleValue, int, &StartGame>("StartGame");
	scriptInterface.RegisterFunction<void, &Script_EndGame>("EndGame");
	scriptInterface.RegisterFunction<int, &GetPlayerID>("GetPlayerID");
	scriptInterface.RegisterFunction<void, int, &SetPlayerID>("SetPlayerID");
	scriptInterface.RegisterFunction<void, int, &SetViewedPlayer>("SetViewedPlayer");
	scriptInterface.RegisterFunction<float, &GetSimRate>("GetSimRate");
	scriptInterface.RegisterFunction<void, float, &SetSimRate>("SetSimRate");
	scriptInterface.RegisterFunction<bool, &IsPaused>("IsPaused");
	scriptInterface.RegisterFunction<void, bool, bool, &SetPaused>("SetPaused");
	scriptInterface.RegisterFunction<bool, &IsVisualReplay>("IsVisualReplay");
	scriptInterface.RegisterFunction<std::wstring, &GetCurrentReplayDirectory>("GetCurrentReplayDirectory");
	scriptInterface.RegisterFunction<void, unsigned int, &EnableTimeWarpRecording>("EnableTimeWarpRecording");
	scriptInterface.RegisterFunction<void, &RewindTimeWarp>("RewindTimeWarp");
	scriptInterface.RegisterFunction<void, &DumpTerrainMipmap>("DumpTerrainMipmap");
}
