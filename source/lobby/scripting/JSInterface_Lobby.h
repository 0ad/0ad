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

	bool HasXmppClient(ScriptInterface::RealmPrivate* pRealmPrivate);
	bool IsRankedGame(ScriptInterface::RealmPrivate* pRealmPrivate);
	void SetRankedGame(ScriptInterface::RealmPrivate* pRealmPrivate, bool isRanked);

#if CONFIG2_LOBBY
	void StartXmppClient(ScriptInterface::RealmPrivate* pRealmPrivate, const std::wstring& username, const std::wstring& password, const std::wstring& room, const std::wstring& nick, int historyRequestSize);
	void StartRegisterXmppClient(ScriptInterface::RealmPrivate* pRealmPrivate, const std::wstring& username, const std::wstring& password);
	void StopXmppClient(ScriptInterface::RealmPrivate* pRealmPrivate);
	void ConnectXmppClient(ScriptInterface::RealmPrivate* pRealmPrivate);
	void DisconnectXmppClient(ScriptInterface::RealmPrivate* pRealmPrivate);
	bool IsXmppClientConnected(ScriptInterface::RealmPrivate* pRealmPrivate);
	void SendGetBoardList(ScriptInterface::RealmPrivate* pRealmPrivate);
	void SendGetProfile(ScriptInterface::RealmPrivate* pRealmPrivate, const std::wstring& player);
	void SendGameReport(ScriptInterface::RealmPrivate* pRealmPrivate, JS::HandleValue data);
	void SendRegisterGame(ScriptInterface::RealmPrivate* pRealmPrivate, JS::HandleValue data);
	void SendUnregisterGame(ScriptInterface::RealmPrivate* pRealmPrivate);
	void SendChangeStateGame(ScriptInterface::RealmPrivate* pRealmPrivate, const std::wstring& nbp, const std::wstring& players);
	JS::Value GetPlayerList(ScriptInterface::RealmPrivate* pRealmPrivate);
	JS::Value GetGameList(ScriptInterface::RealmPrivate* pRealmPrivate);
	JS::Value GetBoardList(ScriptInterface::RealmPrivate* pRealmPrivate);
	JS::Value GetProfile(ScriptInterface::RealmPrivate* pRealmPrivate);
	JS::Value LobbyGuiPollNewMessages(ScriptInterface::RealmPrivate* pRealmPrivate);
	JS::Value LobbyGuiPollHistoricMessages(ScriptInterface::RealmPrivate* pRealmPrivate);
	bool LobbyGuiPollHasPlayerListUpdate(ScriptInterface::RealmPrivate* pRealmPrivate);
	void LobbySendMessage(ScriptInterface::RealmPrivate* pRealmPrivate, const std::wstring& message);
	void LobbySetPlayerPresence(ScriptInterface::RealmPrivate* pRealmPrivate, const std::wstring& presence);
	void LobbySetNick(ScriptInterface::RealmPrivate* pRealmPrivate, const std::wstring& nick);
	std::wstring LobbyGetNick(ScriptInterface::RealmPrivate* pRealmPrivate);
	void LobbyKick(ScriptInterface::RealmPrivate* pRealmPrivate, const std::wstring& nick, const std::wstring& reason);
	void LobbyBan(ScriptInterface::RealmPrivate* pRealmPrivate, const std::wstring& nick, const std::wstring& reason);
	const char* LobbyGetPlayerPresence(ScriptInterface::RealmPrivate* pRealmPrivate, const std::wstring& nickname);
	const char* LobbyGetPlayerRole(ScriptInterface::RealmPrivate* pRealmPrivate, const std::wstring& nickname);
	std::wstring LobbyGetPlayerRating(ScriptInterface::RealmPrivate* pRealmPrivate, const std::wstring& nickname);
	std::wstring LobbyGetRoomSubject(ScriptInterface::RealmPrivate* pRealmPrivate);

	// Non-public secure PBKDF2 hash function with salting and 1,337 iterations
	std::string EncryptPassword(const std::string& password, const std::string& username);

	// Public hash interface.
	std::wstring EncryptPassword(ScriptInterface::RealmPrivate* pRealmPrivate, const std::wstring& pass, const std::wstring& user);
#endif // CONFIG2_LOBBY
}

#endif // INCLUDED_JSI_LOBBY
