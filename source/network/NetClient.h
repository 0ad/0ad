/* Copyright (C) 2009 Wildfire Games.
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

/**
 *-----------------------------------------------------------------------------
 *	FILE			: NetClient.h
 *	PROJECT			: 0 A.D.
 *	DESCRIPTION		: Network client class interface file
 *-----------------------------------------------------------------------------
 */

#ifndef NETCLIENT_H
#define NETCLIENT_H

// INCLUDES
#include "NetSession.h"
#include "ps/CStr.h"
#include "simulation/TurnManager.h"
#include "simulation/ScriptObject.h"
#include "scripting/ScriptableObject.h"
#include "ps/scripting/JSMap.h"

#include <map>

// DECLARATIONS
enum
{
	NCS_CONNECT				= 200,
	NCS_HANDSHAKE			= 300,
	NCS_AUTHENTICATE		= 400,
	NCS_PREGAME				= 500,
	NCS_INGAME				= 600
};

class CPlayerSlot;
class CPlayer;
class CGame;
class CGameAttributes;
class CServerPlayer;

//typedef std::map< uint, CStr > PlayerMap;
typedef std::map< uint, CServerPlayer* > PlayerMap;

/*
	CLASS			: CServerPlayer
	DESCRIPTION		:
	NOTES			:
*/
class CServerPlayer : public CJSObject< CServerPlayer >
{
public:

	CServerPlayer( uint sessionID, const CStr& nickname );
	~CServerPlayer( void );

	static void ScriptingInit( void );
	uint GetSessionID( void ) const { return m_SessionID; }
	const CStr GetNickname( void ) const { return m_Nickname; }
protected:

private:

	CServerPlayer( const CServerPlayer& );
	CServerPlayer& operator=( const CServerPlayer& );

	uint	m_SessionID;				// Player session ID
	CStr	m_Nickname;					// Player nickname
};

/*
	CLASS			: CNetClient
	DESCRIPTION		:
	NOTES			:
*/

class CNetClient: public CNetHost,
				  public CJSObject<CNetClient>,
				  protected CTurnManager
{
public:

	CNetClient( CGame* pGame, CGameAttributes* pGameAttributes );
	~CNetClient( void );

	bool CreateSession	( void );
	void OnPlayer		( uint ID, const CStr& name );
	void OnPlayerLeave	( uint ID );

	/**
	 * Returns true indicating the host acts as a client
	 *
	 * @return					Always true
	 */
	virtual bool IsClient( void ) const { return true; }

	// Get a pointer to our player
	CPlayer* GetLocalPlayer();

	CJSMap< PlayerMap > m_JsPlayers;

	CStr	m_Nickname;
	CStr	m_Password;
	//int		m_SessionID;

	CPlayerSlot *m_pLocalPlayerSlot;
	CGame *m_pGame;
	CGameAttributes *m_pGameAttributes;

	// Are we currently in a locally-yet-unsimulated turn?
	// This is set to true when we receive a command batch and cleared in NewTurn().
	// The server also ensures that it does not send a new turn until we ack one.
	bool m_TurnPending;

	// Mutex for accessing batches
	CMutex m_Mutex;

	// JS event scripts
	CScriptObject m_OnStartGame;
	CScriptObject m_OnChat;
	CScriptObject m_OnConnectComplete;
	CScriptObject m_OnDisconnect;
	CScriptObject m_OnPlayerJoin;
	CScriptObject m_OnPlayerLeave;

	static void ScriptingInit( void );
	int StartGame( void );

protected:

	virtual bool SetupSession			( CNetSession* pSession );
	virtual bool HandleConnect			( CNetSession* pSession );
	virtual bool HandleDisconnect		( CNetSession *pSession );

	virtual void NewTurn				( void );
	virtual bool NewTurnReady			( void ) { return m_TurnPending; }
	virtual void QueueLocalCommand		( CNetMessage* pMessage );
			void QueueIncomingMessage	( CNetMessage* pMessage );

private:

	// Not implemented
	CNetClient( const CNetClient& );
	CNetClient& operator=( const CNetClient& );

	static bool OnError			( void* pContext, CFsmEvent* pEvent );
	static bool OnPlayerJoin	( void* pContext, CFsmEvent* pEvent );
	static bool OnHandshake		( void* pContext, CFsmEvent* pEvent );
	static bool OnAuthenticate	( void* pContext, CFsmEvent* pEvent );
	static bool OnPreGame		( void* pContext, CFsmEvent* pEvent );
	static bool OnInGame		( void* pContext, CFsmEvent* pEvent );
	static bool OnChat			( void* pContext, CFsmEvent* pEvent );
	static bool	OnStartGame		( void* pContext, CFsmEvent* pEvent );

	
	bool SetupConnection( JSContext *cx, uintN argc, jsval *argv );

	//CNetSession*	m_Session;		// Server session
	PlayerMap		m_Players;		// List of online players
};

extern CNetClient *g_NetClient;

#endif	// NETCLIENT_H

