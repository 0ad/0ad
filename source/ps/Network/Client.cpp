#include "precompiled.h"

#include <Network/Client.h>
#include <CStr.h>
#include <CLogger.h>
#include <CConsole.h>

#define LOG_CAT_NET "net"

CNetClient *g_NetClient=NULL;
extern CConsole *g_Console;

CNetClient::CNetClient(CGame *pGame, CGameAttributes *pGameAttribs):
	CNetSession(ConnectHandler),
	m_pLocalPlayer(NULL),
	m_pGame(pGame),
	m_pGameAttributes(pGameAttribs)
{
	m_pGame->GetSimulation()->SetTurnManager(this);
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

bool CNetClient::ConnectHandler(CNetMessage *pMsg, CNetSession *pSession)
{
	CNetClient *pClient=(CNetClient *)pSession;
	LOG(NORMAL, LOG_CAT_NET, "CNetClient::ConnectHandler(): %s.", pMsg->GetString().c_str());
	switch (pMsg->GetType())
	{
	case NMT_CONNECT_COMPLETE:
		pClient->m_pMessageHandler=HandshakeHandler;
		break;
	case NMT_ERROR:
	{
		CNetErrorMessage *msg=(CNetErrorMessage *)pMsg;
		LOG(ERROR, LOG_CAT_NET, "CNetClient::ConnectHandler(): Connect Failed: %s", msg->m_Error);
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
	LOG(NORMAL, LOG_CAT_NET, "CNetClient::PreGameHandler(): %s.", pMsg->GetString().c_str());

	if (ChatHandler(pMsg, pSession))
		return true;

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
	}

	UNHANDLED(pMsg);
}

bool CNetClient::InGameHandler(CNetMessage *pMsg, CNetSession *pSession)
{
	CNetClient *pClient=(CNetClient *)pSession;
	ENetMessageType msgType=pMsg->GetType();

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

	if (ChatHandler(pMsg, pSession))
		return true;

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
		HANDLED(pMsg);
	}
	}
	UNHANDLED(pMsg);
}

void CNetClient::StartGame()
{
	m_pMessageHandler=InGameHandler;
	m_pGame->StartGame(m_pGameAttributes);
}

void CNetClient::NewTurn()
{
	RotateBatches();
	ClearBatch(2);

	Push(new CEndCommandBatch());
}

void CNetClient::QueueLocalCommand(CNetMessage *pMsg)
{
	// Don't save these locally, since they'll be bounced by the server anyway
	Push(pMsg);
}
