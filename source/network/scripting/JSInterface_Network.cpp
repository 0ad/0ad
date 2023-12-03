/* Copyright (C) 2022 Wildfire Games.
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
#include "ps/CStr.h"
#include "ps/Game.h"
#include "ps/GUID.h"
#include "ps/Hashing.h"
#include "ps/Pyrogenesis.h"
#include "ps/Util.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/StructuredClone.h"
#include "scriptinterface/JSON.h"

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

void StartNetworkHost(const ScriptRequest& rq, const CStrW& playerName, const u16 serverPort, bool useSTUN, const CStr& password, bool storeReplay)
{
	ENSURE(!g_NetClient);
	ENSURE(!g_NetServer);
	ENSURE(!g_Game);

	// Always use lobby authentication for lobby matches to prevent impersonation and smurfing, in particular through mods that implemented an UI for arbitrary or other players nicknames.
	bool hasLobby = !!g_XmppClient;
	g_NetServer = new CNetServer(hasLobby);

	if (!g_NetServer->SetupConnection(serverPort))
	{
		ScriptException::Raise(rq, "Failed to start server");
		SAFE_DELETE(g_NetServer);
		return;
	}

	// In lobby, we send our public ip and port on request to the players who want to connect.
	// Thus we need to know our public IP. Use STUN if that's available,
	// otherwise, the lobby's reponse to the game registration stanza will tell us our public IP.
	if (hasLobby)
	{
		if (!useSTUN)
			// Don't store IP - the lobby bot will send it later.
			// (if a client tries to connect before it's setup, they'll be disconnected)
			g_NetServer->SetConnectionData("", serverPort);
		else if (!g_NetServer->SetConnectionDataViaSTUN())
		{
			ScriptException::Raise(rq, "Failed to host via STUN.");
			SAFE_DELETE(g_NetServer);
			return;
		}
	}

	// Generate a secret to identify the host client.
	std::string secret = ps_generate_guid();
	g_NetServer->SetControllerSecret(secret);

	g_Game = new CGame(storeReplay);
	g_NetClient = new CNetClient(g_Game);
	g_NetClient->SetUserName(playerName);

	if (hasLobby)
	{
		CStr hostJID = g_XmppClient->GetJID();

		/**
		 * Password security - we want 0 A.D. to protect players from malicious hosts. We assume that clients
		 * might mistakenly send a personal password instead of the game password (e.g. enter their mail account's password on autopilot).
		 * Malicious dedicated servers might be set up to farm these failed logins and possibly obtain user credentials.
		 * Therefore, we hash the passwords on the client side before sending them to the server.
		 * This still makes the passwords potentially recoverable, but makes it much harder at scale.
		 * To prevent the creation of rainbow tables, hash with:
		 * - the host name
		 * - the client name (this makes rainbow tables completely unworkable unless a specific user is targeted,
		 *   but that would require both computing the matching rainbow table _and_ for that specific user to mistype a personal password,
		 *   at which point we assume the attacker would/could probably just rather use another means of obtaining the password).
		 * - the password itself
		 * - the engine version (so that the hashes change periodically)
		 * TODO: it should be possible to implement SRP or something along those lines to completely protect from this,
		 * but the cost/benefit ratio is probably not worth it.
		 */
		CStr hashedPass = HashCryptographically(password, hostJID + password + engine_version);
		g_NetServer->SetPassword(hashedPass);
		g_NetClient->SetHostJID(hostJID);
		g_NetClient->SetGamePassword(hashedPass);
	}

	g_NetClient->SetupServerData("127.0.0.1", serverPort, false);
	g_NetClient->SetControllerSecret(secret);

	if (!g_NetClient->SetupConnection(nullptr))
	{
		ScriptException::Raise(rq, "Failed to connect to server");
		SAFE_DELETE(g_NetClient);
		SAFE_DELETE(g_Game);
	}
}

void StartNetworkJoin(const ScriptRequest& rq, const CStrW& playerName, const CStr& serverAddress, u16 serverPort, bool storeReplay)
{
	ENSURE(!g_NetClient);
	ENSURE(!g_NetServer);
	ENSURE(!g_Game);

	g_Game = new CGame(storeReplay);
	g_NetClient = new CNetClient(g_Game);
	g_NetClient->SetUserName(playerName);
	g_NetClient->SetupServerData(serverAddress, serverPort, false);

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

	CStr hashedPass = HashCryptographically(password, hostJID + password + engine_version);
	g_Game = new CGame(true);
	g_NetClient = new CNetClient(g_Game);
	g_NetClient->SetUserName(playerName);
	g_NetClient->SetHostJID(hostJID);
	g_NetClient->SetGamePassword(hashedPass);
	g_NetClient->SetupConnectionViaLobby();
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

JS::Value PollNetworkClient(const ScriptInterface& guiInterface)
{
	if (!g_NetClient)
		return JS::UndefinedValue();

	// Convert from net client context to GUI script context
	ScriptRequest rqNet(g_NetClient->GetScriptInterface());
	JS::RootedValue pollNet(rqNet.cx);
	g_NetClient->GuiPoll(&pollNet);
	return Script::CloneValueFromOtherCompartment(guiInterface, g_NetClient->GetScriptInterface(), pollNet);
}

void SendGameSetupMessage(const ScriptInterface& scriptInterface, JS::HandleValue attribs1)
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

void StartNetworkGame(const ScriptInterface& scriptInterface, JS::HandleValue attribs1)
{
	ENSURE(g_NetClient);

	// TODO: This is a workaround because we need to pass a MutableHandle to a JSAPI functions somewhere (with no obvious reason).
	ScriptRequest rq(scriptInterface);
	JS::RootedValue attribs(rq.cx, attribs1);
	g_NetClient->SendStartGameMessage(Script::StringifyJSON(rq, &attribs));
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
	ScriptFunction::Register<&SendGameSetupMessage>(rq, "SendGameSetupMessage");
	ScriptFunction::Register<&AssignNetworkPlayer>(rq, "AssignNetworkPlayer");
	ScriptFunction::Register<&KickPlayer>(rq, "KickPlayer");
	ScriptFunction::Register<&SendNetworkChat>(rq, "SendNetworkChat");
	ScriptFunction::Register<&SendNetworkReady>(rq, "SendNetworkReady");
	ScriptFunction::Register<&ClearAllPlayerReady>(rq, "ClearAllPlayerReady");
	ScriptFunction::Register<&StartNetworkGame>(rq, "StartNetworkGame");
	ScriptFunction::Register<&SetTurnLength>(rq, "SetTurnLength");
}
}
