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

#ifndef INCLUDED_JSI_NETWORK
#define INCLUDED_JSI_NETWORK

#include "lib/types.h"
#include "ps/CStr.h"
#include "scriptinterface/ScriptInterface.h"

namespace JSI_Network
{
	u16 GetDefaultPort(ScriptInterface::RealmPrivate* pRealmPrivate);
	bool HasNetServer(ScriptInterface::RealmPrivate* pRealmPrivate);
	bool HasNetClient(ScriptInterface::RealmPrivate* pRealmPrivate);
	void StartNetworkGame(ScriptInterface::RealmPrivate* pRealmPrivate);
	void SetNetworkGameAttributes(ScriptInterface::RealmPrivate* pRealmPrivate, JS::HandleValue attribs1);
	void StartNetworkHost(ScriptInterface::RealmPrivate* pRealmPrivate, const CStrW& playerName, const u16 serverPort, const CStr& hostLobbyName);
	void StartNetworkJoin(ScriptInterface::RealmPrivate* pRealmPrivate, const CStrW& playerName, const CStr& serverAddress, u16 serverPort, bool useSTUN, const CStr& hostJID);
	JS::Value FindStunEndpoint(ScriptInterface::RealmPrivate* pRealmPrivate, int port);
	void DisconnectNetworkGame(ScriptInterface::RealmPrivate* pRealmPrivate);
	JS::Value PollNetworkClient(ScriptInterface::RealmPrivate* pRealmPrivate);
	CStr GetPlayerGUID(ScriptInterface::RealmPrivate* pRealmPrivate);
	void KickPlayer(ScriptInterface::RealmPrivate* pRealmPrivate, const CStrW& playerName, bool ban);
	void AssignNetworkPlayer(ScriptInterface::RealmPrivate* pRealmPrivate, int playerID, const CStr& guid);
	void ClearAllPlayerReady (ScriptInterface::RealmPrivate* pRealmPrivate);
	void SendNetworkChat(ScriptInterface::RealmPrivate* pRealmPrivate, const CStrW& message);
	void SendNetworkReady(ScriptInterface::RealmPrivate* pRealmPrivate, int message);
	void SetTurnLength(ScriptInterface::RealmPrivate* pRealmPrivate, int length);

	void RegisterScriptFunctions(const ScriptInterface& scriptInterface);
}

#endif // INCLUDED_JSI_NETWORK
