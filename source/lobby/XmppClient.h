/* Copyright (C) 2014 Wildfire Games.
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

#ifndef XXXMPPCLIENT_H
#define XXXMPPCLIENT_H

#include "IXmppClient.h"

#include "glooxwrapper/glooxwrapper.h"

//game - script
#include <deque>
#include "scriptinterface/ScriptVal.h"

//Game - script
class ScriptInterface;

namespace glooxwrapper
{
	class Client;
	struct CertInfo;
}

class XmppClient : public IXmppClient, public glooxwrapper::ConnectionListener, public glooxwrapper::MUCRoomHandler, public glooxwrapper::IqHandler, public glooxwrapper::RegistrationHandler, public glooxwrapper::MessageHandler
{
	NONCOPYABLE(XmppClient);
private:
	//Components
	glooxwrapper::Client* m_client;
	glooxwrapper::MUCRoom* m_mucRoom;
	glooxwrapper::Registration* m_registration;
	//Account infos
	std::string m_username;
	std::string m_password;
	std::string m_nick;
	std::string m_xpartamuppId;

public:
	//Basic
	XmppClient(const std::string& sUsername, const std::string& sPassword, const std::string& sRoom, const std::string& sNick, const int historyRequestSize = 0, const bool regOpt = false);
	virtual ~XmppClient();

	//Network
	void connect();
	void disconnect();
	void recv();
	void SendIqGetGameList();
	void SendIqGetBoardList();
	void SendIqGetRatingList();
	void SendIqGameReport(ScriptInterface& scriptInterface, CScriptVal data);
	void SendIqRegisterGame(ScriptInterface& scriptInterface, CScriptVal data);
	void SendIqUnregisterGame();
	void SendIqChangeStateGame(const std::string& nbp, const std::string& players);
	void SetNick(const std::string& nick);
	void GetNick(std::string& nick);
	void kick(const std::string& nick, const std::string& reason);
	void ban(const std::string& nick, const std::string& reason);
	void SetPresence(const std::string& presence);
	void GetPresence(const std::string& nickname, std::string& presence);
	void GetRole(const std::string& nickname, std::string& role);
	void GetSubject(std::string& subject);

	CScriptValRooted GUIGetPlayerList(ScriptInterface& scriptInterface);
	CScriptValRooted GUIGetGameList(ScriptInterface& scriptInterface);
	CScriptValRooted GUIGetBoardList(ScriptInterface& scriptInterface);
	//Script
	ScriptInterface& GetScriptInterface();

protected:
	/* Xmpp handlers */
	/* MUC handlers */
	virtual void handleMUCParticipantPresence(glooxwrapper::MUCRoom*, const glooxwrapper::MUCRoomParticipant, const glooxwrapper::Presence&);
	virtual void handleMUCError(glooxwrapper::MUCRoom*, gloox::StanzaError);
	virtual void handleMUCMessage(glooxwrapper::MUCRoom* room, const glooxwrapper::Message& msg, bool priv);
	virtual void handleMUCSubject(glooxwrapper::MUCRoom*, const glooxwrapper::string& nick, const glooxwrapper::string& subject);
	/* MUC handlers not supported by glooxwrapper */
	// virtual bool handleMUCRoomCreation(glooxwrapper::MUCRoom*) {return false;}
	// virtual void handleMUCInviteDecline(glooxwrapper::MUCRoom*, const glooxwrapper::JID&, const std::string&) {}
	// virtual void handleMUCInfo(glooxwrapper::MUCRoom*, int, const std::string&, const glooxwrapper::DataForm*) {}
	// virtual void handleMUCItems(glooxwrapper::MUCRoom*, const std::list<gloox::Disco::Item*, std::allocator<gloox::Disco::Item*> >&) {}

	/* Log handler */
	virtual void handleLog(gloox::LogLevel level, gloox::LogArea area, const std::string& message);

	/* ConnectionListener handlers*/
	virtual void onConnect();
	virtual void onDisconnect(gloox::ConnectionError e);
	virtual bool onTLSConnect(const glooxwrapper::CertInfo& info);

	/* Iq Handlers */
	virtual bool handleIq(const glooxwrapper::IQ& iq);
	virtual void handleIqID(const glooxwrapper::IQ&, int) {}

	/* Registration Handlers */
	virtual void handleRegistrationFields(const glooxwrapper::JID& /*from*/, int fields, glooxwrapper::string instructions );
	virtual void handleRegistrationResult(const glooxwrapper::JID& /*from*/, gloox::RegistrationResult result);
	virtual void handleAlreadyRegistered(const glooxwrapper::JID& /*from*/);
	virtual void handleDataForm(const glooxwrapper::JID& /*from*/, const glooxwrapper::DataForm& /*form*/);
	virtual void handleOOB(const glooxwrapper::JID& /*from*/, const glooxwrapper::OOB& oob);

	/* Message Handler */
	virtual void handleMessage(const glooxwrapper::Message& msg, glooxwrapper::MessageSession * session);

	// Helpers
	void GetPresenceString(const gloox::Presence::PresenceType p, std::string& presence) const;
	void GetRoleString(const gloox::MUCRoomRole r, std::string& role) const;
	std::string StanzaErrorToString(gloox::StanzaError err);
public:
	/* Messages */
	struct GUIMessage
	{
		std::wstring type;
		std::wstring level;
		std::wstring text;
		std::wstring data;
		std::wstring from;
		std::wstring message;
	};
	CScriptValRooted GuiPollMessage(ScriptInterface& scriptInterface);
	void SendMUCMessage(const std::string& message);
	protected:
	void PushGuiMessage(XmppClient::GUIMessage message);
	void CreateSimpleMessage(const std::string& type, const std::string& text, const std::string& level = "standard", const std::string& data = "");

private:
	/// Map of players
	std::map<std::string, std::vector<std::string> > m_PlayerMap;
	/// List of games
	std::vector<const glooxwrapper::Tag*> m_GameList;
	/// List of rankings
	std::vector<const glooxwrapper::Tag*> m_BoardList;
	/// Queue of messages for the GUI
	std::deque<GUIMessage> m_GuiMessageQueue;
	/// Current room subject/topic.
	std::string m_Subject;
};

#endif // XMPPCLIENT_H
