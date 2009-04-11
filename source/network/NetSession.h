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
#include "ps/Singleton.h"
#include "ps/GameAttributes.h"
#include "ps/Player.h"
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
	DESCRIPTION		: CNetHost is a wrapper around ENet host conecept
	NOTES			:
*/

class CNetHost : public Singleton< CNetHost >
{
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
	 * Indicates whether the host is currently a client
	 *
	 * @return					Boolean indicating whether the host is a client
	 */
	virtual bool IsClient( void ) const { return false; }
	
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

	/**
	 * Attempts to resize the internal buffer to the size indicated by the
	 * passed parameter.
	 *
	 * @param size						The new size for the buffer
	 */
	void ResizeBuffer( size_t size );

	// Allow application to handle new client connect
	virtual bool SetupSession			( CNetSession* pSession );
	virtual bool HandleConnect			( CNetSession* pSession );
	virtual bool HandleDisconnect		( CNetSession* pSession );
	virtual bool HandleMessageReceive	( 
										 CNetMessage* pMessage,
										 CNetSession* pSession );

	/**
	 * Worker thread function
	 *
	 * @pData					Argument specified on thread creation
	 * @return					NULL
	 */
	//static void* WorkerFunc( void* pData );

private:

	// Not implemented
	CNetHost( const CNetHost& );
	CNetHost& operator=( const CNetHost& );

	u8*				m_Buffer;		// Serialize out messages buffer
	size_t		    m_BufferSize;	// Output buffer size
	ENetHost*		m_Host;			// Represents this host
	PeerSessionList	m_PeerSessions;	// Session list of connected peers
	//pthread_t		m_WorkerID;		// Worker thread
	//sem_t*		m_StopWorker;	// Worker thread stop semaphore
};

/*
	CLASS			: CNetSession
	DESCRIPTION		: CNetSession is a wrapper class around ENet peer concept
					  which represents a peer from a network connection. A 
					  network session is spawned by CNetServer each time a 
					  client connects and destroyed when it disconnects. When a
					  new message is received fom a client, its representing
					  session object's message handler is called for processing
					  that message.
					  CNetSession is also a state machine. All client requests
					  are delegated to the current state. The current 
					  CNetSessionState object's methods will change the current
					  state as appropriate.
	NOTES			:
*/

class CNetSession : public CFsm,
					public CJSObject< CNetSession >,
					public IMessagePipeEnd
{
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

	/**
	 * Allows both client and server to set a callback handler
	 *
	 * @param pCallbackHandler			Callback handler
	 */
	//void SetCallbackHandler( ISessionCallback* pCallbackHandler );

	/**
	 * Disconnects the client from remote host
	 *
	 */
	void Reset( void );

	void SetPlayer( CPlayer* pPlayer );
	CPlayer* GetPlayer( void ) { return m_Player; }
	void SetPlayerSlot( CPlayerSlot* pPlayerSlot );
	CPlayerSlot* GetPlayerSlot( void ) { return m_PlayerSlot; }
	void StartGame( void );
	virtual void Push( CNetMessage* pMessage );
	virtual CNetMessage* TryPop( void );
	bool IsReadyForTurn( void ) const { return m_ReadyForTurn; }
	void SetReadyForTurn( bool newValue ) {	m_ReadyForTurn = newValue; }
	bool JSI_Close( JSContext *cx, uintN argc, jsval *argv );
	static void ScriptingInit( void );

	//bool HandleMessage( CNetMessage* pMessage );

protected:

	/**
	 * Process the message passed as parameter
	 *
	 * @param message					The message to process
	 * @return							true if the message was handler
	 *									successufully, false otherwise
	 */
	//bool ProcessMessage( const CNetMessage& message );

private:

	// Only the hosts can create sessions
	CNetSession( CNetHost* pHost, ENetPeer* pPeer );

	// Not implemented
	CNetSession( void );
	CNetSession( const CNetSession& );
	CNetSession& operator=( const CNetSession& );

	CNetHost*		  m_Host;			 // The associated local host
	ENetPeer*		  m_Peer;			 // Represents the peer host
	uint			  m_ID;				 // Session ID
	CStrW			  m_Name;			 // Session name
	CPlayer*		  m_Player;
	CPlayerSlot*	  m_PlayerSlot;
	bool			  m_ReadyForTurn;	// Next turn ready flag
};

/*
	CLASS			: CNetServerSession
	DESCRIPTION		:
	NOTES			:
*/

/*class CNetServerSession : public CNetSession, 
						  public CJSObject< CNetServerSession >
{
public:

	bool			IsObserver		( void ) const	{ return m_IsObserver; }
	CPlayer*		GetPlayer		( void ) const	{ return m_Player; }
	CPlayerSlot*	GetPlayerSlot	( void ) const	{ return m_PlayerSlot; }
	void			StartGame		( void );
	void			SetPlayer		( CPlayer* pPlayer );
	void			SetPlayerSlot	( CPlayerSlot* pPlayerSlot );

protected:

	CNetServerSession( 
					   CNetServer* pServer,
					   NetMessageHandler* pHandler = m_HandshakeHandler );
	CNetServerSession(
					   CNetServer* pServer,
					   CSocketInternal* pSocketInternal,
					   NetMessageHandler* pHandler = m_HandshakeHandler );
	virtual ~CNetServerSession( void );

private:

	static void		ScriptingInit	( void );
	bool			JSI_Close		( 
									  JSContext* pContext,
									  uintN argc,
									  jsval* argv );

	CNetServer*		m_Server;
	CPlayer*		m_Player;
	CPlayerSlot*	m_PlayerSlot;
	bool			m_IsObserver;

	static bool HandshakeHandler( CNetMessage* pMessage, CNetSession* pSession );
	static bool ObserverHandler	( CNetMessage* pMessage, CNetSession* pSession );
	static bool BaseHandler		( CNetMessage* pMessage, CNetSession* pSession );
	static bool AuthHandler		( CNetMessage* pMessage, CNetSession* pSession );
	static bool PreGameHandler	( CNetMessage* pMessage, CNetSession* pSession );
	static bool InGameHandler	( CNetMessage* pMessage, CNetSession* pSession );
	static bool ChatHandler		( CNetMessage* pMessage, CNetSession* pSession );
};*/

#endif	// NETSESSION_H

