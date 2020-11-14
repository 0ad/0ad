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

#include "JSInterface_Lobby.h"

#include "gui/GUIManager.h"
#include "lib/utf8.h"
#include "lobby/IXmppClient.h"
#include "network/NetServer.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Util.h"
#include "scriptinterface/ScriptInterface.h"

#include "third_party/encryption/pkcs5_pbkdf2.h"

#include <string>

void JSI_Lobby::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	// Lobby functions
	scriptInterface.RegisterFunction<bool, &JSI_Lobby::HasXmppClient>("HasXmppClient");
	scriptInterface.RegisterFunction<void, bool, &JSI_Lobby::SetRankedGame>("SetRankedGame");
#if CONFIG2_LOBBY // Allow the lobby to be disabled
	scriptInterface.RegisterFunction<void, std::wstring, std::wstring, std::wstring, std::wstring, int, &JSI_Lobby::StartXmppClient>("StartXmppClient");
	scriptInterface.RegisterFunction<void, std::wstring, std::wstring, &JSI_Lobby::StartRegisterXmppClient>("StartRegisterXmppClient");
	scriptInterface.RegisterFunction<void, &JSI_Lobby::StopXmppClient>("StopXmppClient");
	scriptInterface.RegisterFunction<void, &JSI_Lobby::ConnectXmppClient>("ConnectXmppClient");
	scriptInterface.RegisterFunction<void, &JSI_Lobby::DisconnectXmppClient>("DisconnectXmppClient");
	scriptInterface.RegisterFunction<bool, &JSI_Lobby::IsXmppClientConnected>("IsXmppClientConnected");
	scriptInterface.RegisterFunction<void, &JSI_Lobby::SendGetBoardList>("SendGetBoardList");
	scriptInterface.RegisterFunction<void, std::wstring, &JSI_Lobby::SendGetProfile>("SendGetProfile");
	scriptInterface.RegisterFunction<void, JS::HandleValue, &JSI_Lobby::SendRegisterGame>("SendRegisterGame");
	scriptInterface.RegisterFunction<void, JS::HandleValue, &JSI_Lobby::SendGameReport>("SendGameReport");
	scriptInterface.RegisterFunction<void, &JSI_Lobby::SendUnregisterGame>("SendUnregisterGame");
	scriptInterface.RegisterFunction<void, std::wstring, std::wstring, &JSI_Lobby::SendChangeStateGame>("SendChangeStateGame");
	scriptInterface.RegisterFunction<JS::Value, &JSI_Lobby::GetPlayerList>("GetPlayerList");
	scriptInterface.RegisterFunction<JS::Value, &JSI_Lobby::GetGameList>("GetGameList");
	scriptInterface.RegisterFunction<JS::Value, &JSI_Lobby::GetBoardList>("GetBoardList");
	scriptInterface.RegisterFunction<JS::Value, &JSI_Lobby::GetProfile>("GetProfile");
	scriptInterface.RegisterFunction<JS::Value, &JSI_Lobby::LobbyGuiPollNewMessages>("LobbyGuiPollNewMessages");
	scriptInterface.RegisterFunction<JS::Value, &JSI_Lobby::LobbyGuiPollHistoricMessages>("LobbyGuiPollHistoricMessages");
	scriptInterface.RegisterFunction<bool, &JSI_Lobby::LobbyGuiPollHasPlayerListUpdate>("LobbyGuiPollHasPlayerListUpdate");
	scriptInterface.RegisterFunction<void, std::wstring, &JSI_Lobby::LobbySendMessage>("LobbySendMessage");
	scriptInterface.RegisterFunction<void, std::wstring, &JSI_Lobby::LobbySetPlayerPresence>("LobbySetPlayerPresence");
	scriptInterface.RegisterFunction<void, std::wstring, &JSI_Lobby::LobbySetNick>("LobbySetNick");
	scriptInterface.RegisterFunction<std::wstring, &JSI_Lobby::LobbyGetNick>("LobbyGetNick");
	scriptInterface.RegisterFunction<void, std::wstring, std::wstring, &JSI_Lobby::LobbyKick>("LobbyKick");
	scriptInterface.RegisterFunction<void, std::wstring, std::wstring, &JSI_Lobby::LobbyBan>("LobbyBan");
	scriptInterface.RegisterFunction<const char*, std::wstring, &JSI_Lobby::LobbyGetPlayerPresence>("LobbyGetPlayerPresence");
	scriptInterface.RegisterFunction<const char*, std::wstring, &JSI_Lobby::LobbyGetPlayerRole>("LobbyGetPlayerRole");
	scriptInterface.RegisterFunction<std::wstring, std::wstring, &JSI_Lobby::LobbyGetPlayerRating>("LobbyGetPlayerRating");
	scriptInterface.RegisterFunction<std::wstring, std::wstring, std::wstring, &JSI_Lobby::EncryptPassword>("EncryptPassword");
	scriptInterface.RegisterFunction<std::wstring, &JSI_Lobby::LobbyGetRoomSubject>("LobbyGetRoomSubject");
#endif // CONFIG2_LOBBY
}

bool JSI_Lobby::HasXmppClient(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	return g_XmppClient;
}

void JSI_Lobby::SetRankedGame(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), bool isRanked)
{
	g_rankedGame = isRanked;
}

#if CONFIG2_LOBBY

void JSI_Lobby::StartXmppClient(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& username, const std::wstring& password, const std::wstring& room, const std::wstring& nick, int historyRequestSize)
{
	if (g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call StartXmppClient with an already initialized XmppClient!");
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

void JSI_Lobby::StartRegisterXmppClient(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& username, const std::wstring& password)
{
	if (g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call StartRegisterXmppClient with an already initialized XmppClient!");
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

void JSI_Lobby::StopXmppClient(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call StopXmppClient without an initialized XmppClient!");
		return;
	}

	SAFE_DELETE(g_XmppClient);
	g_rankedGame = false;
}

void JSI_Lobby::ConnectXmppClient(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call ConnectXmppClient without an initialized XmppClient!");
		return;
	}

	g_XmppClient->connect();
}

void JSI_Lobby::DisconnectXmppClient(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call DisconnectXmppClient without an initialized XmppClient!");
		return;
	}

	g_XmppClient->disconnect();
}

bool JSI_Lobby::IsXmppClientConnected(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call IsXmppClientConnected without an initialized XmppClient!");
		return false;
	}

	return g_XmppClient->isConnected();
}

void JSI_Lobby::SendGetBoardList(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call SendGetBoardList without an initialized XmppClient!");
		return;
	}

	g_XmppClient->SendIqGetBoardList();
}

void JSI_Lobby::SendGetProfile(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& player)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call SendGetProfile without an initialized XmppClient!");
		return;
	}

	g_XmppClient->SendIqGetProfile(utf8_from_wstring(player));
}

void JSI_Lobby::SendGameReport(ScriptInterface::CmptPrivate* pCmptPrivate, JS::HandleValue data)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call SendGameReport without an initialized XmppClient!");
		return;
	}

	g_XmppClient->SendIqGameReport(*(pCmptPrivate->pScriptInterface), data);
}

void JSI_Lobby::SendRegisterGame(ScriptInterface::CmptPrivate* pCmptPrivate, JS::HandleValue data)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call SendRegisterGame without an initialized XmppClient!");
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

void JSI_Lobby::SendUnregisterGame(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call SendUnregisterGame without an initialized XmppClient!");
		return;
	}

	g_XmppClient->SendIqUnregisterGame();
}

void JSI_Lobby::SendChangeStateGame(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& nbp, const std::wstring& players)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call SendChangeStateGame without an initialized XmppClient!");
		return;
	}

	g_XmppClient->SendIqChangeStateGame(utf8_from_wstring(nbp), utf8_from_wstring(players));
}

JS::Value JSI_Lobby::GetPlayerList(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	ScriptInterface::Request rq(pCmptPrivate);

	if (!g_XmppClient)
	{
		JS_ReportError(rq.cx, "Cannot call GetPlayerList without an initialized XmppClient!");
		return JS::UndefinedValue();
	}

	JS::RootedValue playerList(rq.cx);
	g_XmppClient->GUIGetPlayerList(*(pCmptPrivate->pScriptInterface), &playerList);

	return playerList;
}

JS::Value JSI_Lobby::GetGameList(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	ScriptInterface::Request rq(pCmptPrivate);

	if (!g_XmppClient)
	{
		JS_ReportError(rq.cx, "Cannot call GetGameList without an initialized XmppClient!");
		return JS::UndefinedValue();
	}

	JS::RootedValue gameList(rq.cx);
	g_XmppClient->GUIGetGameList(*(pCmptPrivate->pScriptInterface), &gameList);

	return gameList;
}

JS::Value JSI_Lobby::GetBoardList(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	ScriptInterface::Request rq(pCmptPrivate);

	if (!g_XmppClient)
	{
		JS_ReportError(rq.cx, "Cannot call GetBoardList without an initialized XmppClient!");
		return JS::UndefinedValue();
	}

	JS::RootedValue boardList(rq.cx);
	g_XmppClient->GUIGetBoardList(*(pCmptPrivate->pScriptInterface), &boardList);

	return boardList;
}

JS::Value JSI_Lobby::GetProfile(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	ScriptInterface::Request rq(pCmptPrivate);

	if (!g_XmppClient)
	{
		JS_ReportError(rq.cx, "Cannot call GetProfile without an initialized XmppClient!");
		return JS::UndefinedValue();
	}

	JS::RootedValue profileFetch(rq.cx);
	g_XmppClient->GUIGetProfile(*(pCmptPrivate->pScriptInterface), &profileFetch);

	return profileFetch;
}

bool JSI_Lobby::LobbyGuiPollHasPlayerListUpdate(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call LobbyGuiPollHasPlayerListUpdate without an initialized XmppClient!");
		return false;
	}

	return g_XmppClient->GuiPollHasPlayerListUpdate();
}

JS::Value JSI_Lobby::LobbyGuiPollNewMessages(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	if (!g_XmppClient)
		return JS::UndefinedValue();

	return g_XmppClient->GuiPollNewMessages(*(pCmptPrivate->pScriptInterface));
}

JS::Value JSI_Lobby::LobbyGuiPollHistoricMessages(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call LobbyGuiPollHistoricMessages without an initialized XmppClient!");
		return JS::UndefinedValue();
	}

	return g_XmppClient->GuiPollHistoricMessages(*(pCmptPrivate->pScriptInterface));
}

void JSI_Lobby::LobbySendMessage(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& message)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call LobbySendMessage without an initialized XmppClient!");
		return;
	}

	g_XmppClient->SendMUCMessage(utf8_from_wstring(message));
}

void JSI_Lobby::LobbySetPlayerPresence(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& presence)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call LobbySetPlayerPresence without an initialized XmppClient!");
		return;
	}

	g_XmppClient->SetPresence(utf8_from_wstring(presence));
}

void JSI_Lobby::LobbySetNick(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& nick)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call LobbySetNick without an initialized XmppClient!");
		return;
	}

	g_XmppClient->SetNick(utf8_from_wstring(nick));
}

std::wstring JSI_Lobby::LobbyGetNick(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call LobbyGetNick without an initialized XmppClient!");
		return std::wstring();
	}

	std::string nick;
	g_XmppClient->GetNick(nick);
	return wstring_from_utf8(nick);
}

void JSI_Lobby::LobbyKick(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& nick, const std::wstring& reason)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call LobbyKick without an initialized XmppClient!");
		return;
	}

	g_XmppClient->kick(utf8_from_wstring(nick), utf8_from_wstring(reason));
}

void JSI_Lobby::LobbyBan(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& nick, const std::wstring& reason)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call LobbyBan without an initialized XmppClient!");
		return;
	}

	g_XmppClient->ban(utf8_from_wstring(nick), utf8_from_wstring(reason));
}

const char* JSI_Lobby::LobbyGetPlayerPresence(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& nickname)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call LobbyGetPlayerPresence without an initialized XmppClient!");
		return "";
	}

	return g_XmppClient->GetPresence(utf8_from_wstring(nickname));
}

const char* JSI_Lobby::LobbyGetPlayerRole(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& nickname)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call LobbyGetPlayerRole without an initialized XmppClient!");
		return "";
	}

	return g_XmppClient->GetRole(utf8_from_wstring(nickname));
}

std::wstring JSI_Lobby::LobbyGetPlayerRating(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& nickname)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call LobbyGetPlayerRating without an initialized XmppClient!");
		return std::wstring();
	}

	return g_XmppClient->GetRating(utf8_from_wstring(nickname));
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
std::string JSI_Lobby::EncryptPassword(const std::string& password, const std::string& username)
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

std::wstring JSI_Lobby::EncryptPassword(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const std::wstring& pass, const std::wstring& user)
{
	return wstring_from_utf8(JSI_Lobby::EncryptPassword(utf8_from_wstring(pass), utf8_from_wstring(user)));
}

std::wstring JSI_Lobby::LobbyGetRoomSubject(ScriptInterface::CmptPrivate* pCmptPrivate)
{
	if (!g_XmppClient)
	{
		ScriptInterface::Request rq(pCmptPrivate);
		JS_ReportError(rq.cx, "Cannot call LobbyGetRoomSubject without an initialized XmppClient!");
		return std::wstring();
	}

	return g_XmppClient->GetSubject();
}

#endif
