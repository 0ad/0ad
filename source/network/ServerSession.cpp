#include "precompiled.h"

#include "ServerSession.h"
#include "Server.h"
#include "ps/CLogger.h"
#include "ps/CConsole.h"

extern CConsole *g_Console;

CNetServerSession::CNetServerSession(CNetServer *pServer, CSocketInternal *pInt,
		MessageHandler *pMsgHandler):
	CNetSession(pInt, pMsgHandler),
	m_pServer(pServer),
	m_pPlayer(NULL),
	m_pPlayerSlot(NULL),
	m_IsObserver(false),
	m_ID(-1)
{
	ONCE(
		ScriptingInit();
	);
}

CNetServerSession::~CNetServerSession()
{
}

void CNetServerSession::StartGame()
{
	if (m_pMessageHandler==PreGameHandler)
		m_pMessageHandler=InGameHandler;
}

#define UNHANDLED(_pMsg) return false;
#define HANDLED(_pMsg) delete _pMsg; return true;
#define TAKEN(_pMsg) return true;

bool CNetServerSession::BaseHandler(CNetMessage *pMsg, CNetSession *pNetSession)
{
	CNetServerSession *pSession=(CNetServerSession *)pNetSession;
	switch (pMsg->GetType())
	{
		case NMT_ERROR:
		{
			CNetErrorMessage *msg=(CNetErrorMessage *)pMsg;
			if (msg->m_State == SS_UNCONNECTED)
			{
				if (pSession->m_ID != -1)
					pSession->m_pServer->RemoveSession(pSession);
				delete pSession;
			}
			else // error, but not disconnected? something weird is up...
				LOG(WARNING, LOG_CAT_NET, "CNetServerSession::BaseHandler(): NMT_ERROR: %s", msg->GetString().c_str());
			HANDLED(pMsg);
		}
		
		default:
			UNHANDLED(pMsg);
	}
}

bool CNetServerSession::HandshakeHandler(CNetMessage *pMsg, CNetSession *pNetSession)
{
	CNetServerSession *pSession=(CNetServerSession *)pNetSession;
	LOG(NORMAL, LOG_CAT_NET, "CNetServerSession::HandshakeHandler(): %s.", pMsg->GetString().c_str());
	switch (pMsg->GetType())
	{
	case NMT_ClientHandshake:
		{
			CClientHandshake *msg=(CClientHandshake *)pMsg;
		
			if (msg->m_ProtocolVersion != PS_PROTOCOL_VERSION)
			{
				pSession->Push(new CCloseRequestMessage());
				BaseHandler(new CNetErrorMessage(PS_OK, SS_UNCONNECTED), pSession);
			}

			CServerHandshakeResponse *retmsg=new CServerHandshakeResponse();
			retmsg->m_UseProtocolVersion=PS_PROTOCOL_VERSION;
			retmsg->m_Flags=0;
			retmsg->m_Message=pSession->m_pServer->m_WelcomeMessage;
			pSession->Push(retmsg);
			
			pSession->m_pMessageHandler=AuthenticateHandler;

			HANDLED(pMsg);
		}
	
	default:
		return BaseHandler(pMsg, pNetSession);
	}
}

bool CNetServerSession::AuthenticateHandler(CNetMessage *pMsg, CNetSession *pNetSession)
{
	CNetServerSession *pSession=(CNetServerSession *)pNetSession;
	CNetServer *pServer=pSession->m_pServer;
	LOG(NORMAL, LOG_CAT_NET, "CNetServerSession::AuthenticateHandler(): %s.", pMsg->GetString().c_str());
	if (pMsg->GetType() == NMT_Authenticate)
	{
		CAuthenticate *msg=(CAuthenticate *)pMsg;

		if (msg->m_Password == pSession->m_pServer->m_Password)
		{
			LOG(NORMAL, LOG_CAT_NET, "CNetServerSession::AuthenticateHandler(): Login Successful");
			pSession->m_Name=msg->m_Name;

			pServer->AssignSessionID(pSession);
			
			CAuthenticationResult *msg=new CAuthenticationResult();
			msg->m_Code=NRC_OK;
			msg->m_SessionID=pSession->m_ID;
			msg->m_Message=L"Logged in";
			pSession->Push(msg);
			
			pServer->AddSession(pSession);
			if (pServer->GetServerState() == NSS_PreGame)
			{
				pSession->m_pMessageHandler=PreGameHandler;
			}
			else // We're not in pre-game. The session becomes a chatter/observer here.
			{
				pSession->m_pMessageHandler=ObserverHandler;
			}
		}
		else
		{
			LOG(WARNING, LOG_CAT_NET, "CNetServerSession::AuthenticateHandler(): Login Failed");
			CAuthenticationResult *msg=new CAuthenticationResult();
			msg->m_Code=NRC_PasswordInvalid;
			msg->m_SessionID=0;
			msg->m_Message=L"Invalid Password";
			pSession->Push(msg);
		}

		HANDLED(pMsg);
	}
	return BaseHandler(pMsg, pNetSession);
}

bool CNetServerSession::PreGameHandler(CNetMessage *pMsg, CNetSession *pNetSession)
{
	return ChatHandler(pMsg, pNetSession);
}

bool CNetServerSession::ObserverHandler(CNetMessage *pMsg, CNetSession *pNetSession)
{
	printf("CNetServerSession::ObserverHandler(): %s.\n", pMsg->GetString().c_str());

	// TODO Implement observers and chatter => observer promotion
	/*
	if (pMsg->GetType() == NMT_RequestObserve)
	*/

	return ChatHandler(pMsg, pNetSession);
}

bool CNetServerSession::ChatHandler(CNetMessage *pMsg, CNetSession *pNetSession)
{
	CNetServerSession *pSession=(CNetServerSession *)pNetSession;
	if (pMsg->GetType() == NMT_ChatMessage)
	{
		CChatMessage *msg=(CChatMessage *)pMsg;
		msg->m_Sender=pSession->m_Name;
		g_Console->ReceivedChatMessage(pSession->GetName().c_str(), msg->m_Message.c_str());
		pSession->m_pServer->OnChat(msg->m_Sender, msg->m_Message);
		pSession->m_pServer->Broadcast(msg);

		TAKEN(pMsg);
	}
	
	return BaseHandler(pMsg, pNetSession);
}

bool CNetServerSession::InGameHandler(CNetMessage *pMsg, CNetSession *pNetSession)
{
	CNetServerSession *pSession=(CNetServerSession *)pNetSession;
	if (pMsg->GetType() != NMT_EndCommandBatch)
		LOG(NORMAL, LOG_CAT_NET, "CNetServerSession::InGameHandler(): %s.", pMsg->GetString().c_str());

	if (BaseHandler(pMsg, pNetSession))
		return true;

	if (ChatHandler(pMsg, pNetSession))
		return true;
		
	if (pMsg->GetType() >= NMT_COMMAND_FIRST && pMsg->GetType() < NMT_COMMAND_LAST)
	{
		// All Command Messages (i.e. simulation turn synchronized messages)
		//pSession->m_pPlayer->ValidateCommand(pMsg);
		pSession->m_pServer->QueueIncomingCommand(pMsg);
		TAKEN(pMsg);
	}

	switch (pMsg->GetType())
	{
	case NMT_EndCommandBatch:
		// TODO Update client timing information and recalculate turn length
		HANDLED(pMsg);

	default:
		UNHANDLED(pMsg);
	}
}

void CNetServerSession::ScriptingInit()
{
	AddMethod<bool, &CNetServerSession::JSI_Close>("close", 0);

	CJSObject<CNetServerSession>::ScriptingInit("NetSession");
	// Hope this doesn't break anything...
	AddProperty( L"id", &CNetServerSession::m_ID );
	AddProperty( L"name", (CStrW CNetServerSession::*)&CNetServerSession::m_Name );
}

bool CNetServerSession::JSI_Close(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	return false;
}
