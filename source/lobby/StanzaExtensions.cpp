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
#include "StanzaExtensions.h"

/******************************************************
 * GameReport, fairly generic custom stanza extension used
 * to report game statistics.
 */
GameReport::GameReport(const glooxwrapper::Tag* tag)
	: StanzaExtension(EXTGAMEREPORT)
{
	if (!tag || tag->name() != "report" || tag->xmlns() != XMLNS_GAMEREPORT)
		return;
	// TODO if we want to handle receiving this stanza extension.
};

/**
 * Required by gloox, used to serialize the GameReport into XML for sending.
 */
glooxwrapper::Tag* GameReport::tag() const
{
	glooxwrapper::Tag* t = glooxwrapper::Tag::allocate("report");
	t->setXmlns(XMLNS_GAMEREPORT);

	for (const glooxwrapper::Tag* const& tag : m_GameReport)
		t->addChild(tag->clone());

	return t;
}

/**
 * Required by gloox, used to find the GameReport element in a recived IQ.
 */
const glooxwrapper::string& GameReport::filterString() const
{
	static const glooxwrapper::string filter = "/iq/report[@xmlns='" XMLNS_GAMEREPORT "']";
	return filter;
}

glooxwrapper::StanzaExtension* GameReport::clone() const
{
	GameReport* q = new GameReport();
	return q;
}

/******************************************************
 * BoardListQuery, a flexible custom IQ Stanza useful for anything with ratings, used to
 * request and receive leaderboard and rating data from server.
 * Example stanza:
 * <board player="foobar">1200</board>
 */
BoardListQuery::BoardListQuery(const glooxwrapper::Tag* tag)
	: StanzaExtension(EXTBOARDLISTQUERY)
{
	if (!tag || tag->name() != "query" || tag->xmlns() != XMLNS_BOARDLIST)
		return;

	const glooxwrapper::Tag* c = tag->findTag_clone("query/command");
	if (c)
		m_Command = c->cdata();
	glooxwrapper::Tag::free(c);
	for (const glooxwrapper::Tag* const& t : tag->findTagList_clone("query/board"))
		m_StanzaBoardList.emplace_back(t);
}

/**
 * Required by gloox, used to find the BoardList element in a received IQ.
 */
const glooxwrapper::string& BoardListQuery::filterString() const
{
	static const glooxwrapper::string filter = "/iq/query[@xmlns='" XMLNS_BOARDLIST "']";
	return filter;
}

/**
 * Required by gloox, used to serialize the BoardList request into XML for sending.
 */
glooxwrapper::Tag* BoardListQuery::tag() const
{
	glooxwrapper::Tag* t = glooxwrapper::Tag::allocate("query");
	t->setXmlns(XMLNS_BOARDLIST);

	// Check for ratinglist or boardlist command
	if (!m_Command.empty())
		t->addChild(glooxwrapper::Tag::allocate("command", m_Command));

	for (const glooxwrapper::Tag* const& tag : m_StanzaBoardList)
		t->addChild(tag->clone());

	return t;
}

glooxwrapper::StanzaExtension* BoardListQuery::clone() const
{
	BoardListQuery* q = new BoardListQuery();
	return q;
}

BoardListQuery::~BoardListQuery()
{
	for (const glooxwrapper::Tag* const& t : m_StanzaBoardList)
		glooxwrapper::Tag::free(t);
	m_StanzaBoardList.clear();
}

/******************************************************
 * GameListQuery, custom IQ Stanza, used to receive
 * the listing of games from the server, and register/
 * unregister/changestate games on the server.
 */
GameListQuery::GameListQuery(const glooxwrapper::Tag* tag)
	: StanzaExtension(EXTGAMELISTQUERY)
{
	if (!tag || tag->name() != "query" || tag->xmlns() != XMLNS_GAMELIST)
		return;

	const glooxwrapper::Tag* c = tag->findTag_clone("query/command");
	if (c)
		m_Command = c->cdata();
	glooxwrapper::Tag::free(c);

	for (const glooxwrapper::Tag* const& t : tag->findTagList_clone("query/game"))
		m_GameList.emplace_back(t);
}

/**
 * Required by gloox, used to find the GameList element in a received IQ.
 */
const glooxwrapper::string& GameListQuery::filterString() const
{
	static const glooxwrapper::string filter = "/iq/query[@xmlns='" XMLNS_GAMELIST "']";
	return filter;
}

/**
 * Required by gloox, used to serialize the game object into XML for sending.
 */
glooxwrapper::Tag* GameListQuery::tag() const
{
	glooxwrapper::Tag* t = glooxwrapper::Tag::allocate("query");
	t->setXmlns(XMLNS_GAMELIST);

	// Check for register / unregister command
	if (!m_Command.empty())
		t->addChild(glooxwrapper::Tag::allocate("command", m_Command));

	for (const glooxwrapper::Tag* const& tag : m_GameList)
		t->addChild(tag->clone());

	return t;
}

glooxwrapper::StanzaExtension* GameListQuery::clone() const
{
	GameListQuery* q = new GameListQuery();
	return q;
}

GameListQuery::~GameListQuery()
{
	for (const glooxwrapper::Tag* const & t : m_GameList)
		glooxwrapper::Tag::free(t);
	m_GameList.clear();
}

/******************************************************
 * ProfileQuery, a custom IQ Stanza useful for fetching
 * user profiles
 * Example stanza:
 * <profile player="foobar" highestRating="1500" rank="1895" totalGamesPlayed="50"
 * 	wins="25" losses="25" /><command>foobar</command>
 */
ProfileQuery::ProfileQuery(const glooxwrapper::Tag* tag)
	: StanzaExtension(EXTPROFILEQUERY)
{
	if (!tag || tag->name() != "query" || tag->xmlns() != XMLNS_PROFILE)
		return;

	const glooxwrapper::Tag* c = tag->findTag_clone("query/command");
	if (c)
		m_Command = c->cdata();
	glooxwrapper::Tag::free(c);

	for (const glooxwrapper::Tag* const& t : tag->findTagList_clone("query/profile"))
		m_StanzaProfile.emplace_back(t);
}

/**
 * Required by gloox, used to find the Profile element in a received IQ.
 */
const glooxwrapper::string& ProfileQuery::filterString() const
{
	static const glooxwrapper::string filter = "/iq/query[@xmlns='" XMLNS_PROFILE "']";
	return filter;
}

/**
 * Required by gloox, used to serialize the Profile request into XML for sending.
 */
glooxwrapper::Tag* ProfileQuery::tag() const
{
	glooxwrapper::Tag* t = glooxwrapper::Tag::allocate("query");
	t->setXmlns(XMLNS_PROFILE);

	if (!m_Command.empty())
		t->addChild(glooxwrapper::Tag::allocate("command", m_Command));

	for (const glooxwrapper::Tag* const& tag : m_StanzaProfile)
		t->addChild(tag->clone());

	return t;
}

glooxwrapper::StanzaExtension* ProfileQuery::clone() const
{
	ProfileQuery* q = new ProfileQuery();
	return q;
}

ProfileQuery::~ProfileQuery()
{
	for (const glooxwrapper::Tag* const& t : m_StanzaProfile)
		glooxwrapper::Tag::free(t);
	m_StanzaProfile.clear();
}

/******************************************************
 * LobbyAuth, a custom IQ Stanza, used to send and
 * receive a security token for hosting authentication.
 */
LobbyAuth::LobbyAuth(const glooxwrapper::Tag* tag)
	: StanzaExtension(EXTLOBBYAUTH)
{
	if (!tag || tag->name() != "auth" || tag->xmlns() != XMLNS_LOBBYAUTH)
		return;

	const glooxwrapper::Tag* c = tag->findTag_clone("auth/token");
	if (c)
		m_Token = c->cdata();

	glooxwrapper::Tag::free(c);
}

/**
 * Required by gloox, used to find the LobbyAuth element in a received IQ.
 */
const glooxwrapper::string& LobbyAuth::filterString() const
{
	static const glooxwrapper::string filter = "/iq/auth[@xmlns='" XMLNS_LOBBYAUTH "']";
	return filter;
}

/**
 * Required by gloox, used to serialize the auth object into XML for sending.
 */
glooxwrapper::Tag* LobbyAuth::tag() const
{
	glooxwrapper::Tag* t = glooxwrapper::Tag::allocate("auth");
	t->setXmlns(XMLNS_LOBBYAUTH);

	// Check for the auth token
	if (!m_Token.empty())
		t->addChild(glooxwrapper::Tag::allocate("token", m_Token));
	return t;
}

glooxwrapper::StanzaExtension* LobbyAuth::clone() const
{
	return new LobbyAuth();
}

/******************************************************
 * ConnectionData, a custom IQ Stanza, used to send and
 * receive a ip and port of the server.
 */
ConnectionData::ConnectionData(const glooxwrapper::Tag* tag)
	: StanzaExtension(EXTCONNECTIONDATA)
{
	if (!tag || tag->name() != "connectiondata" || tag->xmlns() != XMLNS_CONNECTIONDATA)
		return;

	const glooxwrapper::Tag* c = tag->findTag_clone("connectiondata/ip");
	if (c)
		m_Ip = c->cdata();
	const glooxwrapper::Tag* p= tag->findTag_clone("connectiondata/port");
	if (p)
		m_Port = p->cdata();
	const glooxwrapper::Tag* pip = tag->findTag_clone("connectiondata/isLocalIP");
	if (pip)
		m_IsLocalIP = pip->cdata();
	const glooxwrapper::Tag* s = tag->findTag_clone("connectiondata/useSTUN");
	if (s)
		m_UseSTUN = s->cdata();
	const glooxwrapper::Tag* pw = tag->findTag_clone("connectiondata/password");
	if (pw)
		m_Password = pw->cdata();
	const glooxwrapper::Tag* e = tag->findTag_clone("connectiondata/error");
	if (e)
		m_Error= e->cdata();

	glooxwrapper::Tag::free(c);
	glooxwrapper::Tag::free(p);
	glooxwrapper::Tag::free(pip);
	glooxwrapper::Tag::free(s);
	glooxwrapper::Tag::free(pw);
	glooxwrapper::Tag::free(e);
}

/**
 * Required by gloox, used to find the LobbyAuth element in a received IQ.
 */
const glooxwrapper::string& ConnectionData::filterString() const
{
	static const glooxwrapper::string filter = "/iq/connectiondata[@xmlns='" XMLNS_CONNECTIONDATA "']";
	return filter;
}

/**
 * Required by gloox, used to serialize the auth object into XML for sending.
 */
glooxwrapper::Tag* ConnectionData::tag() const
{
	glooxwrapper::Tag* t = glooxwrapper::Tag::allocate("connectiondata");
	t->setXmlns(XMLNS_CONNECTIONDATA);

	if (!m_Ip.empty())
		t->addChild(glooxwrapper::Tag::allocate("ip", m_Ip));
	if (!m_Port.empty())
		t->addChild(glooxwrapper::Tag::allocate("port", m_Port));
	if (!m_IsLocalIP.empty())
		t->addChild(glooxwrapper::Tag::allocate("isLocalIP", m_IsLocalIP));
	if (!m_UseSTUN.empty())
		t->addChild(glooxwrapper::Tag::allocate("useSTUN", m_UseSTUN));
	if (!m_Password.empty())
		t->addChild(glooxwrapper::Tag::allocate("password", m_Password));
	if (!m_Error.empty())
		t->addChild(glooxwrapper::Tag::allocate("error", m_Error));
	return t;
}

glooxwrapper::StanzaExtension* ConnectionData::clone() const
{
	return new ConnectionData();
}
