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
#include "ps/GUID.h"
#include "ps/Util.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/ScriptInterface.h"

#include "third_party/encryption/pkcs5_pbkdf2.h"

namespace JSI_Network
{
u16 GetDefaultPort()
{
	return PS_DEFAULT_PORT;
}

bool IsNetController()
{
	return !!g_NetClient && g_NetClient->IsController();
}

bool HasNetServer()
{
	return !!g_NetServer;
}

bool HasNetClient()
{
	return !!g_NetClient;
}

CStr HashPassword(const CStr& password)
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

void StartNetworkHost(const ScriptRequest& rq, const CStrW& playerName, const u16 serverPort, const CStr& hostLobbyName, bool useSTUN, const CStr& password)
{
	ENSURE(!g_NetClient);
	ENSURE(!g_NetServer);
	ENSURE(!g_Game);

	// Always use lobby authentication for lobby matches to prevent impersonation and smurfing, in particular through mods that implemented an UI for arbitrary or other players nicknames.
	bool hasLobby = !!g_XmppClient;
	g_NetServer = new CNetServer(hasLobby);
	// In lobby, we send our public ip and port on request to the players, who want to connect.
	// In either case we need to know our public IP. If using STUN, we'll use that,
	// otherwise, the lobby's reponse to the game registration stanza will tell us our public IP.
	if (hasLobby)
	{
		CStr ip;
		if (!useSTUN)
			// Don't store IP - the lobby bot will send it later.
			// (if a client tries to connect before it's setup, they'll be disconnected)
			g_NetServer->SetConnectionData("", serverPort, false);
		else
		{
			u16 port = serverPort;
			// This is using port variable to store return value, do not pass serverPort itself.
			if (!StunClient::FindStunEndpointHost(ip, port))
			{
				ScriptException::Raise(rq, "Failed to host via STUN.");
				SAFE_DELETE(g_NetServer);
				return;
			}
			g_NetServer->SetConnectionData(ip, port, true);
		}
	}

	if (!g_NetServer->SetupConnection(serverPort))
	{
		ScriptException::Raise(rq, "Failed to start server");
		SAFE_DELETE(g_NetServer);
		return;
	}

	// Generate a secret to identify the host client.
	std::string secret = ps_generate_guid();

	// We will get hashed password from clients, so hash it once for server
	CStr hashedPass = HashPassword(password);
	g_NetServer->SetPassword(hashedPass);
	g_NetServer->SetControllerSecret(secret);

	g_Game = new CGame(true);
	g_NetClient = new CNetClient(g_Game);
	g_NetClient->SetUserName(playerName);
	g_NetClient->SetHostingPlayerName(hostLobbyName);
	g_NetClient->SetGamePassword(hashedPass);
	g_NetClient->SetupServerData("127.0.0.1", serverPort, false);
	g_NetClient->SetControllerSecret(secret);

	if (!g_NetClient->SetupConnection(nullptr))
	{
		ScriptException::Raise(rq, "Failed to connect to server");
		SAFE_DELETE(g_NetClient);
		SAFE_DELETE(g_Game);
	}
}

void StartNetworkJoin(const ScriptRequest& rq, const CStrW& playerName, const CStr& serverAddress, u16 serverPort, bool useSTUN, const CStr& hostJID)
{
	ENSURE(!g_NetClient);
	ENSURE(!g_NetServer);
	ENSURE(!g_Game);

	g_Game = new CGame(true);
	g_NetClient = new CNetClient(g_Game);
	g_NetClient->SetUserName(playerName);
	g_NetClient->SetHostingPlayerName(hostJID.substr(0, hostJID.find("@")));
	g_NetClient->SetupServerData(serverAddress, serverPort, useSTUN);

	if (!g_NetClient->SetupConnection(nullptr))
	{
		ScriptException::Raise(rq, "Failed to connect to server");
		SAFE_DELETE(g_NetClient);
		SAFE_DELETE(g_Game);
	}
}

/**
 * Requires XmppClient to send iq request to the server to get server's ip and port based on passed password.
 * This is needed to not force server to share it's public ip with all potential clients in the lobby.
 * XmppClient will also handle logic after receiving the answer.
 */
void StartNetworkJoinLobby(const CStrW& playerName, const CStr& hostJID, const CStr& password)
{
	ENSURE(!!g_XmppClient);
	ENSURE(!g_NetClient);
	ENSURE(!g_NetServer);
	ENSURE(!g_Game);

	CStr hashedPass = HashPassword(password);
	g_Game = new CGame(true);
	g_NetClient = new CNetClient(g_Game);
	g_NetClient->SetUserName(playerName);
	g_NetClient->SetHostingPlayerName(hostJID.substr(0, hostJID.find("@")));
	g_NetClient->SetGamePassword(hashedPass);
	g_XmppClient->SendIqGetConnectionData(hostJID, hashedPass.c_str());
}

void DisconnectNetworkGame()
{
	// TODO: we ought to do async reliable disconnections

	SAFE_DELETE(g_NetServer);
	SAFE_DELETE(g_NetClient);
	SAFE_DELETE(g_Game);
}

CStr GetPlayerGUID()
{
	if (!g_NetClient)
		return "local";

	return g_NetClient->GetGUID();
}

JS::Value PollNetworkClient(const ScriptInterface& scriptInterface)
{
	if (!g_NetClient)
		return JS::UndefinedValue();

	// Convert from net client context to GUI script context
	ScriptRequest rqNet(g_NetClient->GetScriptInterface());
	JS::RootedValue pollNet(rqNet.cx);
	g_NetClient->GuiPoll(&pollNet);
	return scriptInterface.CloneValueFromOtherCompartment(g_NetClient->GetScriptInterface(), pollNet);
}

void SetNetworkInitAttributes(const ScriptInterface& scriptInterface, JS::HandleValue attribs1)
{
	ENSURE(g_NetClient);

	// TODO: This is a workaround because we need to pass a MutableHandle to a JSAPI functions somewhere (with no obvious reason).
	ScriptRequest rq(scriptInterface);
	JS::RootedValue attribs(rq.cx, attribs1);

	g_NetClient->SendGameSetupMessage(&attribs, scriptInterface);
}

void AssignNetworkPlayer(int playerID, const CStr& guid)
{
	ENSURE(g_NetClient);

	g_NetClient->SendAssignPlayerMessage(playerID, guid);
}

void KickPlayer(const CStrW& playerName, bool ban)
{
	ENSURE(g_NetClient);

	g_NetClient->SendKickPlayerMessage(playerName, ban);
}

void SendNetworkChat(const CStrW& message)
{
	ENSURE(g_NetClient);

	g_NetClient->SendChatMessage(message);
}

void SendNetworkReady(int message)
{
	ENSURE(g_NetClient);

	g_NetClient->SendReadyMessage(message);
}

void ClearAllPlayerReady ()
{
	ENSURE(g_NetClient);

	g_NetClient->SendClearAllReadyMessage();
}

void StartNetworkGame()
{
	ENSURE(g_NetClient);
	g_NetClient->SendStartGameMessage();
}

void SetTurnLength(int length)
{
	if (g_NetServer)
		g_NetServer->SetTurnLength(length);
	else
		LOGERROR("Only network host can change turn length");
}

void RegisterScriptFunctions(const ScriptRequest& rq)
{
	ScriptFunction::Register<&GetDefaultPort>(rq, "GetDefaultPort");
	ScriptFunction::Register<&IsNetController>(rq, "IsNetController");
	ScriptFunction::Register<&HasNetServer>(rq, "HasNetServer");
	ScriptFunction::Register<&HasNetClient>(rq, "HasNetClient");
	ScriptFunction::Register<&StartNetworkHost>(rq, "StartNetworkHost");
	ScriptFunction::Register<&StartNetworkJoin>(rq, "StartNetworkJoin");
	ScriptFunction::Register<&StartNetworkJoinLobby>(rq, "StartNetworkJoinLobby");
	ScriptFunction::Register<&DisconnectNetworkGame>(rq, "DisconnectNetworkGame");
	ScriptFunction::Register<&GetPlayerGUID>(rq, "GetPlayerGUID");
	ScriptFunction::Register<&PollNetworkClient>(rq, "PollNetworkClient");
	ScriptFunction::Register<&SetNetworkInitAttributes>(rq, "SetNetworkInitAttributes");
	ScriptFunction::Register<&AssignNetworkPlayer>(rq, "AssignNetworkPlayer");
	ScriptFunction::Register<&KickPlayer>(rq, "KickPlayer");
	ScriptFunction::Register<&SendNetworkChat>(rq, "SendNetworkChat");
	ScriptFunction::Register<&SendNetworkReady>(rq, "SendNetworkReady");
	ScriptFunction::Register<&ClearAllPlayerReady>(rq, "ClearAllPlayerReady");
	ScriptFunction::Register<&StartNetworkGame>(rq, "StartNetworkGame");
	ScriptFunction::Register<&SetTurnLength>(rq, "SetTurnLength");
}
}
