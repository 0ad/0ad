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

#include "XmppClient.h"
#include "StanzaExtensions.h"

#include "i18n/L10n.h"
#include "lib/utf8.h"
#include "network/NetServer.h"
#include "network/NetClient.h"
#include "network/StunClient.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/GUID.h"
#include "ps/Pyrogenesis.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/StructuredClone.h"

#include <iostream>

//debug
#if 1
#define DbgXMPP(x)
#else
#define DbgXMPP(x) std::cout << x << std::endl;

static std::string tag_xml(const glooxwrapper::IQ& iq)
{
	std::string ret;
	glooxwrapper::Tag* tag = iq.tag();
	ret = tag->xml().to_string();
	glooxwrapper::Tag::free(tag);
	return ret;
}
#endif

static std::string tag_name(const glooxwrapper::IQ& iq)
{
	std::string ret;
	glooxwrapper::Tag* tag = iq.tag();
	ret = tag->name().to_string();
	glooxwrapper::Tag::free(tag);
	return ret;
}

IXmppClient* IXmppClient::create(const ScriptInterface* scriptInterface, const std::string& sUsername, const std::string& sPassword, const std::string& sRoom, const std::string& sNick, const int historyRequestSize,bool regOpt)
{
	return new XmppClient(scriptInterface, sUsername, sPassword, sRoom, sNick, historyRequestSize, regOpt);
}

/**
 * Construct the XMPP client.
 *
 * @param scriptInterface - ScriptInterface to be used for storing GUI messages.
 * Can be left blank for non-visual applications.
 * @param sUsername Username to login with of register.
 * @param sPassword Password to login with or register.
 * @param sRoom MUC room to join.
 * @param sNick Nick to join with.
 * @param historyRequestSize Number of stanzas of room history to request.
 * @param regOpt If we are just registering or not.
 */
XmppClient::XmppClient(const ScriptInterface* scriptInterface, const std::string& sUsername, const std::string& sPassword, const std::string& sRoom, const std::string& sNick, const int historyRequestSize, bool regOpt)
	: m_ScriptInterface(scriptInterface),
	  m_client(nullptr),
	  m_mucRoom(nullptr),
	  m_registration(nullptr),
	  m_username(sUsername),
	  m_password(sPassword),
	  m_room(sRoom),
	  m_nick(sNick),
	  m_initialLoadComplete(false),
	  m_isConnected(false),
	  m_sessionManager(nullptr),
	  m_certStatus(gloox::CertStatus::CertOk),
	  m_PlayerMapUpdate(false),
	  m_connectionDataJid(),
	  m_connectionDataIqId()
{
	if (m_ScriptInterface)
		JS_AddExtraGCRootsTracer(m_ScriptInterface->GetGeneralJSContext(), XmppClient::Trace, this);

	// Read lobby configuration from default.cfg
	std::string sXpartamupp;
	std::string sEchelon;
	CFG_GET_VAL("lobby.server", m_server);
	CFG_GET_VAL("lobby.xpartamupp", sXpartamupp);
	CFG_GET_VAL("lobby.echelon", sEchelon);

	m_xpartamuppId = sXpartamupp + "@" + m_server + "/CC";
	m_echelonId = sEchelon + "@" + m_server + "/CC";
	// Generate a unique, unpredictable resource to allow multiple 0 A.D. instances to connect to the lobby.
	glooxwrapper::JID clientJid(sUsername + "@" + m_server + "/0ad-" + ps_generate_guid());
	glooxwrapper::JID roomJid(m_room + "@conference." + m_server + "/" + sNick);

	// If we are connecting, use the full jid and a password
	// If we are registering, only use the server name
	if (!regOpt)
		m_client = new glooxwrapper::Client(clientJid, sPassword);
	else
		m_client = new glooxwrapper::Client(m_server);

	// Optionally join without a TLS certificate, so a local server can be tested  quickly.
	// Security risks from malicious JS mods can be mitigated if this option and also the hostname and login are shielded from JS access.
	bool tls = true;
	CFG_GET_VAL("lobby.tls", tls);
	m_client->setTls(tls ? gloox::TLSRequired : gloox::TLSDisabled);

	// Disable use of the SASL PLAIN mechanism, to prevent leaking credentials
	// if the server doesn't list any supported SASL mechanism or the response
	// has been modified to exclude those.
	const int mechs = gloox::SaslMechAll ^ gloox::SaslMechPlain;
	m_client->setSASLMechanisms(mechs);

	m_client->registerConnectionListener(this);
	m_client->setPresence(gloox::Presence::Available, -1);
	m_client->disco()->setVersion("Pyrogenesis", engine_version);
	m_client->disco()->setIdentity("client", "bot");
	m_client->setCompression(false);

	m_client->registerStanzaExtension(new GameListQuery());
	m_client->registerIqHandler(this, EXTGAMELISTQUERY);

	m_client->registerStanzaExtension(new BoardListQuery());
	m_client->registerIqHandler(this, EXTBOARDLISTQUERY);

	m_client->registerStanzaExtension(new ProfileQuery());
	m_client->registerIqHandler(this, EXTPROFILEQUERY);

	m_client->registerStanzaExtension(new LobbyAuth());
	m_client->registerIqHandler(this, EXTLOBBYAUTH);

	m_client->registerStanzaExtension(new ConnectionData());
	m_client->registerIqHandler(this, EXTCONNECTIONDATA);

	m_client->registerMessageHandler(this);

	// Uncomment to see the raw stanzas
	//m_client->getWrapped()->logInstance().registerLogHandler( gloox::LogLevelDebug, gloox::LogAreaAll, this );

	if (!regOpt)
	{
		// Create a Multi User Chat Room
		m_mucRoom = new glooxwrapper::MUCRoom(m_client, roomJid, this, 0);
		// Get room history.
		m_mucRoom->setRequestHistory(historyRequestSize, gloox::MUCRoom::HistoryMaxStanzas);
	}
	else
	{
		// Registration
		m_registration = new glooxwrapper::Registration(m_client);
		m_registration->registerRegistrationHandler(this);
	}

	m_sessionManager = new glooxwrapper::SessionManager(m_client, this);
	// Register plugins to allow gloox parse them in incoming sessions
	m_sessionManager->registerPlugins();
}

/**
 * Destroy the xmpp client
 */
XmppClient::~XmppClient()
{
	DbgXMPP("XmppClient destroyed");
	delete m_registration;
	delete m_mucRoom;
	delete m_sessionManager;

	// Workaround for memory leak in gloox 1.0/1.0.1
	m_client->removePresenceExtension(gloox::ExtCaps);

	delete m_client;

	for (const glooxwrapper::Tag* const& t : m_GameList)
		glooxwrapper::Tag::free(t);
	for (const glooxwrapper::Tag* const& t : m_BoardList)
		glooxwrapper::Tag::free(t);
	for (const glooxwrapper::Tag* const& t : m_Profile)
		glooxwrapper::Tag::free(t);

	if (m_ScriptInterface)
		JS_RemoveExtraGCRootsTracer(m_ScriptInterface->GetGeneralJSContext(), XmppClient::Trace, this);
}

void XmppClient::TraceMember(JSTracer* trc)
{
	for (JS::Heap<JS::Value>& guiMessage : m_GuiMessageQueue)
		JS::TraceEdge(trc, &guiMessage, "m_GuiMessageQueue");

	for (JS::Heap<JS::Value>& guiMessage : m_HistoricGuiMessages)
		JS::TraceEdge(trc, &guiMessage, "m_HistoricGuiMessages");
}

/// Network
void XmppClient::connect()
{
	m_initialLoadComplete = false;
	m_client->connect(false);
}

void XmppClient::disconnect()
{
	m_client->disconnect();
}

bool XmppClient::isConnected()
{
	return m_isConnected;
}

void XmppClient::recv()
{
	m_client->recv(1);
}

/**
 * Log (debug) Handler
 */
void XmppClient::handleLog(gloox::LogLevel level, gloox::LogArea area, const std::string& message)
{
	std::cout << "log: level: " << level << ", area: " << area << ", message: " << message << std::endl;
}

/*****************************************************
 * Connection handlers                               *
 *****************************************************/

/**
 * Handle connection
 */
void XmppClient::onConnect()
{
	if (m_mucRoom)
	{
		m_isConnected = true;
		CreateGUIMessage("system", "connected", std::time(nullptr));
		m_mucRoom->join();
	}

	if (m_registration)
		m_registration->fetchRegistrationFields();
}

/**
 * Handle disconnection
 */
void XmppClient::onDisconnect(gloox::ConnectionError error)
{
	// Make sure we properly leave the room so that
	// everything works if we decide to come back later
	if (m_mucRoom)
		m_mucRoom->leave();

	// Clear game, board and player lists.
	for (const glooxwrapper::Tag* const& t : m_GameList)
		glooxwrapper::Tag::free(t);
	for (const glooxwrapper::Tag* const& t : m_BoardList)
		glooxwrapper::Tag::free(t);
	for (const glooxwrapper::Tag* const& t : m_Profile)
		glooxwrapper::Tag::free(t);

	m_BoardList.clear();
	m_GameList.clear();
	m_PlayerMap.clear();
	m_PlayerMapUpdate = true;
	m_Profile.clear();
	m_HistoricGuiMessages.clear();
	m_isConnected = false;
	m_initialLoadComplete = false;

	CreateGUIMessage(
		"system",
		"disconnected",
		std::time(nullptr),
		"reason", error,
		"certificate_status", m_certStatus);
}

/**
 * Handle TLS connection.
 */
bool XmppClient::onTLSConnect(const glooxwrapper::CertInfo& info)
{
	DbgXMPP("onTLSConnect");
	DbgXMPP(
		"status: " << info.status <<
		"\nissuer: " << info.issuer <<
		"\npeer: " << info.server <<
		"\nprotocol: " << info.protocol <<
		"\nmac: " << info.mac <<
		"\ncipher: " << info.cipher <<
		"\ncompression: " << info.compression );

	m_certStatus = static_cast<gloox::CertStatus>(info.status);

	// Optionally accept invalid certificates, see require_tls option.
	bool verify_certificate = true;
	CFG_GET_VAL("lobby.verify_certificate", verify_certificate);

	return info.status == gloox::CertOk || !verify_certificate;
}

/**
 * Handle MUC room errors
 */
void XmppClient::handleMUCError(glooxwrapper::MUCRoom& UNUSED(room), gloox::StanzaError err)
{
	DbgXMPP("MUC Error " << ": " << StanzaErrorToString(err));
	CreateGUIMessage("system", "error", std::time(nullptr), "text", err);
}

/*****************************************************
 * Requests to server                                *
 *****************************************************/

/**
 * Request the leaderboard data from the server.
 */
void XmppClient::SendIqGetBoardList()
{
	glooxwrapper::JID echelonJid(m_echelonId);

	// Send IQ
	BoardListQuery* b = new BoardListQuery();
	b->m_Command = "getleaderboard";
	glooxwrapper::IQ iq(gloox::IQ::Get, echelonJid, m_client->getID());
	iq.addExtension(b);
	DbgXMPP("SendIqGetBoardList [" << tag_xml(iq) << "]");
	m_client->send(iq);
}

/**
 * Request the profile data from the server.
 */
void XmppClient::SendIqGetProfile(const std::string& player)
{
	glooxwrapper::JID echelonJid(m_echelonId);

	// Send IQ
	ProfileQuery* b = new ProfileQuery();
	b->m_Command = player;
	glooxwrapper::IQ iq(gloox::IQ::Get, echelonJid, m_client->getID());
	iq.addExtension(b);
	DbgXMPP("SendIqGetProfile [" << tag_xml(iq) << "]");
	m_client->send(iq);
}

/**
 * Request the Connection data (ip, port...) from the server.
 */
void XmppClient::SendIqGetConnectionData(const std::string& jid, const std::string& password, const std::string& clientSalt, bool localIP)
{
	glooxwrapper::JID targetJID(jid);

	ConnectionData* connectionData = new ConnectionData();
	connectionData->m_Password = password;
	connectionData->m_ClientSalt = clientSalt;
	connectionData->m_IsLocalIP = localIP ? "1" : "0";
	glooxwrapper::IQ iq(gloox::IQ::Get, targetJID, m_client->getID());
	iq.addExtension(connectionData);
	m_connectionDataJid = iq.from().full();
	m_connectionDataIqId = iq.id().to_string();
	DbgXMPP("SendIqGetConnectionData [" << tag_xml(iq) << "]");
	m_client->send(iq);
}

/**
 * Send game report containing numerous game properties to the server.
 *
 * @param data A JS array of game statistics
 */
void XmppClient::SendIqGameReport(const ScriptRequest& rq, JS::HandleValue data)
{
	glooxwrapper::JID echelonJid(m_echelonId);

	// Setup some base stanza attributes
	GameReport* game = new GameReport();
	glooxwrapper::Tag* report = glooxwrapper::Tag::allocate("game");

	// Iterate through all the properties reported and add them to the stanza.
	std::vector<std::string> properties;
	Script::EnumeratePropertyNames(rq, data, true, properties);
	for (const std::string& p : properties)
	{
		std::wstring value;
		Script::GetProperty(rq, data, p.c_str(), value);
		report->addAttribute(p, utf8_from_wstring(value));
	}

	// Add stanza to IQ
	game->m_GameReport.emplace_back(report);

	// Send IQ
	glooxwrapper::IQ iq(gloox::IQ::Set, echelonJid, m_client->getID());
	iq.addExtension(game);
	DbgXMPP("SendGameReport [" << tag_xml(iq) << "]");
	m_client->send(iq);
};

/**
 * Send a request to register a game to the server.
 *
 * @param data A JS array of game attributes
 */
void XmppClient::SendIqRegisterGame(const ScriptRequest& rq, JS::HandleValue data)
{
	glooxwrapper::JID xpartamuppJid(m_xpartamuppId);

	// Setup some base stanza attributes
	std::unique_ptr<GameListQuery> g = std::make_unique<GameListQuery>();
	g->m_Command = "register";
	glooxwrapper::Tag* game = glooxwrapper::Tag::allocate("game");

	// Iterate through all the properties reported and add them to the stanza.
	std::vector<std::string> properties;
	Script::EnumeratePropertyNames(rq, data, true, properties);
	for (const std::string& p : properties)
	{
		std::string value;
		if (!Script::GetProperty(rq, data, p.c_str(), value))
		{
			LOGERROR("Could not parse attribute '%s' as string.", p);
			return;
		}
		game->addAttribute(p, value);
	}

	// Overwrite some attributes to make it slightly less trivial to do bad things,
	// and explicit some invariants.

	// The JID must point to ourself.
	game->addAttribute("hostJID", GetJID());

	// Push the stanza onto the IQ
	g->m_GameList.emplace_back(game);

	// Send IQ
	glooxwrapper::IQ iq(gloox::IQ::Set, xpartamuppJid, m_client->getID());
	iq.addExtension(g.release());
	DbgXMPP("SendIqRegisterGame [" << tag_xml(iq) << "]");
	m_client->send(iq);
}

/**
 * Send a request to unregister a game to the server.
 */
void XmppClient::SendIqUnregisterGame()
{
	glooxwrapper::JID xpartamuppJid(m_xpartamuppId);

	// Send IQ
	GameListQuery* g = new GameListQuery();
	g->m_Command = "unregister";
	g->m_GameList.emplace_back(glooxwrapper::Tag::allocate("game"));

	glooxwrapper::IQ iq(gloox::IQ::Set, xpartamuppJid, m_client->getID());
	iq.addExtension(g);
	DbgXMPP("SendIqUnregisterGame [" << tag_xml(iq) << "]");
	m_client->send(iq);
}

/**
 * Send a request to change the state of a registered game on the server.
 *
 * A game can either be in the 'running' or 'waiting' state - the server
 * decides which - but we need to update the current players that are
 * in-game so the server can make the calculation.
 */
void XmppClient::SendIqChangeStateGame(const std::string& nbp, const std::string& players)
{
	glooxwrapper::JID xpartamuppJid(m_xpartamuppId);

	// Send IQ
	GameListQuery* g = new GameListQuery();
	g->m_Command = "changestate";
	glooxwrapper::Tag* game = glooxwrapper::Tag::allocate("game");
	game->addAttribute("nbp", nbp);
	game->addAttribute("players", players);
	g->m_GameList.emplace_back(game);

	glooxwrapper::IQ iq(gloox::IQ::Set, xpartamuppJid, m_client->getID());
	iq.addExtension(g);
	DbgXMPP("SendIqChangeStateGame [" << tag_xml(iq) << "]");
	m_client->send(iq);
}

/*****************************************************
 * iq to clients                                     *
 *****************************************************/

/**
 * Send lobby authentication token.
 */
void XmppClient::SendIqLobbyAuth(const std::string& to, const std::string& token)
{
	LobbyAuth* auth = new LobbyAuth();
	auth->m_Token = token;

	glooxwrapper::JID clientJid(to);
	glooxwrapper::IQ iq(gloox::IQ::Set, clientJid, m_client->getID());
	iq.addExtension(auth);
	DbgXMPP("SendIqLobbyAuth [" << tag_xml(iq) << "]");
	m_client->send(iq);
}

/*****************************************************
 * Account registration                              *
 *****************************************************/

void XmppClient::handleRegistrationFields(const glooxwrapper::JID&, int fields, glooxwrapper::string)
{
	glooxwrapper::RegistrationFields vals;
	vals.username = m_username;
	vals.password = m_password;
	m_registration->createAccount(fields, vals);
}

void XmppClient::handleRegistrationResult(const glooxwrapper::JID&, gloox::RegistrationResult result)
{
	if (result == gloox::RegistrationSuccess)
		CreateGUIMessage("system", "registered", std::time(nullptr));
	else
		CreateGUIMessage("system", "error", std::time(nullptr), "text", result);

	disconnect();
}

void XmppClient::handleAlreadyRegistered(const glooxwrapper::JID&)
{
	DbgXMPP("the account already exists");
}

void XmppClient::handleDataForm(const glooxwrapper::JID&, const glooxwrapper::DataForm&)
{
	DbgXMPP("dataForm received");
}

void XmppClient::handleOOB(const glooxwrapper::JID&, const glooxwrapper::OOB&)
{
	DbgXMPP("OOB registration requested");
}

/*****************************************************
 * Requests from GUI                                 *
 *****************************************************/

/**
 * Handle requests from the GUI for the list of players.
 *
 * @return A JS array containing all known players and their presences
 */
JS::Value XmppClient::GUIGetPlayerList(const ScriptRequest& rq)
{
	JS::RootedValue ret(rq.cx);
	Script::CreateArray(rq, &ret);
	int j = 0;

	for (const std::pair<const glooxwrapper::string, SPlayer>& p : m_PlayerMap)
	{
		JS::RootedValue player(rq.cx);

		Script::CreateObject(
			rq,
			&player,
			"name", p.first,
			"presence", p.second.m_Presence,
			"rating", p.second.m_Rating,
			"role", p.second.m_Role);

		Script::SetPropertyInt(rq, ret, j++, player);
	}
	return ret;
}

/**
 * Handle requests from the GUI for the list of all active games.
 *
 * @return A JS array containing all known games
 */
JS::Value XmppClient::GUIGetGameList(const ScriptRequest& rq)
{
	JS::RootedValue ret(rq.cx);
	Script::CreateArray(rq, &ret);
	int j = 0;

	const char* stats[] = { "name", "hostUsername", "hostJID", "state", "hasPassword",
		"nbp", "maxnbp", "players", "mapName", "niceMapName", "mapSize", "mapType",
		"victoryConditions", "startTime", "mods" };

	for(const glooxwrapper::Tag* const& t : m_GameList)
	{
		JS::RootedValue game(rq.cx);
		Script::CreateObject(rq, &game);

		for (size_t i = 0; i < ARRAY_SIZE(stats); ++i)
			Script::SetProperty(rq, game, stats[i], t->findAttribute(stats[i]));

		Script::SetPropertyInt(rq, ret, j++, game);
	}
	return ret;
}

/**
 * Handle requests from the GUI for leaderboard data.
 *
 * @return A JS array containing all known leaderboard data
 */
JS::Value XmppClient::GUIGetBoardList(const ScriptRequest& rq)
{
	JS::RootedValue ret(rq.cx);
	Script::CreateArray(rq, &ret);
	int j = 0;

	const char* attributes[] = { "name", "rank", "rating" };

	for(const glooxwrapper::Tag* const& t : m_BoardList)
	{
		JS::RootedValue board(rq.cx);
		Script::CreateObject(rq, &board);

		for (size_t i = 0; i < ARRAY_SIZE(attributes); ++i)
			Script::SetProperty(rq, board, attributes[i], t->findAttribute(attributes[i]));

		Script::SetPropertyInt(rq, ret, j++, board);
	}
	return ret;
}

/**
 * Handle requests from the GUI for profile data.
 *
 * @return A JS array containing the specific user's profile data
 */
JS::Value XmppClient::GUIGetProfile(const ScriptRequest& rq)
{
	JS::RootedValue ret(rq.cx);
	Script::CreateArray(rq, &ret);
	int j = 0;

	const char* stats[] = { "player", "rating", "totalGamesPlayed", "highestRating", "wins", "losses", "rank" };

	for (const glooxwrapper::Tag* const& t : m_Profile)
	{
		JS::RootedValue profile(rq.cx);
		Script::CreateObject(rq, &profile);

		for (size_t i = 0; i < ARRAY_SIZE(stats); ++i)
			Script::SetProperty(rq, profile, stats[i], t->findAttribute(stats[i]));

		Script::SetPropertyInt(rq, ret, j++, profile);
	}
	return ret;
}

/*****************************************************
 * Message interfaces                                *
 *****************************************************/

void SetGUIMessageProperty(const ScriptRequest& UNUSED(rq), JS::HandleObject UNUSED(messageObj))
{
}

template<typename T, typename... Args>
void SetGUIMessageProperty(const ScriptRequest& rq, JS::HandleObject messageObj, const std::string& propertyName, const T& propertyValue, Args const&... args)
{
	JS::RootedValue scriptPropertyValue(rq.cx);
	Script::ToJSVal(rq, &scriptPropertyValue, propertyValue);
	JS_DefineProperty(rq.cx, messageObj, propertyName.c_str(), scriptPropertyValue, JSPROP_ENUMERATE);
	SetGUIMessageProperty(rq, messageObj, args...);
}

template<typename... Args>
void XmppClient::CreateGUIMessage(
	const std::string& type,
	const std::string& level,
	const std::time_t time,
	Args const&... args)
{
	if (!m_ScriptInterface)
		return;
	ScriptRequest rq(m_ScriptInterface);
	JS::RootedValue message(rq.cx);
	Script::CreateObject(
		rq,
		&message,
		"type", type,
		"level", level,
		"historic", false,
		"time", static_cast<double>(time));

	JS::RootedObject messageObj(rq.cx, message.toObjectOrNull());
	SetGUIMessageProperty(rq, messageObj, args...);
	Script::FreezeObject(rq, message, true);
	m_GuiMessageQueue.push_back(JS::Heap<JS::Value>(message));
}

bool XmppClient::GuiPollHasPlayerListUpdate()
{
	// The initial playerlist will be received in multiple messages
	// Only inform the GUI after all of these playerlist fragments were received.
	if (!m_initialLoadComplete)
		return false;

	bool hasUpdate = m_PlayerMapUpdate;
	m_PlayerMapUpdate = false;
	return hasUpdate;
}

JS::Value XmppClient::GuiPollNewMessages(const ScriptInterface& guiInterface)
{
	if ((m_isConnected && !m_initialLoadComplete) || m_GuiMessageQueue.empty())
		return JS::UndefinedValue();

	ScriptRequest rq(m_ScriptInterface);

	// Optimize for batch message processing that is more
	// performance demanding than processing a lone message.
	JS::RootedValue messages(rq.cx);
	Script::CreateArray(rq, &messages);

	int j = 0;

	for (const JS::Heap<JS::Value>& message : m_GuiMessageQueue)
	{
		Script::SetPropertyInt(rq, messages, j++, message);

		// Store historic chat messages.
		// Only store relevant messages to minimize memory footprint.
		JS::RootedValue rootedMessage(rq.cx, message);
		std::string type;
		Script::GetProperty(rq, rootedMessage, "type", type);
		if (type != "chat")
			continue;

		std::string level;
		Script::GetProperty(rq, rootedMessage, "level", level);
		if (level != "room-message" && level != "private-message")
			continue;

		JS::RootedValue historicMessage(rq.cx, Script::DeepCopy(rq, rootedMessage));
		if (true)
		{
			Script::SetProperty(rq, historicMessage, "historic", true);
			Script::FreezeObject(rq, historicMessage, true);
			m_HistoricGuiMessages.push_back(JS::Heap<JS::Value>(historicMessage));
		}
		else
			LOGERROR("Could not clone historic lobby GUI message!");
	}
	m_GuiMessageQueue.clear();

	// Copy the messages over to the caller script interface.
	return Script::CloneValueFromOtherCompartment(guiInterface, *m_ScriptInterface, messages);
}

JS::Value XmppClient::GuiPollHistoricMessages(const ScriptInterface& guiInterface)
{
	if (m_HistoricGuiMessages.empty())
		return JS::UndefinedValue();

	ScriptRequest rq(m_ScriptInterface);

	JS::RootedValue messages(rq.cx);
	Script::CreateArray(rq, &messages);

	int j = 0;
	for (const JS::Heap<JS::Value>& message : m_HistoricGuiMessages)
		Script::SetPropertyInt(rq, messages, j++, message);

	// Copy the messages over to the caller script interface.
	return Script::CloneValueFromOtherCompartment(guiInterface, *m_ScriptInterface, messages);
}

/**
 * Send a standard MUC textual message.
 */
void XmppClient::SendMUCMessage(const std::string& message)
{
	m_mucRoom->send(message);
}

/**
 * Handle a room message.
 */
void XmppClient::handleMUCMessage(glooxwrapper::MUCRoom& UNUSED(room), const glooxwrapper::Message& msg, bool priv)
{
	DbgXMPP(msg.from().resource() << " said " << msg.body());

	CreateGUIMessage(
		"chat",
		priv ? "private-message" : "room-message",
		ComputeTimestamp(msg),
		"from", msg.from().resource(),
		"text", msg.body());
}

/**
 * Handle a private message.
 */
void XmppClient::handleMessage(const glooxwrapper::Message& msg, glooxwrapper::MessageSession*)
{
	DbgXMPP("type " << msg.subtype() << ", subject " << msg.subject()
	  << ", message " << msg.body() << ", thread id " << msg.thread());

	CreateGUIMessage(
		"chat",
		"private-message",
		ComputeTimestamp(msg),
		"from", msg.from().resource(),
		"text", msg.body());
}

/**
 * Handle portions of messages containing custom stanza extensions.
 */
bool XmppClient::handleIq(const glooxwrapper::IQ& iq)
{
	DbgXMPP("handleIq [" << tag_xml(iq) << "]");

	if (iq.subtype() == gloox::IQ::Result)
	{
		const GameListQuery* gq = iq.findExtension<GameListQuery>(EXTGAMELISTQUERY);
		const BoardListQuery* bq = iq.findExtension<BoardListQuery>(EXTBOARDLISTQUERY);
		const ProfileQuery* pq = iq.findExtension<ProfileQuery>(EXTPROFILEQUERY);
		const ConnectionData* cd = iq.findExtension<ConnectionData>(EXTCONNECTIONDATA);
		if (cd)
		{
			if (g_NetServer || !g_NetClient)
				return true;

			if (!m_connectionDataJid.empty() && m_connectionDataJid.compare(iq.from().full()) != 0) {
				LOGMESSAGE("XmppClient: Received connection data from invalid host: %s", iq.from().username());
				return true;
			}

			if (!m_connectionDataIqId.empty() && m_connectionDataIqId.compare(iq.id().to_string()) != 0) {
				LOGMESSAGE("XmppClient: Received connection data with invalid id");
				return true;
			}

			if (!cd->m_Error.empty())
			{
				g_NetClient->HandleGetServerDataFailed(cd->m_Error.c_str());
				return true;
			}

			g_NetClient->SetupServerData(cd->m_Ip.to_string(), stoi(cd->m_Port.to_string()), !cd->m_UseSTUN.empty());
			g_NetClient->TryToConnect(iq.from().full(), !cd->m_IsLocalIP.empty());
		}
		if (gq)
		{
			if (iq.from().full() == m_xpartamuppId && gq->m_Command == "register" && g_NetServer && !g_NetServer->GetUseSTUN())
			{
				if (gq->m_GameList.empty())
				{
					LOGWARNING("XmppClient: Received empty game list in response to Game Register");
					return true;
				}
				std::string publicIP = gq->m_GameList.front()->findAttribute("ip").to_string();
				if (publicIP.empty())
				{
					LOGWARNING("XmppClient: Received game with no IP in response to Game Register");
					return true;
				}
				g_NetServer->SetConnectionData(publicIP, g_NetServer->GetPublicPort());
				return true;
			}

			for (const glooxwrapper::Tag* const& t : m_GameList)
				glooxwrapper::Tag::free(t);
			m_GameList.clear();

			for (const glooxwrapper::Tag* const& t : gq->m_GameList)
				m_GameList.emplace_back(t->clone());

			CreateGUIMessage("game", "gamelist", std::time(nullptr));
		}
		if (bq)
		{
			if (bq->m_Command == "boardlist")
			{
				for (const glooxwrapper::Tag* const& t : m_BoardList)
					glooxwrapper::Tag::free(t);
				m_BoardList.clear();

				for (const glooxwrapper::Tag* const& t : bq->m_StanzaBoardList)
					m_BoardList.emplace_back(t->clone());

				CreateGUIMessage("game", "leaderboard", std::time(nullptr));
			}
			else if (bq->m_Command == "ratinglist")
			{
				for (const glooxwrapper::Tag* const& t : bq->m_StanzaBoardList)
				{
					const PlayerMap::iterator it = m_PlayerMap.find(t->findAttribute("name"));
					if (it != m_PlayerMap.end())
					{
						it->second.m_Rating = t->findAttribute("rating");
						m_PlayerMapUpdate = true;
					}
				}
				CreateGUIMessage("game", "ratinglist", std::time(nullptr));
			}
		}
		if (pq)
		{
			for (const glooxwrapper::Tag* const& t : m_Profile)
				glooxwrapper::Tag::free(t);
			m_Profile.clear();

			for (const glooxwrapper::Tag* const& t : pq->m_StanzaProfile)
				m_Profile.emplace_back(t->clone());

			CreateGUIMessage("game", "profile", std::time(nullptr));
		}
	}
	else if (iq.subtype() == gloox::IQ::Set)
	{
		const LobbyAuth* lobbyAuth = iq.findExtension<LobbyAuth>(EXTLOBBYAUTH);
		if (lobbyAuth)
		{
			LOGMESSAGE("XmppClient: Received lobby auth: %s from %s", lobbyAuth->m_Token.to_string(), iq.from().username());

			glooxwrapper::IQ response(gloox::IQ::Result, iq.from(), iq.id());
			m_client->send(response);

			if (g_NetServer)
				g_NetServer->OnLobbyAuth(iq.from().username(), lobbyAuth->m_Token.to_string());
			else
				LOGMESSAGE("Received lobby authentication request, but not hosting currently!");
		}
	}
	else if (iq.subtype() == gloox::IQ::Get)
	{
		const ConnectionData* cd = iq.findExtension<ConnectionData>(EXTCONNECTIONDATA);
		if (cd)
		{
			LOGMESSAGE("XmppClient: Received request for connection data from %s", iq.from().username());
			if (!g_NetServer)
			{
				glooxwrapper::IQ response(gloox::IQ::Result, iq.from(), iq.id());
				ConnectionData* connectionData = new ConnectionData();
				connectionData->m_Error = "not_server";

				response.addExtension(connectionData);

				m_client->send(response);
				return true;
			}
			if (g_NetServer->IsBanned(iq.from().username()))
			{
				glooxwrapper::IQ response(gloox::IQ::Result, iq.from(), iq.id());
				ConnectionData* connectionData = new ConnectionData();
				connectionData->m_Error = "banned";

				response.addExtension(connectionData);

				m_client->send(response);
				return true;
			}
			if (!g_NetServer->CheckPasswordAndIncrement(iq.from().username(), cd->m_Password.to_string(), cd->m_ClientSalt.to_string()))
			{
				glooxwrapper::IQ response(gloox::IQ::Result, iq.from(), iq.id());
				ConnectionData* connectionData = new ConnectionData();
				connectionData->m_Error = "invalid_password";

				response.addExtension(connectionData);

				m_client->send(response);
				return true;
			}

			glooxwrapper::IQ response(gloox::IQ::Result, iq.from(), iq.id());
			ConnectionData* connectionData = new ConnectionData();

			if (cd->m_IsLocalIP.to_string() == "0")
			{
				connectionData->m_Ip = g_NetServer->GetPublicIp();
				connectionData->m_Port = std::to_string(g_NetServer->GetPublicPort());
				connectionData->m_UseSTUN = g_NetServer->GetUseSTUN() ? "true" : "";
				connectionData->m_IsLocalIP = "";
			}
			else
			{
				CStr ip;
				if (StunClient::FindLocalIP(ip))
				{
					connectionData->m_Ip = ip;
					connectionData->m_Port = std::to_string(g_NetServer->GetLocalPort());
					connectionData->m_UseSTUN = "";
					connectionData->m_IsLocalIP = "true";
				}
				else
					connectionData->m_Error = "local_ip_failed";
			}

			response.addExtension(connectionData);

			m_client->send(response);
		}

	}
	else if (iq.subtype() == gloox::IQ::Error)
		CreateGUIMessage("system", "error", std::time(nullptr), "text", iq.error_error());
	else
	{
		CreateGUIMessage("system", "error", std::time(nullptr), "text", wstring_from_utf8(g_L10n.Translate("unknown subtype (see logs)")));
		LOGMESSAGE("unknown subtype '%s'", tag_name(iq).c_str());
	}

	return true;
}

/**
 * Update local data when a user changes presence.
 */
void XmppClient::handleMUCParticipantPresence(glooxwrapper::MUCRoom& UNUSED(room), const glooxwrapper::MUCRoomParticipant participant, const glooxwrapper::Presence& presence)
{
	const glooxwrapper::string& nick = participant.nick->resource();

	if (presence.presence() == gloox::Presence::Unavailable)
	{
		if (!participant.newNick.empty() && (participant.flags & (gloox::UserNickChanged | gloox::UserSelf)))
		{
			// we have a nick change
			if (m_PlayerMap.find(participant.newNick) == m_PlayerMap.end())
				m_PlayerMap.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(participant.newNick),
					std::forward_as_tuple(presence.presence(), participant.role, std::move(m_PlayerMap.at(nick).m_Rating)));
			else
				LOGERROR("Nickname changed to an existing nick!");

			DbgXMPP(nick << " is now known as " << participant.newNick);
			CreateGUIMessage(
				"chat",
				"nick",
				std::time(nullptr),
				"oldnick", nick,
				"newnick", participant.newNick);
		}
		else if (participant.flags & gloox::UserKicked)
		{
			DbgXMPP(nick << " was kicked. Reason: " << participant.reason);
			CreateGUIMessage(
				"chat",
				"kicked",
				std::time(nullptr),
				"nick", nick,
				"reason", participant.reason);
		}
		else if (participant.flags & gloox::UserBanned)
		{
			DbgXMPP(nick << " was banned. Reason: " << participant.reason);
			CreateGUIMessage(
				"chat",
				"banned",
				std::time(nullptr),
				"nick", nick,
				"reason", participant.reason);
		}
		else
		{
			DbgXMPP(nick << " left the room (flags " << participant.flags << ")");
			CreateGUIMessage(
				"chat",
				"leave",
				std::time(nullptr),
				"nick", nick);
		}
		m_PlayerMap.erase(nick);
	}
	else
	{
		const PlayerMap::iterator it = m_PlayerMap.find(nick);

		/* During the initialization process, we receive join messages for everyone
		 * currently in the room. We don't want to display these, so we filter them
		 * out. We will always be the last to join during initialization.
		 */
		if (!m_initialLoadComplete)
		{
			if (m_mucRoom->nick() == nick)
				m_initialLoadComplete = true;
		}
		else if (it == m_PlayerMap.end())
		{
			CreateGUIMessage(
				"chat",
				"join",
				std::time(nullptr),
				"nick", nick);
		}
		else if (it->second.m_Role != participant.role)
		{
			CreateGUIMessage(
				"chat",
				"role",
				std::time(nullptr),
				"nick", nick,
				"oldrole", it->second.m_Role,
				"newrole", participant.role);
		}
		else
		{
			// Don't create a GUI message for regular presence changes, because
			// several hundreds of them accumulate during a match, impacting performance terribly and
			// the only way they are used is to determine whether to update the playerlist.
		}

		DbgXMPP(
			nick << " is in the room, "
			"presence: " << GetPresenceString(presence.presence()) << ", "
			"role: "<< GetRoleString(participant.role));

		if (it == m_PlayerMap.end())
		{
			m_PlayerMap.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(nick),
				std::forward_as_tuple(presence.presence(), participant.role, std::string()));
		}
		else
		{
			it->second.m_Presence = presence.presence();
			it->second.m_Role = participant.role;
		}
	}

	m_PlayerMapUpdate = true;
}

/**
 * Update local cache when subject changes.
 */
void XmppClient::handleMUCSubject(glooxwrapper::MUCRoom& UNUSED(room), const glooxwrapper::string& nick, const glooxwrapper::string& subject)
{
	m_Subject = wstring_from_utf8(subject.to_string());

	CreateGUIMessage(
		"chat",
		"subject",
		std::time(nullptr),
		"nick", nick,
		"subject", m_Subject);
}

/**
 * Get current subject.
 */
const std::wstring& XmppClient::GetSubject()
{
	return m_Subject;
}

/**
 * Request nick change, real change via mucRoomHandler.
 *
 * @param nick Desired nickname
 */
void XmppClient::SetNick(const std::string& nick)
{
	m_mucRoom->setNick(nick);
}

/**
 * Get current nickname.
 */
std::string XmppClient::GetNick() const
{
	return m_mucRoom->nick().to_string();
}

std::string XmppClient::GetJID() const
{
	return m_client->getJID().to_string();
}

/**
 * Kick a player from the current room.
 *
 * @param nick Nickname to be kicked
 * @param reason Reason the player was kicked
 */
void XmppClient::kick(const std::string& nick, const std::string& reason)
{
	m_mucRoom->kick(nick, reason);
}

/**
 * Ban a player from the current room.
 *
 * @param nick Nickname to be banned
 * @param reason Reason the player was banned
 */
void XmppClient::ban(const std::string& nick, const std::string& reason)
{
	m_mucRoom->ban(nick, reason);
}

/**
 * Change the xmpp presence of the client.
 *
 * @param presence A string containing the desired presence
 */
void XmppClient::SetPresence(const std::string& presence)
{
#define IF(x,y) if (presence == x) m_mucRoom->setPresence(gloox::Presence::y)
	IF("available", Available);
	else IF("chat", Chat);
	else IF("away", Away);
	else IF("playing", DND);
	else IF("offline", Unavailable);
	// The others are not to be set
#undef IF
	else LOGERROR("Unknown presence '%s'", presence.c_str());
}

/**
 * Get the current xmpp presence of the given nick.
 */
const char* XmppClient::GetPresence(const std::string& nick)
{
	const PlayerMap::iterator it = m_PlayerMap.find(nick);

	if (it == m_PlayerMap.end())
		return "offline";

	return GetPresenceString(it->second.m_Presence);
}

/**
 * Get the current xmpp role of the given nick.
 */
const char* XmppClient::GetRole(const std::string& nick)
{
	const PlayerMap::iterator it = m_PlayerMap.find(nick);

	if (it == m_PlayerMap.end())
		return "";

	return GetRoleString(it->second.m_Role);
}

/**
 * Get the most recent received rating of the given nick.
 * Notice that this doesn't request a rating profile if it hasn't been received yet.
 */
std::wstring XmppClient::GetRating(const std::string& nick)
{
	const PlayerMap::iterator it = m_PlayerMap.find(nick);

	if (it == m_PlayerMap.end())
		return std::wstring();

	return wstring_from_utf8(it->second.m_Rating.to_string());
}

/*****************************************************
 * Utilities                                         *
 *****************************************************/

/**
 * Parse and return the timestamp of a historic chat message and return the current time for new chat messages.
 * Historic chat messages are implement as DelayedDelivers as specified in XEP-0203.
 * Hence, their timestamp MUST be in UTC and conform to the DateTime format XEP-0082.
 *
 * @returns Seconds since the epoch.
 */
std::time_t XmppClient::ComputeTimestamp(const glooxwrapper::Message& msg)
{
	// Only historic messages contain a timestamp!
	if (!msg.when())
		return std::time(nullptr);

	// The locale is irrelevant, because the XMPP date format doesn't contain written month names
	for (const std::string& format : std::vector<std::string>{ "Y-M-d'T'H:m:sZ", "Y-M-d'T'H:m:s.SZ" })
	{
		UDate dateTime = g_L10n.ParseDateTime(msg.when()->stamp().to_string(), format, icu::Locale::getUS());
		if (dateTime)
			return dateTime / 1000.0;
	}

	return std::time(nullptr);
}

/**
 * Convert a gloox presence type to an untranslated string literal to be used as an identifier by the scripts.
 */
const char* XmppClient::GetPresenceString(const gloox::Presence::PresenceType presenceType)
{
	switch (presenceType)
	{
#define CASE(X,Y) case gloox::Presence::X: return Y
	CASE(Available, "available");
	CASE(Chat, "chat");
	CASE(Away, "away");
	CASE(DND, "playing");
	CASE(XA, "away");
	CASE(Unavailable, "offline");
	CASE(Probe, "probe");
	CASE(Error, "error");
	CASE(Invalid, "invalid");
	default:
		LOGERROR("Unknown presence type '%d'", static_cast<int>(presenceType));
		return "";
#undef CASE
	}
}

/**
 * Convert a gloox role type to an untranslated string literal to be used as an identifier by the scripts.
 */
const char* XmppClient::GetRoleString(const gloox::MUCRoomRole role)
{
	switch (role)
	{
#define CASE(X, Y) case gloox::X: return Y
	CASE(RoleNone, "none");
	CASE(RoleVisitor, "visitor");
	CASE(RoleParticipant, "participant");
	CASE(RoleModerator, "moderator");
	CASE(RoleInvalid, "invalid");
	default:
		LOGERROR("Unknown role type '%d'", static_cast<int>(role));
		return "";
#undef CASE
	}
}

/**
 * Translates a gloox certificate error codes, i.e. gloox certificate statuses except CertOk.
 * Keep in sync with specifications.
 */
std::string XmppClient::CertificateErrorToString(gloox::CertStatus status)
{
	std::map<gloox::CertStatus, std::string> certificateErrorStrings = {
		{ gloox::CertInvalid, g_L10n.Translate("The certificate is not trusted.") },
		{ gloox::CertSignerUnknown, g_L10n.Translate("The certificate hasn't got a known issuer.") },
		{ gloox::CertRevoked, g_L10n.Translate("The certificate has been revoked.") },
		{ gloox::CertExpired, g_L10n.Translate("The certificate has expired.") },
		{ gloox::CertNotActive, g_L10n.Translate("The certificate is not yet active.") },
		{ gloox::CertWrongPeer, g_L10n.Translate("The certificate has not been issued for the peer connected to.") },
		{ gloox::CertSignerNotCa, g_L10n.Translate("The certificate signer is not a certificate authority.") }
	};

	std::string result;

	for (std::map<gloox::CertStatus, std::string>::iterator it = certificateErrorStrings.begin(); it != certificateErrorStrings.end(); ++it)
		if (status & it->first)
			result += "\n" + it->second;

	return result;
}

/**
 * Convert a gloox stanza error type to string.
 * Keep in sync with Gloox documentation
 *
 * @param err Error to be converted
 * @return Converted error string
 */
std::string XmppClient::StanzaErrorToString(gloox::StanzaError err)
{
#define CASE(X, Y) case gloox::X: return Y
#define DEBUG_CASE(X, Y) case gloox::X: return g_L10n.Translate("Error") + " (" + Y + ")"
	switch (err)
	{
	CASE(StanzaErrorUndefined, g_L10n.Translate("No error"));
	DEBUG_CASE(StanzaErrorBadRequest, "Server received malformed XML");
	CASE(StanzaErrorConflict, g_L10n.Translate("Player already logged in"));
	DEBUG_CASE(StanzaErrorFeatureNotImplemented, "Server does not implement requested feature");
	CASE(StanzaErrorForbidden, g_L10n.Translate("Forbidden"));
	DEBUG_CASE(StanzaErrorGone, "Unable to find message receipiant");
	CASE(StanzaErrorInternalServerError, g_L10n.Translate("Internal server error"));
	DEBUG_CASE(StanzaErrorItemNotFound, "Message receipiant does not exist");
	DEBUG_CASE(StanzaErrorJidMalformed, "JID (XMPP address) malformed");
	DEBUG_CASE(StanzaErrorNotAcceptable, "Receipiant refused message. Possible policy issue");
	CASE(StanzaErrorNotAllowed, g_L10n.Translate("Not allowed"));
	CASE(StanzaErrorNotAuthorized, g_L10n.Translate("Not authorized"));
	DEBUG_CASE(StanzaErrorNotModified, "Requested item has not changed since last request");
	DEBUG_CASE(StanzaErrorPaymentRequired, "This server requires payment");
	CASE(StanzaErrorRecipientUnavailable, g_L10n.Translate("Recipient temporarily unavailable"));
	DEBUG_CASE(StanzaErrorRedirect, "Request redirected");
	CASE(StanzaErrorRegistrationRequired, g_L10n.Translate("Registration required"));
	DEBUG_CASE(StanzaErrorRemoteServerNotFound, "Remote server not found");
	DEBUG_CASE(StanzaErrorRemoteServerTimeout, "Remote server timed out");
	DEBUG_CASE(StanzaErrorResourceConstraint, "The recipient is unable to process the message due to resource constraints");
	CASE(StanzaErrorServiceUnavailable, g_L10n.Translate("Service unavailable"));
	DEBUG_CASE(StanzaErrorSubscribtionRequired, "Service requires subscription");
	DEBUG_CASE(StanzaErrorUnexpectedRequest, "Attempt to send from invalid stanza address");
	DEBUG_CASE(StanzaErrorUnknownSender, "Invalid 'from' address");
	default:
		return g_L10n.Translate("Unknown error");
	}
#undef DEBUG_CASE
#undef CASE
}

/**
 * Convert a gloox connection error enum to string
 * Keep in sync with Gloox documentation
 *
 * @param err Error to be converted
 * @return Converted error string
 */
std::string XmppClient::ConnectionErrorToString(gloox::ConnectionError err)
{
#define CASE(X, Y) case gloox::X: return Y
#define DEBUG_CASE(X, Y) case gloox::X: return g_L10n.Translate("Error") + " (" + Y + ")"
	switch (err)
	{
	CASE(ConnNoError, g_L10n.Translate("No error"));
	CASE(ConnStreamError, g_L10n.Translate("Stream error"));
	CASE(ConnStreamVersionError, g_L10n.Translate("The incoming stream version is unsupported"));
	CASE(ConnStreamClosed, g_L10n.Translate("The stream has been closed by the server"));
	DEBUG_CASE(ConnProxyAuthRequired, "The HTTP/SOCKS5 proxy requires authentication");
	DEBUG_CASE(ConnProxyAuthFailed, "HTTP/SOCKS5 proxy authentication failed");
	DEBUG_CASE(ConnProxyNoSupportedAuth, "The HTTP/SOCKS5 proxy requires an unsupported authentication mechanism");
	CASE(ConnIoError, g_L10n.Translate("An I/O error occurred"));
	DEBUG_CASE(ConnParseError, "An XML parse error occurred");
	CASE(ConnConnectionRefused, g_L10n.Translate("The connection was refused by the server"));
	CASE(ConnDnsError, g_L10n.Translate("Resolving the server's hostname failed"));
	CASE(ConnOutOfMemory, g_L10n.Translate("This system is out of memory"));
	DEBUG_CASE(ConnNoSupportedAuth, "The authentication mechanisms the server offered are not supported or no authentication mechanisms were available");
	CASE(ConnTlsFailed, g_L10n.Translate("The server's certificate could not be verified or the TLS handshake did not complete successfully"));
	CASE(ConnTlsNotAvailable, g_L10n.Translate("The server did not offer required TLS encryption"));
	DEBUG_CASE(ConnCompressionFailed, "Negotiation/initializing compression failed");
	CASE(ConnAuthenticationFailed, g_L10n.Translate("Authentication failed. Incorrect password or account does not exist"));
	CASE(ConnUserDisconnected, g_L10n.Translate("The user or system requested a disconnect"));
	CASE(ConnNotConnected, g_L10n.Translate("There is no active connection"));
	default:
		return g_L10n.Translate("Unknown error");
	}
#undef DEBUG_CASE
#undef CASE
}

/**
 * Convert a gloox registration result enum to string
 * Keep in sync with Gloox documentation
 *
 * @param err Enum to be converted
 * @return Converted string
 */
std::string XmppClient::RegistrationResultToString(gloox::RegistrationResult res)
{
#define CASE(X, Y) case gloox::X: return Y
#define DEBUG_CASE(X, Y) case gloox::X: return g_L10n.Translate("Error") + " (" + Y + ")"
	switch (res)
	{
	CASE(RegistrationSuccess, g_L10n.Translate("Your account has been successfully registered"));
	CASE(RegistrationNotAcceptable, g_L10n.Translate("Not all necessary information provided"));
	CASE(RegistrationConflict, g_L10n.Translate("Username already exists"));
	DEBUG_CASE(RegistrationNotAuthorized, "Account removal timeout or insufficiently secure channel for password change");
	DEBUG_CASE(RegistrationBadRequest, "Server received an incomplete request");
	DEBUG_CASE(RegistrationForbidden, "Registration forbidden");
	DEBUG_CASE(RegistrationRequired, "Account cannot be removed as it does not exist");
	DEBUG_CASE(RegistrationUnexpectedRequest, "This client is unregistered with the server");
	DEBUG_CASE(RegistrationNotAllowed, "Server does not permit password changes");
	default:
		return "";
	}
#undef DEBUG_CASE
#undef CASE
}

void XmppClient::SendStunEndpointToHost(const std::string& ip, u16 port, const std::string& hostJIDStr)
{
	DbgXMPP("SendStunEndpointToHost " << hostJIDStr);

	glooxwrapper::JID hostJID(hostJIDStr);
	glooxwrapper::Jingle::Session session = m_sessionManager->createSession(hostJID);
	session.sessionInitiate(ip.c_str(), port);
}

void XmppClient::handleSessionAction(gloox::Jingle::Action action, glooxwrapper::Jingle::Session& session, const glooxwrapper::Jingle::Session::Jingle& jingle)
{
	if (action == gloox::Jingle::SessionInitiate)
		handleSessionInitiation(session, jingle);
}

void XmppClient::handleSessionInitiation(glooxwrapper::Jingle::Session& UNUSED(session), const glooxwrapper::Jingle::Session::Jingle& jingle)
{
	glooxwrapper::Jingle::ICEUDP::Candidate candidate = jingle.getCandidate();

	if (candidate.ip.empty())
	{
		LOGERROR("Failed to retrieve Jingle candidate");
		return;
	}

	if (!g_NetServer)
	{
		LOGERROR("Received STUN connection request, but not hosting currently!");
		return;
	}

	g_NetServer->SendHolePunchingMessage(candidate.ip.to_string(), candidate.port);
}
