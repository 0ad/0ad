/* Copyright (C) 2018 Wildfire Games.
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

#ifdef WIN32
#  include <winsock2.h>
#endif

#include "i18n/L10n.h"
#include "lib/external_libraries/enet.h"
#include "lib/utf8.h"
#include "network/NetServer.h"
#include "network/StunClient.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Pyrogenesis.h"
#include "scriptinterface/ScriptInterface.h"

#include <gloox/gloox.h>

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

IXmppClient* IXmppClient::create(const std::string& sUsername, const std::string& sPassword, const std::string& sRoom, const std::string& sNick, const int historyRequestSize,bool regOpt)
{
	return new XmppClient(sUsername, sPassword, sRoom, sNick, historyRequestSize, regOpt);
}

/**
 * Construct the XMPP client.
 *
 * @param sUsername Username to login with of register.
 * @param sPassword Password to login with or register.
 * @param sRoom MUC room to join.
 * @param sNick Nick to join with.
 * @param historyRequestSize Number of stanzas of room history to request.
 * @param regOpt If we are just registering or not.
 */
XmppClient::XmppClient(const std::string& sUsername, const std::string& sPassword, const std::string& sRoom, const std::string& sNick, const int historyRequestSize, bool regOpt)
	: m_client(NULL), m_mucRoom(NULL), m_registration(NULL), m_username(sUsername), m_password(sPassword), m_room(sRoom), m_nick(sNick), m_initialLoadComplete(false), m_isConnected(false), m_sessionManager()
{
	// Read lobby configuration from default.cfg
	std::string sXpartamupp;
	std::string sEchelon;
	CFG_GET_VAL("lobby.server", m_server);
	CFG_GET_VAL("lobby.xpartamupp", sXpartamupp);
	CFG_GET_VAL("lobby.echelon", sEchelon);

	m_xpartamuppId = sXpartamupp + "@" + m_server + "/CC";
	m_echelonId = sEchelon + "@" + m_server + "/CC";
	glooxwrapper::JID clientJid(sUsername + "@" + m_server + "/0ad");
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

	// Workaround for memory leak in gloox 1.0/1.0.1
	m_client->removePresenceExtension(gloox::ExtCaps);

	delete m_client;

	for (const glooxwrapper::Tag* const& t : m_GameList)
		glooxwrapper::Tag::free(t);
	for (const glooxwrapper::Tag* const& t : m_BoardList)
		glooxwrapper::Tag::free(t);
	for (const glooxwrapper::Tag* const& t : m_Profile)
		glooxwrapper::Tag::free(t);
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
		CreateGUIMessage("system", "connected");
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
	m_Profile.clear();
	m_HistoricGuiMessages.clear();

	m_isConnected = false;
	CreateGUIMessage("system", "disconnected", "reason", ConnectionErrorToString(error));
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
void XmppClient::handleMUCError(glooxwrapper::MUCRoom*, gloox::StanzaError err)
{
	CreateGUIMessage("system", "error", "text", StanzaErrorToString(err));
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
 * Send game report containing numerous game properties to the server.
 *
 * @param data A JS array of game statistics
 */
void XmppClient::SendIqGameReport(const ScriptInterface& scriptInterface, JS::HandleValue data)
{
	glooxwrapper::JID echelonJid(m_echelonId);

	// Setup some base stanza attributes
	GameReport* game = new GameReport();
	glooxwrapper::Tag* report = glooxwrapper::Tag::allocate("game");

	// Iterate through all the properties reported and add them to the stanza.
	std::vector<std::string> properties;
	scriptInterface.EnumeratePropertyNamesWithPrefix(data, "", properties);
	for (const std::string& p : properties)
	{
		std::wstring value;
		scriptInterface.GetProperty(data, p.c_str(), value);
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
void XmppClient::SendIqRegisterGame(const ScriptInterface& scriptInterface, JS::HandleValue data)
{
	glooxwrapper::JID xpartamuppJid(m_xpartamuppId);

	// Setup some base stanza attributes
	GameListQuery* g = new GameListQuery();
	g->m_Command = "register";
	glooxwrapper::Tag* game = glooxwrapper::Tag::allocate("game");
	// Add a fake ip which will be overwritten by the ip stamp XMPP module on the server.
	game->addAttribute("ip", "fake");

	// Iterate through all the properties reported and add them to the stanza.
	std::vector<std::string> properties;
	scriptInterface.EnumeratePropertyNamesWithPrefix(data, "", properties);
	for (const std::string& p : properties)
	{
		std::wstring value;
		scriptInterface.GetProperty(data, p.c_str(), value);
		game->addAttribute(p, utf8_from_wstring(value));
	}

	// Push the stanza onto the IQ
	g->m_GameList.emplace_back(game);

	// Send IQ
	glooxwrapper::IQ iq(gloox::IQ::Set, xpartamuppJid, m_client->getID());
	iq.addExtension(g);
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

	glooxwrapper::JID clientJid(to + "@" + m_server + "/0ad");
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
		CreateGUIMessage("system", "registered");
	else
		CreateGUIMessage("system", "error", "text", RegistrationResultToString(result));

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
void XmppClient::GUIGetPlayerList(const ScriptInterface& scriptInterface, JS::MutableHandleValue ret)
{
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	scriptInterface.Eval("([])", ret);

	// Convert the internal data structure to a Javascript object.
	for (const std::pair<std::string, std::vector<std::string> >& p : m_PlayerMap)
	{
		JS::RootedValue player(cx);
		scriptInterface.Eval("({})", &player);
		scriptInterface.SetProperty(player, "name", wstring_from_utf8(p.first));
		scriptInterface.SetProperty(player, "presence", wstring_from_utf8(p.second[0]));
		scriptInterface.SetProperty(player, "rating", wstring_from_utf8(p.second[1]));
		scriptInterface.SetProperty(player, "role", wstring_from_utf8(p.second[2]));
		scriptInterface.CallFunctionVoid(ret, "push", player);
	}
}

/**
 * Handle requests from the GUI for the list of all active games.
 *
 * @return A JS array containing all known games
 */
void XmppClient::GUIGetGameList(const ScriptInterface& scriptInterface, JS::MutableHandleValue ret)
{
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	scriptInterface.Eval("([])", ret);
	const char* stats[] = { "name", "ip", "port", "stunIP", "stunPort", "hostUsername", "state",
		"nbp", "maxnbp", "players", "mapName", "niceMapName", "mapSize", "mapType",
		"victoryCondition", "startTime", "mods" };
	for(const glooxwrapper::Tag* const& t : m_GameList)
	{
		JS::RootedValue game(cx);
		scriptInterface.Eval("({})", &game);

		for (size_t i = 0; i < ARRAY_SIZE(stats); ++i)
			scriptInterface.SetProperty(game, stats[i], wstring_from_utf8(t->findAttribute(stats[i]).to_string()));

		scriptInterface.CallFunctionVoid(ret, "push", game);
	}
}

/**
 * Handle requests from the GUI for leaderboard data.
 *
 * @return A JS array containing all known leaderboard data
 */
void XmppClient::GUIGetBoardList(const ScriptInterface& scriptInterface, JS::MutableHandleValue ret)
{
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	scriptInterface.Eval("([])", ret);
	const char* attributes[] = { "name", "rank", "rating" };
	for(const glooxwrapper::Tag* const& t : m_BoardList)
	{
		JS::RootedValue board(cx);
		scriptInterface.Eval("({})", &board);

		for (size_t i = 0; i < ARRAY_SIZE(attributes); ++i)
			scriptInterface.SetProperty(board, attributes[i], wstring_from_utf8(t->findAttribute(attributes[i]).to_string()));

		scriptInterface.CallFunctionVoid(ret, "push", board);
	}
}

/**
 * Handle requests from the GUI for profile data.
 *
 * @return A JS array containing the specific user's profile data
 */
void XmppClient::GUIGetProfile(const ScriptInterface& scriptInterface, JS::MutableHandleValue ret)
{
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	scriptInterface.Eval("([])", ret);
	const char* stats[] = { "player", "rating", "totalGamesPlayed", "highestRating", "wins", "losses", "rank" };
	for (const glooxwrapper::Tag* const& t : m_Profile)
	{
		JS::RootedValue profile(cx);
		scriptInterface.Eval("({})", &profile);

		for (size_t i = 0; i < ARRAY_SIZE(stats); ++i)
			scriptInterface.SetProperty(profile, stats[i], wstring_from_utf8(t->findAttribute(stats[i]).to_string()));

		scriptInterface.CallFunctionVoid(ret, "push", profile);
	}
}

/*****************************************************
 * Message interfaces                                *
 *****************************************************/

void XmppClient::CreateGUIMessage(
	const std::string& type,
	const std::string& level,
	const std::string& property1_name,
	const std::string& property1_value,
	const std::string& property2_name,
	const std::string& property2_value,
	const std::time_t time)
{
	GUIMessage message;
	message.type = type;
	message.level = level;
	message.property1_name = property1_name;
	message.property1_value = property1_value;
	message.property2_name = property2_name;
	message.property2_value = property2_value;
	message.time = time;
	m_GuiMessageQueue.push_back(std::move(message));
}

JS::Value XmppClient::GuiMessageToJSVal(const ScriptInterface& scriptInterface, const GUIMessage& message, const bool historic)
{
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue ret(cx);
	scriptInterface.Eval("({})", &ret);
	scriptInterface.SetProperty(ret, "type", wstring_from_utf8(message.type));
	if (!message.level.empty())
		scriptInterface.SetProperty(ret, "level", wstring_from_utf8(message.level));
	if (!message.property1_name.empty())
		scriptInterface.SetProperty(ret, message.property1_name.c_str(), wstring_from_utf8(message.property1_value));
	if (!message.property2_name.empty())
		scriptInterface.SetProperty(ret, message.property2_name.c_str(), wstring_from_utf8(message.property2_value));
	scriptInterface.SetProperty(ret, "time", (double)message.time);
	scriptInterface.SetProperty(ret, "historic", historic);
	return ret;
}

JS::Value XmppClient::GuiPollNewMessage(const ScriptInterface& scriptInterface)
{
	if (m_GuiMessageQueue.empty())
		return JS::UndefinedValue();

	GUIMessage message = m_GuiMessageQueue.front();
	m_GuiMessageQueue.pop_front();

	// Since there can be hundreds of presence changes while playing a game, ignore these for performance
	if (message.type == "chat" && message.level != "presence")
		m_HistoricGuiMessages.push_back(message);

	return GuiMessageToJSVal(scriptInterface, message, false);
}

JS::Value XmppClient::GuiPollHistoricMessages(const ScriptInterface& scriptInterface)
{
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);
	JS::RootedObject ret(cx, JS_NewArrayObject(cx, 0));

	uint32_t i = 0;
	for (const GUIMessage& message : m_HistoricGuiMessages)
	{
		JS::RootedValue msg(cx, GuiMessageToJSVal(scriptInterface, message, true));
		JS_SetElement(cx, ret, i++, msg);
	}
	return JS::ObjectValue(*ret);
}

/**
 * Send a standard MUC textual message.
 */
void XmppClient::SendMUCMessage(const std::string& message)
{
	m_mucRoom->send(message);
}

/**
 * Clears all presence updates from the message queue.
 * Used when rejoining the lobby, since we don't need to handle past presence changes.
 */
void XmppClient::ClearPresenceUpdates()
{
	m_GuiMessageQueue.erase(
		std::remove_if(m_GuiMessageQueue.begin(), m_GuiMessageQueue.end(),
			[](XmppClient::GUIMessage& message)
			{
				return message.type == "chat" && message.level == "presence";
			}
	), m_GuiMessageQueue.end());
}

/**
 * Handle a room message.
 */
void XmppClient::handleMUCMessage(glooxwrapper::MUCRoom*, const glooxwrapper::Message& msg, bool priv)
{
	DbgXMPP(msg.from().resource() << " said " << msg.body());

	CreateGUIMessage(
		"chat",
		priv ? "private-message" : "room-message",
		"from", msg.from().resource().to_string(),
		"text", msg.body().to_string(),
		ComputeTimestamp(msg));
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
		"from", msg.from().resource().to_string(),
		"text", msg.body().to_string(),
		ComputeTimestamp(msg));
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
		if (gq)
		{
			for (const glooxwrapper::Tag* const& t : m_GameList)
				glooxwrapper::Tag::free(t);
			m_GameList.clear();

			for (const glooxwrapper::Tag* const& t : gq->m_GameList)
				m_GameList.emplace_back(t->clone());

			CreateGUIMessage("game", "gamelist");
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

				CreateGUIMessage("game", "leaderboard");
			}
			else if (bq->m_Command == "ratinglist")
			{
				for (const glooxwrapper::Tag* const& t : bq->m_StanzaBoardList)
				{
					std::string name = t->findAttribute("name").to_string();
					if (m_PlayerMap.find(name) != m_PlayerMap.end())
						m_PlayerMap[name][1] = t->findAttribute("rating").to_string();
				}

				CreateGUIMessage("game", "ratinglist");
			}
		}
		if (pq)
		{
			for (const glooxwrapper::Tag* const& t : m_Profile)
				glooxwrapper::Tag::free(t);
			m_Profile.clear();

			for (const glooxwrapper::Tag* const& t : pq->m_StanzaProfile)
				m_Profile.emplace_back(t->clone());

			CreateGUIMessage("game", "profile");
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
				LOGERROR("Received lobby authentication request, but not hosting currently!");
		}
	}
	else if (iq.subtype() == gloox::IQ::Error)
		CreateGUIMessage("system", "error", "text", StanzaErrorToString(iq.error_error()));
	else
	{
		CreateGUIMessage("system", "error", "text", g_L10n.Translate("unknown subtype (see logs)"));
		LOGMESSAGE("unknown subtype '%s'", tag_name(iq).c_str());
	}
	return true;
}

/*****************************************************
 * Presence, nickname, and subject                   *
 *****************************************************/

/**
 * Update local data when a user changes presence.
 */
void XmppClient::handleMUCParticipantPresence(glooxwrapper::MUCRoom*, const glooxwrapper::MUCRoomParticipant participant, const glooxwrapper::Presence& presence)
{
	std::string nick = participant.nick->resource().to_string();
	gloox::Presence::PresenceType presenceType = presence.presence();
	std::string presenceString, roleString;
	GetPresenceString(presenceType, presenceString);
	GetRoleString(participant.role, roleString);

	if (presenceType == gloox::Presence::Unavailable)
	{
		if (!participant.newNick.empty() && (participant.flags & (gloox::UserNickChanged | gloox::UserSelf)))
		{
			// we have a nick change
			std::string newNick = participant.newNick.to_string();
			m_PlayerMap[newNick].resize(3);
			m_PlayerMap[newNick][0] = presenceString;
			m_PlayerMap[newNick][2] = roleString;

			DbgXMPP(nick << " is now known as " << participant.newNick.to_string());
			CreateGUIMessage("chat", "nick", "oldnick", nick, "newnick", participant.newNick.to_string());
		}
		else if (participant.flags & gloox::UserKicked)
		{
			DbgXMPP(nick << " was kicked. Reason: " << participant.reason.to_string());
			CreateGUIMessage("chat", "kicked", "nick", nick, "reason", participant.reason.to_string());
		}
		else if (participant.flags & gloox::UserBanned)
		{
			DbgXMPP(nick << " was banned. Reason: " << participant.reason.to_string());
			CreateGUIMessage("chat", "banned", "nick", nick, "reason", participant.reason.to_string());
		}
		else
		{
			DbgXMPP(nick << " left the room (flags " << participant.flags << ")");
			CreateGUIMessage("chat", "leave", "nick", nick);
		}
		m_PlayerMap.erase(nick);
	}
	else
	{
		/* During the initialization process, we receive join messages for everyone
		 * currently in the room. We don't want to display these, so we filter them
		 * out. We will always be the last to join during initialization.
		 */
		if (!m_initialLoadComplete)
		{
			if (m_mucRoom->nick().to_string() == nick)
				m_initialLoadComplete = true;
		}
		else if (m_PlayerMap.find(nick) == m_PlayerMap.end())
			CreateGUIMessage("chat", "join", "nick", nick);
		else if (m_PlayerMap[nick][2] != roleString)
			CreateGUIMessage("chat", "role", "nick", nick, "oldrole", m_PlayerMap[nick][2]);
		else
			CreateGUIMessage("chat", "presence", "nick", nick);

		DbgXMPP(nick << " is in the room, presence : " << (int)presenceType);
		m_PlayerMap[nick].resize(3);
		m_PlayerMap[nick][0] = presenceString;
		m_PlayerMap[nick][2] = roleString;
	}
}

/**
 * Update local cache when subject changes.
 */
void XmppClient::handleMUCSubject(glooxwrapper::MUCRoom*, const glooxwrapper::string& nick, const glooxwrapper::string& subject)
{
	m_Subject = subject.c_str();
	CreateGUIMessage("chat", "subject", "nick", nick.c_str(), "subject", m_Subject);
}

/**
 * Get current subject.
 *
 * @param topic Variable to store subject in.
 */
void XmppClient::GetSubject(std::string& subject)
{
	subject = m_Subject;
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
 *
 * @param nick Variable to store the nickname in.
 */
void XmppClient::GetNick(std::string& nick)
{
	nick = m_mucRoom->nick().to_string();
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
 *
 * @param nick Nickname to look up presence for
 * @param presence Variable to store the presence in
 */
void XmppClient::GetPresence(const std::string& nick, std::string& presence)
{
	if (m_PlayerMap.find(nick) != m_PlayerMap.end())
		presence = m_PlayerMap[nick][0];
	else
		presence = "offline";
}

/**
 * Get the current xmpp role of the given nick.
 *
 * @param nick Nickname to look up presence for
 * @param role Variable to store the role in
 */
void XmppClient::GetRole(const std::string& nick, std::string& role)
{
	if (m_PlayerMap.find(nick) != m_PlayerMap.end())
		role = m_PlayerMap[nick][2];
	else
		role = "";
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
std::time_t XmppClient::ComputeTimestamp(const glooxwrapper::Message& msg) const
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
 * Convert a gloox presence type to string.
 *
 * @param p Presence to be converted
 * @param presence Variable to store the converted presence string in
 */
void XmppClient::GetPresenceString(const gloox::Presence::PresenceType p, std::string& presence) const
{
	switch(p)
	{
#define CASE(x,y) case gloox::Presence::x: presence = y; break
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
		LOGERROR("Unknown presence type '%d'", (int)p);
		break;
#undef CASE
	}
}

/**
 * Convert a gloox role type to string.
 *
 * @param p Role to be converted
 * @param presence Variable to store the converted role string in
 */
void XmppClient::GetRoleString(const gloox::MUCRoomRole r, std::string& role) const
{
	switch(r)
	{
#define CASE(X, Y) case gloox::X: role = Y; break
	CASE(RoleNone, "none");
	CASE(RoleVisitor, "visitor");
	CASE(RoleParticipant, "participant");
	CASE(RoleModerator, "moderator");
	CASE(RoleInvalid, "invalid");
	default:
		LOGERROR("Unknown role type '%d'", (int)r);
		break;
#undef CASE
	}
}

/**
 * Translates a gloox certificate error codes, i.e. gloox certificate statuses except CertOk.
 * Keep in sync with specifications.
 */
std::string XmppClient::TLSErrorToString(gloox::CertStatus status) const
{
	// TODO: Use translation
	std::map<gloox::CertStatus, std::string> certificateErrorStrings = {
		{ gloox::CertInvalid, ("The certificate is not trusted.") },
		{ gloox::CertSignerUnknown, ("The certificate hasn't got a known issuer.") },
		{ gloox::CertRevoked, ("The certificate has been revoked.") },
		{ gloox::CertExpired, ("The certificate has expired.") },
		{ gloox::CertNotActive, ("The certifiacte is not yet active.") },
		{ gloox::CertWrongPeer, ("The certificate has not been issued for the peer we're connected to.") },
		{ gloox::CertSignerNotCa, ("The signer is not a CA.") }
	};

	std::string result = "";

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
std::string XmppClient::StanzaErrorToString(gloox::StanzaError err) const
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
std::string XmppClient::ConnectionErrorToString(gloox::ConnectionError err) const
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
	CASE(ConnTlsFailed, g_L10n.Translate("The server's certificate could not be verified or the TLS handshake did not complete successfully") + TLSErrorToString(m_certStatus));
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
std::string XmppClient::RegistrationResultToString(gloox::RegistrationResult res) const
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

void XmppClient::SendStunEndpointToHost(StunClient::StunEndpoint* stunEndpoint, const std::string& hostJIDStr)
{
	ENSURE(stunEndpoint);

	char ipStr[256] = "(error)";
	ENetAddress addr;
	addr.host = ntohl(stunEndpoint->ip);
	enet_address_get_host_ip(&addr, ipStr, ARRAY_SIZE(ipStr));

	glooxwrapper::JID hostJID(hostJIDStr);
	glooxwrapper::Jingle::Session session = m_sessionManager->createSession(hostJID);
	session.sessionInitiate(ipStr, stunEndpoint->port);
}

void XmppClient::handleSessionAction(gloox::Jingle::Action action, glooxwrapper::Jingle::Session* UNUSED(session), const glooxwrapper::Jingle::Session::Jingle* jingle)
{
	if (action == gloox::Jingle::SessionInitiate)
		handleSessionInitiation(jingle);
}

void XmppClient::handleSessionInitiation(const glooxwrapper::Jingle::Session::Jingle* jingle)
{
	glooxwrapper::Jingle::ICEUDP::Candidate candidate = jingle->getCandidate();

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
