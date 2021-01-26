/* Copyright (C) 2021 Wildfire Games.
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
#include "ps/Util.h"
#include "scriptinterface/ScriptInterface.h"

#include "third_party/encryption/pkcs5_pbkdf2.h"

u16 JSI_Network::GetDefaultPort(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	return PS_DEFAULT_PORT;
}

bool JSI_Network::HasNetServer(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	return !!g_NetServer;
}

bool JSI_Network::HasNetClient(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	return !!g_NetClient;
}

CStr JSI_Network::HashPassword(const CStr& password)
{
	if (password.empty())
		return password;

	ENSURE(sodium_init() >= 0);
	const int DIGESTSIZE = crypto_hash_sha256_BYTES;
	constexpr int ITERATIONS = 1737;

	cassert(DIGESTSIZE == 32);

	static const unsigned char salt_base[DIGESTSIZE] = {
			244, 243, 249, 244, 32, 33, 19, 35, 16, 11, 12, 13, 14, 15, 16, 17,
			18, 19, 20, 32, 33, 244, 224, 127, 129, 130, 140, 153, 88, 123, 234, 123 };

	// initialize the salt buffer
	unsigned char salt_buffer[DIGESTSIZE] = { 0 };
	crypto_hash_sha256_state state;
	crypto_hash_sha256_init(&state);
	crypto_hash_sha256_update(&state, salt_base, sizeof(salt_base));

	crypto_hash_sha256_final(&state, salt_buffer);

	// PBKDF2 to create the buffer
	unsigned char encrypted[DIGESTSIZE];
	pbkdf2(encrypted, (unsigned char*)password.c_str(), password.length(), salt_buffer, DIGESTSIZE, ITERATIONS);
	return CStr(Hexify(encrypted, DIGESTSIZE)).UpperCase();
}

void JSI_Network::StartNetworkHost(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& playerName, const u16 serverPort, const CStr& hostLobbyName, bool useSTUN, const CStr& password)
{
	ENSURE(!g_NetClient);
	ENSURE(!g_NetServer);
	ENSURE(!g_Game);

	// Always use lobby authentication for lobby matches to prevent impersonation and smurfing, in particular through mods that implemented an UI for arbitrary or other players nicknames.
	bool hasLobby = !!g_XmppClient;
	g_NetServer = new CNetServer(hasLobby);
	// In lobby, we send our public ip and port on request to the players, who want to connect.
	// In both cases we need to ping stun server to get our public ip. If we want to host via stun,
	// we need port as well.
	if (hasLobby)
	{
		CStr ip;
		if (!useSTUN)
		{
			if (!StunClient::GetPublicIp(ip, serverPort))
			{
				ScriptRequest rq(pCmptPrivate->pScriptInterface);
				ScriptException::Raise(rq, "Failed to get public ip.");
				SAFE_DELETE(g_NetServer);
				return;
			}
			g_NetServer->SetConnectionData(ip, serverPort, false);
		}
		else
		{
			u16 port = serverPort;
			// This is using port variable to store return value, do not pass serverPort itself.
			if (!StunClient::FindStunEndpointHost(ip, port))
			{
				ScriptRequest rq(pCmptPrivate->pScriptInterface);
				ScriptException::Raise(rq, "Failed to host via STUN.");
				SAFE_DELETE(g_NetServer);
				return;
			}
			g_NetServer->SetConnectionData(ip, port, true);
		}
	}

	if (!g_NetServer->SetupConnection(serverPort))
	{
		ScriptRequest rq(pCmptPrivate->pScriptInterface);
		ScriptException::Raise(rq, "Failed to start server");
		SAFE_DELETE(g_NetServer);
		return;
	}

	// We will get hashed password from clients, so hash it once for server
	CStr hashedPass = HashPassword(password);
	g_NetServer->SetPassword(hashedPass);

	g_Game = new CGame(true);
	g_NetClient = new CNetClient(g_Game, true);
	g_NetClient->SetUserName(playerName);
	g_NetClient->SetHostingPlayerName(hostLobbyName);
	g_NetClient->SetGamePassword(hashedPass);
	g_NetClient->SetupServerData("127.0.0.1", serverPort, false);

	if (!g_NetClient->SetupConnection(nullptr))
	{
		ScriptRequest rq(pCmptPrivate->pScriptInterface);
		ScriptException::Raise(rq, "Failed to connect to server");
		SAFE_DELETE(g_NetClient);
		SAFE_DELETE(g_Game);
	}
}

void JSI_Network::StartNetworkJoin(ScriptInterface::CmptPrivate* pCmptPrivate, const CStrW& playerName, const CStr& serverAddress, u16 serverPort, bool useSTUN, const CStr& hostJID)
{
	ENSURE(!g_NetClient);
	ENSURE(!g_NetServer);
	ENSURE(!g_Game);

	g_Game = new CGame(true);
	g_NetClient = new CNetClient(g_Game, false);
	g_NetClient->SetUserName(playerName);
	g_NetClient->SetHostingPlayerName(hostJID.substr(0, hostJID.find("@")));
	g_NetClient->SetupServerData(serverAddress, serverPort, useSTUN);

	if (!g_NetClient->SetupConnection(nullptr))
	{
		ScriptRequest rq(pCmptPrivate->pScriptInterface);
		ScriptException::Raise(rq, "Failed to connect to server");
		SAFE_DELETE(g_NetClient);
		SAFE_DELETE(g_Game);
	}
}

void JSI_Network::StartNetworkJoinLobby(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const CStrW& playerName, const CStr& hostJID, const CStr& password)
{
	ENSURE(!!g_XmppClient);
	ENSURE(!g_NetClient);
	ENSURE(!g_NetServer);
	ENSURE(!g_Game);

	CStr hashedPass = HashPassword(password);
	g_Game = new CGame(true);
	g_NetClient = new CNetClient(g_Game, false);
	g_NetClient->SetUserName(playerName);
	g_NetClient->SetHostingPlayerName(hostJID.substr(0, hostJID.find("@")));
	g_NetClient->SetGamePassword(hashedPass);
	g_XmppClient->SendIqGetConnectionData(hostJID, hashedPass.c_str());
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
	ScriptRequest rqNet(g_NetClient->GetScriptInterface());
	JS::RootedValue pollNet(rqNet.cx);
	g_NetClient->GuiPoll(&pollNet);
	return pCmptPrivate->pScriptInterface->CloneValueFromOtherCompartment(g_NetClient->GetScriptInterface(), pollNet);
}

void JSI_Network::SetNetworkGameAttributes(ScriptInterface::CmptPrivate* pCmptPrivate, JS::HandleValue attribs1)
{
	ENSURE(g_NetClient);

	// TODO: This is a workaround because we need to pass a MutableHandle to a JSAPI functions somewhere (with no obvious reason).
	ScriptRequest rq(pCmptPrivate->pScriptInterface);
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
	scriptInterface.RegisterFunction<void, CStrW, u16, CStr, bool, CStr, &StartNetworkHost>("StartNetworkHost");
	scriptInterface.RegisterFunction<void, CStrW, CStr, u16, bool, CStr, &StartNetworkJoin>("StartNetworkJoin");
	scriptInterface.RegisterFunction<void, CStrW, CStr, CStr, &StartNetworkJoinLobby>("StartNetworkJoinLobby");
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
