#include "precompiled.h"

#include "Network/ServerSession.h"
#include "Network/Server.h"
#include "CLogger.h"
#include "CConsole.h"

extern CConsole *g_Console;

CNetServerSession::~CNetServerSession()
{
	m_pServer->RemoveSession(this);
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
			LOG(WARNING, LOG_CAT_NET, "CNetServerSession::BaseHandler(): NMT_ERROR: %s", msg->GetString().c_str());
			if (msg->m_State == SS_UNCONNECTED)
			{
				/* We were disconnected... What happens with our session?
				 * 
				 * Note that deleting a session also removes it from m_Sessions
				 * in CNetServer. If the session is an observer it is also
				 * removed from m_Observers.
				 *
				 * Sessions that are observers or chatters should perhaps
				 * generate an exit message that is sent to all other sessions.
				 *
				 * Player sessions require more care.
				 * Pre-Game: each player session has an associated CPlayer
				 * object and player session slot requiring special care when
				 * deleting.
				 * In-Game: Revert all player's entities to Gaia control,
				 * awaiting the client's reconnect attempts [IF we implement it]
				 * Post-Game: Just do your basic clean-up
				 */
				if (!pSession->m_pPlayer)
				{
					delete pSession;
				}
				else
				{
					LOG(ERROR, LOG_CAT_NET, "CNetServerSession::BaseHandler(): Player disconnection not implemented!!");
				}
			}
			HANDLED(pMsg);
		}
	}
	UNHANDLED(pMsg);
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
	}
	return BaseHandler(pMsg, pNetSession);
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

			pSession->m_pServer->m_Sessions.push_back(pSession);

			CResult *msg=new CResult();
			msg->m_Code=NRC_OK;
			msg->m_Message=L"Logged in";
			pSession->Push(msg);
			if (pServer->GetServerState() == NSS_PreGame)
			{
				// We're in pre-game. The connected client becomes a Player.
				// Find the first free player slot and assign the player to the
				// client.
				// If no free player slots could be found - demote to chatter/
				// observer.
				pSession->m_pMessageHandler=PreGameHandler;
				if (!pServer->AddNewPlayer(pSession))
					pSession->m_pMessageHandler=ObserverHandler;
			}
			else // We're not in pre-game. The session becomes a chatter/observer here.
			{
				pSession->m_pMessageHandler=ObserverHandler;
			}
		}
		else
		{
			LOG(WARNING, LOG_CAT_NET, "CNetServerSession::AuthenticateHandler(): Login Failed");
			CResult *msg=new CResult();
			msg->m_Code=NRC_PasswordInvalid;
			msg->m_Message=L"Invalid Password";
			pSession->Push(msg);
		}

		HANDLED(pMsg);
	}
	return BaseHandler(pMsg, pNetSession);
}

bool CNetServerSession::PreGameHandler(CNetMessage *pMsg, CNetSession *pNetSession)
{
	CNetServerSession *pSession=(CNetServerSession *)pNetSession;
	LOG(NORMAL, LOG_CAT_NET, "CNetServerSession::PreGameHandler(): %s.", pMsg->GetString().c_str());

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
		CStrW wstr=msg->m_Message;
		g_Console->ReceivedChatMessage(pSession->GetName().c_str(), wstr.c_str());
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
		
	if (pMsg->GetType() >= NMT_COMMAND_FIRST && pMsg->GetType() <= NMT_COMMAND_LAST)
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
