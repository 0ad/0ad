/**
 *-----------------------------------------------------------------------------
 *	FILE			: NetSession.cpp
 *	PROJECT			: 0 A.D.
 *	DESCRIPTION		: Network session class implementation
 *-----------------------------------------------------------------------------
 */

// INCLUDES
#include "precompiled.h"
//#include "SessionManager.h"
#include "NetSession.h"
//#include "NetServer.h"
#include "NetLog.h"

#pragma warning( disable : 4100 )

// DECLARATIONS

//-----------------------------------------------------------------------------
// Name: CNetHost()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetHost::CNetHost( void )
{
	m_Host		 = NULL;
	m_Buffer	 = NULL;
	m_BufferSize = 0;

//	m_WorkerID	 = 0;
//	m_StopWorker = NULL;
}

//-----------------------------------------------------------------------------
// Name: ~CNetHost()
// Desc: Destructor
//-----------------------------------------------------------------------------
CNetHost::~CNetHost( void )
{
	// Release host
	if ( m_Host ) enet_host_destroy( m_Host );
	if ( m_Buffer )	delete [] m_Buffer;

	// Release running semaphore
//	if ( m_StopWorker ) sem_close( m_StopWorker );

	m_Host		 = NULL;
	m_Buffer	 = NULL;
	m_BufferSize = 0;
//	m_WorkerID	 = 0;
//	m_StopWorker = NULL;
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

	// Stop worker thread
//	sem_post( m_StopWorker );
//	if ( m_WorkerID ) pthread_join( m_WorkerID, NULL );

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
//	m_WorkerID  = 0;
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

	assert( m_Host );

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

		// Successfully handled?
		if ( !HandleConnect( pNewSession ) )
		{
			delete pNewSession;

			return false;
		}

		NET_LOG3( "Successfully connected to server %s:%d succeeded", host.c_str(), port );

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
   
	assert( m_Host );
	assert( pSession->m_Peer );

	// Disconnect peer
    enet_peer_disconnect( pSession->m_Peer );

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
// Name: Run()
// Desc:
//-----------------------------------------------------------------------------
/*bool CNetHost::Run( void )
{
	assert( m_Host );

	// Host created?
	if ( !m_Host ) return false;

	// Already running?
	if ( m_WorkerID != 0 ) return true;

	// Create run semaphore
	m_StopWorker = sem_open( "//WFG_HostWorkerRun", O_CREAT | O_EXCL, 0700, 0 );
	if ( !m_StopWorker ) return false;

	// Create worker thread
	if ( pthread_create( &m_WorkerID, 0, &WorkerFunc, this ) < 0 ) return false;

	return true;
}

//-----------------------------------------------------------------------------
// Name: WorkerFunc()
// Desc: Worker thread function
//-----------------------------------------------------------------------------
void* CNetHost::WorkerFunc( void* pData )
{
	ENetEvent					event;
	CNetSession*				pSession = NULL;
	PeerSession					item;
	PeerSessionList::iterator	it;

	// Validate parameters
	if ( !pData ) return NULL;

	CNetHost* pHost = ( CNetHost* )pData;

	// Poll host for events
	while ( true )
	{
		// Decide whether to stop or not
		if ( !sem_timedwait( pHost->m_StopWorker, NULL ) ) break;

		int retval = enet_host_service( pHost->m_Host, &event, 100 );

		// Any event?
		if ( !retval ) continue;

		// Any error?
		if ( !retval ) break;

		// Handle occured event
		switch( event.type )
		{
		case ENET_EVENT_TYPE_CONNECT:

			// A new client has connected, handle it
			pSession = new CNetSession( pHost, event.peer );
			if ( !pSession ) return NULL;

			// Successfully handled?
			if ( !pHost->HandleConnect( pSession ) ) return NULL;

			// Add new item to internal list
			item.pPeer	  = event.peer;
			item.pSession = pSession;
			pHost->m_PeerSessions.push_back( item );

			break;

		case ENET_EVENT_TYPE_DISCONNECT:

			// Client has disconnected, handle it
			it = pHost->m_PeerSessions.begin();;
			for ( ; it != pHost->m_PeerSessions.end(); it++ )
			{
				// Is this our session?
				if ( it->pPeer == event.peer )
				{
					// Successfully handled?
					if ( !pHost->HandleDisconnect( it->pSession ) ) return NULL;

					pHost->m_PeerSessions.erase( it );
				}
			}

			break;

		case ENET_EVENT_TYPE_RECEIVE:

			// A new data packet was received from client, handle message
			it = pHost->m_PeerSessions.begin();
			for ( ; it != pHost->m_PeerSessions.end(); it++ )
			{
				// Is this our session?
				if ( it->pPeer == event.peer )
				{
					// Create message from raw data
					CNetMessage* pNewMessage = CNetMessageFactory::CreateMessage( event.packet->data, event.packet->dataLength );
					if ( !pNewMessage ) return NULL;

					// Successfully handled?
					if ( !pHost->HandleMessageReceive( pNewMessage, it->pSession ) ) return NULL;
				}
			}

			break;
		}
	}

	return NULL;
}*/

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

	assert( m_Host );

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

			// Successfully handled?
			if ( !HandleConnect( pSession ) ) return false;

			NET_LOG3( "A new client connected from %x:%u", event.peer->address.host, event.peer->address.port );
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
					// Successfully handled?
					if ( !HandleDisconnect( it->pSession ) ) return false;

					m_PeerSessions.erase( it );

					NET_LOG2( "%x disconnected", event.peer->data );

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
					// Create message from raw data
					CNetMessage* pNewMessage = CNetMessageFactory::CreateMessage( event.packet->data, event.packet->dataLength );
					if ( !pNewMessage ) return false;

					// Successfully handled?
					if ( !HandleMessageReceive( pNewMessage, it->pSession ) ) return false;

					NET_LOG4( "Message %s of size %u was received from %x", pNewMessage->ToString().c_str(), pNewMessage->GetSerializedLength(), event.peer->data );

					// Done using the packet
					enet_packet_destroy( event.packet );
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
// Note: Reference counting for a sending message requires multithreading 
//		 locking mechanisms so a clone of the message is made and sent out
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
// Name: ResizeBuffer()
// Desc: Resizes the internal buffer
//-----------------------------------------------------------------------------
void CNetHost::ResizeBuffer( uint size )
{
	// Already enough space?
	if ( size <= m_BufferSize ) return;

	// Allocate enough space for the new buffer
	u8* pBuffer = new u8[ ALIGN_BLOCK( m_BufferSize + size ) ];
	if ( !pBuffer ) return;

	// Any old data?
	if ( m_Buffer )
	{
		// Copy old data
		memcpy( pBuffer, m_Buffer, m_BufferSize );

		delete [] m_Buffer;
	}

	// Store new buffer
	m_Buffer = pBuffer;

	// Store new buffer size
	m_BufferSize = ALIGN_BLOCK( m_BufferSize + size );
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

	assert( pSession->m_Peer );
	assert( m_Host );

	uint size = pMessage->GetSerializedLength();

	// Adjust buffer for message
	ResizeBuffer( size );

	// Save message to internal buffer
	pMessage->Serialize( m_Buffer );
	
	// Create a reliable packet
	ENetPacket* pPacket = enet_packet_create( m_Buffer, size, ENET_PACKET_FLAG_RELIABLE );
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
		NET_LOG( "Successfully sent ENet packet to peer" );
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

	assert( pSession->m_Peer );

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
bool CNetHost::SetupSession( CNetSession* pSession )
{
	return true;
}

//-----------------------------------------------------------------------------
// Name: HandleConnect()
// Desc: Allow application to handle client connect
//-----------------------------------------------------------------------------
bool CNetHost::HandleConnect( CNetSession* pSession )
{
	return true;
}

//-----------------------------------------------------------------------------
// Name: HandleDisconnect()
// Desc: Allow application to handle client disconnect
//-----------------------------------------------------------------------------
bool CNetHost::HandleDisconnect( CNetSession* pSession )
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
	m_ReadyForTurn		= false;

	// Register the network session
	//g_SessionManager.Register( this );
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

	// Unregister the network session
	//g_SessionManager.Unregister( this );
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
// Name: SetPlayer()
// Desc: Set the player for this session
//-----------------------------------------------------------------------------
void CNetSession::SetPlayer( CPlayer* pPlayer )
{
	m_Player = pPlayer;
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
// Name: StartGame()
// Desc: Called by server after informing all clients about starting the game
//-----------------------------------------------------------------------------
void CNetSession::StartGame( void )
{
}

//-----------------------------------------------------------------------------
// Name: Push()
// Desc: Sends a message through ENet
//-----------------------------------------------------------------------------
void CNetSession::Push( CNetMessage* pMessage )
{
	// Validate parameters
	if ( !pMessage ) return;

	assert( m_Host );

	m_Host->SendMessage( this, pMessage );
}

//-----------------------------------------------------------------------------
// Name: TryPop()
// Desc: Receives a message through ENet
//-----------------------------------------------------------------------------
CNetMessage* CNetSession::TryPop( void )
{
	assert( m_Host );

	return m_Host->ReceiveMessage( this );
}

//-----------------------------------------------------------------------------
// Name: ScriptingInit()
// Desc:
//-----------------------------------------------------------------------------
void CNetSession::ScriptingInit( void )
{
	AddProperty( L"id", &CNetSession::m_ID );
	AddProperty( L"name", &CNetSession::m_Name );
	AddMethod<bool, &CNetSession::JSI_Close>( "close", 0 );

	CJSObject<CNetSession>::ScriptingInit( "NetSession" );
}

//-----------------------------------------------------------------------------
// Name: JSI_Close()
// Desc:
//-----------------------------------------------------------------------------
bool CNetSession::JSI_Close( JSContext* cx, uintN argc, jsval* argv )
{
	return false;
}

//-----------------------------------------------------------------------------
// Name: HandleMessage()
// Desc: 
//-----------------------------------------------------------------------------
//bool CNetSession::HandleMessage( CNetMessage* pMessage )
//{
//	return true;
//}

/*
//-----------------------------------------------------------------------------
// Name: CNetServerSession()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetServerSession::CNetSession(
							    CNetServer* pServer,
							    NetMessageHandler* pHandler )
: CNetSession( pHandler )
{
	m_Server		= pServer;
	m_Player		= NULL;
	m_PlayerSlot	= NULL;
	m_IsObserver	= false;
}

//-----------------------------------------------------------------------------
// Name: CNetServerSession()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetServerSession::CNetSession(
							    CNetServer* pServer,
								CSocketInternal* pSocketInternal,
								NetMessageHandler* pHandler )
: CNetSession( pSocketInternal, pHandler )
{
	m_Server		= pServer;
	m_Player		= NULL;
	m_PlayerSlot	= NULL;
	m_IsObserver	= false;
}

//-----------------------------------------------------------------------------
// Name: ~CNetServerSession()
// Desc: Destructor
//-----------------------------------------------------------------------------
CNetServerSession::~CNetServerSession( void )
{
}

//-----------------------------------------------------------------------------
// Name: StartGame()
// Desc: Called by server after informing all clients about starting the game
//-----------------------------------------------------------------------------
void CNetServerSession::StartGame( void )
{
}

//-----------------------------------------------------------------------------
// Name: SetPlayer()
// Desc: Set the player for this session
//-----------------------------------------------------------------------------
void CNetServerSession::SetPlayer( CPlayer* pPlayer )
{
	m_Player = pPlayer;
}

//-----------------------------------------------------------------------------
// Name: SetPlayerSlot()
// Desc: Set the player slot for this session
//-----------------------------------------------------------------------------
void CNetServerSession::SetPlayerSlot( CPlayerSlot* pPlayerSlot )
{
	m_PlayerSlot = pPlayerSlot;
}

//-----------------------------------------------------------------------------
// Name: SetID()
// Desc: Set new session ID
//-----------------------------------------------------------------------------
void CNetServerSession::SetID( uint ID )
{
	m_ID = ID;
}

//-----------------------------------------------------------------------------
// Name: BaseHandler()
// Desc:
//-----------------------------------------------------------------------------
bool CNetServerSession::BaseHandler( 
									CNetMessage* pMessage,
									CNetSession* pSession )
{
	assert( pMessage );
	assert( pSession );

	// Validate parameters
	if ( !pMessage || !pSession ) return false;

	CNetServerSession* pSrvSession = dynamic_cast< CNetSession* >( pSession );
	if ( pSrvSession ) return false;

	// Handler NMT_ERROR message only
	if ( pMessage->GetType() != NMT_ERROR ) return false;

	CNetErrorMessage* pErrMessage = dynamic_cast< CNetErrorMessage* >( pMessage );
	if ( !pErrMessage ) return false;

	if ( pErrMessage->GetState() == SERVER_STATE_DISCONNECTED )
	{
		if ( pSrvSession->m_ID != -1 )
		{
			pSrvSession->m_Server->RemoveSession( pSrvSession );
		}

		delete pSrvSession;
	}
	else
	{
		// Not disconnected? Weired...
		LOG( WARNING, LOG_CAT_NET, "CNetServerSession::BaseHandler() NMT_ERROR: %s", pErrMessage->ToString().c_str() );
	}

	delete pMessage;

	return true;
}

//-----------------------------------------------------------------------------
// Name: HandshakeHandler()
// Desc:
//-----------------------------------------------------------------------------
bool CNetServerSession::HandshakeHandler(
										 CNetMessage* pMessage,
										 CNetSession* pSession )
{
	assert( pMessage );
	assert( pSession );
	assert( m_Server );

	// Validate parameters
	if ( !pMessage || !pSession ) return false;

	CNetServerSession* pSrvSession = dynamic_cast< CNetServerSession* >( pSession );
	if ( !pSrvSession ) return false;

	LOG( NORMAL, LOG_CAT_NET, "CNetServerSession::HandshakeHandler() %s", pMessage->ToString().c_str() );

	// Call base handler if other message thant NMT_ClientHandshake
	if ( pMessage->GetType() != NMT_ClientHandshake ) BaseHandler( pMessage, pSession );

	CClientHandshake* pHandshakeMessage = dynamic_cast< CClientHandshake* >( pMessage );
	if ( !pHandshakeMessage ) return false;

	if ( pHandshakeMessage->m_ProtocolVersion != PS_PROTOCOL_VERSION )
	{
		pSrvSession->Push( new CCloseRequestMessage() );
		BaseHandler( new CNetErrorMessage( PS_OK, SERVER_STATE_DISCONNECTED ), pSrvSession );
	}

	//??? (else)
	CServerHandshakeResponse* pNewMessage = new CServerHandshakeResponse();
	pNewMessage->m_UseProtocolVersion	= PS_PROTOCOL_VERSION;
	pNewMessage->m_Flags				= 0;
	pNewMessage->m_Message				= pSrvSession->m_Server->m_WelcomeMessage;
	
	pSrvSession->Push( pNewMessage );

	pSrvSession->m_MessageHandler = AuthHandler;

	delete pMessage;

	return true;
}*/
