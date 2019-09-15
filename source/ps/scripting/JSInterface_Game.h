/* Copyright (C) 2018 Wildfire Games.
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

#ifndef INCLUDED_JSI_GAME
#define INCLUDED_JSI_GAME

#include "scriptinterface/ScriptInterface.h"

namespace JSI_Game
{
	bool IsGameStarted(ScriptInterface::CxPrivate* pCxPrivate);
	void StartGame(ScriptInterface::CxPrivate* pCxPrivate, JS::HandleValue attribs, int playerID);
	void Script_EndGame(ScriptInterface::CxPrivate* pCxPrivate);
	int GetPlayerID(ScriptInterface::CxPrivate* pCxPrivate);
	void SetPlayerID(ScriptInterface::CxPrivate* pCxPrivate, int id);
	void SetViewedPlayer(ScriptInterface::CxPrivate* pCxPrivate, int id);
	float GetSimRate(ScriptInterface::CxPrivate* pCxPrivate);
	void SetSimRate(ScriptInterface::CxPrivate* pCxPrivate, float rate);
	bool IsPaused(ScriptInterface::CxPrivate* pCxPrivate);
	void SetPaused(ScriptInterface::CxPrivate* pCxPrivate, bool pause, bool sendMessage);
	bool IsVisualReplay(ScriptInterface::CxPrivate* pCxPrivate);
	std::wstring GetCurrentReplayDirectory(ScriptInterface::CxPrivate* pCxPrivate);
	void RewindTimeWarp(ScriptInterface::CxPrivate* pCxPrivate);
	void EnableTimeWarpRecording(ScriptInterface::CxPrivate* pCxPrivate, unsigned int numTurns);
	void DumpTerrainMipmap(ScriptInterface::CxPrivate* pCxPrivate);

	void RegisterScriptFunctions(const ScriptInterface& ScriptInterface);
}

#endif // INCLUDED_JSI_GAME
