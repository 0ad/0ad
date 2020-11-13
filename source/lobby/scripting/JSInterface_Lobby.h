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

	bool HasXmppClient(ScriptInterface::CmptPrivate* pCmptPrivate);
	bool IsRankedGame(ScriptInterface::CmptPrivate* pCmptPrivate);
	void SetRankedGame(ScriptInterface::CmptPrivate* pCmptPrivate, bool isRanked);

#if CONFIG2_LOBBY
	void StartXmppClient(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& username, const std::wstring& password, const std::wstring& room, const std::wstring& nick, int historyRequestSize);
	void StartRegisterXmppClient(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& username, const std::wstring& password);
	void StopXmppClient(ScriptInterface::CmptPrivate* pCmptPrivate);
	void ConnectXmppClient(ScriptInterface::CmptPrivate* pCmptPrivate);
	void DisconnectXmppClient(ScriptInterface::CmptPrivate* pCmptPrivate);
	bool IsXmppClientConnected(ScriptInterface::CmptPrivate* pCmptPrivate);
	void SendGetBoardList(ScriptInterface::CmptPrivate* pCmptPrivate);
	void SendGetProfile(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& player);
	void SendGameReport(ScriptInterface::CmptPrivate* pCmptPrivate, JS::HandleValue data);
	void SendRegisterGame(ScriptInterface::CmptPrivate* pCmptPrivate, JS::HandleValue data);
	void SendUnregisterGame(ScriptInterface::CmptPrivate* pCmptPrivate);
	void SendChangeStateGame(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& nbp, const std::wstring& players);
	JS::Value GetPlayerList(ScriptInterface::CmptPrivate* pCmptPrivate);
	JS::Value GetGameList(ScriptInterface::CmptPrivate* pCmptPrivate);
	JS::Value GetBoardList(ScriptInterface::CmptPrivate* pCmptPrivate);
	JS::Value GetProfile(ScriptInterface::CmptPrivate* pCmptPrivate);
	JS::Value LobbyGuiPollNewMessages(ScriptInterface::CmptPrivate* pCmptPrivate);
	JS::Value LobbyGuiPollHistoricMessages(ScriptInterface::CmptPrivate* pCmptPrivate);
	bool LobbyGuiPollHasPlayerListUpdate(ScriptInterface::CmptPrivate* pCmptPrivate);
	void LobbySendMessage(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& message);
	void LobbySetPlayerPresence(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& presence);
	void LobbySetNick(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& nick);
	std::wstring LobbyGetNick(ScriptInterface::CmptPrivate* pCmptPrivate);
	void LobbyKick(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& nick, const std::wstring& reason);
	void LobbyBan(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& nick, const std::wstring& reason);
	const char* LobbyGetPlayerPresence(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& nickname);
	const char* LobbyGetPlayerRole(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& nickname);
	std::wstring LobbyGetPlayerRating(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& nickname);
	std::wstring LobbyGetRoomSubject(ScriptInterface::CmptPrivate* pCmptPrivate);

	// Non-public secure PBKDF2 hash function with salting and 1,337 iterations
	std::string EncryptPassword(const std::string& password, const std::string& username);

	// Public hash interface.
	std::wstring EncryptPassword(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& pass, const std::wstring& user);
#endif // CONFIG2_LOBBY
}

#endif // INCLUDED_JSI_LOBBY
