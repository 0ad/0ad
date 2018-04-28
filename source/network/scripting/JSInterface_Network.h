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

#include "ps/CStr.h"
#include "scriptinterface/ScriptInterface.h"

namespace JSI_Network
{
	u16 GetDefaultPort(ScriptInterface::CxPrivate* pCxPrivate);
	bool HasNetServer(ScriptInterface::CxPrivate* pCxPrivate);
	bool HasNetClient(ScriptInterface::CxPrivate* pCxPrivate);
	void StartNetworkGame(ScriptInterface::CxPrivate* pCxPrivate);
	void SetNetworkGameAttributes(ScriptInterface::CxPrivate* pCxPrivate, JS::HandleValue attribs1);
	void StartNetworkHost(ScriptInterface::CxPrivate* pCxPrivate, const CStrW& playerName, const u16 serverPort, const CStr& hostLobbyName, bool useLobbyAuth);
	void StartNetworkJoin(ScriptInterface::CxPrivate* pCxPrivate, const CStrW& playerName, const CStr& serverAddress, u16 serverPort, bool useSTUN, const CStr& hostJID);
	JS::Value FindStunEndpoint(ScriptInterface::CxPrivate* pCxPrivate, int port);
	void DisconnectNetworkGame(ScriptInterface::CxPrivate* pCxPrivate);
	JS::Value PollNetworkClient(ScriptInterface::CxPrivate* pCxPrivate);
	CStr GetPlayerGUID(ScriptInterface::CxPrivate* pCxPrivate);
	void KickPlayer(ScriptInterface::CxPrivate* pCxPrivate, const CStrW& playerName, bool ban);
	void AssignNetworkPlayer(ScriptInterface::CxPrivate* pCxPrivate, int playerID, const CStr& guid);
	void ClearAllPlayerReady (ScriptInterface::CxPrivate* pCxPrivate);
	void SendNetworkChat(ScriptInterface::CxPrivate* pCxPrivate, const CStrW& message);
	void SendNetworkReady(ScriptInterface::CxPrivate* pCxPrivate, int message);
	void SetTurnLength(ScriptInterface::CxPrivate* pCxPrivate, int length);

	void RegisterScriptFunctions(const ScriptInterface& scriptInterface);
}

#endif // INCLUDED_JSI_NETWORK
