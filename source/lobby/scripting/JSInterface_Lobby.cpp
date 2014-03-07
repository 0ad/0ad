/* Copyright (C) 2013 Wildfire Games.
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
#include "third_party/encryption/pkcs5_pbkdf2.h"
#include "third_party/encryption/sha.h"

#include "scriptinterface/ScriptInterface.h"

bool JSI_Lobby::HasXmppClient(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return (g_XmppClient ? true : false);
}

#if CONFIG2_LOBBY

void JSI_Lobby::StartXmppClient(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring username, std::wstring password, std::wstring room, std::wstring nick, int historyRequestSize)
{
	ENSURE(!g_XmppClient);

	g_XmppClient = IXmppClient::create(utf8_from_wstring(username), utf8_from_wstring(password),
		utf8_from_wstring(room), utf8_from_wstring(nick), historyRequestSize);
	g_rankedGame = true;
}

void JSI_Lobby::StartRegisterXmppClient(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring username, std::wstring password)
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

void JSI_Lobby::RecvXmppClient(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_XmppClient)
		return;
	g_XmppClient->recv();
}

void JSI_Lobby::SendGetGameList(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_XmppClient)
		return;
	g_XmppClient->SendIqGetGameList();
}

void JSI_Lobby::SendGetBoardList(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_XmppClient)
		return;
	g_XmppClient->SendIqGetBoardList();
}

void JSI_Lobby::SendGetRatingList(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_XmppClient)
		return;
	g_XmppClient->SendIqGetRatingList();
}

void JSI_Lobby::SendGameReport(ScriptInterface::CxPrivate* pCxPrivate, CScriptVal data)
{
	if (!g_XmppClient)
		return;

	g_XmppClient->SendIqGameReport(*(pCxPrivate->pScriptInterface), data);
}

void JSI_Lobby::SendRegisterGame(ScriptInterface::CxPrivate* pCxPrivate, CScriptVal data)
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

void JSI_Lobby::SendChangeStateGame(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring nbp, std::wstring players)
{
	if (!g_XmppClient)
		return;
	g_XmppClient->SendIqChangeStateGame(utf8_from_wstring(nbp), utf8_from_wstring(players));
}

CScriptVal JSI_Lobby::GetPlayerList(ScriptInterface::CxPrivate* pCxPrivate)
{
	if (!g_XmppClient)
		return CScriptVal();
		
	CScriptValRooted playerList = g_XmppClient->GUIGetPlayerList(*(pCxPrivate->pScriptInterface));

	return playerList.get();
}

CScriptVal JSI_Lobby::GetGameList(ScriptInterface::CxPrivate* pCxPrivate)
{
	if (!g_XmppClient)
		return CScriptVal();

	CScriptValRooted gameList = g_XmppClient->GUIGetGameList(*(pCxPrivate->pScriptInterface));

	return gameList.get();
}

CScriptVal JSI_Lobby::GetBoardList(ScriptInterface::CxPrivate* pCxPrivate)
{
	if (!g_XmppClient)
		return CScriptVal();

	CScriptValRooted boardList = g_XmppClient->GUIGetBoardList(*(pCxPrivate->pScriptInterface));

	return boardList.get();
}

CScriptVal JSI_Lobby::LobbyGuiPollMessage(ScriptInterface::CxPrivate* pCxPrivate)
{
	if (!g_XmppClient)
		return CScriptVal();
	
	CScriptValRooted poll = g_XmppClient->GuiPollMessage(*(pCxPrivate->pScriptInterface));

	return poll.get();
}

void JSI_Lobby::LobbySendMessage(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring message)
{
	if (!g_XmppClient)
		return;
		
	g_XmppClient->SendMUCMessage(utf8_from_wstring(message));
}

void JSI_Lobby::LobbySetPlayerPresence(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring presence)
{
	if (!g_XmppClient)
		return;

	g_XmppClient->SetPresence(utf8_from_wstring(presence));
}

void JSI_Lobby::LobbySetNick(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring nick)
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

void JSI_Lobby::LobbyKick(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring nick, std::wstring reason)
{
	if (!g_XmppClient)
		return;

	g_XmppClient->kick(utf8_from_wstring(nick), utf8_from_wstring(reason));
}

void JSI_Lobby::LobbyBan(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring nick, std::wstring reason)
{
	if (!g_XmppClient)
		return;

	g_XmppClient->ban(utf8_from_wstring(nick), utf8_from_wstring(reason));
}

std::wstring JSI_Lobby::LobbyGetPlayerPresence(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring nickname)
{
	if (!g_XmppClient)
		return L"";

	std::string presence;
	g_XmppClient->GetPresence(utf8_from_wstring(nickname), presence);
	return wstring_from_utf8(presence);
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

std::wstring JSI_Lobby::EncryptPassword(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), std::wstring pass, std::wstring user)
{
	return wstring_from_utf8(JSI_Lobby::EncryptPassword(utf8_from_wstring(pass), utf8_from_wstring(user)));
}

bool JSI_Lobby::IsRankedGame(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return g_rankedGame;
}

void JSI_Lobby::SetRankedGame(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), bool isRanked)
{
	g_rankedGame = isRanked;
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
