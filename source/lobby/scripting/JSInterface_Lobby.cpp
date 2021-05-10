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

#include "JSInterface_Lobby.h"

#include "gui/GUIManager.h"
#include "lib/utf8.h"
#include "lobby/IXmppClient.h"
#include "network/NetServer.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Util.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/ScriptInterface.h"

#include "third_party/encryption/pkcs5_pbkdf2.h"

#include <string>

namespace JSI_Lobby
{
bool HasXmppClient()
{
	return g_XmppClient;
}

void SetRankedGame(bool isRanked)
{
	g_rankedGame = isRanked;
}

#if CONFIG2_LOBBY

void StartXmppClient(const ScriptRequest& rq, const std::wstring& username, const std::wstring& password, const std::wstring& room, const std::wstring& nick, int historyRequestSize)
{
	if (g_XmppClient)
	{
		ScriptException::Raise(rq, "Cannot call StartXmppClient with an already initialized XmppClient!");
		return;
	}

	g_XmppClient =
		IXmppClient::create(
			g_GUI->GetScriptInterface().get(),
			utf8_from_wstring(username),
			utf8_from_wstring(password),
			utf8_from_wstring(room),
			utf8_from_wstring(nick),
			historyRequestSize);

	g_rankedGame = true;
}

void StartRegisterXmppClient(const ScriptRequest& rq, const std::wstring& username, const std::wstring& password)
{
	if (g_XmppClient)
	{
		ScriptException::Raise(rq, "Cannot call StartRegisterXmppClient with an already initialized XmppClient!");
		return;
	}

	g_XmppClient =
		IXmppClient::create(
			g_GUI->GetScriptInterface().get(),
			utf8_from_wstring(username),
			utf8_from_wstring(password),
			std::string(),
			std::string(),
			0,
			true);
}

void StopXmppClient(const ScriptRequest& rq)
{
	if (!g_XmppClient)
	{
		ScriptException::Raise(rq, "Cannot call StopXmppClient without an initialized XmppClient!");
		return;
	}

	SAFE_DELETE(g_XmppClient);
	g_rankedGame = false;
}

////////////////////////////////////////////////
////////////////////////////////////////////////

IXmppClient* XmppGetter(const ScriptRequest&, JS::CallArgs&)
{
	if (!g_XmppClient)
	{
		LOGERROR("Cannot use XMPPClient functions without an initialized XmppClient!");
		return nullptr;
	}
	return g_XmppClient;
}

void SendRegisterGame(ScriptInterface::CmptPrivate* pCmptPrivate, JS::HandleValue data)
{
	if (!g_XmppClient)
	{
		ScriptRequest rq(pCmptPrivate->pScriptInterface);
		ScriptException::Raise(rq, "Cannot call SendRegisterGame without an initialized XmppClient!");
		return;
	}

	// Prevent JS mods to register matches in the lobby that were started with lobby authentication disabled
	if (!g_NetServer || !g_NetServer->UseLobbyAuth())
	{
		LOGERROR("Registering games in the lobby requires lobby authentication to be enabled!");
		return;
	}

	g_XmppClient->SendIqRegisterGame(*(pCmptPrivate->pScriptInterface), data);
}

// Unlike other functions, this one just returns Undefined if XmppClient isn't initialised.
JS::Value GuiPollNewMessages(const ScriptInterface& scriptInterface)
{
	if (!g_XmppClient)
		return JS::UndefinedValue();

	return g_XmppClient->GuiPollNewMessages(scriptInterface);
}


// Non-public secure PBKDF2 hash function with salting and 1,337 iterations
//
// TODO: We should use libsodium's crypto_pwhash instead of this. The first reason is that
// libsodium doesn't propose a bare PBKDF2 hash in its API and it's too bad to rely on custom
// code when we have a fully-fledged library available; the second reason is that Argon2 (the
// default algorithm for crypto_pwhash) is better than what we use (and it's the default one
// in the lib for a reason).
// However changing the hashing method should be planned carefully, by trying to login with a
// password hashed the old way, and, if successful, updating the password in the database using
// the new hashing method. Dropping the old hashing code can only be done either by giving users
// a way to reset their password, or by keeping track of successful password updates and dropping
// old unused accounts after some time.
std::string EncryptPassword(const std::string& password, const std::string& username)
{
	ENSURE(sodium_init() >= 0);

	const int DIGESTSIZE = crypto_hash_sha256_BYTES;
	const int ITERATIONS = 1337;

	cassert(DIGESTSIZE == 32);

	static const unsigned char salt_base[DIGESTSIZE] = {
			244, 243, 249, 244, 32, 33, 34, 35, 10, 11, 12, 13, 14, 15, 16, 17,
			18, 19, 20, 32, 33, 244, 224, 127, 129, 130, 140, 153, 133, 123, 234, 123 };

	// initialize the salt buffer
	unsigned char salt_buffer[DIGESTSIZE] = {0};
	crypto_hash_sha256_state state;
	crypto_hash_sha256_init(&state);

	crypto_hash_sha256_update(&state, salt_base, sizeof(salt_base));
	crypto_hash_sha256_update(&state, (unsigned char*)username.c_str(), username.length());

	crypto_hash_sha256_final(&state, salt_buffer);

	// PBKDF2 to create the buffer
	unsigned char encrypted[DIGESTSIZE];
	pbkdf2(encrypted, (unsigned char*)password.c_str(), password.length(), salt_buffer, DIGESTSIZE, ITERATIONS);

	return CStr(Hexify(encrypted, DIGESTSIZE)).UpperCase();
}

#endif

void RegisterScriptFunctions(const ScriptRequest& rq)
{
	// Lobby functions
	ScriptFunction::Register<&HasXmppClient>(rq, "HasXmppClient");
	ScriptFunction::Register<&SetRankedGame>(rq, "SetRankedGame");
#if CONFIG2_LOBBY // Allow the lobby to be disabled
	ScriptFunction::Register<&StartXmppClient>(rq, "StartXmppClient");
	ScriptFunction::Register<&StartRegisterXmppClient>(rq, "StartRegisterXmppClient");
	ScriptFunction::Register<&StopXmppClient>(rq, "StopXmppClient");

#define REGISTER_XMPP(func, name) \
	ScriptFunction::Register<&IXmppClient::func, &XmppGetter>(rq, name)

	REGISTER_XMPP(connect, "ConnectXmppClient");
	REGISTER_XMPP(disconnect, "DisconnectXmppClient");
	REGISTER_XMPP(isConnected, "IsXmppClientConnected");
	REGISTER_XMPP(SendIqGetBoardList, "SendGetBoardList");
	REGISTER_XMPP(SendIqGetProfile, "SendGetProfile");
	REGISTER_XMPP(SendIqGameReport, "SendGameReport");
	ScriptFunction::Register<&SendRegisterGame>(rq, "SendRegisterGame");
	REGISTER_XMPP(SendIqUnregisterGame, "SendUnregisterGame");
	REGISTER_XMPP(SendIqChangeStateGame, "SendChangeStateGame");
	REGISTER_XMPP(GUIGetPlayerList, "GetPlayerList");
	REGISTER_XMPP(GUIGetGameList, "GetGameList");
	REGISTER_XMPP(GUIGetBoardList, "GetBoardList");
	REGISTER_XMPP(GUIGetProfile, "GetProfile");

	ScriptFunction::Register<&GuiPollNewMessages>(rq, "LobbyGuiPollNewMessages");
	REGISTER_XMPP(GuiPollHistoricMessages, "LobbyGuiPollHistoricMessages");
	REGISTER_XMPP(GuiPollHasPlayerListUpdate, "LobbyGuiPollHasPlayerListUpdate");
	REGISTER_XMPP(SendMUCMessage, "LobbySendMessage");
	REGISTER_XMPP(SetPresence, "LobbySetPlayerPresence");
	REGISTER_XMPP(SetNick, "LobbySetNick");
	REGISTER_XMPP(GetNick, "LobbyGetNick");
	REGISTER_XMPP(GetJID, "LobbyGetJID");
	REGISTER_XMPP(kick, "LobbyKick");
	REGISTER_XMPP(ban, "LobbyBan");
	REGISTER_XMPP(GetPresence, "LobbyGetPlayerPresence");
	REGISTER_XMPP(GetRole, "LobbyGetPlayerRole");
	REGISTER_XMPP(GetRating, "LobbyGetPlayerRating");
	REGISTER_XMPP(GetSubject, "LobbyGetRoomSubject");
#undef REGISTER_XMPP

	ScriptFunction::Register<&EncryptPassword>(rq, "EncryptPassword");
#endif // CONFIG2_LOBBY
}
}
