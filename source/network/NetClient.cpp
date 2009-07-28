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
 *	FILE			: NetClient.cpp
 *	PROJECT			: 0 A.D.
 *	DESCRIPTION		: Network client class implementation file
 *-----------------------------------------------------------------------------
 */

// INCLUDES
#include "precompiled.h"
#include "NetClient.h"
#include "NetJsEvents.h"
#include "Network.h"
#include "NetServer.h"
#include "scripting/DOMEvent.h"
#include "scripting/JSConversions.h"
#include "scripting/ScriptableObject.h"
#include "ps/CStr.h"
#include "ps/CLogger.h"
#include "ps/CConsole.h"
#include "ps/Game.h"
#include "ps/Globals.h"
#include "ps/GameAttributes.h"
#include "simulation/Simulation.h"

// DECLARATIONS

#define LOG_CAT_NET "net"

CNetClient *g_NetClient=NULL;
extern int fps;

//-----------------------------------------------------------------------------
// Name: CServerPlayer()
// Desc: Constructor
//-----------------------------------------------------------------------------
CServerPlayer::CServerPlayer( uint sessionID, const CStr& nickname )
{
	m_SessionID	= sessionID;
	m_Nickname	= nickname;
}

//-----------------------------------------------------------------------------
// Name: ~CServerPlayer()
// Desc: Destructor
//-----------------------------------------------------------------------------
CServerPlayer::~CServerPlayer( void )
{
}

//-----------------------------------------------------------------------------
// Name: ScriptingInit()
// Desc:
//-----------------------------------------------------------------------------
void CServerPlayer::ScriptingInit( void )
{
	AddProperty(L"id", &CServerPlayer::m_SessionID, true);
	AddProperty(L"name", &CServerPlayer::m_Nickname, true);

	CJSObject<CServerPlayer>::ScriptingInit( "NetClient_ServerSession" );
}

//-----------------------------------------------------------------------------
// Name: CNetClient()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetClient::CNetClient( CGame* pGame, CGameAttributes* pGameAttribs )
: m_JsPlayers( &m_Players )
{
	m_pLocalPlayerSlot = NULL;
	m_pGame = pGame;
	m_pGameAttributes = pGameAttribs;
	m_TurnPending = false;

	//ONCE( ScriptingInit(); );

	m_pGame->GetSimulation()->SetTurnManager(this);
	
	g_ScriptingHost.SetGlobal("g_NetClient", OBJECT_TO_JSVAL(GetScript()));
}

//-----------------------------------------------------------------------------
// Name: ~CNetClient()
// Desc: Destructor
//-----------------------------------------------------------------------------
CNetClient::~CNetClient()
{
	// Release resources
	PlayerMap::iterator it = m_Players.begin();
	for ( ; it != m_Players.end(); it++ )
	{
		CServerPlayer *pCurrPlayer = it->second;
		if ( pCurrPlayer ) delete pCurrPlayer;
	}

	m_Players.clear();

	g_ScriptingHost.SetGlobal("g_NetClient", JSVAL_NULL);
}

//-----------------------------------------------------------------------------
// Name: ScriptingInit()
// Desc:
//-----------------------------------------------------------------------------
void CNetClient::ScriptingInit()
{
	AddMethod<bool, &CNetClient::SetupConnection>("beginConnect", 1);

	AddProperty(L"onStartGame", &CNetClient::m_OnStartGame);
	AddProperty(L"onChat", &CNetClient::m_OnChat);
	AddProperty(L"onConnectComplete", &CNetClient::m_OnConnectComplete);
	AddProperty(L"onDisconnect", &CNetClient::m_OnDisconnect);
	AddProperty(L"onClientConnect", &CNetClient::m_OnPlayerJoin);
	AddProperty(L"onClientDisconnect", &CNetClient::m_OnPlayerLeave);

	AddProperty(L"password", &CNetClient::m_Password);
	AddProperty<CStr>(L"playerName", &CNetClient::m_Nickname);
	//AddProperty(L"sessionId", &CNetClient::m_SessionID);
	
	AddProperty(L"sessions", &CNetClient::m_JsPlayers);
	
	CJSMap< PlayerMap >::ScriptingInit("NetClient_SessionMap");
	CJSObject<CNetClient>::ScriptingInit("NetClient");
	//CGameAttributes::ScriptingInit();
	//CServerPlayer::ScriptingInit();
}

//-----------------------------------------------------------------------------
// Name: Run()
// Desc: Connect to server and start main loop
//-----------------------------------------------------------------------------
bool CNetClient::SetupConnection( JSContext* UNUSED(pContext), uintN argc, jsval* argv )
{
	uint port = DEFAULT_HOST_PORT;

	// Validate parameters
	if ( argc == 0 ) return false;

	// Build host information
	CStr host = g_ScriptingHost.ValueToString( argv[0] );
	if ( argc == 2 ) port = ToPrimitive< uint >( argv[ 1 ] );

	// Create client host
	if ( !Create() ) return false;

	// Connect to server
	return Connect( host, port );
}

//-----------------------------------------------------------------------------
// Name: SetupSession()
// Desc: Setup client session upon creation
//-----------------------------------------------------------------------------
bool CNetClient::SetupSession( CNetSession* pSession )
{
	// Validate parameters
	if ( !pSession ) return false;

	FsmActionCtx* pContext = new FsmActionCtx;
	if ( !pContext ) return false;

	pContext->pHost		= this;
	pContext->pSession	= pSession;

	// Setup transitions for session
	pSession->AddTransition( NCS_CONNECT, ( uint )NMT_SERVER_HANDSHAKE, NCS_HANDSHAKE, (void*)&OnHandshake, pContext );

	pSession->AddTransition( NCS_HANDSHAKE, ( uint )NMT_ERROR, NCS_CONNECT, (void*)&OnError, pContext );
	pSession->AddTransition( NCS_HANDSHAKE, ( uint )NMT_SERVER_HANDSHAKE_RESPONSE, NCS_AUTHENTICATE, (void*)&OnHandshake, pContext );

	pSession->AddTransition( NCS_AUTHENTICATE, ( uint )NMT_ERROR, NCS_CONNECT, (void*)&OnError, pContext );
	pSession->AddTransition( NCS_AUTHENTICATE, ( uint )NMT_AUTHENTICATE_RESULT, NCS_PREGAME, (void*)&OnAuthenticate, pContext );

	pSession->AddTransition( NCS_PREGAME, ( uint )NMT_ERROR, NCS_CONNECT, (void*)&OnError, pContext );
	pSession->AddTransition( NCS_PREGAME, ( uint )NMT_GAME_SETUP, NCS_PREGAME, (void*)&OnPreGame, pContext );
	pSession->AddTransition( NCS_PREGAME, ( uint )NMT_ASSIGN_PLAYER_SLOT, NCS_PREGAME, (void*)&OnPreGame, pContext );
	pSession->AddTransition( NCS_PREGAME, ( uint )NMT_PLAYER_CONFIG, NCS_PREGAME, (void*)&OnPreGame, pContext );
	pSession->AddTransition( NCS_PREGAME, ( uint )NMT_PLAYER_JOIN, NCS_PREGAME, (void*)&OnPlayerJoin, pContext );
	pSession->AddTransition( NCS_PREGAME, ( uint )NMT_GAME_START, NCS_INGAME, (void*)&OnStartGame, pContext );

	pSession->AddTransition( NCS_INGAME, ( uint )NMT_CHAT, NCS_INGAME, (void*)&OnChat, pContext );
	pSession->AddTransition( NCS_INGAME, ( uint )NMT_GOTO, NCS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NCS_INGAME, ( uint )NMT_PATROL, NCS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NCS_INGAME, ( uint )NMT_ADD_WAYPOINT, NCS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NCS_INGAME, ( uint )NMT_CONTACT_ACTION, NCS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NCS_INGAME, ( uint )NMT_PRODUCE, NCS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NCS_INGAME, ( uint )NMT_PLACE_OBJECT, NCS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NCS_INGAME, ( uint )NMT_RUN, NCS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NCS_INGAME, ( uint )NMT_SET_RALLY_POINT, NCS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NCS_INGAME, ( uint )NMT_SET_STANCE, NCS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NCS_INGAME, ( uint )NMT_NOTIFY_REQUEST, NCS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NCS_INGAME, ( uint )NMT_FORMATION_GOTO, NCS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NCS_INGAME, ( uint )NMT_FORMATION_CONTACT_ACTION, NCS_INGAME, (void*)&OnInGame, pContext );
	pSession->AddTransition( NCS_INGAME, ( uint )NMT_END_COMMAND_BATCH, NCS_INGAME, (void*)&OnInGame, pContext );

	// Set first state
	pSession->SetFirstState( NCS_CONNECT );

	return true;
}

//-----------------------------------------------------------------------------
// Name: HandleConnect()
// Desc: Called when the client successfully connected to server
//-----------------------------------------------------------------------------
bool CNetClient::HandleConnect( CNetSession* pSession )
{
	// Validate parameters
	if ( !pSession ) return false;

	return true;
}

//-----------------------------------------------------------------------------
// Name: HandleDisconnect()
// Desc: Called when the client disconnected from the server
//-----------------------------------------------------------------------------
bool CNetClient::HandleDisconnect( CNetSession *pSession )
{
	// Validate parameters
	if ( !pSession ) return false;

	return true;
}

//-----------------------------------------------------------------------------
// Name: OnError()
// Desc:
//-----------------------------------------------------------------------------
bool CNetClient::OnError( void* pContext, CFsmEvent* pEvent )
{
	// Validate parameters
	if ( !pEvent || !pContext ) return false;

	// Error event?
	if ( pEvent->GetType() != (uint)NMT_ERROR ) return true;

	CNetClient*	pClient = ( CNetClient* )( ( FsmActionCtx* )pContext )->pHost;
	assert( pClient );

	CErrorMessage* pMessage = ( CErrorMessage* )pEvent->GetParamRef();
	if ( pMessage )
	{
		LOG( CLogger::Error, LOG_CAT_NET, "CNetClient::OnError(): Error description %s", pMessage->m_Error );

		if ( pClient->m_OnConnectComplete.Defined() )
		{
			CConnectCompleteEvent connectComplete( ( CStrW )pMessage->m_Error, false );
			pClient->m_OnConnectComplete.DispatchEvent( pClient->GetScript(), &connectComplete );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Name: OnPlayer()
// Desc:
//-----------------------------------------------------------------------------
bool CNetClient::OnPlayerJoin( void* pContext, CFsmEvent* pEvent )
{
	// Validate parameters
	if ( !pEvent || !pContext ) return false;

	// Connect event?
	if ( pEvent->GetType() != NMT_PLAYER_JOIN ) return true;

	CNetClient* pClient = ( CNetClient* )( ( FsmActionCtx* )pContext )->pHost;
	assert( pClient );

	CPlayerJoinMessage* pMessage = ( CPlayerJoinMessage* )pEvent->GetParamRef();
	if ( pMessage )
	{
		for ( uint i = 0; i < pMessage->m_Clients.size(); i++ )
		{
			pClient->OnPlayer( pMessage->m_Clients[ i ].m_SessionID, pMessage->m_Clients[ i ].m_Name );
		}
			
		if ( pClient->m_OnConnectComplete.Defined() )
		{
			CConnectCompleteEvent connectComplete( ( CStrW )PS_OK, true );
			pClient->m_OnConnectComplete.DispatchEvent( pClient->GetScript(), &connectComplete );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Name: OnHandshake()
// Desc:
//-----------------------------------------------------------------------------
bool CNetClient::OnHandshake( void* pContext, CFsmEvent* pEvent )
{
	// Validate parameters
	if ( !pEvent || !pContext ) return false;

	CNetClient*  pClient	= ( CNetClient* )( ( FsmActionCtx* )pContext )->pHost;
	CNetSession* pSession	= ( ( FsmActionCtx* )pContext )->pSession;

	assert( pClient );
	assert( pSession );

	switch ( pEvent->GetType() )
	{
	//case NMT_ERROR:
		
	//	CNetClient::OnError( pContext, pEvent );
	//	break;

	case NMT_SERVER_HANDSHAKE:
		{
			CCliHandshakeMessage handshake;
			handshake.m_MagicResponse	= PS_PROTOCOL_MAGIC_RESPONSE;
			handshake.m_ProtocolVersion	= PS_PROTOCOL_VERSION;
			handshake.m_SoftwareVersion = PS_PROTOCOL_VERSION;
			( ( CNetHost* )pClient )->SendMessage( pSession, &handshake );
		}
		break;

	case NMT_SERVER_HANDSHAKE_RESPONSE:
		{
			CAuthenticateMessage authenticate;
			authenticate.m_Name		= pClient->m_Nickname;
			authenticate.m_Password = pClient->m_Password;
			( ( CNetHost* )pClient )->SendMessage( pSession, &authenticate );
		}
		break;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Name: OnAuthenticate()
// Desc:
//-----------------------------------------------------------------------------
bool CNetClient::OnAuthenticate( void* pContext, CFsmEvent* pEvent )
{
	// Validate parameters
	if ( !pEvent || !pContext ) return false;
	
	CNetClient*	 pClient	= ( CNetClient* )( ( FsmActionCtx* )pContext )->pHost;
	UNUSED2(pClient);
	CNetSession* pSession	= ( ( FsmActionCtx* )pContext )->pSession;

	assert( pClient );
	assert( pSession );

	if ( pEvent->GetType() == (uint)NMT_ERROR )
	{
		// return CNetClient::OnError( pContext, pEvent );
	}
	else if ( pEvent->GetType() == NMT_AUTHENTICATE_RESULT )
	{
		CAuthenticateResultMessage* pMessage =( CAuthenticateResultMessage* )pEvent->GetParamRef();
		if ( !pMessage ) return true;

		LOG(CLogger::Error, LOG_CAT_NET, "CNetClient::OnAuthenticate(): Authentication result: %ls", pMessage->m_Message.c_str() );

		pSession->SetID( pMessage->m_SessionID );

		LOG(CLogger::Error, LOG_CAT_NET, "CNetClient::OnAuthenticate(): My session ID is %d", pMessage->m_SessionID);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Name: OnPreGame()
// Desc:
//-----------------------------------------------------------------------------
bool CNetClient::OnPreGame( void* pContext, CFsmEvent* pEvent )
{
	// Validate parameters
	if ( !pEvent || !pContext ) return false;

	CNetClient*		pClient  = ( CNetClient* )( ( FsmActionCtx* )pContext )->pHost;
	CNetSession*	pSession = ( CNetSession* )( ( FsmActionCtx* )pContext )->pSession;

	assert( pClient );
	assert( pSession );

	switch ( pEvent->GetType() )
	{
	
	//CHAIN(BaseHandler);
	//CHAIN(ChatHandler);
	
//	case NMT_GAME_START:

//		pClient->StartGame();
//		break;

	case NMT_PLAYER_LEAVE:
		{
			CPlayerLeaveMessage* pMessage = ( CPlayerLeaveMessage* )pEvent->GetParamRef();
			if ( !pMessage ) return false;

			pClient->OnPlayerLeave( pMessage->m_SessionID );
		}
		break;

	case NMT_GAME_SETUP:
		{
			CGameSetupMessage* pMessage = ( CGameSetupMessage* )pEvent->GetParamRef();

			for ( uint i = 0; i < pMessage->m_Values.size(); i++ )
			{
				pClient->m_pGameAttributes->SetValue( pMessage->m_Values[ i ].m_Name, pMessage->m_Values[ i ].m_Value );
			}
		}
		break;

	case NMT_ASSIGN_PLAYER_SLOT:
		{
			CAssignPlayerSlotMessage* pMessage = ( CAssignPlayerSlotMessage* )pEvent->GetParamRef();

			// FIXME Validate slot id to prevent us from going boom
			CPlayerSlot* pSlot = pClient->m_pGameAttributes->GetSlot( pMessage->m_SlotID );
			if ( pSlot == pClient->m_pLocalPlayerSlot ) {
				pClient->m_pLocalPlayerSlot = NULL;
			}

			switch ( pMessage->m_Assignment )
			{
				case ASSIGN_SESSION:
					{
						// TODO: Check where is the best place to assign client's session ID
						if ( pSession->GetID() == pMessage->m_SessionID )
						{
							pClient->m_pLocalPlayerSlot = pSlot;
						}
						pSlot->AssignToSessionID( pMessage->m_SessionID );
					}
					break;

				case ASSIGN_CLOSED:
					pSlot->AssignClosed();
					break;

				case ASSIGN_OPEN:
					pSlot->AssignOpen();
					break;

				default:
					LOG( CLogger::Warning, LOG_CAT_NET, "Invalid slot assignment %s", pMessage->ToString().c_str() );
					break;
			}
		}
		break;

	case NMT_PLAYER_CONFIG:
		{
			CPlayerConfigMessage* pMessage = ( CPlayerConfigMessage* )pEvent->GetParamRef();

			// FIXME Check player ID
			CPlayer* pPlayer = pClient->m_pGameAttributes->GetPlayer( pMessage->m_PlayerID );

			for ( uint i = 0; i < pMessage->m_Values.size(); i++ )
			{
				pPlayer->SetValue( pMessage->m_Values[i].m_Name, pMessage->m_Values[i].m_Value );
			}	
		}
		break;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Name: OnInGame()
// Desc:
//-----------------------------------------------------------------------------
bool CNetClient::OnInGame( void *pContext, CFsmEvent* pEvent )
{
	// Validate parameters
	if ( !pEvent || !pContext ) return false;

	CNetClient* pClient = ( CNetClient* )( ( FsmActionCtx* )pContext )->pHost;

	CNetMessage* pMessage = ( CNetMessage* )pEvent->GetParamRef();
	if ( pMessage )
	{
		if ( pMessage->GetType() >= NMT_COMMAND_FIRST && pMessage->GetType() < NMT_COMMAND_LAST )
		{
			pClient->QueueIncomingMessage( pMessage );

			return true;
		}

		if ( pMessage->GetType() == NMT_END_COMMAND_BATCH )
		{
			CEndCommandBatchMessage* pMessage = ( CEndCommandBatchMessage* )pEvent->GetParamRef();
			if ( !pMessage ) return false;

			pClient->SetTurnLength( 1, pMessage->m_TurnLength );

			pClient->m_TurnPending = true;
		
			// FIXME When the command batch has ended, we should start accepting
			// commands for the next turn. This will be accomplished by calling
			// NewTurn. *BUT* we shouldn't prematurely proceed game simulation
			// since this will produce jerky playback (everything expects a sim
			// turn to have a certain duration).

			// We should make sure that any commands received after this message
			// are queued in the next batch (#2 instead of #1). If we're already
			// putting everything new in batch 2 - we should fast-forward a bit to
			// catch up with the server.
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Name: OnChat()
// Desc:
//-----------------------------------------------------------------------------
bool CNetClient::OnChat( void* pContext, CFsmEvent* pEvent )
{
	// Validate parameters
	if ( !pEvent || !pContext ) return false;

	CNetClient* pClient = ( CNetClient* )( ( FsmActionCtx* )pContext )->pHost;

	if ( pEvent->GetType() == NMT_CHAT )
	{
		CChatMessage* pMessage = ( CChatMessage* )pEvent->GetParamRef();
		if ( !pMessage ) return false;

		g_Console->ReceivedChatMessage( pMessage->m_Sender, pMessage->m_Message );
			
		if ( pClient->m_OnChat.Defined() )
		{
			CChatEvent evt( pMessage->m_Sender, pMessage->m_Message );
			pClient->m_OnChat.DispatchEvent( pClient->GetScript(), &evt );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Name: OnStartGame()
// Desc:
//-----------------------------------------------------------------------------
bool CNetClient::OnStartGame( void* pContext, CFsmEvent* pEvent )
{
	// Validate parameters
	if ( !pEvent || !pContext ) return false;

	CNetClient* pClient = ( CNetClient* )( ( FsmActionCtx* )pContext )->pHost;

	if ( pClient->m_OnStartGame.Defined() )
	{
		CStartGameEvent event;
		pClient->m_OnStartGame.DispatchEvent( pClient->GetScript(), &event );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Name: OnPlayerJoin()
// Desc:
//-----------------------------------------------------------------------------
void CNetClient::OnPlayer( uint ID, const CStr& name )
{
	CServerPlayer* pNewPlayer = new CServerPlayer( ID, name );
	if ( !pNewPlayer ) return;

	// Store new player
	m_Players[ ID ] = pNewPlayer;

	// Call JS Callback
	if ( m_OnPlayerJoin.Defined() )
	{
		CClientConnectEvent event( ID, name );
		m_OnPlayerJoin.DispatchEvent( GetScript(), &event );
	}
}

//-----------------------------------------------------------------------------
// Name: OnPlayerLeave()
// Desc:
//-----------------------------------------------------------------------------
void CNetClient::OnPlayerLeave( uint ID )
{
	// Lookup player
	PlayerMap::iterator it = m_Players.find( ID );
	if ( it == m_Players.end() )
	{
		LOG( CLogger::Warning, LOG_CAT_NET, "CNetClient::OnPlayerLeav(): No such player %d.", ID );
		return;
	}

	// Call JS Callback
	if ( m_OnPlayerLeave.Defined() && it->second )
	{
		CClientDisconnectEvent event( it->second->GetSessionID(), it->second->GetNickname() );
		m_OnPlayerLeave.DispatchEvent( GetScript(), &event );
	}

	// Remove player from internal map
	m_Players.erase( it );
}

//-----------------------------------------------------------------------------
// Name: StartGame()
// Desc:
//-----------------------------------------------------------------------------
int CNetClient::StartGame( void )
{
	assert ( m_pGame );
	assert ( m_pGameAttributes );

	if ( m_pGame->StartGame( m_pGameAttributes ) != PSRETURN_OK ) return -1;

	// Send an end-of-batch message for turn 0 to signal that we're ready.
	CEndCommandBatchMessage endBatch;
	endBatch.m_TurnLength = 1000 / g_frequencyFilter->StableFrequency();

	CNetSession* pSession = GetSession( 0 );
	CNetHost::SendMessage( pSession, &endBatch );

	return 0;
}

//-----------------------------------------------------------------------------
// Name: GetLocalPlayer()
// Desc:
//-----------------------------------------------------------------------------
CPlayer* CNetClient::GetLocalPlayer()
{
	return m_pLocalPlayerSlot->GetPlayer();
}

//-----------------------------------------------------------------------------
// Name: NewTurn()
// Desc:
//-----------------------------------------------------------------------------
void CNetClient::NewTurn()
{
	CScopeLock lock(m_Mutex);
	
	RotateBatches();
	ClearBatch(2);
	m_TurnPending = false;

	//debug_printf("In NewTurn - sending ack\n");
	CEndCommandBatchMessage* pMsg = new CEndCommandBatchMessage;
	pMsg->m_TurnLength=1000/g_frequencyFilter->StableFrequency();	// JW: it'd probably be nicer to get the FPS as a parameter

	CNetSession* pSession = GetSession( 0 );
	CNetHost::SendMessage( pSession, pMsg );
}

//-----------------------------------------------------------------------------
// Name: QueueLocalCommand()
// Desc:
//-----------------------------------------------------------------------------
void CNetClient::QueueLocalCommand( CNetMessage* pMessage )
{
	if ( !pMessage ) return;

	// Don't save these locally, since they'll be bounced by the server anyway
	CNetSession* pSession = GetSession( 0 );
	CNetHost::SendMessage( pSession, pMessage );
}

//-----------------------------------------------------------------------------
// Name: QueueIncomingMessage()
// Desc:
//-----------------------------------------------------------------------------
void CNetClient::QueueIncomingMessage( CNetMessage* pMessage )
{
	CScopeLock lock( m_Mutex );

	QueueMessage( 2, pMessage );
}
