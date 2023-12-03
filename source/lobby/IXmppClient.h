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

#ifndef IXMPPCLIENT_H
#define IXMPPCLIENT_H

#include "scriptinterface/ScriptTypes.h"

class ScriptRequest;

class IXmppClient
{
public:
	static IXmppClient* create(const ScriptInterface* scriptInterface, const std::string& sUsername, const std::string& sPassword, const std::string& sRoom, const std::string& sNick, const int historyRequestSize = 0, bool regOpt = false);
	virtual ~IXmppClient() {}

	virtual void connect() = 0;
	virtual void disconnect() = 0;
	virtual bool isConnected() = 0;
	virtual void recv() = 0;
	virtual void SendIqGetBoardList() = 0;
	virtual void SendIqGetProfile(const std::string& player) = 0;
	virtual void SendIqGameReport(const ScriptRequest& rq, JS::HandleValue data) = 0;
	virtual void SendIqRegisterGame(const ScriptRequest& rq, JS::HandleValue data) = 0;
	virtual void SendIqGetConnectionData(const std::string& jid, const std::string& password, const std::string& clientSalt, bool localIP) = 0;
	virtual void SendIqUnregisterGame() = 0;
	virtual void SendIqChangeStateGame(const std::string& nbp, const std::string& players) = 0;
	virtual void SendIqLobbyAuth(const std::string& to, const std::string& token) = 0;
	virtual void SetNick(const std::string& nick) = 0;
	virtual std::string GetNick() const = 0;
	virtual std::string GetJID() const = 0;
	virtual void kick(const std::string& nick, const std::string& reason) = 0;
	virtual void ban(const std::string& nick, const std::string& reason) = 0;
	virtual void SetPresence(const std::string& presence) = 0;
	virtual const char* GetPresence(const std::string& nickname) = 0;
	virtual const char* GetRole(const std::string& nickname) = 0;
	virtual std::wstring GetRating(const std::string& nickname) = 0;
	virtual const std::wstring& GetSubject() = 0;
	virtual JS::Value GUIGetPlayerList(const ScriptRequest& rq) = 0;
	virtual JS::Value GUIGetGameList(const ScriptRequest& rq) = 0;
	virtual JS::Value GUIGetBoardList(const ScriptRequest& rq) = 0;
	virtual JS::Value GUIGetProfile(const ScriptRequest& rq) = 0;

	virtual JS::Value GuiPollNewMessages(const ScriptInterface& guiInterface) = 0;
	virtual JS::Value GuiPollHistoricMessages(const ScriptInterface& guiInterface) = 0;
	virtual bool GuiPollHasPlayerListUpdate() = 0;

	virtual void SendMUCMessage(const std::string& message) = 0;
	virtual void SendStunEndpointToHost(const std::string& ip, u16 port, const std::string& hostJID) = 0;
};

extern IXmppClient *g_XmppClient;
extern bool g_rankedGame;

#endif // XMPPCLIENT_H
