/* Copyright (C) 2020 Wildfire Games.
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

#include "precompiled.h"

#include "JSInterface_Network.h"

#include "lib/external_libraries/enet.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/types.h"
#include "lobby/IXmppClient.h"
#include "network/NetClient.h"
#include "network/NetMessage.h"
#include "network/NetServer.h"
#include "network/StunClient.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "scriptinterface/ScriptInterface.h"

u16 JSI_Network::GetDefaultPort(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	return PS_DEFAULT_PORT;
}

bool JSI_Network::HasNetServer(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	return g_NetServer;
}

bool JSI_Network::HasNetClient(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	return g_NetClient;
}

JS::Value JSI_Network::FindStunEndpoint(ScriptInterface::CmptPrivate* pCmptPrivate, int port)
{
	return StunClient::FindStunEndpointHost(*(pCmptPrivate->pScriptInterface), port);
}

void JSI_Network::StartNetworkHost(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& playerName, const u16 serverPort, const CStr& hostLobbyName)
{
	ENSURE(!g_NetClient);
	ENSURE(!g_NetServer);
	ENSURE(!g_Game);

	// Always use lobby authentication for lobby matches to prevent impersonation and smurfing, in particular through mods that implemented an UI for arbitrary or other players nicknames.
	g_NetServer = new CNetServer(static_cast<bool>(g_XmppClient));
	if (!g_NetServer->SetupConnection(serverPort))
	{
		pCmptPrivate->pScriptInterface->ReportError("Failed to start server");
		SAFE_DELETE(g_NetServer);
		return;
	}

	g_Game = new CGame(true);
	g_NetClient = new CNetClient(g_Game, true);
	g_NetClient->SetUserName(playerName);
	g_NetClient->SetHostingPlayerName(hostLobbyName);

	if (!g_NetClient->SetupConnection("127.0.0.1", serverPort, nullptr))
	{
		pCmptPrivate->pScriptInterface->ReportError("Failed to connect to server");
		SAFE_DELETE(g_NetClient);
		SAFE_DELETE(g_Game);
	}
}

void JSI_Network::StartNetworkJoin(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& playerName, const CStr& serverAddress, u16 serverPort, bool useSTUN, const CStr& hostJID)
{
	ENSURE(!g_NetClient);
	ENSURE(!g_NetServer);
	ENSURE(!g_Game);

	ENetHost* enetClient = nullptr;
	if (g_XmppClient && useSTUN)
	{
		// Find an unused port
		for (int i = 0; i < 5 && !enetClient; ++i)
		{
			// Ports below 1024 are privileged on unix
			u16 port = 1024 + rand() % (UINT16_MAX - 1024);
			ENetAddress hostAddr{ENET_HOST_ANY, port};
			enetClient = enet_host_create(&hostAddr, 1, 1, 0, 0);
			++hostAddr.port;
		}

		if (!enetClient)
		{
			pCmptPrivate->pScriptInterface->ReportError("Could not find an unused port for the enet STUN client");
			return;
		}

		StunClient::StunEndpoint stunEndpoint;
		if (!StunClient::FindStunEndpointJoin(*enetClient, stunEndpoint))
		{
			pCmptPrivate->pScriptInterface->ReportError("Could not find the STUN endpoint");
			return;
		}

		g_XmppClient->SendStunEndpointToHost(stunEndpoint, hostJID);

		SDL_Delay(1000);
	}

	g_Game = new CGame(true);
	g_NetClient = new CNetClient(g_Game, false);
	g_NetClient->SetUserName(playerName);
	g_NetClient->SetHostingPlayerName(hostJID.substr(0, hostJID.find("@")));

	if (g_XmppClient && useSTUN)
		StunClient::SendHolePunchingMessages(*enetClient, serverAddress, serverPort);

	if (!g_NetClient->SetupConnection(serverAddress, serverPort, enetClient))
	{
		pCmptPrivate->pScriptInterface->ReportError("Failed to connect to server");
		SAFE_DELETE(g_NetClient);
		SAFE_DELETE(g_Game);
	}
}

void JSI_Network::DisconnectNetworkGame(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	// TODO: we ought to do async reliable disconnections

	SAFE_DELETE(g_NetServer);
	SAFE_DELETE(g_NetClient);
	SAFE_DELETE(g_Game);
}

CStr JSI_Network::GetPlayerGUID(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	if (!g_NetClient)
		return "local";

	return g_NetClient->GetGUID();
}

JS::Value JSI_Network::PollNetworkClient(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	if (!g_NetClient)
		return JS::UndefinedValue();

	// Convert from net client context to GUI script context
	ScriptInterface::Request rqNet(g_NetClient->GetScriptInterface());
	JS::RootedValue pollNet(rqNet.cx);
	g_NetClient->GuiPoll(&pollNet);
	return pCmptPrivate->pScriptInterface->CloneValueFromOtherCompartment(g_NetClient->GetScriptInterface(), pollNet);
}

void JSI_Network::SetNetworkGameAttributes(ScriptInterface::CmptPrivate* pCmptPrivate, JS::HandleValue attribs1)
{
	ENSURE(g_NetClient);

	// TODO: This is a workaround because we need to pass a MutableHandle to a JSAPI functions somewhere (with no obvious reason).
	ScriptInterface::Request rq(pCmptPrivate);
	JS::RootedValue attribs(rq.cx, attribs1);

	g_NetClient->SendGameSetupMessage(&attribs, *(pCmptPrivate->pScriptInterface));
}

void JSI_Network::AssignNetworkPlayer(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), int playerID, const CStr& guid)
{
	ENSURE(g_NetClient);

	g_NetClient->SendAssignPlayerMessage(playerID, guid);
}

void JSI_Network::KickPlayer(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const CStrW& playerName, bool ban)
{
	ENSURE(g_NetClient);

	g_NetClient->SendKickPlayerMessage(playerName, ban);
}

void JSI_Network::SendNetworkChat(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const CStrW& message)
{
	ENSURE(g_NetClient);

	g_NetClient->SendChatMessage(message);
}

void JSI_Network::SendNetworkReady(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), int message)
{
	ENSURE(g_NetClient);

	g_NetClient->SendReadyMessage(message);
}

void JSI_Network::ClearAllPlayerReady (ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	ENSURE(g_NetClient);

	g_NetClient->SendClearAllReadyMessage();
}

void JSI_Network::StartNetworkGame(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	ENSURE(g_NetClient);
	g_NetClient->SendStartGameMessage();
}

void JSI_Network::SetTurnLength(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), int length)
{
	if (g_NetServer)
		g_NetServer->SetTurnLength(length);
	else
		LOGERROR("Only network host can change turn length");
}

void JSI_Network::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<u16, &GetDefaultPort>("GetDefaultPort");
	scriptInterface.RegisterFunction<bool, &HasNetServer>("HasNetServer");
	scriptInterface.RegisterFunction<bool, &HasNetClient>("HasNetClient");
	scriptInterface.RegisterFunction<JS::Value, int, &FindStunEndpoint>("FindStunEndpoint");
	scriptInterface.RegisterFunction<void, CStrW, u16, CStr, &StartNetworkHost>("StartNetworkHost");
	scriptInterface.RegisterFunction<void, CStrW, CStr, u16, bool, CStr, &StartNetworkJoin>("StartNetworkJoin");
	scriptInterface.RegisterFunction<void, &DisconnectNetworkGame>("DisconnectNetworkGame");
	scriptInterface.RegisterFunction<CStr, &GetPlayerGUID>("GetPlayerGUID");
	scriptInterface.RegisterFunction<JS::Value, &PollNetworkClient>("PollNetworkClient");
	scriptInterface.RegisterFunction<void, JS::HandleValue, &SetNetworkGameAttributes>("SetNetworkGameAttributes");
	scriptInterface.RegisterFunction<void, int, CStr, &AssignNetworkPlayer>("AssignNetworkPlayer");
	scriptInterface.RegisterFunction<void, CStrW, bool, &KickPlayer>("KickPlayer");
	scriptInterface.RegisterFunction<void, CStrW, &SendNetworkChat>("SendNetworkChat");
	scriptInterface.RegisterFunction<void, int, &SendNetworkReady>("SendNetworkReady");
	scriptInterface.RegisterFunction<void, &ClearAllPlayerReady>("ClearAllPlayerReady");
	scriptInterface.RegisterFunction<void, &StartNetworkGame>("StartNetworkGame");
	scriptInterface.RegisterFunction<void, int, &SetTurnLength>("SetTurnLength");
}
