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
#ifndef STANZAEXTENSIONS_H
#define STANZAEXTENSIONS_H

#include "glooxwrapper/glooxwrapper.h"

/// Global Gamelist Extension
#define EXTGAMELISTQUERY 1403
#define XMLNS_GAMELIST "jabber:iq:gamelist"

/// Global Boardlist Extension
#define EXTBOARDLISTQUERY 1404
#define XMLNS_BOARDLIST "jabber:iq:boardlist"

/// Global Gamereport Extension
#define EXTGAMEREPORT 1405
#define XMLNS_GAMEREPORT "jabber:iq:gamereport"

/// Global Profile Extension
#define EXTPROFILEQUERY 1406
#define XMLNS_PROFILE "jabber:iq:profile"

/// Global Lobby Authentication Extension
#define EXTLOBBYAUTH 1407
#define XMLNS_LOBBYAUTH "jabber:iq:lobbyauth"

class GameReport : public glooxwrapper::StanzaExtension
{
public:
	GameReport(const glooxwrapper::Tag* tag = 0);

	// Following four methods are all required by gloox
	virtual StanzaExtension* newInstance(const glooxwrapper::Tag* tag) const
	{
		return new GameReport(tag);
	}
	virtual const glooxwrapper::string& filterString() const;
	virtual glooxwrapper::Tag* tag() const;
	virtual glooxwrapper::StanzaExtension* clone() const;

	std::vector<const glooxwrapper::Tag*> m_GameReport;
};

class GameListQuery : public glooxwrapper::StanzaExtension
{
public:
	GameListQuery(const glooxwrapper::Tag* tag = 0);

	// Following four methods are all required by gloox
	virtual StanzaExtension* newInstance(const glooxwrapper::Tag* tag) const
	{
		return new GameListQuery(tag);
	}
	virtual const glooxwrapper::string& filterString() const;
	virtual glooxwrapper::Tag* tag() const;
	virtual glooxwrapper::StanzaExtension* clone() const;

	~GameListQuery();

	glooxwrapper::string m_Command;
	std::vector<const glooxwrapper::Tag*> m_GameList;
};

class BoardListQuery : public glooxwrapper::StanzaExtension
{
public:
	BoardListQuery(const glooxwrapper::Tag* tag = 0);

	// Following four methods are all required by gloox
	virtual StanzaExtension* newInstance(const glooxwrapper::Tag* tag) const
	{
		return new BoardListQuery(tag);
	}
	virtual const glooxwrapper::string& filterString() const;
	virtual glooxwrapper::Tag* tag() const;
	virtual glooxwrapper::StanzaExtension* clone() const;

	~BoardListQuery();

	glooxwrapper::string m_Command;
	std::vector<const glooxwrapper::Tag*> m_StanzaBoardList;
};

class ProfileQuery : public glooxwrapper::StanzaExtension
{
public:
	ProfileQuery(const glooxwrapper::Tag* tag = 0);

	// Following four methods are all required by gloox
	virtual StanzaExtension* newInstance(const glooxwrapper::Tag* tag) const
	{
		return new ProfileQuery(tag);
	}
	virtual const glooxwrapper::string& filterString() const;
	virtual glooxwrapper::Tag* tag() const;
	virtual glooxwrapper::StanzaExtension* clone() const;

	~ProfileQuery();

	glooxwrapper::string m_Command;
	std::vector<const glooxwrapper::Tag*> m_StanzaProfile;
};

class LobbyAuth : public glooxwrapper::StanzaExtension
{
public:
	LobbyAuth(const glooxwrapper::Tag* tag = 0);

	// Following four methods are all required by gloox
	virtual StanzaExtension* newInstance(const glooxwrapper::Tag* tag) const
	{
		return new LobbyAuth(tag);
	}
	virtual const glooxwrapper::string& filterString() const;
	virtual glooxwrapper::Tag* tag() const;
	virtual glooxwrapper::StanzaExtension* clone() const;

	glooxwrapper::string m_Token;
};
#endif // STANZAEXTENSIONS_H
