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
	bool IsGameStarted(ScriptInterface::RealmPrivate* pRealmPrivate);
	void StartGame(ScriptInterface::RealmPrivate* pRealmPrivate, JS::HandleValue attribs, int playerID);
	void Script_EndGame(ScriptInterface::RealmPrivate* pRealmPrivate);
	int GetPlayerID(ScriptInterface::RealmPrivate* pRealmPrivate);
	void SetPlayerID(ScriptInterface::RealmPrivate* pRealmPrivate, int id);
	void SetViewedPlayer(ScriptInterface::RealmPrivate* pRealmPrivate, int id);
	float GetSimRate(ScriptInterface::RealmPrivate* pRealmPrivate);
	void SetSimRate(ScriptInterface::RealmPrivate* pRealmPrivate, float rate);
	bool IsPaused(ScriptInterface::RealmPrivate* pRealmPrivate);
	void SetPaused(ScriptInterface::RealmPrivate* pRealmPrivate, bool pause, bool sendMessage);
	bool IsVisualReplay(ScriptInterface::RealmPrivate* pRealmPrivate);
	std::wstring GetCurrentReplayDirectory(ScriptInterface::RealmPrivate* pRealmPrivate);
	void RewindTimeWarp(ScriptInterface::RealmPrivate* pRealmPrivate);
	void EnableTimeWarpRecording(ScriptInterface::RealmPrivate* pRealmPrivate, unsigned int numTurns);
	void DumpTerrainMipmap(ScriptInterface::RealmPrivate* pRealmPrivate);

	void RegisterScriptFunctions(const ScriptInterface& ScriptInterface);
}

#endif // INCLUDED_JSI_GAME
