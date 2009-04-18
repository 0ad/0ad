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
 *	FILE			: NetServer.h
 *	PROJECT			: 0 A.D.
 *	DESCRIPTION		: Network server class interface file
 *-----------------------------------------------------------------------------
 */

#ifndef NETSERVER_H
#define NETSERVER_H

// INCLUDES
#include "Network.h"
#include "NetSession.h"
#include "simulation/TurnManager.h"
#include "scripting/ScriptableObject.h"
#include "ps/GameAttributes.h"
#include "ps/scripting/JSMap.h"
#include "ps/Player.h"
#include "ps/Game.h"
#include "simulation/ScriptObject.h"

#include <map>
#include <vector>

// DECLARATIONS
#define SERVER_SESSIONID			1
#define CLIENT_MIN_SESSIONID		100
#define MAX_CLIENTS					8
#define MAX_OBSERVERS				5
#define DEFAULT_SERVER_SESSION_ID	1
#define	DEFAULT_SERVER_NAME			L"Noname Server"
#define DEFAULT_PLAYER_NAME			L"Noname Player"
#define DEFAULT_WELCOME_MESSAGE		L"Noname Server Welcome Message"
#define DEFAULT_HOST_PORT			0x5073

enum NetServerState
{
	// We haven't opened the port yet, we're just setting some stuff up.
	// This is probably equivalent to the first "Start Network Game" screen
	SERVER_STATE_PREBIND,

	// The server is open and accepting connections. This is the screen where
	// rules are set up by the operator and where players join and select civs
	// and stuff.
	SERVER_STATE_PREGAME,

	// The one with all the killing ;-)
	SERVER_STATE_INGAME,

	// The game is over and someone has won. Players might linger to chat or
	// download the replay log.
	SERVER_STATE_POSTGAME
};

enum
{
	NSS_HANDSHAKE		= 1300,
	NSS_AUTHENTICATE	= 1400,
	NSS_PREGAME			= 1500,
	NSS_INGAME			= 1600
};

enum
{
	NMT_APP_PLAYER_LEAVE	= NMT_LAST + 100,
	NMT_APP_PREGAME			= NMT_LAST + 200,
	NMT_APP_OBSERVER		= NMT_LAST + 300
};

typedef std::map< uint, CNetSession* >		IDSessionMap;
typedef std::vector< CNetSession* >			SessionList;

/*
	CLASS			: CNetServer
	DESCRIPTION		: CNetServer implements a network server for the game.
					  It receives data and connection requests from clients.
					  Under the hood, it uses ENet library to manage connected
					  peers and bandwidth among these.
	NOTES			:
*/

class CNetServer : 	public CNetHost,
					public CJSObject<CNetServer>,
					public CTurnManager
{
public:

	CNetServer( CGame* pGame, CGameAttributes* pGameAttributes );
	virtual ~CNetServer( void );

	bool Start		( JSContext *pContext, uintN argc, jsval *argv );
//	void Shutdown	( void );

	/**
	 * Returns true indicating the host acts as a server
	 *
	 * @return					Always true
	 */
	virtual bool IsServer( void ) const { return true; }

	/**
	 * Adds a new session to the list of sessions
	 *
	 * @param pSession			New session to add
	 */
	void AddSession( CNetSession* pSession );

	/**
	 * Removes the specified session from the list of sessions. If the session
	 * isn't found it returns NULL otherwise it returns the session object found.
	 *
	 * @param pSession			Session to remove
	 * @return					The session object if found, NULL otherwise
	 */
	CNetSession* RemoveSession( CNetSession* pSession );

	/**
	 * Removes all the sessions managed by the network server
	 *
	 */
	//void RemoveAllSessions( void );

	/**
	 * Returns the number of session the server manages
	 *
	 * @return					The number of sessions
	 */
	//uint GetSessionCount( void ) const;

	/**
	 * Returns the session object for the specified ID
	 *
	 * @param sessionID			The session ID
	 * @return					A pointer to session for the specified ID or
	 *							NULL if not found
	 */
	CNetSession* GetSessionByID( uint sessionID );

protected:

	virtual bool SetupSession			( CNetSession* pSession );
	virtual bool HandleConnect			( CNetSession* pSession );
	virtual bool HandleDisconnect		( CNetSession *pSession );

private:

	// Not implemented
	CNetServer( const CNetServer& );
	CNetServer& operator=( const CNetServer& );

	//void ClientConnect		( ENetPeer* pPeer );
	//void ClientDisconnect	( ENetPeer* pPeer );
	//void ClientReceive		( ENetPeer* pPeer, ENetPacket* pPacket );

	/**
	 * Returns the session associated with the specified ENet peer
	 *
	 * @param pPeer				ENet peer
	 * @return					The session object if found or NULL
	 */
	//CNetSession* GetSessionByPeer( const ENetPeer* pPeer );

	/**
	 * Setup client game by sending the apropiate network messages. It also
	 * inform the client about the other connected clients as well as player
	 * slot assignment and attributes.
	 *
	 */
	//void SetupNewSession( CNetSession* pSession );

	/**
	 * Loads the player properties into the specified message
	 *
	 * @param pMessage			Message where to load player properties
	 * @param pPlayer			Player for which we load the properties
	 */
	void BuildPlayerConfigMessage( 
								 CPlayerConfigMessage* pMessage,
								 CPlayer* pPlayer );

	/**
	 * Callback function used by the BuildPlayerSetupMessage to iterate over
	 * the player properties. It will be called for each property of the player
	 *
	 * @param name				Property name
	 * @param pProperty			Pointer to player property
	 * @param pData				Context pointer passed on iteration startup
	 */
	static void PlayerConfigMessageCallback( 
											const CStrW& name,
											ISynchedJSProperty* pProperty,
											void* pData );

	/**
	 * Loads game properties into the specified message
	 *
	 * @param pMessage			Message where to load game properties
	 */
	void BuildGameSetupMessage( CGameSetupMessage* pMessage );
	
	/**
	 * Loads player slot properties into the specified message
	 *
	 * @param pMessage			Message where to load player properties
	 * @param pPlayerSlot		Player slot properties
	 */
	void BuildPlayerSlotAssignmentMessage( 
										  CAssignPlayerSlotMessage* pMessage,
										  CPlayerSlot* pPlayerSlot );

	/**
	 * Callback function used by the BuildGameSetupMessage to iterate over the
	 * game properties. It will be called for each property of the game
	 *
	 * @param name				Property name
	 * @param pProperty			Pointer to game property
	 * @param pData				Context pointer passed on iteration startup
	 */
	// IterateCB GameSetupMessageCallbak;
	static void GameSetupMessageCallback( 
										 const CStrW& name,
										 ISynchedJSProperty *pProperty,
										 void *pData );

	/**
	 * Retrieves a free session ID from the recycled sessions list
	 *
	 * @return					Free session ID
	 */
	uint GetFreeSessionID( void ) const;

	IDSessionMap	m_IDSessions;		// List of connected ID and session pairs
	//CScriptObject	m_ScriptConnect;	// Script client connect dispatch
	//CScriptObject	m_ScriptDisconnect;	// Script client disconnect dispatch
	//CScriptObject	m_ScriptChat;		// Script client chat dispatch
	CPlayer*		m_Player;			// Server player


public:

	/**
	 *
	 *
	 * @param addr				Address where to bind
	 * @return					PS_OK if bind successfully, error code otherwise
	 */
	//PS_RESULT Bind( const CSocketAddress& addr );
	void			SetPlayerPassword	( const CStr& password );
	CStrW			GetPlayerName		( void ) const	{ return m_PlayerName; }
	NetServerState	GetState			( void ) const	{ return m_State; }
	int				StartGame			( void );
	static void		ScriptingInit		( void );

protected:

	// Assign a session ID to the session. Do this just before calling AddSession
	void AssignSessionID( CNetSession* pSession );

	// Add the session. This will be called after the session passes the
	// handshake and authentication stages. AssignSessionID should've been called
	// on the session prior to calling this method.
	//void AddSession( CNetServerSession* pSession );

	// Remove the session from the server
	//void RemoveSession( CNetServerSession* pSession );

	// Queue a command coming in from the wire. The command has been validated
	// by the caller.
	void QueueIncomingCommand( CNetMessage* pMessage );
	
	// Call the JS callback for incoming events
	void OnPlayerChat	( const CStrW& from, const CStrW& message );
	void OnPlayerJoin	( CNetSession* pSession );
	void OnPlayerLeave	( CNetSession* pSession );
	void SetupPlayer	( CNetSession* pSession );

	//static bool OnPlayerJoin	( void* pContext, CFsmEvent* pEvent );
	static bool OnError			( void* pContext, CFsmEvent* pEvent );
	static bool OnHandshake		( void* pContext, CFsmEvent* pEvent );
	static bool OnAuthenticate	( void* pContext, CFsmEvent* pEvent );
	static bool OnPreGame		( void* pContext, CFsmEvent* pEvent );
	static bool OnInGame		( void* pContext, CFsmEvent* pEvent );
	static bool OnChat			( void* pContext, CFsmEvent* pEvent );
	
	// OVERRIDES FROM CServerSocket
	//virtual void OnAccept( const CSocketAddress& address );

	// OVERRIDES FROM CTurnManager
	virtual void NewTurn( void );
	virtual bool NewTurnReady( void );
	virtual void QueueLocalCommand( CNetMessage* pMessage );

	// Will only be called from the Network Thread, by the OnAccept handler
	//virtual CNetServerSession* CreateSession( CSocketInternal* pSocketInternal);

	// Ask the server if the session is allowed to start observing.
	//
	// Returns:
	//	true if the session should be made an observer
	//	false otherwise
	virtual bool AllowObserver( CNetSession* pSession );

private:

	CGameAttributes*		m_GameAttributes;	// Stores game attributes
	//int					m_LastSessionID;	// Stores the last session ID
	//SessionMap			m_Sessions;			// Managed sessions
	CJSMap< IDSessionMap >	m_JsSessions;
	CMutex					m_Mutex;			// Synchronization object for batches

	/*
		All sessions that have observer status (observer as in watcher - simple
		chatters don't have an entry here, only in m_Sessions).
		Sessions are added here after they have successfully requested observer
		status.
	*/
	SessionList				m_Observers;
	uint					m_MaxObservers;		// Maximum number of observers
	CGame*					m_Game;				// Pointer to actual game
	NetServerState			m_State;			// Holds server state	
	CStrW					m_Name;				// Server name
	CStrW					m_WelcomeMessage;	// Nice welcome message
	//CPlayer*				m_Player;			// Pointer to 'server' player
	CStrW					m_PlayerName;		// Player name
	CStrW					m_PlayerPassword;	// Player password
	int						m_Port;				// The listening port
	CScriptObject			m_OnChat;
	CScriptObject			m_OnClientConnect;
	CScriptObject			m_OnClientDisconnect;

//	static CGameAttributes::UpdateCallback	AttributeUpdate;
//	static CPlayer::UpdateCallback			PlayerAttributeUpdate;
//	static PlayerSlotAssignmentCB			PlayerSlotAssignmentCallback;

	static void AttributeUpdate			( const CStrW& name, const CStrW& newValue, void* pData);
	static void PlayerAttributeUpdate	( const CStrW& name, const CStrW& value, CPlayer* pPlayer, void* pData );
	static void PlayerSlotAssignment	( void* pData, CPlayerSlot* pPlayerSlot );
};

extern CNetServer *g_NetServer;

#endif // NETSERVER_H


