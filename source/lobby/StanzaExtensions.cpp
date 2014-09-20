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
#include "precompiled.h"
#include "StanzaExtensions.h"

/******************************************************
 * GameReport, fairly generic custom stanza extension used
 * to report game statistics.
 */
GameReport::GameReport( const glooxwrapper::Tag* tag ):StanzaExtension( ExtGameReport )
{
	if( !tag || tag->name() != "report" || tag->xmlns() != XMLNS_GAMEREPORT )
		return;
	// TODO if we want to handle receiving this stanza extension.
};

/**
 * Required by gloox, used to serialize the GameReport into XML for sending.
 */
glooxwrapper::Tag* GameReport::tag() const
{
	glooxwrapper::Tag* t = glooxwrapper::Tag::allocate( "report" );
	t->setXmlns( XMLNS_GAMEREPORT );

	std::vector<const glooxwrapper::Tag*>::const_iterator it = m_GameReport.begin();
	for( ; it != m_GameReport.end(); ++it )
		t->addChild( (*it)->clone() );

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
BoardListQuery::BoardListQuery( const glooxwrapper::Tag* tag ):StanzaExtension( ExtBoardListQuery )
{
	if( !tag || tag->name() != "query" || tag->xmlns() != XMLNS_BOARDLIST )
		return;
	
	const glooxwrapper::Tag* c = tag->findTag_clone( "query/command" );
	if (c)
		m_Command = c->cdata();
	glooxwrapper::Tag::free(c);
	const glooxwrapper::ConstTagList boardTags = tag->findTagList_clone( "query/board" );
	glooxwrapper::ConstTagList::const_iterator it = boardTags.begin();
	for ( ; it != boardTags.end(); ++it )
	{
		m_StanzaBoardList.push_back( *it );
	}
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
	glooxwrapper::Tag* t = glooxwrapper::Tag::allocate( "query" );
	t->setXmlns( XMLNS_BOARDLIST );
	
	// Check for ratinglist or boardlist command
	if(!m_Command.empty())
		t->addChild(glooxwrapper::Tag::allocate("command", m_Command));

	std::vector<const glooxwrapper::Tag*>::const_iterator it = m_StanzaBoardList.begin();
	for( ; it != m_StanzaBoardList.end(); ++it )
		t->addChild( (*it)->clone() );

	return t;
}

glooxwrapper::StanzaExtension* BoardListQuery::clone() const
{
	BoardListQuery* q = new BoardListQuery();
	return q;
}

BoardListQuery::~BoardListQuery()
{
	std::vector<const glooxwrapper::Tag*>::const_iterator it = m_StanzaBoardList.begin();
	for( ; it != m_StanzaBoardList.end(); ++it )
		glooxwrapper::Tag::free(*it);
	m_StanzaBoardList.clear();
}

/******************************************************
 * GameListQuery, custom IQ Stanza, used to receive
 * the listing of games from the server, and register/
 * unregister/changestate games on the server.
 */
GameListQuery::GameListQuery( const glooxwrapper::Tag* tag ):StanzaExtension( ExtGameListQuery )
{
	if( !tag || tag->name() != "query" || tag->xmlns() != XMLNS_GAMELIST )
		return;

	const glooxwrapper::Tag* c = tag->findTag_clone( "query/game" );
	if (c)
		m_Command = c->cdata();
	glooxwrapper::Tag::free(c);

	const glooxwrapper::ConstTagList games = tag->findTagList_clone( "query/game" );
	glooxwrapper::ConstTagList::const_iterator it = games.begin();
	for ( ; it != games.end(); ++it )
		m_GameList.push_back( *it );
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
	glooxwrapper::Tag* t = glooxwrapper::Tag::allocate( "query" );
	t->setXmlns( XMLNS_GAMELIST );

	// Check for register / unregister command
	if(!m_Command.empty())
		t->addChild(glooxwrapper::Tag::allocate("command", m_Command));

	std::vector<const glooxwrapper::Tag*>::const_iterator it = m_GameList.begin();
	for( ; it != m_GameList.end(); ++it )
		t->addChild( (*it)->clone() );

	return t;
}

glooxwrapper::StanzaExtension* GameListQuery::clone() const
{
	GameListQuery* q = new GameListQuery();
	return q;
}

GameListQuery::~GameListQuery()
{
	std::vector<const glooxwrapper::Tag*>::const_iterator it = m_GameList.begin();
	for( ; it != m_GameList.end(); ++it )
		glooxwrapper::Tag::free(*it);
	m_GameList.clear();
}

/******************************************************
 * ProfileQuery, a custom IQ Stanza useful for fetching
 * user profiles
 * Example stanza:
 * <profile player="foobar" highestRating="1500" rank="1895" totalGamesPlayed="50"
 * 	wins="25" losses="25" /><command>foobar</command>
 */
ProfileQuery::ProfileQuery(const glooxwrapper::Tag* tag):StanzaExtension(ExtProfileQuery)
{
	if (!tag || tag->name() != "query" || tag->xmlns() != XMLNS_PROFILE)
		return;

	const glooxwrapper::Tag* c = tag->findTag_clone("query/command");
	if (c)
		m_Command = c->cdata();
	glooxwrapper::Tag::free(c);

	const glooxwrapper::ConstTagList profileTags = tag->findTagList_clone("query/profile");
	glooxwrapper::ConstTagList::const_iterator it = profileTags.begin();
	for (; it != profileTags.end(); ++it)
		m_StanzaProfile.push_back(*it);
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

	std::vector<const glooxwrapper::Tag*>::const_iterator it = m_StanzaProfile.begin();
	for (; it != m_StanzaProfile.end(); ++it)
		t->addChild((*it)->clone());

	return t;
}

glooxwrapper::StanzaExtension* ProfileQuery::clone() const
{
	ProfileQuery* q = new ProfileQuery();
	return q;
}

ProfileQuery::~ProfileQuery()
{
	std::vector<const glooxwrapper::Tag*>::const_iterator it = m_StanzaProfile.begin();
	for (; it != m_StanzaProfile.end(); ++it)
		glooxwrapper::Tag::free(*it);
	m_StanzaProfile.clear();
}
