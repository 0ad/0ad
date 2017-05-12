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

#ifndef IXMPPCLIENT_H
#define IXMPPCLIENT_H

#include "scriptinterface/ScriptTypes.h"

class ScriptInterface;

class IXmppClient
{
public:
	static IXmppClient* create(const std::string& sUsername, const std::string& sPassword, const std::string& sRoom, const std::string& sNick, const int historyRequestSize = 0, bool regOpt = false);
	virtual ~IXmppClient() {}

	virtual void connect() = 0;
	virtual void disconnect() = 0;
	virtual void recv() = 0;
	virtual void SendIqGetBoardList() = 0;
	virtual void SendIqGetProfile(const std::string& player) = 0;
	virtual void SendIqGameReport(ScriptInterface& scriptInterface, JS::HandleValue data) = 0;
	virtual void SendIqRegisterGame(ScriptInterface& scriptInterface, JS::HandleValue data) = 0;
	virtual void SendIqUnregisterGame() = 0;
	virtual void SendIqChangeStateGame(const std::string& nbp, const std::string& players) = 0;
	virtual void SetNick(const std::string& nick) = 0;
	virtual void GetNick(std::string& nick) = 0;
	virtual void kick(const std::string& nick, const std::string& reason) = 0;
	virtual void ban(const std::string& nick, const std::string& reason) = 0;
	virtual void SetPresence(const std::string& presence) = 0;
	virtual void GetPresence(const std::string& nickname, std::string& presence) = 0;
	virtual void GetRole(const std::string& nickname, std::string& role) = 0;
	virtual void GetSubject(std::string& subject) = 0;
	virtual void GUIGetPlayerList(ScriptInterface& scriptInterface, JS::MutableHandleValue ret) = 0;
	virtual void ClearPresenceUpdates() = 0;
	virtual int GetMucMessageCount() = 0;
	virtual void GUIGetGameList(ScriptInterface& scriptInterface, JS::MutableHandleValue ret) = 0;
	virtual void GUIGetBoardList(ScriptInterface& scriptInterface, JS::MutableHandleValue ret) = 0;
	virtual void GUIGetProfile(ScriptInterface& scriptInterface, JS::MutableHandleValue ret) = 0;

	virtual void GuiPollMessage(ScriptInterface& scriptInterface, JS::MutableHandleValue ret) = 0;
	virtual void SendMUCMessage(const std::string& message) = 0;
};

extern IXmppClient *g_XmppClient;
extern bool g_rankedGame;

#endif // XMPPCLIENT_H
