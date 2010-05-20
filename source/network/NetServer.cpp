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
 * FILE				: NetServer.cpp
 * PROJECT			: 0 A.D.
 * DESCRIPTION		: Network server class implementation
 *-----------------------------------------------------------------------------
 */

// INCLUDES
#include "precompiled.h"
#include "ps/CLogger.h"
#include "ps/CConsole.h"
#include "simulation/Simulation.h"
#include "NetJsEvents.h"
#include "NetSession.h"
#include "NetServer.h"
#include "simulation2/Simulation2.h"

#define LOG_CATEGORY L"net"

// DECLARATIONS
CNetServer*	g_NetServer = NULL;

class CServerTurnManager : public CTurnManager
{
public:
	CServerTurnManager(CNetServer& server) : m_Server(server) { }
	virtual void QueueLocalCommand(CNetMessage* pMessage);
	virtual void NewTurn();
	virtual bool NewTurnReady();
private:
	CNetServer& m_Server;
};

//-----------------------------------------------------------------------------
// Name: CNetServer()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetServer::CNetServer( CGame *pGame, CGameAttributes *pGameAttributes )
: m_JsSessions( &m_IDSessions )
{
	m_TurnManager = new CServerTurnManager(*this);
	m_ServerTurnManager = NULL;

	m_Game				= pGame;
	m_GameAttributes	= pGameAttributes;
	m_MaxObservers		= MAX_OBSERVERS;
	//m_LastSessionID		= 1;
	m_Port				= DEFAULT_HOST_PORT;
	m_Name				= DEFAULT_SERVER_NAME;
	m_PlayerName		= DEFAULT_PLAYER_NAME;
	m_WelcomeMessage	= DEFAULT_WELCOME_MESSAGE;

	// Set callbacks
	m_GameAttributes->SetUpdateCallback( AttributeUpdate, this );
	m_GameAttributes->SetPlayerUpdateCallback( PlayerAttributeUpdate, this );
	m_GameAttributes->SetPlayerSlotAssignmentCallback( PlayerSlotAssignment, this );

	if (!g_UseSimulation2)
		m_Game->GetSimulation()->SetTurnManager(m_TurnManager);

	// Set an incredibly long turn length for debugging
	// (e.g. less command batch spam that way)
	for ( uint i = 0; i < 3; i++ )
	{
		m_TurnManager->SetTurnLength( i, CTurnManager::DEFAULT_TURN_LENGTH );
	}

	g_ScriptingHost.SetGlobal( "g_NetServer", OBJECT_TO_JSVAL( GetScript() ) );
}

//-----------------------------------------------------------------------------
// Name: ~CNetServer()
// Desc: Destructor
//-----------------------------------------------------------------------------
CNetServer::~CNetServer()
{
	m_IDSessions.clear();

	g_ScriptingHost.SetGlobal( "g_NetServer", JSVAL_NULL );
}

//-----------------------------------------------------------------------------
// Name: ScriptingInit()
// Desc:
//-----------------------------------------------------------------------------
void CNetServer::ScriptingInit( void )
{
	CJSMap< IDSessionMap >::ScriptingInit( "NetServer_SessionMap" );

	AddMethod< bool, &CNetServer::Start >( "open", 0 );

	AddProperty( L"sessions", &CNetServer::m_JsSessions );
	AddProperty( L"serverPlayerName", &CNetServer::m_PlayerName );
	AddProperty( L"serverName", &CNetServer::m_Name );
	AddProperty( L"welcomeMessage", &CNetServer::m_WelcomeMessage );
	AddProperty( L"port", &CNetServer::m_Port );
	AddProperty( L"onChat", &CNetServer::m_OnChat );
	AddProperty( L"onClientConnect", &CNetServer::m_OnClientConnect );
	AddProperty( L"onClientDisconnect", &CNetServer::m_OnClientDisconnect );

	CJSObject< CNetServer >::ScriptingInit( "NetServer" );

	//CGameAttributes::ScriptingInit();
}

//-----------------------------------------------------------------------------
// Name: Start()
// Desc: Initialize the server
//-----------------------------------------------------------------------------
bool CNetServer::Start( JSContext* UNUSED(pContext), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	// Setup initial state
	m_State = SERVER_STATE_PREGAME;

	// Create new session
	if ( !Create( m_Port, MAX_CLIENTS ) )
	{
		LOG( CLogger::Error, LOG_CATEGORY, L"CNetServer::Run(): Initialize failed" );

		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Name: SetupSession()
// Desc: Setup new session
//-----------------------------------------------------------------------------
bool CNetServer::SetupSession( CNetSession* pSession )
{
	// Validate parameters
	if ( !pSession ) return false;

	FsmActionCtx* pContext = new FsmActionCtx;
	if ( !pContext ) return false;

	pContext->pHost		= this;
	pContext->pSession	= pSession;

	// Setup transitions for session
	pSession->AddTransition( NSS_HANDSHAKE, ( uint )NMT_ERROR, NSS_HANDSHAKE, (void*)&OnError, pContext );
	pSession->AddTransition( NSS_HANDSHAKE, ( uint )NMT_CLIENT_HANDSHAKE, NSS_AUTHENTICATE, (void*)&OnHandshake, pContext );
	pSession->AddTransition( NSS_AUTHENTICATE, ( uint )NMT_ERROR, NSS_AUTHENTICATE, (void*)&OnError, pContext );
	pSession->AddTransition( NSS_AUTHENTICATE, ( uint )NMT_AUTHENTICATE, NSS_PREGAME, (void*)&OnAuthenticate, pContext );
	pSession->AddTransition( NSS_AUTHENTICATE, ( uint )NMT_APP_PREGAME, NSS_PREGAME, (void*)&OnPreGame, pContext );
	pSession->AddTransition( NSS_AUTHENTICATE, ( uint )NMT_APP_OBSERVER, NSS_INGAME, (void*)&OnChat, pContext );
	pSession->AddTransition( NSS_PREGAME, ( uint )NMT_END_COMMAND_BATCH, NSS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NSS_PREGAME, ( uint )NMT_CHAT, NSS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_ERROR, NSS_INGAME, (void*)&OnError, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_CHAT, NSS_INGAME, (void*)&OnChat, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_GOTO, NSS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_PATROL, NSS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_ADD_WAYPOINT, NSS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_CONTACT_ACTION, NSS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_PRODUCE, NSS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_PLACE_OBJECT, NSS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_RUN, NSS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_SET_RALLY_POINT, NSS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_SET_STANCE, NSS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_NOTIFY_REQUEST, NSS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_FORMATION_GOTO, NSS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_FORMATION_CONTACT_ACTION, NSS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_SIMULATION_COMMAND, NSS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NSS_INGAME, ( uint )NMT_END_COMMAND_BATCH, NSS_INGAME, (void*)&OnInGame, pContext );

	// Set first state
	pSession->SetFirstState( NSS_HANDSHAKE );

	return true;
}

//-----------------------------------------------------------------------------
// Name: HandleConnect()
// Desc: Called when a new player joins the current game
//-----------------------------------------------------------------------------
bool CNetServer::HandleConnect( CNetSession* pSession )
{
	// Validate parameters
	if ( !pSession ) return false;

	// Player joined the game, start authentication
	CSrvHandshakeMessage handshake;
	handshake.m_Magic			= PS_PROTOCOL_MAGIC;
	handshake.m_ProtocolVersion	= PS_PROTOCOL_VERSION;
	handshake.m_SoftwareVersion	= PS_PROTOCOL_VERSION;
	SendMessage( pSession, &handshake );

	// Store new session into the map
	m_IDSessions[ pSession->GetID() ] = pSession;

	//OnPlayerJoin( pSession );

	return true;
}

//-----------------------------------------------------------------------------
// Name: HandleDisconnect()
// Desc: Called when a client has disconnected
//-----------------------------------------------------------------------------
bool CNetServer::HandleDisconnect( CNetSession* pSession )
{
	// Validate parameters
	if ( !pSession ) return false;

	OnPlayerLeave( pSession );

	// Free session slot so it can be recycled
	m_IDSessions[ pSession->GetID() ] = NULL;

	return true;
}

//-----------------------------------------------------------------------------
// Name: SetupPlayer()
// Desc: Sends the necessary messages for the player setup
//-----------------------------------------------------------------------------
void CNetServer::SetupPlayer( CNetSession* pSession )
{
	CGameSetupMessage			gameSetup;
	CPlayerConfigMessage		playerConfig;
	CAssignPlayerSlotMessage	assignSlot;
	CPlayerJoinMessage			playerJoin;

	// Validate parameters
	if ( !pSession ) return;

	debug_assert( m_GameAttributes );

	// Send a new config message to the connected client
	BuildGameSetupMessage( &gameSetup );
	SendMessage( pSession, &gameSetup );

	// Add information for already connected clients and the server
	playerJoin.m_Clients.resize( GetSessionCount() );
	playerJoin.m_Clients[ 0 ].m_SessionID	= SERVER_SESSIONID;
	playerJoin.m_Clients[ 0 ].m_Name		= GetPlayerName();

	for ( uint i = 0; i < GetSessionCount(); i++ )
	{
		CNetSession* pCurrSession = GetSession( i );

		// Skip the player being setup
		if ( !pCurrSession || pCurrSession == pSession ) continue;

		playerJoin.m_Clients[ i + 1 ].m_SessionID	= pCurrSession->GetID();
		playerJoin.m_Clients[ i + 1 ].m_Name		= pCurrSession->GetName();
	}

	SendMessage( pSession, &playerJoin );

	// TODO: Handle observers

	// Send a message informing about the newly connected player
	playerJoin.m_Clients.resize( 1 );
	playerJoin.m_Clients[ 0 ].m_SessionID	= pSession->GetID();
	playerJoin.m_Clients[ 0 ].m_Name		= pSession->GetName();

	for ( uint i = 0; i < GetSessionCount(); i++ )
	{
		CNetSession* pCurrSession = GetSession( i );

		// Skip the player being setup
		if ( !pCurrSession || pCurrSession == pSession ) continue;

		SendMessage( pCurrSession, &playerJoin );
	}
	//Broadcast( &playerJoin );

	// Sync player slot assignments and player attributes
	for ( uint i = 0; i < m_GameAttributes->GetSlotCount(); i++ )
	{
		CPlayerSlot* pCurrSlot = m_GameAttributes->GetSlot( i );
		if ( !pCurrSlot ) continue;
	
		assignSlot.m_SlotID = (u32)pCurrSlot->GetSlotID();
		assignSlot.m_SessionID = (u32)pCurrSlot->GetSessionID();
		switch ( pCurrSlot->GetAssignment() )
		{
		case SLOT_CLOSED:
			assignSlot.m_Assignment = ASSIGN_CLOSED;
			break;

		case SLOT_OPEN:
			assignSlot.m_Assignment = ASSIGN_OPEN;
			break;

		case SLOT_SESSION:
			assignSlot.m_Assignment = ASSIGN_SESSION;
			break;

		case SLOT_AI:
			break;
		}

		SendMessage( pSession, &assignSlot );
	
		if ( pCurrSlot->GetAssignment() == SLOT_SESSION )
		{
			// Setup player
			BuildPlayerConfigMessage( &playerConfig, pCurrSlot->GetPlayer() );
			SendMessage( pSession, &playerConfig );
		}
	}
}

//-----------------------------------------------------------------------------
// Name: OnClientDisconnect()
// Desc: Called when a player joins the game
//-----------------------------------------------------------------------------
void CNetServer::OnPlayerJoin( CNetSession* pSession )
{
	if ( m_OnClientConnect.Defined() )
	{
		// Dispatch a client connect event
		CClientConnectEvent event( pSession );
		m_OnClientConnect.DispatchEvent( GetScript(), &event );
	}
}

//-----------------------------------------------------------------------------
// Name: OnClientDisconnect()
// Desc: Called when a player quits the game
//-----------------------------------------------------------------------------
void CNetServer::OnPlayerLeave( CNetSession* pSession )
{
	// Validate parameters
	if ( !pSession ) return;

	CPlayer*			pPlayer		= NULL;
	CPlayerSlot*		pPlayerSlot	= NULL;
	CPlayerLeaveMessage playerLeave;

	// Validate parameters
	if ( !pSession ) return;

	pPlayer		= pSession->GetPlayer();
	pPlayerSlot = pSession->GetPlayerSlot();

	switch ( m_State )
	{
	case SERVER_STATE_PREGAME:
				
		// Delete player's slot and sync client disconnection
		if ( pPlayerSlot ) pPlayerSlot->AssignClosed();
		break;

	case SERVER_STATE_INGAME:

		// Revert player entities to Gaia control and 
		// wait for client reconnection
		// TODO Reassign entities to Gaia control
		// TODO Set everything up for re-connect and resume
		if ( pPlayerSlot )
		{
			m_TurnManager->SetClientPipe( pPlayerSlot->GetSlotID(), NULL );
			pPlayerSlot->AssignClosed();
		}
		break;

	case SERVER_STATE_POSTGAME:

		// Synchronize disconnection
		break;
	}
	
	// Inform other clients about client disconnection
	playerLeave.m_SessionID = pSession->GetID();
	
	Broadcast( &playerLeave );

	// Script object defined?
	if ( m_OnClientDisconnect.Defined() )
	{
		// Dispatch a client disconnect event
		CClientDisconnectEvent event( pSession->GetID(), pSession->GetName() );
		m_OnClientDisconnect.DispatchEvent( GetScript(), &event );
	}
}

//-----------------------------------------------------------------------------
// Name: OnError()
// Desc:
//-----------------------------------------------------------------------------
bool CNetServer::OnError( void* pContext, CFsmEvent* pEvent )
{
	// Validate parameters
	if ( !pContext || !pEvent ) return false;

	// Error event?
	if ( pEvent->GetType() != (uint)NMT_ERROR ) return true;

	CNetServer*	 pServer  = ( CNetServer* )( ( FsmActionCtx* )pContext )->pHost;
	UNUSED2(pServer);
	CNetSession* pSession = ( ( FsmActionCtx* )pContext )->pSession;
	UNUSED2(pSession);

	CErrorMessage* pMessage = ( CErrorMessage* )pEvent->GetParamRef();
	if ( pMessage )
	{
		if ( pMessage->m_State == SS_UNCONNECTED )
		{
			//if ( pSession->GetID() != -1)
			//	pServer->RemoveSession( pSession );

			//delete pSession;
		}
		else
		{
			// Weird stuff...
			LOG( CLogger::Warning, LOG_CATEGORY, L"NMT_ERROR: %hs", pMessage->ToString().c_str() );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Name: OnHandshake()
// Desc:
//-----------------------------------------------------------------------------
bool CNetServer::OnHandshake( void* pContext, CFsmEvent* pEvent )
{
	// Validate parameters
	if ( !pEvent || !pContext ) return false;

	// Client handshake event?
	if ( pEvent->GetType() != NMT_CLIENT_HANDSHAKE ) return true;

	CNetServer*		pServer  = ( CNetServer* )( ( FsmActionCtx* )pContext )->pHost;
	CNetSession*	pSession = ( ( FsmActionCtx* )pContext )->pSession;

	debug_assert( pServer );
	debug_assert( pSession );

	CCliHandshakeMessage* pMessage = ( CCliHandshakeMessage* )pEvent->GetParamRef();
	if ( pMessage )
	{
		if ( pMessage->m_ProtocolVersion != PS_PROTOCOL_VERSION )
		{
			CCloseRequestMessage closeRequest;
			pServer->SendMessage( pSession, &closeRequest );

			CErrorMessage error( PS_OK, SS_UNCONNECTED );
			pSession->Update( ( uint )NMT_ERROR, &error );
		}
		else
		{
			CSrvHandshakeResponseMessage handshakeResponse;
			handshakeResponse.m_UseProtocolVersion = PS_PROTOCOL_VERSION;
			handshakeResponse.m_Message			   = pServer->m_WelcomeMessage;
			handshakeResponse.m_Flags			   = 0;
			pServer->SendMessage( pSession, &handshakeResponse );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Name: OnAuthenticate()
// Desc:
//-----------------------------------------------------------------------------
bool CNetServer::OnAuthenticate( void* pContext, CFsmEvent* pEvent )
{
	// Validate parameters
	if ( !pContext || !pEvent ) return false;

	// Authenticate event?
	if ( pEvent->GetType() != NMT_AUTHENTICATE ) return true;

	CNetServer*	 pServer  = ( CNetServer* )( ( FsmActionCtx* )pContext )->pHost;
	CNetSession* pSession = ( ( FsmActionCtx* )pContext )->pSession;

	CAuthenticateMessage* pMessage = ( CAuthenticateMessage* )pEvent->GetParamRef();
	if ( pMessage )
	{
		if ( pMessage->m_Password == pServer->m_PlayerPassword )
		{
			LOG( CLogger::Normal, LOG_CATEGORY, L"Player authentication successful");
			
			pSession->SetName( pMessage->m_Name );		
			pSession->SetID( pServer->GetFreeSessionID() );

			// Store new session
			//pServer->m_IDSessions[ pSession->GetID() ] = pSession;

			CAuthenticateResultMessage authenticateResult;
			authenticateResult.m_Code		= ARC_OK;
			authenticateResult.m_SessionID	= pSession->GetID();
			authenticateResult.m_Message	= L"Logged in";
			pServer->SendMessage( pSession, &authenticateResult );

			//pServer->AddSession( pSession );

			//if ( pServer->GetServerState() == NSS_PreGame )
			//{
				pServer->SetupPlayer( pSession );
			//}
			//else
			//{
				// Chatter / observer
			//	SetupObserver( pSession );
			//}

			pServer->OnPlayerJoin( pSession );
		}
		else
		{
			LOG( CLogger::Warning, LOG_CATEGORY, L"Player authentication failed" );

			CAuthenticateResultMessage authenticateResult;
			authenticateResult.m_Code		= ARC_PASSWORD_INVALID;
			authenticateResult.m_SessionID	= 0;
			authenticateResult.m_Message	= L"Invalid Password";
			pServer->SendMessage( pSession, &authenticateResult );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Name: OnPreGame()
// Desc:
//-----------------------------------------------------------------------------
bool CNetServer::OnPreGame( void* pContext, CFsmEvent* pEvent )
{
	// Validate parameters
	if ( !pContext || !pEvent ) return false;

	return CNetServer::OnChat( pContext, pEvent );
}

//-----------------------------------------------------------------------------
// Name: OnInGame()
// Desc:
//-----------------------------------------------------------------------------
bool CNetServer::OnInGame( void* pContext, CFsmEvent* pEvent )
{
	// Validate parameters
	if ( !pContext || !pEvent ) return false;

	CNetServer* pServer		= ( CNetServer* )( ( FsmActionCtx* )pContext )->pHost;
	CNetSession* pSession	= ( ( FsmActionCtx* )pContext )->pSession;

	CNetMessage* pMessage = ( CNetMessage* )pEvent->GetParamRef();
	if ( pMessage )
	{
		if (pMessage->GetType() == NMT_SIMULATION_COMMAND)
		{
			CSimulationMessage* simMessage = static_cast<CSimulationMessage*> (pMessage);
			pServer->m_ServerTurnManager->OnSimulationMessage(simMessage);
			return true;
		}

		if ( pMessage->GetType() >= NMT_COMMAND_FIRST && pMessage->GetType() < NMT_COMMAND_LAST )
		{
			//pSession->m_pPlayer->ValidateCommand(pMsg);
			pServer->QueueIncomingCommand( pMessage );
			
			return true;
		}

		if ( pMessage->GetType() == NMT_END_COMMAND_BATCH )
		{
			if (g_UseSimulation2)
			{
				CEndCommandBatchMessage* endMessage = static_cast<CEndCommandBatchMessage*> (pMessage);
				pServer->m_ServerTurnManager->NotifyFinishedClientCommands(pSession->GetID(), endMessage->m_Turn);
			}

			// TODO Update client timing information and recalculate turn length
			pSession->SetReadyForTurn( true );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Name: OnChat()
// Desc:
//-----------------------------------------------------------------------------
bool CNetServer::OnChat( void* pContext, CFsmEvent* pEvent )
{
	// Validate parameters
	if ( !pContext || !pEvent ) return false;

	// Chatting?
	if ( pEvent->GetType() != NMT_CHAT ) return true;

	CNetServer*	 pServer  = ( CNetServer* )( ( FsmActionCtx* )pContext )->pHost;
	CNetSession* pSession = ( ( FsmActionCtx* )pContext )->pSession;

	debug_assert( pSession );
	debug_assert( pServer );

	CChatMessage* pMessage = ( CChatMessage* )pEvent->GetParamRef();
	if ( pMessage )
	{
		pMessage->m_Sender = pSession->GetName();
		
		g_Console->ReceivedChatMessage( pSession->GetName(), pMessage->m_Message.c_str() );
		
		pServer->OnPlayerChat( pMessage->m_Sender, pMessage->m_Message );
		
		( ( CNetHost*)pServer )->Broadcast( pMessage );
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Name: GetSessionByID()
// Desc: Retrieve the session by its ID
//-----------------------------------------------------------------------------
CNetSession* CNetServer::GetSessionByID( uint sessionID )
{
	// Lookup session
	IDSessionMap::const_iterator it = m_IDSessions.find( sessionID );
	if ( it == m_IDSessions.end() ) return NULL;

	return it->second;
}

//-----------------------------------------------------------------------------
// Name: AddSession()
// Desc: Adds a new session to the list of managed sessions
//-----------------------------------------------------------------------------
/*void CNetServer::AddSession( CNetSession* pSession )
{
	// Validate parameter
	if ( !pSession ) return;

	// Setup new session
	//SetupNewSession();

	// Broadcase a new message informing about the newly connected client
	CPlayerJoinMessage playerJoin;
	playerJoin.m_Clients.resize( 1 );
	playerJoin.m_Clients[ 0 ].m_SessionID	= pSession->GetID();
	playerJoin.m_Clients[ 0 ].m_Name		= pSession->GetName();

	Broadcast( playerJoin );

	// Store new session
	m_IDSessions[ pSession->GetID() ] = pSession;
}*/

//-----------------------------------------------------------------------------
// Name: RemoveSession()
// Desc: Removes the specified session from the list of sessions
//-----------------------------------------------------------------------------
/*CNetSession* CNetServer::RemoveSession( CNetSession* pSession )
{
	CPlayer*			pPlayer		= NULL;
	CPlayerSlot*		pPlayerSlot	= NULL;
	CPlayerLeaveMessage playerLeave;
	uint				sessionID;

	// Validate parameters
	if ( !pSession ) return;

	pPlayer		= pSession->GetPlayer();
	pPlayerSlot = pSession->GetPlayerSlot();
	sessionID	= pSession->GetID();

	switch ( m_State )
	{
	case SERVER_STATE_PREGAME:
				
		// Delete player's slot and sync client disconnection
		if ( pPlayerSlot ) pPlayerSlot->AssignClosed();

		break;

	case SERVER_STATE_INGAME:

		// Revert player entities to Gaia control and 
		// wait for client reconnection
		// TODO Reassign entities to Gaia control
		// TODO Set everything up for re-connect and resume
		if ( pPlayerSlot )
		{
			SetClientPipe( pPlayerSlot->GetSlotID(), NULL );
			pPlayerSlot->AssignClosed();
		}

		break;

	case SERVER_STATE_POSTGAME:

		// Synchronize disconnection

		break;
	}
	
	// Inform other clients about client disconnection
	playerLeave.m_SessionID = sessionID;
	
	Broadcast( playerLeave );

	// Free session slot from the list for later reuse
	m_IDSessions[ sessionID ] = NULL;

	// TODO Handle observers
}*/

//-----------------------------------------------------------------------------
// Name: RemoveAllSessions()
// Desc: Removes all sessions from the list
//-----------------------------------------------------------------------------
/*void CNetServer::RemoveAllSessions( void )
{
	SessionList::iterator it = m_Sessions.begin();
	for ( ; it != m_Sessions.end(); it++ )
	{
		CNetSession* pCurrSession = it->second;
		if ( !pCurrSession ) continue;

		RemoveSession( pCurrSession );
	}

	m_Sessions.clear();
}*/

//-----------------------------------------------------------------------------
// Name: SetPlayerPassword()
// Desc: Sets player new password
//-----------------------------------------------------------------------------
void CNetServer::SetPlayerPassword( const CStr& password )
{
	m_PlayerPassword = password;
}

//-----------------------------------------------------------------------------
// Name: StartGame()
// Desc:
//-----------------------------------------------------------------------------
int CNetServer::StartGame( void )
{
	uint i;

	debug_assert( m_Game );
	debug_assert( m_GameAttributes );

	if ( m_Game->StartGame( m_GameAttributes ) != PSRETURN_OK ) return -1;

	if (g_UseSimulation2)
	{
		m_ServerTurnManager = new CNetServerTurnManager(*m_Game->GetSimulation2(), *this, m_Game->GetLocalPlayer()->GetPlayerID(), SERVER_SESSIONID);
		m_Game->SetTurnManager(m_ServerTurnManager);
	}

	m_TurnManager->Initialize( m_GameAttributes->GetSlotCount() );

	for ( i = 0; i < m_GameAttributes->GetSlotCount(); i++ )
	{
		CPlayerSlot* pCurrSlot = m_GameAttributes->GetSlot( i );
		if ( !pCurrSlot ) continue;

		if ( pCurrSlot->GetAssignment() == SLOT_SESSION )
		{
			m_TurnManager->SetClientPipe( i, pCurrSlot->GetSession() );
			if (g_UseSimulation2)
				m_ServerTurnManager->InitialiseClient(pCurrSlot->GetSessionID());
		}
	}

	m_State = SERVER_STATE_INGAME;

	for ( i = 0; i < GetSessionCount(); i++ )
	{
		CNetSession* pCurrSession = GetSession( i );
		if ( !pCurrSession ) continue;

		pCurrSession->StartGame();
	}
		
	CGameStartMessage gameStart;
	Broadcast( &gameStart );

	// This is the signal for everyone to start their simulations.
	//SendBatch( 1 );

	return 0;
}

//-----------------------------------------------------------------------------
// Name: BuildGameSetupMessage()
// Desc: Loads the game properties into the specified message
//-----------------------------------------------------------------------------
void CNetServer::BuildGameSetupMessage( CGameSetupMessage* pMessage )
{
	// Validate parameters
	if ( !pMessage ) return;

	debug_assert( m_GameAttributes );

	// Iterate through game properties and load them into message
	m_GameAttributes->IterateSynchedProperties( GameSetupMessageCallback, pMessage );
}

//-----------------------------------------------------------------------------
// Name: GameSetupMessageCallback()
// Desc: Callback called for each game attribute
//-----------------------------------------------------------------------------
void CNetServer::GameSetupMessageCallback( 
										  const CStrW& name,
										  ISynchedJSProperty* pProperty,
										  void *pData )
{
	// Validate parameters
	if ( !pProperty || !pData ) return;

	CGameSetupMessage* pMessage = ( CGameSetupMessage* )pData;

	// Add new property to list
	size_t valueCount = pMessage->m_Values.size();
	pMessage->m_Values.resize( valueCount + 1 );

	// Store property into list
	pMessage->m_Values[ valueCount ].m_Name	 = name;
	pMessage->m_Values[ valueCount ].m_Value = pProperty->ToString();
}

//-----------------------------------------------------------------------------
// Name: BuildPlayerConfigMessage()
// Desc: Loads the player properties into the specified message
//-----------------------------------------------------------------------------
void CNetServer::BuildPlayerConfigMessage(
										  CPlayerConfigMessage* pMessage,
										  CPlayer* pPlayer )
{
	// Validare parameters
	if ( !pMessage || !pPlayer ) return;

	pMessage->m_PlayerID = (u32)pPlayer->GetPlayerID();

	// Iterate through player properties and load them into message
	pPlayer->IterateSynchedProperties( PlayerConfigMessageCallback, pMessage );
}

//-----------------------------------------------------------------------------
// Name: PlayerConfigMessageCallback()
// Desc: Callback called for each property of the player
//-----------------------------------------------------------------------------
void CNetServer::PlayerConfigMessageCallback(
											 const CStrW& name,
											 ISynchedJSProperty* pProperty,
											 void* pData )
{
	// Validate parameters
	if ( !pProperty || !pData ) return;

	CPlayerConfigMessage* pMessage = ( CPlayerConfigMessage* )pData;

	// Add new property to list
	size_t valueCount = pMessage->m_Values.size();
	pMessage->m_Values.resize( valueCount + 1 );

	// Store property into the list
	pMessage->m_Values[ valueCount ].m_Name  = name;
	pMessage->m_Values[ valueCount ].m_Value = pProperty->ToString();
}

//-----------------------------------------------------------------------------
// Name: GetFreeSessionID()
// Desc:
//-----------------------------------------------------------------------------
uint CNetServer::GetFreeSessionID( void ) const
{
	// No need to be conservative with session IDs; just use a global counter.
	static uint lastSessionID = CLIENT_MIN_SESSIONID;
	return lastSessionID++;

	/*
	// Loop through the list of sessions and return the first
	// ID for which the associated session is NULL. If no such
	// free slot is found, return a new session ID which is higher
	// than the last session ID from the list.
	IDSessionMap::const_iterator it = m_IDSessions.begin();
	for ( ; it != m_IDSessions.end(); it++ )
	{
		CNetSession* pCurrSession = it->second;
		if ( !pCurrSession ) return it->first;

		sessionID++;
	}

	return sessionID;
	*/
}

//-----------------------------------------------------------------------------
// Name: AttributeUpdate()
// Desc:
//-----------------------------------------------------------------------------
void CNetServer::AttributeUpdate(
								 const CStrW& name, 
								 const CStrW& newValue, 
								 void* pData )
{
	// Validate parameters
	if ( !pData ) return;

	CNetServer* pServer = ( CNetServer* )pData;
	
	g_Console->InsertMessage( L"AttributeUpdate: %ls = \"%ls\"", name.c_str(), newValue.c_str() );

	CGameSetupMessage gameSetup;
	gameSetup.m_Values.resize( 1 );
	gameSetup.m_Values[ 0 ].m_Name  = name;
	gameSetup.m_Values[ 0 ].m_Value = newValue;
	pServer->Broadcast( &gameSetup );
}

//-----------------------------------------------------------------------------
// Name: PlayerAttributeUpdate()
// Desc: 
//-----------------------------------------------------------------------------
void CNetServer::PlayerAttributeUpdate(
									   const CStrW& name, 
									   const CStrW& newValue, 
									   CPlayer* pPlayer, 
									   void* pData )
{
	// Validate parameters
	if ( !pData ) return;

	CNetServer* pServer = ( CNetServer* )pData;

	g_Console->InsertMessage( L"PlayerAttributeUpdate(%ld): %ls = \"%ls\"", pPlayer->GetPlayerID(), name.c_str(), newValue.c_str() );

	CPlayerConfigMessage* pNewMessage = new CPlayerConfigMessage;
	if ( !pNewMessage ) return;

	pNewMessage->m_PlayerID = (u32)pPlayer->GetPlayerID();
	pNewMessage->m_Values.resize( 1 );
	pNewMessage->m_Values[ 0 ].m_Name  = name;
	pNewMessage->m_Values[ 0 ].m_Value = newValue;

	pServer->Broadcast( pNewMessage );
}

//-----------------------------------------------------------------------------
// Name: PlayerSlotAssignment()
// Desc:
//-----------------------------------------------------------------------------
void CNetServer::PlayerSlotAssignment( 
									  void* pData,
									  CPlayerSlot* pPlayerSlot )
{
	// Validate parameters
	if ( !pData || !pPlayerSlot ) return;

	CNetServer* pServer = ( CNetServer* )pData;

	if ( pPlayerSlot->GetAssignment() == SLOT_SESSION )
		pPlayerSlot->GetSession()->SetPlayerSlot( pPlayerSlot );

	CAssignPlayerSlotMessage assignPlayerSlot;
	
	pServer->BuildPlayerSlotAssignmentMessage( &assignPlayerSlot, pPlayerSlot );

	g_Console->InsertMessage( L"Player Slot Assignment: %hs\n", assignPlayerSlot.ToString().c_str() );
	
	pServer->Broadcast( &assignPlayerSlot );
}

//-----------------------------------------------------------------------------
// Name: AllowObserver()
// Desc:
//-----------------------------------------------------------------------------
bool CNetServer::AllowObserver( CNetSession* UNUSED( pSession ) )
{
	return m_Observers.size() < m_MaxObservers;
}

//-----------------------------------------------------------------------------
// Name: NewTurnReady()
// Desc:
//-----------------------------------------------------------------------------
bool CServerTurnManager::NewTurnReady()
{
	// Check whether all sessions are ready for the next turn
	for ( uint i = 0; i < m_Server.GetSessionCount(); i++ )
	{
		CNetSession* pCurrSession = m_Server.GetSession( i );
		if ( !pCurrSession ) continue;

		if ( !pCurrSession->IsReadyForTurn() )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Name: NewTurn()
// Desc:
//-----------------------------------------------------------------------------
void CServerTurnManager::NewTurn()
{
	CScopeLock lock(m_Server.m_Mutex);
	
	// Reset session ready for next turn flag
	for ( uint i = 0; i < m_Server.GetSessionCount(); i++ )
	{
		CNetSession* pCurrSession = m_Server.GetSession( i );
		if ( !pCurrSession ) continue;

		pCurrSession->SetReadyForTurn( false );
	}
	
	RecordBatch( 2 );
	RotateBatches();
	ClearBatch( 2 );
	IterateBatch( 1, CSimulation::GetMessageMask, m_Server.m_Game->GetSimulation() );
	SendBatch( 1 );
	//IterateBatch( 1, SendToObservers, this );
}

//-----------------------------------------------------------------------------
// Name: QueueLocalCommand()
// Desc:
//-----------------------------------------------------------------------------
void CServerTurnManager::QueueLocalCommand( CNetMessage *pMessage )
{
	// Validate parameters
	if ( !pMessage ) return;

	//LOG( NORMAL, LOG_CATEGORY, L"CServerTurnManager::QueueLocalCommand(): %hs.", pMessage->ToString().c_str() );

	QueueMessage( 2, pMessage );
}

//-----------------------------------------------------------------------------
// Name: QueueIncomingCommand()
// Desc:
//-----------------------------------------------------------------------------
void CNetServer::QueueIncomingCommand( CNetMessage* pMessage )
{
	// Validate parameters
	if ( !pMessage ) return;

	//LOG( NORMAL, LOG_CATEGORY, L"CNetServer::QueueIncomingCommand(): %hs.", pMessage->ToString().c_str() );

	m_TurnManager->QueueMessage( 2, pMessage );
}

//-----------------------------------------------------------------------------
// Name: OnChat()
// Desc:
//-----------------------------------------------------------------------------
void CNetServer::OnPlayerChat( const CStrW& from, const CStrW& message )
{
	if ( m_OnChat.Defined() )
	{
		CChatEvent event( from, message );
		m_OnChat.DispatchEvent( GetScript(), &event );
	}
}

//-----------------------------------------------------------------------------
// Name: BuildPlayerSlotAssignementMessage()
// Desc: 
//-----------------------------------------------------------------------------
void CNetServer::BuildPlayerSlotAssignmentMessage( 
												CAssignPlayerSlotMessage* pMessage,
												CPlayerSlot* pPlayerSlot )
{
	// Validate parameters
	if ( !pMessage || !pPlayerSlot ) return;

	pMessage->m_SlotID	 = (u32)pPlayerSlot->GetSlotID();
	pMessage->m_SessionID = pPlayerSlot->GetSessionID();

	switch ( pPlayerSlot->GetAssignment() )
	{
	case SLOT_CLOSED:
		pMessage->m_Assignment = ASSIGN_CLOSED;
		break;

	case SLOT_OPEN:
		pMessage->m_Assignment = ASSIGN_OPEN;
		break;

	case SLOT_SESSION:
		pMessage->m_Assignment = ASSIGN_SESSION;
		break;
	}
}
