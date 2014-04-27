/* Copyright (C) 2013 Wildfire Games.
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

#ifndef INCLUDED_JSI_LOBBY
#define INCLUDED_JSI_LOBBY

#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptVal.h"
#include "lib/config2.h" // for CONFIG2_LOBBY

namespace JSI_Lobby
{
	bool HasXmppClient(ScriptInterface::CxPrivate* pCxPrivate);
	bool IsRankedGame(ScriptInterface::CxPrivate* pCxPrivate);
	void SetRankedGame(ScriptInterface::CxPrivate* pCxPrivate, bool isRanked);

#if CONFIG2_LOBBY
	void StartXmppClient(ScriptInterface::CxPrivate* pCxPrivate, std::wstring username, std::wstring password, std::wstring room, std::wstring nick, int historyRequestSize);
	void StartRegisterXmppClient(ScriptInterface::CxPrivate* pCxPrivate, std::wstring username, std::wstring password);
	void StopXmppClient(ScriptInterface::CxPrivate* pCxPrivate);
	void ConnectXmppClient(ScriptInterface::CxPrivate* pCxPrivate);
	void DisconnectXmppClient(ScriptInterface::CxPrivate* pCxPrivate);
	void SendGetGameList(ScriptInterface::CxPrivate* pCxPrivate);
	void SendGetBoardList(ScriptInterface::CxPrivate* pCxPrivate);
	void SendGetRatingList(ScriptInterface::CxPrivate* pCxPrivate);
	void SendGameReport(ScriptInterface::CxPrivate* pCxPrivate, CScriptVal data);
	void SendRegisterGame(ScriptInterface::CxPrivate* pCxPrivate, CScriptVal data);
	void SendUnregisterGame(ScriptInterface::CxPrivate* pCxPrivate);
	void SendChangeStateGame(ScriptInterface::CxPrivate* pCxPrivate, std::wstring nbp, std::wstring players);
	CScriptVal GetPlayerList(ScriptInterface::CxPrivate* pCxPrivate);
	CScriptVal GetGameList(ScriptInterface::CxPrivate* pCxPrivate);
	CScriptVal GetBoardList(ScriptInterface::CxPrivate* pCxPrivate);
	CScriptVal LobbyGuiPollMessage(ScriptInterface::CxPrivate* pCxPrivate);
	void LobbySendMessage(ScriptInterface::CxPrivate* pCxPrivate, std::wstring message);
	void LobbySetPlayerPresence(ScriptInterface::CxPrivate* pCxPrivate, std::wstring presence);
	void LobbySetNick(ScriptInterface::CxPrivate* pCxPrivate, std::wstring nick);
	std::wstring LobbyGetNick(ScriptInterface::CxPrivate* pCxPrivate);
	void LobbyKick(ScriptInterface::CxPrivate* pCxPrivate, std::wstring nick, std::wstring reason);
	void LobbyBan(ScriptInterface::CxPrivate* pCxPrivate, std::wstring nick, std::wstring reason);
	std::wstring LobbyGetPlayerPresence(ScriptInterface::CxPrivate* pCxPrivate, std::wstring nickname);
	std::wstring LobbyGetPlayerRole(ScriptInterface::CxPrivate* pCxPrivate, std::wstring nickname);
	std::wstring LobbyGetRoomSubject(ScriptInterface::CxPrivate* pCxPrivate);

	// Non-public secure PBKDF2 hash function with salting and 1,337 iterations
	std::string EncryptPassword(const std::string& password, const std::string& username);

	// Public hash interface.
	std::wstring EncryptPassword(ScriptInterface::CxPrivate* pCxPrivate, std::wstring pass, std::wstring user);
#endif // CONFIG2_LOBBY
}

#endif

