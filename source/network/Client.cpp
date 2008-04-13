#include "precompiled.h"

#include "scripting/DOMEvent.h"
#include "scripting/JSConversions.h"
#include "scripting/ScriptableObject.h"
#include "Client.h"
#include "JSEvents.h"
#include "ps/CStr.h"
#include "ps/CLogger.h"
#include "ps/CConsole.h"
#include "ps/Game.h"
#include "ps/Globals.h"	// g_frequencyFilter
#include "ps/GameAttributes.h"
#include "simulation/Simulation.h"

#define LOG_CAT_NET "net"

CNetClient *g_NetClient=NULL;

CNetClient::CServerSession::CServerSession(int sessionID, const CStrW& name):
	m_SessionID(sessionID),
	m_Name(name)
{
}

void CNetClient::CServerSession::ScriptingInit()
{
	AddProperty(L"id", &CNetClient::CServerSession::m_SessionID, true);
	AddProperty(L"name", &CNetClient::CServerSession::m_Name, true);

	CJSObject<CServerSession>::ScriptingInit("NetClient_ServerSession");
}

CNetClient::CNetClient(CGame *pGame, CGameAttributes *pGameAttribs):
	CNetSession(ConnectHandler),
	m_JSI_ServerSessions(&m_ServerSessions),
	m_pLocalPlayerSlot(NULL),
	m_pGame(pGame),
	m_pGameAttributes(pGameAttribs),
	m_TurnPending(false)
{
	m_pGame->GetSimulation()->SetTurnManager(this);
	
	g_ScriptingHost.SetGlobal("g_NetClient", OBJECT_TO_JSVAL(GetScript()));
}

CNetClient::~CNetClient()
{
	g_ScriptingHost.SetGlobal("g_NetClient", JSVAL_NULL);
	
	SessionMap::iterator it=m_ServerSessions.begin();
	for (; it != m_ServerSessions.end(); ++it)
	{
		delete it->second;
	}
}

void CNetClient::ScriptingInit()
{
	AddMethod<bool, &CNetClient::JSI_BeginConnect>("beginConnect", 1);

	AddProperty(L"onStartGame", &CNetClient::m_OnStartGame);
	AddProperty(L"onChat", &CNetClient::m_OnChat);
	AddProperty(L"onConnectComplete", &CNetClient::m_OnConnectComplete);
	AddProperty(L"onDisconnect", &CNetClient::m_OnDisconnect);
	AddProperty(L"onClientConnect", &CNetClient::m_OnClientConnect);
	AddProperty(L"onClientDisconnect", &CNetClient::m_OnClientDisconnect);

	AddProperty(L"password", &CNetClient::m_Password);
	AddProperty<CStrW>(L"playerName", &CNetClient::m_Name);
	AddProperty(L"sessionId", &CNetClient::m_SessionID);
	
	AddProperty(L"sessions", &CNetClient::m_JSI_ServerSessions);
	CJSMap<SessionMap>::ScriptingInit("NetClient_SessionMap");
	CJSObject<CNetClient>::ScriptingInit("NetClient");

	// Also initialize session objects
	CNetClient::CServerSession::ScriptingInit();
}

bool CNetClient::JSI_BeginConnect(JSContext* UNUSED(cx), uintN argc, jsval *argv)
{
	CStr connectHostName;
	uint connectPort=PS_DEFAULT_PORT;
	if (argc >= 1)
	{
		connectHostName=g_ScriptingHost.ValueToString(argv[0]);
	}
	if (argc >= 2)
	{
		connectPort=ToPrimitive<int>(argv[1]);
	}
	
	PS_RESULT res=BeginConnect(connectHostName.c_str(), connectPort);
	if (res != PS_OK)
	{
		LOG(CLogger::Error, LOG_CAT_NET, "CNetClient::JSI_Connect(): BeginConnect error: %s", res);
		return false;
	}
	else
		return true;
}

/* TEMPLATE FOR MESSAGE HANDLERS:

bool CNetClient::<X>Handler(CNetMessage *pMsg, CNetSession *pSession)
{
	CNetClient *pClient=(CNetClient *)pSession;
	switch (pMsg->GetType())
	{
	case XXX:
		break;
	default:
		UNHANDLED(pMsg);
	}
	HANDLED(pMsg);
}
*/

#define UNHANDLED(_pMsg) return false;
#define HANDLED(_pMsg) delete _pMsg; return true;
#define TAKEN(_pMsg) return true;
// Uglily assumes the arguments are called pMsg and pSession (which they are
// all through this file)
#define CHAIN(_chainHandler) STMT(if (_chainHandler(pMsg, pSession)) return true;)

bool CNetClient::ConnectHandler(CNetMessage *pMsg, CNetSession *pSession)
{
	CNetClient *pClient=(CNetClient *)pSession;

	switch (pMsg->GetType())
	{
	case NMT_CONNECT_COMPLETE:
		pClient->m_pMessageHandler=HandshakeHandler;
		if (pClient->m_OnConnectComplete.Defined())
		{
			CConnectCompleteEvent evt(CStr((char *)PS_OK), true);
			pClient->m_OnConnectComplete.DispatchEvent(pClient->GetScript(), &evt);
		}
		break;
	case NMT_ERROR:
	{
		CNetErrorMessage *msg=(CNetErrorMessage *)pMsg;
		LOG(CLogger::Error, LOG_CAT_NET, "CNetClient::ConnectHandler(): Connect Failed: %s", msg->m_Error);
		if (pClient->m_OnConnectComplete.Defined())
		{
			CConnectCompleteEvent evt(CStr(msg->m_Error), false);
			pClient->m_OnConnectComplete.DispatchEvent(pClient->GetScript(), &evt);
		}
		break;
	}
	default:
		UNHANDLED(pMsg);
	}
	HANDLED(pMsg);
}

bool CNetClient::BaseHandler(CNetMessage *pMsg, CNetSession *pSession)
{
	CNetClient *pClient=(CNetClient *)pSession;
	switch (pMsg->GetType())
	{
	case NMT_ERROR:
	{
		CNetErrorMessage *msg=(CNetErrorMessage *)pMsg;
		if (msg->m_State == SS_UNCONNECTED)
		{
			CStr message=msg->m_Error;
			CDisconnectEvent evt(message);
			if (pClient->m_OnDisconnect.Defined())
				pClient->m_OnDisconnect.DispatchEvent(pClient->GetScript(), &evt);
		}
		break;
	}
	default:
		UNHANDLED(pMsg);
	}
	HANDLED(pMsg);
}

bool CNetClient::HandshakeHandler(CNetMessage *pMsg, CNetSession *pSession)
{
	CNetClient *pClient=(CNetClient *)pSession;
	
	CHAIN(BaseHandler);
	
	switch (pMsg->GetType())
	{
	case NMT_ServerHandshake:
		{
			CClientHandshake *msg=new CClientHandshake();
			msg->m_MagicResponse=PS_PROTOCOL_MAGIC_RESPONSE;
			msg->m_ProtocolVersion=PS_PROTOCOL_VERSION;
			msg->m_SoftwareVersion=PS_PROTOCOL_VERSION;
			pClient->Push(msg);
			break;
		}
	case NMT_ServerHandshakeResponse:
		{
			CAuthenticate *msg=new CAuthenticate();
			msg->m_Name=pClient->m_Name;
			msg->m_Password=pClient->m_Password;

			pClient->m_pMessageHandler=AuthenticateHandler;
			pClient->Push(msg);

			break;
		}
	default:
		UNHANDLED(pMsg);
	}
	HANDLED(pMsg);
}

bool CNetClient::AuthenticateHandler(CNetMessage *pMsg, CNetSession *pSession)
{
	CNetClient *pClient=(CNetClient *)pSession;
	
	CHAIN(BaseHandler);
	
	switch (pMsg->GetType())
	{
	case NMT_AuthenticationResult:
		{
			CAuthenticationResult *msg=(CAuthenticationResult *)pMsg;
			if (msg->m_Code != NRC_OK)
			{
				LOG(CLogger::Error, LOG_CAT_NET, "CNetClient::AuthenticateHandler(): Authentication failed: %ls", msg->m_Message.c_str());
			}
			else
			{
				LOG(CLogger::Normal,  LOG_CAT_NET, "CNetClient::AuthenticateHandler(): Authenticated!");
				pClient->m_SessionID=msg->m_SessionID;
				pClient->m_pMessageHandler=PreGameHandler;
			}
			break;
		}
	default:
		UNHANDLED(pMsg);
	}
	HANDLED(pMsg);
}

bool CNetClient::PreGameHandler(CNetMessage *pMsg, CNetSession *pSession)
{
	CNetClient *pClient=(CNetClient *)pSession;
	
	CHAIN(BaseHandler);
	CHAIN(ChatHandler);
	
	switch (pMsg->GetType())
	{
		case NMT_StartGame:
		{
			pClient->OnStartGameMessage();
			HANDLED(pMsg);
		}
		case NMT_ClientConnect:
		{
			CClientConnect *msg=(CClientConnect *)pMsg;
			for (uint i=0;i<msg->m_Clients.size();i++)
			{
				pClient->OnClientConnect(msg->m_Clients[i].m_SessionID,
					msg->m_Clients[i].m_Name);
			}
			HANDLED(pMsg);
		}
		case NMT_ClientDisconnect:
		{
			CClientDisconnect *msg=(CClientDisconnect *)pMsg;
			pClient->OnClientDisconnect(msg->m_SessionID);
			HANDLED(pMsg);
		}
		case NMT_SetGameConfig:
		{
			CSetGameConfig *msg=(CSetGameConfig *)pMsg;
			for (uint i=0;i<msg->m_Values.size();i++)
			{
				pClient->m_pGameAttributes->SetValue(msg->m_Values[i].m_Name, msg->m_Values[i].m_Value);
			}
			HANDLED(pMsg);
		}
		case NMT_AssignPlayerSlot:
		{
			CAssignPlayerSlot *msg=(CAssignPlayerSlot *)pMsg;
			// FIXME Validate slot id to prevent us from going boom
			CPlayerSlot *pSlot=pClient->m_pGameAttributes->GetSlot(msg->m_SlotID);
			if (pSlot == pClient->m_pLocalPlayerSlot)
				pClient->m_pLocalPlayerSlot=NULL;
			switch (msg->m_Assignment)
			{
				case PS_ASSIGN_SESSION:
					if ((int)msg->m_SessionID == (int)pClient->m_SessionID)	// squelch bogus sign mismatch warning
						pClient->m_pLocalPlayerSlot=pSlot;
					pSlot->AssignToSessionID(msg->m_SessionID);
					break;
				case PS_ASSIGN_CLOSED:
					pSlot->AssignClosed();
					break;
				case PS_ASSIGN_OPEN:
					pSlot->AssignOpen();
					break;
				default:
					LOG(CLogger::Warning, LOG_CAT_NET, "CNetClient::PreGameHandler(): Received an invalid slot assignment %s", msg->GetString().c_str());
			}
			HANDLED(pMsg);
		}
		case NMT_SetPlayerConfig:
		{
			CSetPlayerConfig *msg=(CSetPlayerConfig *)pMsg;
			// FIXME Check player ID
			CPlayer *pPlayer=pClient->m_pGameAttributes->GetPlayer(msg->m_PlayerID);
			for (uint i=0;i<msg->m_Values.size();i++)
			{
				pPlayer->SetValue(msg->m_Values[i].m_Name, msg->m_Values[i].m_Value);
			}
			HANDLED(pMsg);
		}
		default:
		{
			UNHANDLED(pMsg);
		}
	}
}

bool CNetClient::InGameHandler(CNetMessage *pMsg, CNetSession *pSession)
{
	CNetClient *pClient=(CNetClient *)pSession;
	ENetMessageType msgType=pMsg->GetType();

	CHAIN(BaseHandler);
	CHAIN(ChatHandler);

	if (msgType >= NMT_COMMAND_FIRST && msgType < NMT_COMMAND_LAST)
	{
		pClient->QueueIncomingMessage(pMsg);
		TAKEN(pMsg);
	}

	switch (msgType)
	{
	case NMT_EndCommandBatch:
	{
		CEndCommandBatch *msg=(CEndCommandBatch *)pMsg;
		pClient->SetTurnLength(1, msg->m_TurnLength);
		
		//debug_printf("Got end of batch, setting NewTurnPending\n");
		pClient->m_TurnPending = true;
		// We will ack the turn when our simulation calls NewTurn.

		HANDLED(pMsg);
	}
		
	default:
		UNHANDLED(pMsg);
	}
}

void CNetClient::QueueIncomingMessage(CNetMessage *pMsg)
{
	CScopeLock lock(m_Mutex);
	debug_printf("Got a command! queueing it to 2 turns from now\n");
	QueueMessage(2, pMsg);
}

bool CNetClient::ChatHandler(CNetMessage *pMsg, CNetSession *pSession)
{
	CNetClient *pClient=(CNetClient *)pSession;
	switch (pMsg->GetType())
	{
	case NMT_ChatMessage:
	{
		CChatMessage *msg=(CChatMessage *)pMsg;
		g_Console->ReceivedChatMessage(msg->m_Sender, msg->m_Message);
		if (pClient->m_OnChat.Defined())
		{
			CChatEvent evt(msg->m_Sender, msg->m_Message);
			pClient->m_OnChat.DispatchEvent(pClient->GetScript(), &evt);
		}
		HANDLED(pMsg);
	}
	default:
		UNHANDLED(pMsg);
	}
}

void CNetClient::OnClientConnect(int sessionID, const CStrW& name)
{
	// Find existing server session, if any, and delete it
	SessionMap::iterator it=m_ServerSessions.find(sessionID);
	if (it != m_ServerSessions.end())
	{
		delete it->second;
		m_ServerSessions.erase(it);
	}
	
	// Insert new serversession
	m_ServerSessions[sessionID]=new CServerSession(sessionID, name);
	
	// Call JS Callback
	if (m_OnClientConnect.Defined())
	{
		CClientConnectEvent evt(sessionID, name);
		m_OnClientConnect.DispatchEvent(GetScript(), &evt);
	}
}

void CNetClient::OnClientDisconnect(int sessionID)
{
	SessionMap::iterator it=m_ServerSessions.find(sessionID);
	if (it == m_ServerSessions.end())
	{
		LOG(CLogger::Warning, LOG_CAT_NET, "Server said session %d disconnected. I don't know of such a session.");
	}
	
	// Call JS Callback
	if (m_OnClientConnect.Defined())
	{
		CClientDisconnectEvent evt(it->second->m_SessionID, it->second->m_Name);
		m_OnClientConnect.DispatchEvent(GetScript(), &evt);
	}
	
	// Delete server session and remove from map
	delete it->second;
	m_ServerSessions.erase(it);
}

void CNetClient::OnStartGameMessage()
{
	m_pMessageHandler=InGameHandler;
	debug_assert( m_OnStartGame.Defined() );
	CStartGameEvent evt;
	m_OnStartGame.DispatchEvent(GetScript(), &evt);
}

int CNetClient::StartGame()
{
	if (m_pGame->StartGame(m_pGameAttributes) != PSRETURN_OK)
	{
		// TODO: Send a failed-to-launch-game message and drop out.
		return -1;
	}
	else
	{
		debug_printf("Client StartGame - sending end-of-batch ack\n");
		// Send an end-of-batch message for turn 0 to signal that we're ready.
		CEndCommandBatch *pMsg=new CEndCommandBatch();
		pMsg->m_TurnLength=1000/g_frequencyFilter->StableFrequency();
		Push(pMsg);
		return 0;
	}
}

CPlayer* CNetClient::GetLocalPlayer()
{
	return m_pLocalPlayerSlot->GetPlayer();
}

bool CNetClient::NewTurnReady()
{
	return m_TurnPending;
}

void CNetClient::NewTurn()
{
	CScopeLock lock(m_Mutex);
	
	RotateBatches();
	ClearBatch(2);
	m_TurnPending = false;

	//debug_printf("In NewTurn - sending ack\n");
	CEndCommandBatch *pMsg=new CEndCommandBatch();
	pMsg->m_TurnLength=1000/g_frequencyFilter->StableFrequency();	// JW: it'd probably be nicer to get the FPS as a parameter
	Push(pMsg);
}

void CNetClient::QueueLocalCommand(CNetMessage *pMsg)
{
	// Don't save these locally, since they'll be bounced by the server anyway
	//debug_printf("Sending command from client\n");
	Push(pMsg);
}
