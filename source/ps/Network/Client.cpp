#include "precompiled.h"

#include "lib.h"

#include <scripting/DOMEvent.h>
#include <scripting/JSConversions.h>
#include <scripting/ScriptableObject.h>
#include <Network/Client.h>
#include <Network/JSEvents.h>
#include <CStr.h>
#include <CLogger.h>
#include <CConsole.h>
#include <Game.h>

#define LOG_CAT_NET "net"

CNetClient *g_NetClient=NULL;
extern "C" int fps;
extern CConsole *g_Console;

CNetClient::CServerSession::CServerSession(int sessionID, const CStrW &name):
	m_SessionID(sessionID),
	m_Name(name)
{
	ONCE( ScriptingInit(); );
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
	m_pGameAttributes(pGameAttribs)
{
	ONCE( ScriptingInit(); );

	m_pGame->GetSimulation()->SetTurnManager(this);
	
	g_ScriptingHost.SetGlobal("g_NetClient", OBJECT_TO_JSVAL(GetScript()));
}

CNetClient::~CNetClient()
{
	g_ScriptingHost.SetGlobal("g_NetClient", JSVAL_NULL);
	
	SessionMap::iterator it=m_ServerSessions.begin();
	while (it != m_ServerSessions.end())
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
}

bool CNetClient::JSI_BeginConnect(JSContext *cx, uintN argc, jsval *argv)
{
	CStr connectHostName;
	uint connectPort=PS_DEFAULT_PORT;
	if (argc >= 1)
	{
		connectHostName=g_ScriptingHost.ValueToString(argv[0]);
	}
	if (argc >= 2)
	{
		connectPort=g_ScriptingHost.ValueToInt(argv[1]);
	}
	
	PS_RESULT res=BeginConnect(connectHostName.c_str(), connectPort);
	if (res != PS_OK)
	{
		LOG(ERROR, LOG_CAT_NET, "CNetClient::JSI_Connect(): BeginConnect error: %s", res);
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
		LOG(ERROR, LOG_CAT_NET, "CNetClient::ConnectHandler(): Connect Failed: %s", msg->m_Error);
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
				LOG(ERROR, LOG_CAT_NET, "CNetClient::AuthenticateHandler(): Authentication failed: %ls", msg->m_Message.c_str());
			}
			else
			{
				LOG(NORMAL, LOG_CAT_NET, "CNetClient::AuthenticateHandler(): Authenticated!");
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
			pClient->StartGame();
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
					if (msg->m_SessionID == pClient->m_SessionID)
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
					LOG(WARNING, LOG_CAT_NET, "CNetClient::PreGameHandler(): Received an invalid slot assignment %s", msg->GetString().c_str());
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
	}

	UNHANDLED(pMsg);
}

bool CNetClient::InGameHandler(CNetMessage *pMsg, CNetSession *pSession)
{
	CNetClient *pClient=(CNetClient *)pSession;
	ENetMessageType msgType=pMsg->GetType();

	CHAIN(BaseHandler);
	CHAIN(ChatHandler);

	if (msgType >= NMT_COMMAND_FIRST && msgType < NMT_COMMAND_LAST)
	{
		pClient->QueueMessage(1, pMsg);
		TAKEN(pMsg);
	}

	switch (msgType)
	{
	case NMT_EndCommandBatch:
		CEndCommandBatch *msg=(CEndCommandBatch *)pMsg;
		pClient->SetTurnLength(1, msg->m_TurnLength);
	
		// FIXME When the command batch has ended, we should start accepting
		// commands for the next turn. This will be accomplished by calling
		// NewTurn. *BUT* we shouldn't prematurely proceed game simulation
		// since this will produce jerky playback (everything expects a sim
		// turn to have a certain duration).

		// We should make sure that any commands received after this message
		// are queued in the next batch (#2 instead of #1). If we're already
		// putting everything new in batch 2 - we should fast-forward a bit to
		// catch up with the server.

		HANDLED(pMsg);
	}

	UNHANDLED(pMsg);
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
	}
	UNHANDLED(pMsg);
}

void CNetClient::OnClientConnect(int sessionID, const CStrW &name)
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
		LOG(WARNING, LOG_CAT_NET, "Server said session %d disconnected. I don't know of such a session.");
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

void CNetClient::StartGame()
{
	m_pMessageHandler=InGameHandler;
	m_pGame->StartGame(m_pGameAttributes);
	
	if (m_OnStartGame.Defined())
	{
		CStartGameEvent evt;
		m_OnStartGame.DispatchEvent(GetScript(), &evt);
	}
}

void CNetClient::NewTurn()
{
	RotateBatches();
	ClearBatch(2);

	CEndCommandBatch *pMsg=new CEndCommandBatch();
	pMsg->m_TurnLength=1000/fps;
	Push(pMsg);
}

void CNetClient::QueueLocalCommand(CNetMessage *pMsg)
{
	// Don't save these locally, since they'll be bounced by the server anyway
	Push(pMsg);
}
