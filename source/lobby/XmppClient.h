/* Copyright (C) 2019 Wildfire Games.
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

#include <deque>

#include "glooxwrapper/glooxwrapper.h"
#include "scriptinterface/ScriptVal.h"

class ScriptInterface;

namespace glooxwrapper
{
	class Client;
	struct CertInfo;
}

class XmppClient : public IXmppClient, public glooxwrapper::ConnectionListener, public glooxwrapper::MUCRoomHandler, public glooxwrapper::IqHandler, public glooxwrapper::RegistrationHandler, public glooxwrapper::MessageHandler, public glooxwrapper::Jingle::SessionHandler
{
	NONCOPYABLE(XmppClient);

private:
	// Components
	glooxwrapper::Client* m_client;
	glooxwrapper::MUCRoom* m_mucRoom;
	glooxwrapper::Registration* m_registration;
	glooxwrapper::SessionManager* m_sessionManager;

	// Account infos
	std::string m_username;
	std::string m_password;
	std::string m_server;
	std::string m_room;
	std::string m_nick;
	std::string m_xpartamuppId;
	std::string m_echelonId;

	// State
	gloox::CertStatus m_certStatus;
	bool m_initialLoadComplete;
	bool m_isConnected;

public:
	// Basic
	XmppClient(const ScriptInterface* scriptInterface, const std::string& sUsername, const std::string& sPassword, const std::string& sRoom, const std::string& sNick, const int historyRequestSize = 0, const bool regOpt = false);
	virtual ~XmppClient();

	// JS::Heap is better for GC performance than JS::PersistentRooted
	static void Trace(JSTracer *trc, void *data)
	{
		static_cast<XmppClient*>(data)->TraceMember(trc);
	}

	void TraceMember(JSTracer *trc);

	// Network
	void connect();
	void disconnect();
	bool isConnected();
	void recv();
	void SendIqGetBoardList();
	void SendIqGetProfile(const std::string& player);
	void SendIqGameReport(const ScriptInterface& scriptInterface, JS::HandleValue data);
	void SendIqRegisterGame(const ScriptInterface& scriptInterface, JS::HandleValue data);
	void SendIqUnregisterGame();
	void SendIqChangeStateGame(const std::string& nbp, const std::string& players);
	void SendIqLobbyAuth(const std::string& to, const std::string& token);
	void SetNick(const std::string& nick);
	void GetNick(std::string& nick);
	void kick(const std::string& nick, const std::string& reason);
	void ban(const std::string& nick, const std::string& reason);
	void SetPresence(const std::string& presence);
	const char* GetPresence(const std::string& nickname);
	const char* GetRole(const std::string& nickname);
	const std::wstring& GetSubject();

	void GUIGetPlayerList(const ScriptInterface& scriptInterface, JS::MutableHandleValue ret);
	void GUIGetGameList(const ScriptInterface& scriptInterface, JS::MutableHandleValue ret);
	void GUIGetBoardList(const ScriptInterface& scriptInterface, JS::MutableHandleValue ret);
	void GUIGetProfile(const ScriptInterface& scriptInterface, JS::MutableHandleValue ret);

	void SendStunEndpointToHost(const StunClient::StunEndpoint& stunEndpoint, const std::string& hostJID);

	/**
	 * Convert gloox values to string or time.
	 */
	static const char* GetPresenceString(const gloox::Presence::PresenceType presenceType);
	static const char* GetRoleString(const gloox::MUCRoomRole role);
	static std::string StanzaErrorToString(gloox::StanzaError err);
	static std::string RegistrationResultToString(gloox::RegistrationResult res);
	static std::string ConnectionErrorToString(gloox::ConnectionError err);
	static std::string CertificateErrorToString(gloox::CertStatus status);
	static std::time_t ComputeTimestamp(const glooxwrapper::Message& msg);

protected:
	/* Xmpp handlers */
	/* MUC handlers */
	virtual void handleMUCParticipantPresence(glooxwrapper::MUCRoom& room, const glooxwrapper::MUCRoomParticipant, const glooxwrapper::Presence&);
	virtual void handleMUCError(glooxwrapper::MUCRoom& room, gloox::StanzaError);
	virtual void handleMUCMessage(glooxwrapper::MUCRoom& room, const glooxwrapper::Message& msg, bool priv);
	virtual void handleMUCSubject(glooxwrapper::MUCRoom& room, const glooxwrapper::string& nick, const glooxwrapper::string& subject);
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
	virtual void handleMessage(const glooxwrapper::Message& msg, glooxwrapper::MessageSession* session);

	/* Session Handler */
	virtual void handleSessionAction(gloox::Jingle::Action action, glooxwrapper::Jingle::Session& session, const glooxwrapper::Jingle::Session::Jingle& jingle);
	virtual void handleSessionInitiation(glooxwrapper::Jingle::Session& session, const glooxwrapper::Jingle::Session::Jingle& jingle);

public:
	JS::Value GuiPollNewMessage(const ScriptInterface& scriptInterface);
	JS::Value GuiPollHistoricMessages(const ScriptInterface& scriptInterface);
	bool GuiPollHasPlayerListUpdate();
	void SendMUCMessage(const std::string& message);

protected:
	template<typename... Args>
	void CreateGUIMessage(
		const std::string& type,
		const std::string& level,
		const std::time_t time,
		Args const&... args);

private:
	struct SPlayer {
		SPlayer(const gloox::Presence::PresenceType presence, const gloox::MUCRoomRole role, const glooxwrapper::string& rating)
		: m_Presence(presence), m_Role(role), m_Rating(rating)
		{
		}
		gloox::Presence::PresenceType m_Presence;
		gloox::MUCRoomRole m_Role;
		glooxwrapper::string m_Rating;
	};
	using PlayerMap = std::map<glooxwrapper::string, SPlayer>;

	/// Map of players
	PlayerMap m_PlayerMap;
	/// Whether or not the playermap has changed since the last time the GUI checked.
	bool m_PlayerMapUpdate;
	/// List of games
	std::vector<const glooxwrapper::Tag*> m_GameList;
	/// List of rankings
	std::vector<const glooxwrapper::Tag*> m_BoardList;
	/// Profile data
	std::vector<const glooxwrapper::Tag*> m_Profile;
	/// ScriptInterface to root the values
	const ScriptInterface* m_ScriptInterface;
	/// Queue of messages for the GUI
	std::deque<JS::Heap<JS::Value> > m_GuiMessageQueue;
	/// Cache of all GUI messages received since the login
	std::vector<JS::Heap<JS::Value> > m_HistoricGuiMessages;
	/// Current room subject/topic.
	std::wstring m_Subject;
};

#endif // XMPPCLIENT_H
