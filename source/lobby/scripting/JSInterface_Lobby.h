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

#ifndef INCLUDED_JSI_LOBBY
#define INCLUDED_JSI_LOBBY

#include "lib/config2.h"
#include "scriptinterface/ScriptInterface.h"

#include <string>

namespace JSI_Lobby
{
	void RegisterScriptFunctions(const ScriptInterface& scriptInterface);

	bool HasXmppClient(ScriptInterface::CxPrivate* pCxPrivate);
	bool IsRankedGame(ScriptInterface::CxPrivate* pCxPrivate);
	void SetRankedGame(ScriptInterface::CxPrivate* pCxPrivate, bool isRanked);

#if CONFIG2_LOBBY
	void StartXmppClient(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& username, const std::wstring& password, const std::wstring& room, const std::wstring& nick, int historyRequestSize);
	void StartRegisterXmppClient(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& username, const std::wstring& password);
	void StopXmppClient(ScriptInterface::CxPrivate* pCxPrivate);
	void ConnectXmppClient(ScriptInterface::CxPrivate* pCxPrivate);
	void DisconnectXmppClient(ScriptInterface::CxPrivate* pCxPrivate);
	bool IsXmppClientConnected(ScriptInterface::CxPrivate* pCxPrivate);
	void SendGetBoardList(ScriptInterface::CxPrivate* pCxPrivate);
	void SendGetProfile(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& player);
	void SendGameReport(ScriptInterface::CxPrivate* pCxPrivate, JS::HandleValue data);
	void SendRegisterGame(ScriptInterface::CxPrivate* pCxPrivate, JS::HandleValue data);
	void SendUnregisterGame(ScriptInterface::CxPrivate* pCxPrivate);
	void SendChangeStateGame(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& nbp, const std::wstring& players);
	JS::Value GetPlayerList(ScriptInterface::CxPrivate* pCxPrivate);
	JS::Value GetGameList(ScriptInterface::CxPrivate* pCxPrivate);
	JS::Value GetBoardList(ScriptInterface::CxPrivate* pCxPrivate);
	JS::Value GetProfile(ScriptInterface::CxPrivate* pCxPrivate);
	JS::Value LobbyGuiPollNewMessage(ScriptInterface::CxPrivate* pCxPrivate);
	JS::Value LobbyGuiPollHistoricMessages(ScriptInterface::CxPrivate* pCxPrivate);
	bool LobbyGuiPollHasPlayerListUpdate(ScriptInterface::CxPrivate* pCxPrivate);
	void LobbySendMessage(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& message);
	void LobbySetPlayerPresence(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& presence);
	void LobbySetNick(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& nick);
	std::wstring LobbyGetNick(ScriptInterface::CxPrivate* pCxPrivate);
	void LobbyKick(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& nick, const std::wstring& reason);
	void LobbyBan(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& nick, const std::wstring& reason);
	std::wstring LobbyGetPlayerPresence(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& nickname);
	std::wstring LobbyGetPlayerRole(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& nickname);
	std::wstring LobbyGetRoomSubject(ScriptInterface::CxPrivate* pCxPrivate);

	// Non-public secure PBKDF2 hash function with salting and 1,337 iterations
	std::string EncryptPassword(const std::string& password, const std::string& username);

	// Public hash interface.
	std::wstring EncryptPassword(ScriptInterface::CxPrivate* pCxPrivate, const std::wstring& pass, const std::wstring& user);
#endif // CONFIG2_LOBBY
}

#endif // INCLUDED_JSI_LOBBY
