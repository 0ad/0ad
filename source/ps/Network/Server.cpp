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
	LOG(NORMAL, "CNetServer::OnAccept(): Accepted connection from %s port %d\n", addr.GetString().c_str(), addr.GetPort());

	CSocketInternal *pInt=Accept();
	CNetServerSession *pSession=CreateSession(pInt);
	{
		CScopeLock scopeLock(m_Mutex);
		m_Sessions.push_back(pSession);
	}
}

CNetServerSession::~CNetServerSession()
{
	CScopeLock scopeLock(m_pServer->m_Mutex);
	vector<CNetServerSession*>::iterator it=find(m_pServer->m_Sessions.begin(), m_pServer->m_Sessions.end(), this);
	if (it != m_pServer->m_Sessions.end())
		m_pServer->m_Sessions.erase(it);
}

void CNetServerSession::StartGame()
{
	if (m_pMessageHandler==PreGameHandler)
		m_pMessageHandler=InGameHandler;
}

#define UNHANDLED(_pMsg) return false;
#define HANDLED(_pMsg) delete _pMsg; return true;
#define TAKEN(_pMsg) return true;

bool CNetServerSession::HandshakeHandler(CNetMessage *pMsg, CNetSession *pNetSession)
{
	CNetServerSession *pSession=(CNetServerSession *)pNetSession;
	printf("CNetServerSession::HandshakeHandler(): %s.\n", pMsg->GetString().c_str());
	switch (pMsg->GetType())
	{
	case NMT_ClientHandshake:
		{
			CClientHandshake * /*SmartPointer<CClientHandshake>*/ msg=(CClientHandshake *)pMsg/*.GetRawPointer()*/;
			printf("ClientHandshake: %s.\n", pMsg->GetString().c_str());
		
			if (msg->m_ProtocolVersion != PS_PROTOCOL_VERSION)
				do {} while(0); // This will never happen to us here, but anyways ;-)

			CServerHandshakeResponse *retmsg=new CServerHandshakeResponse();
			retmsg->m_UseProtocolVersion=PS_PROTOCOL_VERSION;
			retmsg->m_Flags=0;
			retmsg->m_Message=CStrW(L"Hello and Welcome!");
			pSession->Push(retmsg);
			
			pSession->m_pMessageHandler=AuthenticateHandler;

			HANDLED(pMsg);
		}
	default:
		UNHANDLED(pMsg);
	}
}

bool CNetServerSession::AuthenticateHandler(CNetMessage *pMsg, CNetSession *pNetSession)
{
	CNetServerSession *pSession=(CNetServerSession *)pNetSession;
	CNetServer *pServer=pSession->m_pServer;
	printf("CNetServerSession::AuthenticateHandler(): %s.\n", pMsg->GetString().c_str());
	if (pMsg->GetType() == NMT_Authenticate)
	{
		CAuthenticate *msg=(CAuthenticate *)pMsg;
		LOG(NORMAL, LOG_CAT_NET, "CNetServerSession::AuthenticateHandler(): Client Authentication Received: username: %ls password: \"%hs\"", msg->m_Name.c_str(), msg->m_Password.c_str());

		if (msg->m_Password == pSession->m_pServer->m_Password)
		{
			LOG(NORMAL, LOG_CAT_NET, "CNetServerSession::AuthenticateHandler(): Login Successful");
			pSession->m_Name=msg->m_Name;

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
	UNHANDLED(pMsg);
}

bool CNetServerSession::PreGameHandler(CNetMessage *pMsg, CNetSession *pNetSession)
{
	CNetServerSession *pSession=(CNetServerSession *)pNetSession;
	printf("CNetServerSession::PreGameHandler(): %s.\n", pMsg->GetString().c_str());

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
	UNHANDLED(pMsg);
}

bool CNetServerSession::InGameHandler(CNetMessage *pMsg, CNetSession *pNetSession)
{
	CNetServerSession *pSession=(CNetServerSession *)pNetSession;
	printf("CNetServerSession::InGameHandler(): %s.\n", pMsg->GetString().c_str());

	if (ChatHandler(pMsg, pNetSession))
		return true;

	switch (pMsg->GetType())
	{
	/* All Command Messages (i.e. simulation turn synchronized messages) */
	case NMT_GotoCommand:
		//pSession->m_pPlayer->ValidateCommand(pMsg);
		pSession->m_pServer->QueueIncomingCommand(pMsg);
		TAKEN(pMsg);

	default:
		UNHANDLED(pMsg);
	}
}

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
	CScopeLock scopeLock(m_Mutex);

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

	if (m_PlayerSessions.size() < m_NumPlayers)
	{
		m_PlayerSessions.push_back(pSession);

		CPlayerConnect *pMsg=new CPlayerConnect();
		pMsg->m_Players.resize(1);
		pMsg->m_Players[0].m_PlayerID=(u32)m_PlayerSessions.size();
		pMsg->m_Players[0].m_Nick=pSession->GetName();
		BroadcastUnsafe(pMsg);

		pMsg=new CPlayerConnect();
		for (uint i=0;i<m_PlayerSessions.size()-1;i++)
		{
			pMsg->m_Players.resize(i+1);
			pMsg->m_Players.back().m_PlayerID=i;
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
	pMsg->m_Values[1].m_Value=newValue;

	pServer->Broadcast(pMsg);
}

bool CNetServer::AllowObserver(CNetServerSession *pSession)
{
	CScopeLock scopeLock(m_Mutex);
	return m_Observers.size() < m_MaxObservers;
}

// Unfortunately, the message queueing model is made so that each message has
// to be copied once for each socket its sent over, messages are deleted when
// sent by CMessageSocket
void CNetServer::Broadcast(CNetMessage *pMsg)
{
	CScopeLock scopeLock(m_Mutex);

	BroadcastUnsafe(pMsg);
}

void CNetServer::BroadcastUnsafe(CNetMessage *pMsg)
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
	if (m_pGame->StartGame(m_pGameAttributes) != PSRETURN_OK)
		return -1;
	else
	{
		CTurnManager::Initialize(m_PlayerSessions.size());
		for (uint i=0;i<m_PlayerSessions.size();i++)
		{
			CTurnManager::SetClientPipe(i, m_PlayerSessions[i]);
		}
		m_ServerState=NSS_InGame;
		Broadcast(new CStartGame());

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
	RotateBatches();
	ClearBatch(2);

	IterateBatch(1, CSimulation::GetMessageMask, m_pGame->GetSimulation());
	SendBatch(1);
}

void CNetServer::QueueLocalCommand(CNetMessage *pMsg)
{
	QueueIncomingCommand(pMsg);
}

void CNetServer::QueueIncomingCommand(CNetMessage *pMsg)
{
	CScopeLock scopeLock(m_Mutex);
	QueueMessage(2, pMsg);
}
