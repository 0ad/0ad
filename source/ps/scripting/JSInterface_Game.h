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
	bool IsGameStarted(ScriptInterface::CmptPrivate* pCmptPrivate);
	void StartGame(ScriptInterface::CmptPrivate* pCmptPrivate, JS::HandleValue attribs, int playerID);
	void Script_EndGame(ScriptInterface::CmptPrivate* pCmptPrivate);
	int GetPlayerID(ScriptInterface::CmptPrivate* pCmptPrivate);
	void SetPlayerID(ScriptInterface::CmptPrivate* pCmptPrivate, int id);
	void SetViewedPlayer(ScriptInterface::CmptPrivate* pCmptPrivate, int id);
	float GetSimRate(ScriptInterface::CmptPrivate* pCmptPrivate);
	void SetSimRate(ScriptInterface::CmptPrivate* pCmptPrivate, float rate);
	bool IsPaused(ScriptInterface::CmptPrivate* pCmptPrivate);
	void SetPaused(ScriptInterface::CmptPrivate* pCmptPrivate, bool pause, bool sendMessage);
	bool IsVisualReplay(ScriptInterface::CmptPrivate* pCmptPrivate);
	std::wstring GetCurrentReplayDirectory(ScriptInterface::CmptPrivate* pCmptPrivate);
	void RewindTimeWarp(ScriptInterface::CmptPrivate* pCmptPrivate);
	void EnableTimeWarpRecording(ScriptInterface::CmptPrivate* pCmptPrivate, unsigned int numTurns);
	void DumpTerrainMipmap(ScriptInterface::CmptPrivate* pCmptPrivate);

	void RegisterScriptFunctions(const ScriptInterface& ScriptInterface);
}

#endif // INCLUDED_JSI_GAME
