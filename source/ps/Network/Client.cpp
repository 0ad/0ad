#include "precompiled.h"

#include "lib.h"

#include <scripting/DOMEvent.h>
#include <scripting/JSConversions.h>
#include <scripting/ScriptableObject.h>
#include <Network/Client.h>
#include <CStr.h>
#include <CLogger.h>
#include <CConsole.h>

#define LOG_CAT_NET "net"

CNetClient *g_NetClient=NULL;
extern "C" int fps;
extern CConsole *g_Console;

enum CClientEvents
{
	CLIENT_EVENT_START_GAME,
	CLIENT_EVENT_CHAT,
	CLIENT_EVENT_CONNECT_COMPLETE,
	CLIENT_EVENT_DISCONNECT,
	CLIENT_EVENT_LAST
};

class CStartGameEvent: public CScriptEvent
{
public:
	CStartGameEvent():
		CScriptEvent(L"startGame", false, CLIENT_EVENT_START_GAME)
	{}
};

class CChatEvent: public CScriptEvent
{
	CStrW m_Sender;
	CStrW m_Message;
	
public:
	CChatEvent(CStrW sender, CStrW message):
		CScriptEvent(L"chat", false, CLIENT_EVENT_CHAT),
		m_Sender(sender),
		m_Message(message)
	{
		AddReadOnlyProperty(L"sender", &m_Sender);
		AddReadOnlyProperty(L"message", &m_Message);
	}
};

class CConnectCompleteEvent: public CScriptEvent
{
	CStrW m_Message;
	bool m_Success;
	
public:
	CConnectCompleteEvent(CStrW message, bool success):
		CScriptEvent(L"connectComplete", false, CLIENT_EVENT_CONNECT_COMPLETE),
		m_Message(message),
		m_Success(success)
	{
		AddReadOnlyProperty(L"message", &m_Message);
		AddReadOnlyProperty(L"success", &m_Success);
	}
};

class CDisconnectEvent: public CScriptEvent
{
	CStrW m_Message;
	
public:
	CDisconnectEvent(CStrW message):
		CScriptEvent(L"disconnect", false, CLIENT_EVENT_DISCONNECT),
		m_Message(message)
	{
		AddReadOnlyProperty(L"message", &m_Message);
	}
};

CNetClient::CNetClient(CGame *pGame, CGameAttributes *pGameAttribs):
	CNetSession(ConnectHandler),
	m_pLocalPlayer(NULL),
	m_pGame(pGame),
	m_pGameAttributes(pGameAttribs)
{
	ONCE(
		// This one's funny: if you remove the parantheses around this stmt
		// the preprocessor will take the comma inside the template decl and
		// interpret it as a macro argument delimiter ;-)
		(AddMethod<bool, &CNetClient::JSI_BeginConnect>("beginConnect", 1));

		CJSObject<CNetClient>::ScriptingInit("NetClient");
	);

	m_pGame->GetSimulation()->SetTurnManager(this);
	
	AddProperty(L"onStartGame", &m_OnStartGame);
	AddProperty(L"onChat", &m_OnChat);
	AddProperty(L"onConnectComplete", &m_OnConnectComplete);
	
	AddProperty(L"password", &m_Password);
	AddProperty(L"playerName", &m_Name);
	
	g_ScriptingHost.SetGlobal("g_NetClient", OBJECT_TO_JSVAL(GetScript()));
}

CNetClient::~CNetClient()
{
	g_ScriptingHost.SetGlobal("g_NetClient", JSVAL_NULL);
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
	LOG(NORMAL, LOG_CAT_NET, "CNetClient::<X>Handler(): %s.", pMsg->GetString().c_str());
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

	LOG(NORMAL, LOG_CAT_NET, "CNetClient::ConnectHandler(): %s.", pMsg->GetString().c_str());
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
	LOG(NORMAL, LOG_CAT_NET, "CNetClient::BaseHandler(): %s.", pMsg->GetString().c_str());
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
	
	LOG(NORMAL, LOG_CAT_NET, "CNetClient::HandshakeHandler(): %s.", pMsg->GetString().c_str());
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
	
	LOG(NORMAL, LOG_CAT_NET, "CNetClient::AuthenticateHandler(): %s.", pMsg->GetString().c_str());
	switch (pMsg->GetType())
	{
	case NMT_Result:
		{
			CResult *msg=(CResult *)pMsg;
			if (msg->m_Code != NRC_OK)
			{
				LOG(ERROR, LOG_CAT_NET, "CNetClient::AuthenticateHandler(): Authentication failed: %ls", msg->m_Message.c_str());
			}
			else
			{
				LOG(NORMAL, LOG_CAT_NET, "CNetClient::AuthenticateHandler(): Authenticated!");
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
	
	LOG(NORMAL, LOG_CAT_NET, "CNetClient::PreGameHandler(): %s.", pMsg->GetString().c_str());

	switch (pMsg->GetType())
	{
		case NMT_StartGame:
		{
			pClient->StartGame();
			HANDLED(pMsg);
		}
		case NMT_PlayerConnect:
		{
			// Add players to local client list
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
		case NMT_SetPlayerConfig:
		{
			CSetPlayerConfig *msg=(CSetPlayerConfig *)pMsg;
			// FIXME Check player ID
			CPlayer *pPlayer=pClient->m_pGameAttributes->m_Players[msg->m_PlayerID];
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

	if (msgType != NMT_EndCommandBatch)
		LOG(NORMAL, LOG_CAT_NET, "CNetClient::InGameHandler(): %s.", pMsg->GetString().c_str());

	if (msgType >= NMT_COMMAND_FIRST && msgType <= NMT_COMMAND_LAST)
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
