/* Copyright (C) 2010 Wildfire Games.
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
 *	FILE			: NetSession.h
 *	PROJECT			: 0 A.D.
 *	DESCRIPTION		: Network session class interface file
 *-----------------------------------------------------------------------------
 */

#ifndef NETSESSION_H
#define NETSESSION_H

// INCLUDES
#include "Network.h"
#include "ps/GameAttributes.h"
#include "fsm.h"
#include <enet/enet.h>

#include <vector>

// DECLARATIONS
#define	INVALID_SESSION				0
#define ENET_DEFAULT_CHANNEL		0

class CNetSession;
class CNetHost;

typedef struct
{
	ENetPeer*		pPeer;
	CNetSession*	pSession;

} PeerSession;

typedef struct
{
	CNetHost*		pHost;
	CNetSession*	pSession;

} FsmActionCtx;

typedef std::vector< PeerSession >	PeerSessionList;

/*
	CLASS			: CNetHost
	DESCRIPTION		: CNetHost is a wrapper around ENet host concept
	NOTES			:
*/

class CNetHost
{
	NONCOPYABLE(CNetHost);

public:

	CNetHost( void );
	virtual ~CNetHost( void );

	bool Create( void );
	bool Create( uint port, uint maxPeers );
	void Shutdown( void );

	/**
	 * Indicates whether the host is currently a server
	 *
	 * @return					Boolean indicating whether the host is a server
	 */
	virtual bool IsServer( void ) const { return false; }

	/**
	 * Returns the number of sessions for the host
	 *
	 * @return					The number of sessions
	 */
	uint GetSessionCount( void ) const;

	/**
	 * Returns the session object for the specified index
	 *
	 * @param index				Index for session
	 * @return					Session object for index or	NULL if not found
	 */
	CNetSession* GetSession( uint index );

	/**
	 * Connects to foreign host
	 *
	 * @param host						Foreign host name
	 * @param port						Port on which the foreign host listens
	 * @return							true on success, false on failure
	 */
	bool Connect( const CStr& host, uint port );

	/**
	 * Disconnects session from host
	 *
	 * @param pSession					Session representing peer
	 * @return							true on success, false otherwise
	 */
	bool Disconnect( CNetSession* pSession );

	/** 
	 * Listens for incoming connections and dispatches host events
	 *
	 * @return							true on exit, false dispatch failure
	 */
	//bool Run( void );
	bool Poll( void );

	/**
	 * Broadcast the specified message to connected clients
	 *
	 * @param pMessage			Message to broadcast
	 */
	void Broadcast( const CNetMessage* pMessage );

	/**
	 * Send the specified message to client
	 *
	 * @param pMessage					The message to send
	 */
	virtual bool SendMessage( 
							 const CNetSession* pSession,
							 const CNetMessage* pMessage );

	/**
	 * Receive a message from client if available
	 *
	 */
	virtual CNetMessage* ReceiveMessage( const CNetSession* pSession );

protected:

	// Allow application to handle new client connect
	virtual bool SetupSession			( CNetSession* pSession );
	virtual bool HandleConnect			( CNetSession* pSession );
	virtual bool HandleDisconnect		( CNetSession* pSession );
	virtual bool HandleMessageReceive	( 
										 CNetMessage* pMessage,
										 CNetSession* pSession );
private:

	std::vector<u8>	m_Buffer;		// Serialize out messages buffer
	ENetHost*		m_Host;			// Represents this host
	PeerSessionList	m_PeerSessions;	// Session list of connected peers
};

/*
	CLASS			: CNetSession
	DESCRIPTION		: CNetSession is a wrapper class around ENet peer concept
					  which represents a peer from a network connection. A 
					  network session is spawned by CNetServer each time a 
					  client connects and destroyed when it disconnects. When a
					  new message is received from a client, its representing
					  session object's message handler is called for processing
					  that message.
					  CNetSession is also a state machine. All client requests
					  are delegated to the current state. The current 
					  CNetSessionState object's methods will change the current
					  state as appropriate.
	NOTES			:
*/

class CNetSession : public CFsm,
					public CJSObject< CNetSession >
{
	NONCOPYABLE(CNetSession);

	friend class CNetHost;

public:

	virtual ~CNetSession( void );

	/**
	 * Retrieves the name of the session
	 *
	 * @return							Session name
	 */
	const CStrW& GetName( void ) const		{ return m_Name; }

	/**
	 * Set the new name for the session
	 *
	 * @param name						The session new name
	 */
	void SetName( const CStr& name );

	/**
	 * Retrieves the ID of the session
	 *
	 * @return							Session ID
	 */
	uint GetID( void ) const { return m_ID; }

	/**
	 * Set the ID for this session
	 *
	 * @param							New session ID
	 */
	void SetID( uint ID );

	void SetPlayerSlot( CPlayerSlot* pPlayerSlot );
	CPlayerSlot* GetPlayerSlot( void ) { return m_PlayerSlot; }
	static void ScriptingInit( void );

private:

	// Only the hosts can create sessions
	CNetSession( CNetHost* pHost, ENetPeer* pPeer );

	CNetHost*		  m_Host;			 // The associated local host
	ENetPeer*		  m_Peer;			 // Represents the peer host
	uint			  m_ID;				 // Session ID
	CStrW			  m_Name;			 // Session name
	CPlayerSlot*	  m_PlayerSlot;
};

#endif	// NETSESSION_H

