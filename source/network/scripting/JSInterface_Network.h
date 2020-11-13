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
	u16 GetDefaultPort(ScriptInterface::CmptPrivate* pCmptPrivate);
	bool HasNetServer(ScriptInterface::CmptPrivate* pCmptPrivate);
	bool HasNetClient(ScriptInterface::CmptPrivate* pCmptPrivate);
	void StartNetworkGame(ScriptInterface::CmptPrivate* pCmptPrivate);
	void SetNetworkGameAttributes(ScriptInterface::CmptPrivate* pCmptPrivate, JS::HandleValue attribs1);
	void StartNetworkHost(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& playerName, const u16 serverPort, const CStr& hostLobbyName);
	void StartNetworkJoin(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& playerName, const CStr& serverAddress, u16 serverPort, bool useSTUN, const CStr& hostJID);
	JS::Value FindStunEndpoint(ScriptInterface::CmptPrivate* pCmptPrivate, int port);
	void DisconnectNetworkGame(ScriptInterface::CmptPrivate* pCmptPrivate);
	JS::Value PollNetworkClient(ScriptInterface::CmptPrivate* pCmptPrivate);
	CStr GetPlayerGUID(ScriptInterface::CmptPrivate* pCmptPrivate);
	void KickPlayer(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& playerName, bool ban);
	void AssignNetworkPlayer(ScriptInterface::CmptPrivate* pCmptPrivate, int playerID, const CStr& guid);
	void ClearAllPlayerReady (ScriptInterface::CmptPrivate* pCmptPrivate);
	void SendNetworkChat(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& message);
	void SendNetworkReady(ScriptInterface::CmptPrivate* pCmptPrivate, int message);
	void SetTurnLength(ScriptInterface::CmptPrivate* pCmptPrivate, int length);

	void RegisterScriptFunctions(const ScriptInterface& scriptInterface);
}

#endif // INCLUDED_JSI_NETWORK
