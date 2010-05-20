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
 *	FILE			: NetSession.cpp
 *	PROJECT			: 0 A.D.
 *	DESCRIPTION		: Network session class implementation
 *-----------------------------------------------------------------------------
 */

// INCLUDES
#include "precompiled.h"
#include "NetSession.h"
#include "NetLog.h"

// DECLARATIONS

//-----------------------------------------------------------------------------
// Name: CNetHost()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetHost::CNetHost( void )
{
	m_Host		 = NULL;
}

//-----------------------------------------------------------------------------
// Name: ~CNetHost()
// Desc: Destructor
//-----------------------------------------------------------------------------
CNetHost::~CNetHost( void )
{
	// Release host
	if ( m_Host ) enet_host_destroy( m_Host );

	m_Host		 = NULL;
}

//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Creates a client host
//-----------------------------------------------------------------------------
bool CNetHost::Create( void )
{
	// Create ENet host 
	m_Host = enet_host_create( NULL, 1, 0, 0 );
	if ( !m_Host ) return false;

	return true;
}

//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Creates a server host
//-----------------------------------------------------------------------------
bool CNetHost::Create( uint port, uint maxPeers )
{
	ENetAddress	addr;

	// Bind to default host
	addr.host	= ENET_HOST_ANY;
	addr.port	= port;

	// Create ENet server
	m_Host = enet_host_create( &addr, maxPeers, 0, 0 );
	if ( !m_Host ) return false;

	return true;
}

//-----------------------------------------------------------------------------
// Name: Shutdown()
// Desc: Shuts down network server and releases any resources
//-----------------------------------------------------------------------------
void CNetHost::Shutdown( void )
{
	// Destroy server
	if ( m_Host ) enet_host_destroy( m_Host );

	// Disconnect and release each peer
	PeerSessionList::iterator it = m_PeerSessions.begin();
	for ( ; it != m_PeerSessions.end(); it++ )
	{
		if ( !it->pSession ) continue;

		Disconnect( it->pSession );

		delete it->pSession;
	}

	m_PeerSessions.clear();

	m_Host		= NULL;
}

//-----------------------------------------------------------------------------
// Name: Connect()
// Desc: Connects to the specified remote host
// Note: Only clients use this method for connection to server
//-----------------------------------------------------------------------------
bool CNetHost::Connect( const CStr& host, uint port )
{
	ENetEvent		event;
	ENetAddress		addr;
	PeerSession		item;

	debug_assert( m_Host );

	// Bind to specified host
	addr.port = port;
	if ( enet_address_set_host( &addr, host.c_str() ) < 0 ) return false;

	// Initiate connection, allocate one channel
	ENetPeer* pPeer = enet_host_connect( m_Host, &addr, 1 );
	if ( !pPeer ) return false;

	// Wait 3 seconds for the connection to succeed
	if ( enet_host_service( m_Host, &event, 5000 ) > 0 &&
		 event.type == ENET_EVENT_TYPE_CONNECT )
	{
		// Connection succeeded
		CNetSession* pNewSession = new CNetSession( this, event.peer );
		if ( !pNewSession ) return false;

		if ( !SetupSession( pNewSession ) ) return false;

		NET_LOG3( "Successfully connected to server %s:%d succeeded", host.c_str(), port );

		// Successfully handled?
		if ( !HandleConnect( pNewSession ) )
		{
			delete pNewSession;

			return false;
		}

		// Store the only server session
		item.pPeer		= event.peer;
		item.pSession	= pNewSession;
		m_PeerSessions.push_back( item );

		return true;
	}

	NET_LOG3( "Connection to server %s:%d failed", host.c_str(), port );

	// 3 seconds are up or a host was disconnected
	enet_peer_reset( pPeer );

	return false;
}

//-----------------------------------------------------------------------------
// Name: Disconnect()
// Desc: Disconnects the specified session from the host
//-----------------------------------------------------------------------------
bool CNetHost::Disconnect( CNetSession* pSession )
{
	ENetEvent event;

	// Validate parameters
	if ( !pSession ) return false;
   
	debug_assert( m_Host );
	debug_assert( pSession->m_Peer );

	// Disconnect peer
    enet_peer_disconnect( pSession->m_Peer, 0 );

	// Allow up to 3 seconds for the disconnect to succeed
	while ( enet_host_service( m_Host, &event, 5000 ) > 0 )
    {
        switch ( event.type )
        {
        case ENET_EVENT_TYPE_RECEIVE:

			// Drop any received packets
            enet_packet_destroy( event.packet );
            break;

        case ENET_EVENT_TYPE_DISCONNECT:

			// Disconnect received for peer
			if ( !HandleDisconnect( pSession ) ) return false;
			break;
        }
    }
    
    // Disconnect attempt didn't succeed, force connection down
    enet_peer_reset( pSession->m_Peer );

	return true;
}

//-----------------------------------------------------------------------------
// Name: ProcessEvents()
// Desc: Wait for events and shuttles packets between the host and its peers
//-----------------------------------------------------------------------------
bool CNetHost::Poll( void )
{
	ENetEvent					event;
	CNetSession*				pSession = NULL;
	PeerSession					item;
	PeerSessionList::iterator	it;

	debug_assert( m_Host );

	// Poll host for events
	while ( enet_host_service( m_Host, &event, 0 ) > 0 )
	{
		// Handle occured event
		switch( event.type )
		{
		case ENET_EVENT_TYPE_CONNECT:

			// A new client has connected, handle it
			pSession = new CNetSession( this, event.peer );
			if ( !pSession ) return false;

			// Setup new session
			if ( !SetupSession( pSession ) ) return false;

			NET_LOG3( "A new client connected from %x:%u", event.peer->address.host, event.peer->address.port );

			// Successfully handled?
			if ( !HandleConnect( pSession ) ) return false;

			event.peer->data = pSession;
				
			// Add new item to internal list
			item.pPeer	  = event.peer;
			item.pSession = pSession;
			m_PeerSessions.push_back( item );

			break;

		case ENET_EVENT_TYPE_DISCONNECT:

			// Client has disconnected, handle it
			it = m_PeerSessions.begin();;
			for ( ; it != m_PeerSessions.end(); it++ )
			{
				// Is this our session?
				if ( it->pPeer == event.peer )
				{
					NET_LOG2( "%p disconnected", event.peer->data );

					// Successfully handled?
					if ( !HandleDisconnect( it->pSession ) ) return false;

					m_PeerSessions.erase( it );

					return true;
				}
			}

			break;

		case ENET_EVENT_TYPE_RECEIVE:

			// A new data packet was received from client, handle message
			it = m_PeerSessions.begin();
			for ( ; it != m_PeerSessions.end(); it++ )
			{
				// Is this our session?
				if ( it->pPeer == event.peer )
				{
					bool ok = false;

					// Create message from raw data
					CNetMessage* pNewMessage = CNetMessageFactory::CreateMessage( event.packet->data, event.packet->dataLength );
					if ( pNewMessage )
					{
						NET_LOG4( "Message %s of size %lu was received from %p", pNewMessage->ToString().c_str(), (unsigned long)pNewMessage->GetSerializedLength(), event.peer->data );

						ok = HandleMessageReceive( pNewMessage, it->pSession );

						delete pNewMessage;
					}

					// Done using the packet
					enet_packet_destroy( event.packet );

					if (! ok)
						return false;
				}
			}

			break;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Name: Broadcast()
// Desc: Broadcast the specified message to connected clients
//-----------------------------------------------------------------------------
void CNetHost::Broadcast( const CNetMessage* pMessage )
{
	// Validate parameters
	if ( !pMessage ) return;

	// Loop through the list of sessions and send the message to each
	for ( uint i = 0; i < GetSessionCount(); i++ )
	{
		CNetSession* pCurrSession = GetSession( i );
		if ( !pCurrSession ) continue;

		SendMessage( pCurrSession, pMessage );
	}
}

//-----------------------------------------------------------------------------
// Name: SendMessage()
// Desc: Sends the specified message to peer
//-----------------------------------------------------------------------------
bool CNetHost::SendMessage( 
						   const CNetSession* pSession, 
						   const CNetMessage* pMessage )
{
	// Validate parameters
	if ( !pMessage || !pSession ) return false;

	debug_assert( pSession->m_Peer );
	debug_assert( m_Host );

	size_t size = pMessage->GetSerializedLength();
	debug_assert( size );

	// Adjust buffer for message
	m_Buffer.resize( size );

	// Save message to internal buffer
	pMessage->Serialize( &m_Buffer[0] );
	
	// Create a reliable packet
	ENetPacket* pPacket = enet_packet_create( &m_Buffer[0], size, ENET_PACKET_FLAG_RELIABLE );
	if ( !pPacket ) return false;

	// Let ENet send the message to peer
	if ( enet_peer_send( pSession->m_Peer, ENET_DEFAULT_CHANNEL, pPacket ) < 0 )
	{
		// ENet failed to send the packet
		NET_LOG( "Failed to send ENet packet to peer" );

		return false;
	}
	else
	{
		NET_LOG4( "Message %s of size %lu was sent to %p",
			pMessage->ToString().c_str(), (unsigned long)size, pSession->m_Peer->data );
	}

	enet_host_flush( m_Host );

	return true;
}

//-----------------------------------------------------------------------------
// Name: ReceiveMessage()
// Desc: Receives a message from client if incoming packets are available
//-----------------------------------------------------------------------------
CNetMessage* CNetHost::ReceiveMessage( const CNetSession* pSession )
{
	// Validate parameters
	if ( !pSession ) return NULL;

	debug_assert( pSession->m_Peer );

	// Let ENet receive a message from peer
	ENetPacket* pPacket = enet_peer_receive( pSession->m_Peer, ENET_DEFAULT_CHANNEL );
	if ( !pPacket ) return NULL;

	// Create new message
	return CNetMessageFactory::CreateMessage( pPacket->data, pPacket->dataLength );
}

//-----------------------------------------------------------------------------
// Name: SetupSession()
// Desc: Setup new session upon creation
//-----------------------------------------------------------------------------
bool CNetHost::SetupSession( CNetSession* UNUSED(pSession) )
{
	return true;
}

//-----------------------------------------------------------------------------
// Name: HandleConnect()
// Desc: Allow application to handle client connect
//-----------------------------------------------------------------------------
bool CNetHost::HandleConnect( CNetSession* UNUSED(pSession) )
{
	return true;
}

//-----------------------------------------------------------------------------
// Name: HandleDisconnect()
// Desc: Allow application to handle client disconnect
//-----------------------------------------------------------------------------
bool CNetHost::HandleDisconnect( CNetSession* UNUSED(pSession) )
{
	return true;
}

//-----------------------------------------------------------------------------
// Name: HandleMessageReceive()
// Desc: Allow application to handle message recive
//-----------------------------------------------------------------------------
bool CNetHost::HandleMessageReceive( 
									CNetMessage* pMessage,
									CNetSession* pSession )
{
	// Validate parameters
	if ( !pSession || !pMessage ) return false;

	// Update FSM
	return pSession->Update( pMessage->GetType(), pMessage );
}

//-----------------------------------------------------------------------------
// Name: GetSessionCount()
// Desc: Returns the number of sessions the host manages
//-----------------------------------------------------------------------------
uint CNetHost::GetSessionCount( void ) const
{
	return ( uint )m_PeerSessions.size();
}

//-----------------------------------------------------------------------------
// Name: GetSession()
// Desc: Rteurns the session for the index
//-----------------------------------------------------------------------------
CNetSession* CNetHost::GetSession( uint index )
{
	// Validate parameter
	if ( index >= GetSessionCount() ) return NULL;

	return m_PeerSessions[ index ].pSession;
};

//-----------------------------------------------------------------------------
// Name: CNetSession()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetSession::CNetSession( CNetHost* pHost, ENetPeer* pPeer )
{
	m_Host				= pHost;
	m_Peer				= pPeer;
	m_ID				= INVALID_SESSION;
	m_PlayerSlot		= NULL;
}

//-----------------------------------------------------------------------------
// Name: ~CNetSession()
// Desc: Destructor
//-----------------------------------------------------------------------------
CNetSession::~CNetSession( void )
{
	// Release any resources
	//if ( m_Host ) enet_host_destroy( m_Host );

	//m_Host				= NULL;
	m_Peer				= NULL;
}

//-----------------------------------------------------------------------------
// Name: SetName()
// Desc: Set a new name for the session
//-----------------------------------------------------------------------------
void CNetSession::SetName( const CStr& name )
{
	m_Name = name;
}

//-----------------------------------------------------------------------------
// Name: SetID()
// Desc: Set new ID for this session
//-----------------------------------------------------------------------------
void CNetSession::SetID( uint ID )
{
	m_ID = ID;
}

//-----------------------------------------------------------------------------
// Name: SetPlayerSlot()
// Desc: Set the player slot for this session
//-----------------------------------------------------------------------------
void CNetSession::SetPlayerSlot( CPlayerSlot* pPlayerSlot )
{
	m_PlayerSlot = pPlayerSlot;
}

//-----------------------------------------------------------------------------
// Name: ScriptingInit()
// Desc:
//-----------------------------------------------------------------------------
void CNetSession::ScriptingInit( void )
{
	AddProperty( L"id", &CNetSession::m_ID );
	AddProperty( L"name", &CNetSession::m_Name );

	CJSObject<CNetSession>::ScriptingInit( "NetSession" );
}
