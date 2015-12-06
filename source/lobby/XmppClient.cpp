/* Copyright (C) 2015 Wildfire Games.
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

#include "glooxwrapper/glooxwrapper.h"
#include "i18n/L10n.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Pyrogenesis.h"
#include "scriptinterface/ScriptInterface.h"

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
	: m_client(NULL), m_mucRoom(NULL), m_registration(NULL), m_username(sUsername), m_password(sPassword), m_nick(sNick), m_initialLoadComplete(false)
{
	// Read lobby configuration from default.cfg
	std::string sServer;
	std::string sXpartamupp;
	CFG_GET_VAL("lobby.server", sServer);
	CFG_GET_VAL("lobby.xpartamupp", sXpartamupp);

	m_xpartamuppId = sXpartamupp + "@" + sServer + "/CC";
	glooxwrapper::JID clientJid(sUsername + "@" + sServer + "/0ad");
	glooxwrapper::JID roomJid(sRoom + "@conference." + sServer + "/" + sNick);

	// If we are connecting, use the full jid and a password
	// If we are registering, only use the server name
	if (!regOpt)
		m_client = new glooxwrapper::Client(clientJid, sPassword);
	else
		m_client = new glooxwrapper::Client(sServer);

	// Disable TLS as we haven't set a certificate on the server yet
	m_client->setTls(gloox::TLSDisabled);

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
		CreateSimpleMessage("system", "connected");
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

	if (error == gloox::ConnAuthenticationFailed)
		CreateSimpleMessage("system", g_L10n.Translate("Authentication failed"), "error");
	else
		CreateSimpleMessage("system", "disconnected");
}

/**
 * Handle TLS connection
 */
bool XmppClient::onTLSConnect(const glooxwrapper::CertInfo& info)
{
	UNUSED2(info);
	DbgXMPP("onTLSConnect");
	DbgXMPP(
		"status: " << info.status <<
		"\nissuer: " << info.issuer <<
		"\npeer: " << info.server <<
		"\nprotocol: " << info.protocol <<
		"\nmac: " << info.mac <<
		"\ncipher: " << info.cipher <<
		"\ncompression: " << info.compression );
	return true;
}

/**
 * Handle MUC room errors
 */
void XmppClient::handleMUCError(glooxwrapper::MUCRoom*, gloox::StanzaError err)
{
	std::string msg = StanzaErrorToString(err);
	CreateSimpleMessage("system", msg, "error");
}

/*****************************************************
 * Requests to server                                *
 *****************************************************/

/**
 * Request a listing of active games from the server.
 */
void XmppClient::SendIqGetGameList()
{
	glooxwrapper::JID xpartamuppJid(m_xpartamuppId);

	// Send IQ
	glooxwrapper::IQ iq(gloox::IQ::Get, xpartamuppJid);
	iq.addExtension(new GameListQuery());
	DbgXMPP("SendIqGetGameList [" << tag_xml(iq) << "]");
	m_client->send(iq);
}

/**
 * Request the leaderboard data from the server.
 */
void XmppClient::SendIqGetBoardList()
{
	glooxwrapper::JID xpartamuppJid(m_xpartamuppId);

	// Send IQ
	BoardListQuery* b = new BoardListQuery();
	b->m_Command = "getleaderboard";
	glooxwrapper::IQ iq(gloox::IQ::Get, xpartamuppJid);
	iq.addExtension(b);
	DbgXMPP("SendIqGetBoardList [" << tag_xml(iq) << "]");
	m_client->send(iq);
}

/**
 * Request the profile data from the server.
 */
void XmppClient::SendIqGetProfile(const std::string& player)
{
	glooxwrapper::JID xpartamuppJid(m_xpartamuppId);

	// Send IQ
	ProfileQuery* b = new ProfileQuery();
	b->m_Command = player;
	glooxwrapper::IQ iq(gloox::IQ::Get, xpartamuppJid);
	iq.addExtension(b);
	DbgXMPP("SendIqGetProfile [" << tag_xml(iq) << "]");
	m_client->send(iq);
}

/**
 * Request the rating data from the server.
 */
void XmppClient::SendIqGetRatingList()
{
	glooxwrapper::JID xpartamuppJid(m_xpartamuppId);

	// Send IQ
	BoardListQuery* b = new BoardListQuery();
	b->m_Command = "getratinglist";
	glooxwrapper::IQ iq(gloox::IQ::Get, xpartamuppJid);
	iq.addExtension(b);
	DbgXMPP("SendIqGetRatingList [" << tag_xml(iq) << "]");
	m_client->send(iq);
}

/**
 * Send game report containing numerous game properties to the server.
 *
 * @param data A JS array of game statistics
 */
void XmppClient::SendIqGameReport(ScriptInterface& scriptInterface, JS::HandleValue data)
{
	glooxwrapper::JID xpartamuppJid(m_xpartamuppId);

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
	glooxwrapper::IQ iq(gloox::IQ::Set, xpartamuppJid);
	iq.addExtension(game);
	DbgXMPP("SendGameReport [" << tag_xml(iq) << "]");
	m_client->send(iq);
};

/**
 * Send a request to register a game to the server.
 *
 * @param data A JS array of game attributes
 */
void XmppClient::SendIqRegisterGame(ScriptInterface& scriptInterface, JS::HandleValue data)
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
	glooxwrapper::IQ iq(gloox::IQ::Set, xpartamuppJid);
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

	glooxwrapper::IQ iq( gloox::IQ::Set, xpartamuppJid );
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

	glooxwrapper::IQ iq(gloox::IQ::Set, xpartamuppJid);
	iq.addExtension(g);
	DbgXMPP("SendIqChangeStateGame [" << tag_xml(iq) << "]");
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
	{
		CreateSimpleMessage("system", "registered");
	}
	else
	{
		std::string msg;
#define CASE(X, Y) case gloox::X: msg = Y; break
		switch(result)
		{
		CASE(RegistrationNotAcceptable, g_L10n.Translate("Registration not acceptable"));
		CASE(RegistrationConflict, g_L10n.Translate("Registration conflict"));
		CASE(RegistrationNotAuthorized, g_L10n.Translate("Registration not authorized"));
		CASE(RegistrationBadRequest, g_L10n.Translate("Registration bad request"));
		CASE(RegistrationForbidden, g_L10n.Translate("Registration forbidden"));
		CASE(RegistrationRequired, g_L10n.Translate("Registration required"));
		CASE(RegistrationUnexpectedRequest, g_L10n.Translate("Registration unexpected request"));
		CASE(RegistrationNotAllowed, g_L10n.Translate("Registration not allowed"));
		default: msg = g_L10n.Translate("Registration unknown error");
		}
#undef CASE
		CreateSimpleMessage("system", msg, "error");
	}
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
void XmppClient::GUIGetPlayerList(ScriptInterface& scriptInterface, JS::MutableHandleValue ret)
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
void XmppClient::GUIGetGameList(ScriptInterface& scriptInterface, JS::MutableHandleValue ret)
{
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	scriptInterface.Eval("([])", ret);
	const char* stats[] = { "name", "ip", "state", "nbp", "tnbp", "players", "mapName", "niceMapName", "mapSize", "mapType", "victoryCondition" };
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
void XmppClient::GUIGetBoardList(ScriptInterface& scriptInterface, JS::MutableHandleValue ret)
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
void XmppClient::GUIGetProfile(ScriptInterface& scriptInterface, JS::MutableHandleValue ret)
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

/**
 * Send GUI message queue when queried.
 */
void XmppClient::GuiPollMessage(ScriptInterface& scriptInterface, JS::MutableHandleValue ret)
{
	if (m_GuiMessageQueue.empty())
	{
		ret.setUndefined();
		return;
	}

	GUIMessage message = m_GuiMessageQueue.front();
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	scriptInterface.Eval("({})", ret);
	scriptInterface.SetProperty(ret, "type", message.type);
	if (!message.from.empty())
		scriptInterface.SetProperty(ret, "from", message.from);
	if (!message.text.empty())
		scriptInterface.SetProperty(ret, "text", message.text);
	if (!message.level.empty())
		scriptInterface.SetProperty(ret, "level", message.level);
	if (!message.message.empty())
		scriptInterface.SetProperty(ret, "message", message.message);
	if (!message.data.empty())
		scriptInterface.SetProperty(ret, "data", message.data);
	if (!message.datetime.empty())
		scriptInterface.SetProperty(ret, "datetime", message.datetime);

	m_GuiMessageQueue.pop_front();
}

/**
 * Send a standard MUC textual message.
 */
void XmppClient::SendMUCMessage(const std::string& message)
{
	m_mucRoom->send(message);
}

/**
 * Push a message onto the GUI queue.
 *
 * @param message Message to add to the queue
 */
void XmppClient::PushGuiMessage(XmppClient::GUIMessage message)
{
	m_GuiMessageQueue.push_back(std::move(message));
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
				return message.type == L"muc" && message.level == L"presence";
			}
	), m_GuiMessageQueue.end());
}

/**
 * Used in order to update the GUI only once when multiple updates are queued.
 */
int XmppClient::GetMucMessageCount()
{
	return std::count_if(m_GuiMessageQueue.begin(), m_GuiMessageQueue.end(),
		[](XmppClient::GUIMessage& message)
		{
			return message.type == L"muc";
		});
}

/**
 * Handle a standard MUC textual message.
 */
void XmppClient::handleMUCMessage(glooxwrapper::MUCRoom*, const glooxwrapper::Message& msg, bool)
{
	DbgXMPP(msg.from().resource() << " said " << msg.body());

	GUIMessage message;
	message.type = L"mucmessage";
	message.from = wstring_from_utf8(msg.from().resource().to_string());
	message.text = wstring_from_utf8(msg.body().to_string());
	if (msg.when())
		// See http://xmpp.org/extensions/xep-0082.html#sect-idp285136 for format
		message.datetime = msg.when()->stamp().to_string();
	PushGuiMessage(message);
}

/**
 * Handle a standard textual message.
 */
void XmppClient::handleMessage(const glooxwrapper::Message& msg, glooxwrapper::MessageSession *)
{
	DbgXMPP("type " << msg.subtype() << ", subject " << msg.subject()
	  << ", message " << msg.body() << ", thread id " << msg.thread());

	GUIMessage message;
	message.from = wstring_from_utf8(msg.from().username().to_string());
	message.message = wstring_from_utf8(msg.body().to_string());
	if (msg.when())
		//See http://xmpp.org/extensions/xep-0082.html#sect-idp285136 for format
		message.datetime = msg.when()->stamp().to_string();
	PushGuiMessage(message);
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

			CreateSimpleMessage("system", "gamelist updated", "internal");
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

				CreateSimpleMessage("system", "boardlist updated", "internal");
			}
			else if (bq->m_Command == "ratinglist")
			{
				for (const glooxwrapper::Tag* const& t : bq->m_StanzaBoardList)
				{
					std::string name = t->findAttribute("name").to_string();
					if (m_PlayerMap.find(name) != m_PlayerMap.end())
						m_PlayerMap[name][1] = t->findAttribute("rating").to_string();
				}

				CreateSimpleMessage("system", "ratinglist updated", "internal");
			}
		}
		if (pq)
		{
			for (const glooxwrapper::Tag* const& t : m_Profile)
				glooxwrapper::Tag::free(t);
			m_Profile.clear();

			for (const glooxwrapper::Tag* const& t : pq->m_StanzaProfile)
				m_Profile.emplace_back(t->clone());

			CreateSimpleMessage("system", "profile updated", "internal");
		}
	}
	else if (iq.subtype() == gloox::IQ::Error)
	{
		gloox::StanzaError err = iq.error_error();
		std::string msg = StanzaErrorToString(err);
		CreateSimpleMessage("system", msg, "error");
	}
	else
	{
		CreateSimpleMessage("system", g_L10n.Translate("unknown subtype (see logs)"), "error");
		std::string tag = tag_name(iq);
		LOGMESSAGE("unknown subtype '%s'", tag.c_str());
	}

	return true;
}

/**
 * Create a new detail message for the GUI.
 *
 * @param type General message type
 * @param level Detailed message type
 * @param text Body of the message
 * @param data Optional field, used for auxiliary data
 */
void XmppClient::CreateSimpleMessage(const std::string& type, const std::string& text, const std::string& level, const std::string& data)
{
	GUIMessage message;
	message.type = wstring_from_utf8(type);
	message.level = wstring_from_utf8(level);
	message.text = wstring_from_utf8(text);
	message.data = wstring_from_utf8(data);
	PushGuiMessage(message);
}

/*****************************************************
 * Presence, nickname, and subject                   *
 *****************************************************/

/**
 * Update local data when a user changes presence.
 */
void XmppClient::handleMUCParticipantPresence(glooxwrapper::MUCRoom*, const glooxwrapper::MUCRoomParticipant participant, const glooxwrapper::Presence& presence)
{
	//std::string jid = participant.jid->full();
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
			CreateSimpleMessage("muc", nick, "nick", participant.newNick.to_string());
		}
		else
			CreateSimpleMessage("muc", nick, "leave");

		DbgXMPP(nick << " left the room");
		m_PlayerMap.erase(nick);
	}
	else
	{
		/* During the initialization process, we recieve join messages for everyone
		 * currently in the room. We don't want to display these, so we filter them
		 * out. We will always be the last to join during initialization.
		 */
		if (!m_initialLoadComplete)
		{
			if (m_mucRoom->nick().to_string() == nick)
				m_initialLoadComplete = true;
		}
		else if (m_PlayerMap.find(nick) == m_PlayerMap.end())
			CreateSimpleMessage("muc", nick, "join");
		else
			CreateSimpleMessage("muc", nick, "presence");

		DbgXMPP(nick << " is in the room, presence : " << (int)presenceType);
		m_PlayerMap[nick].resize(3);
		m_PlayerMap[nick][0] = presenceString;
		m_PlayerMap[nick][2] = roleString;
	}
}

/**
 * Update local cache when subject changes.
 */
void XmppClient::handleMUCSubject(glooxwrapper::MUCRoom*, const glooxwrapper::string& UNUSED(nick), const glooxwrapper::string& subject)
{
	m_Subject = subject.c_str();
	CreateSimpleMessage("muc", m_Subject, "subject");
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
#define CASE(x,y) case gloox::x: role = y; break
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
 * Convert a gloox stanza error type to string.
 *
 * @param err Error to be converted
 * @return Converted error string
 */
std::string XmppClient::StanzaErrorToString(gloox::StanzaError err)
{
	std::string msg;
#define CASE(X, Y) case gloox::X: return Y
	switch (err)
	{
	CASE(StanzaErrorBadRequest, g_L10n.Translate("Bad request"));
	CASE(StanzaErrorConflict, g_L10n.Translate("Player name already in use"));
	CASE(StanzaErrorFeatureNotImplemented, g_L10n.Translate("Feature not implemented"));
	CASE(StanzaErrorForbidden, g_L10n.Translate("Forbidden"));
	CASE(StanzaErrorGone, g_L10n.Translate("Recipient or server gone"));
	CASE(StanzaErrorInternalServerError, g_L10n.Translate("Internal server error"));
	CASE(StanzaErrorItemNotFound, g_L10n.Translate("Item not found"));
	CASE(StanzaErrorJidMalformed, g_L10n.Translate("Jid malformed"));
	CASE(StanzaErrorNotAcceptable, g_L10n.Translate("Not acceptable"));
	CASE(StanzaErrorNotAllowed, g_L10n.Translate("Not allowed"));
	CASE(StanzaErrorNotAuthorized, g_L10n.Translate("Not authorized"));
	CASE(StanzaErrorNotModified, g_L10n.Translate("Not modified"));
	CASE(StanzaErrorPaymentRequired, g_L10n.Translate("Payment required"));
	CASE(StanzaErrorRecipientUnavailable, g_L10n.Translate("Recipient unavailable"));
	CASE(StanzaErrorRedirect, g_L10n.Translate("Redirect"));
	CASE(StanzaErrorRegistrationRequired, g_L10n.Translate("Registration required"));
	CASE(StanzaErrorRemoteServerNotFound, g_L10n.Translate("Remote server not found"));
	CASE(StanzaErrorRemoteServerTimeout, g_L10n.Translate("Remote server timeout"));
	CASE(StanzaErrorResourceConstraint, g_L10n.Translate("Resource constraint"));
	CASE(StanzaErrorServiceUnavailable, g_L10n.Translate("Service unavailable"));
	CASE(StanzaErrorSubscribtionRequired, g_L10n.Translate("Subscription Required"));
	CASE(StanzaErrorUndefinedCondition, g_L10n.Translate("Undefined condition"));
	CASE(StanzaErrorUnexpectedRequest, g_L10n.Translate("Unexpected request"));
	CASE(StanzaErrorUnknownSender, g_L10n.Translate("Unknown sender"));
	default:
		return g_L10n.Translate("Error undefined");
	}
#undef CASE
}
