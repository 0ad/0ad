#include "precompiled.h"

#include <algorithm>

#include "Network/Server.h"
#include "Network/Network.h"

#include "Game.h"
#include "Player.h"
#include "CLogger.h"
#include "CConsole.h"

#define LOG_CAT_NET "net"

extern CConsole *g_Console;

CNetServer *g_NetServer=NULL;
CNetServerAttributes g_NetServerAttributes;

using namespace std;

CNetServerSession *CNetServer::CreateSession(CSocketInternal *pInt)
{
	CNetServerSession *pRet=new CNetServerSession(this, pInt);

	CServerHandshake *pMsg=new CServerHandshake();
	pMsg->m_Magic=PS_PROTOCOL_MAGIC;
	pMsg->m_ProtocolVersion=PS_PROTOCOL_VERSION;
	pMsg->m_SoftwareVersion=PS_PROTOCOL_VERSION;
	pRet->Push(pMsg);

	return pRet;
}

void CNetServer::OnAccept(const CSocketAddress &addr)
{
	LOG(NORMAL, LOG_CAT_NET, "CNetServer::OnAccept(): Accepted connection from %s port %d", addr.GetString().c_str(), addr.GetPort());

	CSocketInternal *pInt=Accept();
	CNetServerSession *pSession=CreateSession(pInt);
}

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
				 * Player sessions require more care. In Pre-Game, each player
				 * session has an associated CPlayer object and player session
				 * slot requiring special care when deleting.
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
			CClientHandshake * /*SmartPointer<CClientHandshake>*/ msg=(CClientHandshake *)pMsg/*.GetRawPointer()*/;
		
			if (msg->m_ProtocolVersion != PS_PROTOCOL_VERSION)
				do {} while(0); // This will never happen to us here, but anyways ;-)

			CServerHandshakeResponse *retmsg=new CServerHandshakeResponse();
			retmsg->m_UseProtocolVersion=PS_PROTOCOL_VERSION;
			retmsg->m_Flags=0;
			retmsg->m_Message=pSession->m_pServer->GetAttributes()->GetValue("welcomeMessage");
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

// TODO Replace with next generation CNetServer JS Interface
CNetServerAttributes::CNetServerAttributes()
{
	AddValue("serverPlayerName", L"Noname Server Player");
	AddValue("serverName", L"Noname Server");
	AddValue("welcomeMessage", L"Noname Server Welcome Message");
}

CNetServer::CNetServer(CNetServerAttributes *pServerAttribs, CGame *pGame, CGameAttributes *pGameAttribs):
	m_pServerAttributes(pServerAttribs),
	m_pGame(pGame),
	m_pGameAttributes(pGameAttribs),
	m_MaxObservers(5)
{
	m_pGameAttributes->SetUpdateCallback(AttributeUpdate, this);
	m_ServerPlayerName=m_pServerAttributes->GetValue("serverPlayerName");
	m_NumPlayers=m_pGameAttributes->GetValue("numPlayers").ToUInt();

	m_pGame->GetSimulation()->SetTurnManager(this);
	// Set an incredibly long turn length - less command batch spam that way
	for (int i=0;i<3;i++)
		CTurnManager::SetTurnLength(i, 3000);
}

PS_RESULT CNetServer::Bind(const CSocketAddress &address)
{
	PS_RESULT res=CServerSocket::Bind(address);
	if (res==PS_OK)
		m_ServerState=NSS_PreGame;
	return res;
}

bool CNetServer::AddNewPlayer(CNetServerSession *pSession)
{
	CAttributeMap::MapType &attribs=m_pGameAttributes->GetInternalValueMap();
	CAttributeMap::MapType::iterator it=attribs.begin();

	CSetGameConfig *pMsg=new CSetGameConfig();
	uint n=0;
	while (it != attribs.end())
	{
		pMsg->m_Values.resize(++n);
		pMsg->m_Values.back().m_Name=it->first;
		pMsg->m_Values.back().m_Value=it->second;
		++it;
	}
	pSession->Push(pMsg);

	if (m_PlayerSessions.size() < m_NumPlayers-1)
	{
		m_PlayerSessions.push_back(pSession);

		// Broadcast a message for the newly added player session
		CPlayerConnect *pMsg=new CPlayerConnect();
		pMsg->m_Players.resize(1);
		// The player ID is the player session index plus one
		pMsg->m_Players[0].m_PlayerID=(u32)m_PlayerSessions.size();
		pMsg->m_Players[0].m_Nick=pSession->GetName();
		Broadcast(pMsg);

		pMsg=new CPlayerConnect();
		
		// Server Player
		pMsg->m_Players.resize(1);
		pMsg->m_Players.back().m_PlayerID=0;
		pMsg->m_Players.back().m_Nick=m_ServerPlayerName;
		
		// All the other players
		for (uint i=0;i<m_PlayerSessions.size()-1;i++)
		{
			pMsg->m_Players.resize(i+2);
			pMsg->m_Players.back().m_PlayerID=i+1;
			pMsg->m_Players.back().m_Nick=m_PlayerSessions[i]->GetName();
		}
		pSession->Push(pMsg);

		return true;
	}
	else
		return false;
}

void CNetServer::AttributeUpdate(CStr name, CStrW newValue, void *userdata)
{
	CNetServer *pServer=(CNetServer *)userdata;

	if (name == CStr("numPlayers"))
	{
		pServer->m_NumPlayers=newValue.ToUInt();
	}

	CSetGameConfig *pMsg=new CSetGameConfig;
	pMsg->m_Values.resize(1);
	pMsg->m_Values[0].m_Name=name;
	pMsg->m_Values[0].m_Value=newValue;

	pServer->Broadcast(pMsg);
}

bool CNetServer::AllowObserver(CNetServerSession *pSession)
{
	return m_Observers.size() < m_MaxObservers;
}

void CNetServer::RemoveSession(CNetServerSession *pSession)
{
	vector<CNetServerSession*>::iterator it=find(m_Sessions.begin(), m_Sessions.end(), pSession);
	if (it != m_Sessions.end())
		m_Sessions.erase(it);

	// TODO Correct handling of players and observers
}

// Unfortunately, the message queueing model is made so that each message has
// to be copied once for each socket its sent over, messages are deleted when
// sent by CMessageSocket. We could ref-count, but that requires a lot of
// thread safety stuff => hard work
void CNetServer::Broadcast(CNetMessage *pMsg)
{
	if (m_Sessions.empty())
		return;

	size_t i=0;
	for (;i<m_Sessions.size()-1;i++)
	{
		m_Sessions[i]->Push(pMsg->Copy());
	}
	m_Sessions[i]->Push(pMsg);
}

int CNetServer::StartGame()
{
	// TODO Check for the case where we haven't yet filled all player slots
	// CGame expects to have numPlayer players when it starts the game...

	Broadcast(new CStartGame());

	if (m_pGame->StartGame(m_pGameAttributes) != PSRETURN_OK)
		return -1;
	else
	{
		for (uint i=0;i<m_PlayerSessions.size();i++)
		{
			m_PlayerSessions[i]->SetPlayer(m_pGame->GetPlayer(i+1));
		}

		CTurnManager::Initialize(m_PlayerSessions.size());
		for (uint i=0;i<m_PlayerSessions.size();i++)
		{
			CTurnManager::SetClientPipe(i, m_PlayerSessions[i]);
		}
		m_ServerState=NSS_InGame;

		vector<CNetServerSession*>::iterator it=m_Sessions.begin();
		while (it != m_Sessions.end())
		{
			(*it)->StartGame();
			++it;
		}

		return 0;
	}
}

void CNetServer::GetDefaultListenAddress(CSocketAddress &address)
{
	address=CSocketAddress(PS_DEFAULT_PORT, IPv4);
}

void CNetServer::NewTurn()
{
	RecordBatch(2);

	RotateBatches();
	ClearBatch(2);

	IterateBatch(1, CSimulation::GetMessageMask, m_pGame->GetSimulation());
	SendBatch(1);
	//SendBatchToList(1, m_Observers);
}

void CNetServer::QueueLocalCommand(CNetMessage *pMsg)
{
	QueueIncomingCommand(pMsg);
}

void CNetServer::QueueIncomingCommand(CNetMessage *pMsg)
{
	LOG(NORMAL, LOG_CAT_NET, "CNetServer::QueueIncomingCommand(): %s.", pMsg->GetString().c_str());
	QueueMessage(2, pMsg);
}
