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

#include "scriptinterface/ScriptVal.h"
#include "lib/config2.h" // for CONFIG2_LOBBY

class ScriptInterface;

namespace JSI_Lobby
{
	bool HasXmppClient(void* cbdata);
	
#if CONFIG2_LOBBY 
	void StartXmppClient(void* cbdata, std::wstring username, std::wstring password, std::wstring room, std::wstring nick);
	void StartRegisterXmppClient(void* cbdata, std::wstring username, std::wstring password);
	void StopXmppClient(void* cbdata);
	void ConnectXmppClient(void* cbdata);
	void DisconnectXmppClient(void* cbdata);
	void RecvXmppClient(void* cbdata);
	void SendGetGameList(void* cbdata);
	void SendGetBoardList(void* cbdata);
	void SendGameReport(void* cbdata, CScriptVal data);
	void SendRegisterGame(void* cbdata, CScriptVal data);
	void SendUnregisterGame(void* cbdata);
	void SendChangeStateGame(void* cbdata, std::wstring nbp, std::wstring players);
	CScriptVal GetPlayerList(void* cbdata);
	CScriptVal GetGameList(void* cbdata);
	CScriptVal GetBoardList(void* cbdata);
	CScriptVal LobbyGuiPollMessage(void* cbdata);
	void LobbySendMessage(void* cbdata, std::wstring message);
	void LobbySetPlayerPresence(void* cbdata, std::wstring presence);
	void LobbySetNick(void* cbdata, std::wstring nick);
	std::wstring LobbyGetNick(void* cbdata);
	void LobbyKick(void* cbdata, std::wstring nick, std::wstring reason);
	void LobbyBan(void* cbdata, std::wstring nick, std::wstring reason);
	std::wstring LobbyGetPlayerPresence(void* cbdata, std::wstring nickname);

	// Non-public secure PBKDF2 hash function with salting and 1,337 iterations
	std::string EncryptPassword(const std::string& password, const std::string& username);

	// Public hash interface.
	std::wstring EncryptPassword(void* cbdata, std::wstring pass, std::wstring user);
	
	bool IsRankedGame(void* cbdata);
	void SetRankedGame(void* cbdata, bool isRanked);
#endif // CONFIG2_LOBBY 
}

#endif