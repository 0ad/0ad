/* Copyright (C) 2017 Wildfire Games.
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
#include "ps/Profile.h"
#include "scriptinterface/ScriptInterface.h"
#include "third_party/encryption/pkcs5_pbkdf2.h"
#include "third_party/encryption/sha.h"

void JSI_Lobby::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	// Lobby functions
	scriptInterface.RegisterFunction<bool, &JSI_Lobby::HasXmppClient>("HasXmppClient");
	scriptInterface.RegisterFunction<bool, &JSI_Lobby::IsRankedGame>("IsRankedGame");
	scriptInterface.RegisterFunction<void, bool, &JSI_Lobby::SetRankedGame>("SetRankedGame");
#if CONFIG2_LOBBY // Allow the lobby to be disabled
	scriptInterface.RegisterFunction<void, std::wstring, std::wstring, std::wstring, std::wstring, int, &JSI_Lobby::StartXmppClient>("StartXmppClient");
	scriptInterface.RegisterFunction<void, std::wstring, std::wstring, &JSI_Lobby::StartRegisterXmppClient>("StartRegisterXmppClient");
	scriptInterface.RegisterFunction<void, &JSI_Lobby::StopXmppClient>("StopXmppClient");
	scriptInterface.RegisterFunction<void, &JSI_Lobby::ConnectXmppClient>("ConnectXmppClient");
	scriptInterface.RegisterFunction<void, &JSI_Lobby::DisconnectXmppClient>("DisconnectXmppClient");
	scriptInterface.RegisterFunction<void, &JSI_Lobby::SendGetBoardList>("SendGetBoardList");
	scriptInterface.RegisterFunction<void, std::wstring, &JSI_Lobby::SendGetProfile>("SendGetProfile");
	scriptInterface.RegisterFunction<void, JS::HandleValue, &JSI_Lobby::SendRegisterGame>("SendRegisterGame");
	scriptInterface.RegisterFunction<void, JS::HandleValue, &JSI_Lobby::SendGameReport>("SendGameReport");
	scriptInterface.RegisterFunction<void, &JSI_Lobby::SendUnregisterGame>("SendUnregisterGame");
	scriptInterface.RegisterFunction<void, std::wstring, std::wstring, &JSI_Lobby::SendChangeStateGame>("SendChangeStateGame");
	scriptInterface.RegisterFunction<JS::Value, &JSI_Lobby::GetPlayerList>("GetPlayerList");
	scriptInterface.RegisterFunction<void, &JSI_Lobby::LobbyClearPresenceUpdates>("LobbyClearPresenceUpdates");
	scriptInterface.RegisterFunction<int, &JSI_Lobby::LobbyGetMucMessageCount>("LobbyGetMucMessageCount");
	scriptInterface.RegisterFunction<JS::Value, &JSI_Lobby::GetGameList>("GetGameList");
	scriptInterface.RegisterFunction<JS::Value, &JSI_Lobby::GetBoardList>("GetBoardList");
	scriptInterface.RegisterFunction<JS::Value, &JSI_Lobby::GetProfile>("GetProfile");
	scriptInterface.RegisterFunction<JS::Value, &JSI_Lobby::LobbyGuiPollMessage>("LobbyGuiPollMessage");
	scriptInterface.RegisterFunction<void, std::wstring, &JSI_Lobby::LobbySendMessage>("LobbySendMessage");
	scriptInterface.RegisterFunction<void, std::wstring, &JSI_Lobby::LobbySetPlayerPresence>("LobbySetPlayerPresence");
	scriptInterface.RegisterFunction<void, std::wstring, &JSI_Lobby::LobbySetNick>("LobbySetNick");
	scriptInterface.RegisterFunction<std::wstring, &JSI_Lobby::LobbyGetNick>("LobbyGetNick");
	scriptInterface.RegisterFunction<void, std::wstring, std::wstring, &JSI_Lobby::LobbyKick>("LobbyKick");
	scriptInterface.RegisterFunction<void, std::wstring, std::wstring, &JSI_Lobby::LobbyBan>("LobbyBan");
	scriptInterface.RegisterFunction<std::wstring, std::wstring, &JSI_Lobby::LobbyGetPlayerPresence>("LobbyGetPlayerPresence");
	scriptInterface.RegisterFunction<std::wstring, std::wstring, &JSI_Lobby::LobbyGetPlayerRole>("LobbyGetPlayerRole");
	scriptInterface.RegisterFunction<std::wstring, std::wstring, std::wstring, &JSI_Lobby::EncryptPassword>("EncryptPassword");
	scriptInterface.RegisterFunction<std::wstring, &JSI_Lobby::LobbyGetRoomSubject>("LobbyGetRoomSubject");
#endif // CONFIG2_LOBBY
}

bool JSI_Lobby::HasXmppClient(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return g_XmppClient;
}

bool JSI_Lobby::IsRankedGame(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return g_rankedGame;
}

void JSI_Lobby::SetRankedGame(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), bool isRanked)
{
	g_rankedGame = isRanked;
}

#if CONFIG2_LOBBY

void JSI_Lobby::StartXmppClient(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& username, const std::wstring& password, const std::wstring& room, const std::wstring& nick, int historyRequestSize)
{
	ENSURE(!g_XmppClient);

	g_XmppClient = IXmppClient::create(utf8_from_wstring(username), utf8_from_wstring(password),
		utf8_from_wstring(room), utf8_from_wstring(nick), historyRequestSize);
	g_rankedGame = true;
}

void JSI_Lobby::StartRegisterXmppClient(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& username, const std::wstring& password)
{
	ENSURE(!g_XmppClient);

	g_XmppClient = IXmppClient::create(utf8_from_wstring(username), utf8_from_wstring(password),
		"", "", 0, true);
}

void JSI_Lobby::StopXmppClient(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	ENSURE(g_XmppClient);
	SAFE_DELETE(g_XmppClient);
	g_rankedGame = false;
}

void JSI_Lobby::ConnectXmppClient(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	ENSURE(g_XmppClient);
	g_XmppClient->connect();
}

void JSI_Lobby::DisconnectXmppClient(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	ENSURE(g_XmppClient);
	g_XmppClient->disconnect();
}

void JSI_Lobby::SendGetBoardList(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_XmppClient)
		return;
	g_XmppClient->SendIqGetBoardList();
}

void JSI_Lobby::SendGetProfile(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& player)
{
	if (!g_XmppClient)
		return;
	g_XmppClient->SendIqGetProfile(utf8_from_wstring(player));
}

void JSI_Lobby::SendGameReport(ScriptInterface::CxPrivate* pCxPrivate, JS::HandleValue data)
{
	if (!g_XmppClient)
		return;

	g_XmppClient->SendIqGameReport(*(pCxPrivate->pScriptInterface), data);
}

void JSI_Lobby::SendRegisterGame(ScriptInterface::CxPrivate* pCxPrivate, JS::HandleValue data)
{
	if (!g_XmppClient)
		return;

	g_XmppClient->SendIqRegisterGame(*(pCxPrivate->pScriptInterface), data);
}

void JSI_Lobby::SendUnregisterGame(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_XmppClient)
		return;
	g_XmppClient->SendIqUnregisterGame();
}

void JSI_Lobby::SendChangeStateGame(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& nbp, const std::wstring& players)
{
	if (!g_XmppClient)
		return;
	g_XmppClient->SendIqChangeStateGame(utf8_from_wstring(nbp), utf8_from_wstring(players));
}

JS::Value JSI_Lobby::GetPlayerList(ScriptInterface::CxPrivate* pCxPrivate)
{
	if (!g_XmppClient)
		return JS::UndefinedValue();

	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue playerList(cx);
	g_XmppClient->GUIGetPlayerList(*(pCxPrivate->pScriptInterface), &playerList);

	return playerList;
}

void JSI_Lobby::LobbyClearPresenceUpdates(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_XmppClient)
		return;

	g_XmppClient->ClearPresenceUpdates();
}

int JSI_Lobby::LobbyGetMucMessageCount(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return g_XmppClient ? g_XmppClient->GetMucMessageCount() : 0;
}

JS::Value JSI_Lobby::GetGameList(ScriptInterface::CxPrivate* pCxPrivate)
{
	if (!g_XmppClient)
		return JS::UndefinedValue();

	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue gameList(cx);
	g_XmppClient->GUIGetGameList(*(pCxPrivate->pScriptInterface), &gameList);

	return gameList;
}

JS::Value JSI_Lobby::GetBoardList(ScriptInterface::CxPrivate* pCxPrivate)
{
	if (!g_XmppClient)
		return JS::UndefinedValue();

	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue boardList(cx);
	g_XmppClient->GUIGetBoardList(*(pCxPrivate->pScriptInterface), &boardList);

	return boardList;
}

JS::Value JSI_Lobby::GetProfile(ScriptInterface::CxPrivate* pCxPrivate)
{
	if (!g_XmppClient)
		return JS::UndefinedValue();

	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue profileFetch(cx);
	g_XmppClient->GUIGetProfile(*(pCxPrivate->pScriptInterface), &profileFetch);

	return profileFetch;
}

JS::Value JSI_Lobby::LobbyGuiPollMessage(ScriptInterface::CxPrivate* pCxPrivate)
{
	if (!g_XmppClient)
		return JS::UndefinedValue();

	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue poll(cx);
	g_XmppClient->GuiPollMessage(*(pCxPrivate->pScriptInterface), &poll);

	return poll;
}

void JSI_Lobby::LobbySendMessage(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& message)
{
	if (!g_XmppClient)
		return;

	g_XmppClient->SendMUCMessage(utf8_from_wstring(message));
}

void JSI_Lobby::LobbySetPlayerPresence(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& presence)
{
	if (!g_XmppClient)
		return;

	g_XmppClient->SetPresence(utf8_from_wstring(presence));
}

void JSI_Lobby::LobbySetNick(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& nick)
{
	if (!g_XmppClient)
		return;

	g_XmppClient->SetNick(utf8_from_wstring(nick));
}

std::wstring JSI_Lobby::LobbyGetNick(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_XmppClient)
		return L"";

	std::string nick;
	g_XmppClient->GetNick(nick);
	return wstring_from_utf8(nick);
}

void JSI_Lobby::LobbyKick(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& nick, const std::wstring& reason)
{
	if (!g_XmppClient)
		return;

	g_XmppClient->kick(utf8_from_wstring(nick), utf8_from_wstring(reason));
}

void JSI_Lobby::LobbyBan(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& nick, const std::wstring& reason)
{
	if (!g_XmppClient)
		return;

	g_XmppClient->ban(utf8_from_wstring(nick), utf8_from_wstring(reason));
}

std::wstring JSI_Lobby::LobbyGetPlayerPresence(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& nickname)
{
	if (!g_XmppClient)
		return L"";

	std::string presence;
	g_XmppClient->GetPresence(utf8_from_wstring(nickname), presence);
	return wstring_from_utf8(presence);
}

std::wstring JSI_Lobby::LobbyGetPlayerRole(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& nickname)
{
	if (!g_XmppClient)
		return L"";

	std::string role;
	g_XmppClient->GetRole(utf8_from_wstring(nickname), role);
	return wstring_from_utf8(role);
}

// Non-public secure PBKDF2 hash function with salting and 1,337 iterations
std::string JSI_Lobby::EncryptPassword(const std::string& password, const std::string& username)
{
	const int DIGESTSIZE = SHA_DIGEST_SIZE;
	const int ITERATIONS = 1337;

	static const unsigned char salt_base[DIGESTSIZE] = {
			244, 243, 249, 244, 32, 33, 34, 35, 10, 11, 12, 13, 14, 15, 16, 17,
			18, 19, 20, 32, 33, 244, 224, 127, 129, 130, 140, 153, 133, 123, 234, 123 };

	// initialize the salt buffer
	unsigned char salt_buffer[DIGESTSIZE] = {0};
	SHA256 hash;
	hash.update(salt_base, sizeof(salt_base));
	hash.update(username.c_str(), username.length());
	hash.finish(salt_buffer);

	// PBKDF2 to create the buffer
	unsigned char encrypted[DIGESTSIZE];
	pbkdf2(encrypted, (unsigned char*)password.c_str(), password.length(), salt_buffer, DIGESTSIZE, ITERATIONS);

	static const char base16[] = "0123456789ABCDEF";
	char hex[2 * DIGESTSIZE];
	for (int i = 0; i < DIGESTSIZE; ++i)
	{
		hex[i*2] = base16[encrypted[i] >> 4];		// 4 high bits
		hex[i*2 + 1] = base16[encrypted[i] & 0x0F];	// 4 low bits
	}
	return std::string(hex, sizeof(hex));
}

std::wstring JSI_Lobby::EncryptPassword(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& pass, const std::wstring& user)
{
	return wstring_from_utf8(JSI_Lobby::EncryptPassword(utf8_from_wstring(pass), utf8_from_wstring(user)));
}

std::wstring JSI_Lobby::LobbyGetRoomSubject(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_XmppClient)
		return L"";

	std::string subject;
	g_XmppClient->GetSubject(subject);
	return wstring_from_utf8(subject);
}

#endif
