#include "precompiled.h"

#include <algorithm>

#include "Network/Server.h"
#include "Network/Network.h"
#include "Network/JSEvents.h"

#include "Game.h"
#include "Player.h"
#include "CLogger.h"
#include "CConsole.h"

#define LOG_CAT_NET "net"

extern CConsole *g_Console;

CNetServer *g_NetServer=NULL;

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

CNetServer::CNetServer(CGame *pGame, CGameAttributes *pGameAttribs):
	m_pGame(pGame),
	m_pGameAttributes(pGameAttribs),
	m_NumPlayers(pGameAttribs->m_NumPlayers),
	m_MaxObservers(5),
	m_ServerPlayerName(L"Noname Server Player"),
	m_ServerName(L"Noname Server"),
	m_WelcomeMessage(L"Noname Server Welcome Message"),
	m_Port(-1)
{
	ONCE(
		(AddMethod<bool, &CNetServer::JSI_Open>("open", 0));
	
		CJSObject<CNetServer>::ScriptingInit("NetServer");
	);

	AddProperty(L"serverPlayerName", &m_ServerPlayerName);
	AddProperty(L"serverName", &m_ServerName);
	AddProperty(L"welcomeMessage", &m_WelcomeMessage);
	
	AddProperty(L"port", &m_Port);
	
	AddProperty(L"onChat", &m_OnChat);

	m_pGameAttributes->SetUpdateCallback(AttributeUpdate, this);
	m_pGameAttributes->SetPlayerUpdateCallback(PlayerAttributeUpdate, this);

	m_pGame->GetSimulation()->SetTurnManager(this);
	// Set an incredibly long turn length - less command batch spam that way
	for (int i=0;i<3;i++)
		CTurnManager::SetTurnLength(i, 3000);

	g_ScriptingHost.SetGlobal("g_NetServer", OBJECT_TO_JSVAL(GetScript()));
}

CNetServer::~CNetServer()
{
	g_ScriptingHost.SetGlobal("g_NetServer", JSVAL_NULL);
}

bool CNetServer::JSI_Open(JSContext *cx, uintN argc, jsval *argv)
{
	CSocketAddress addr;
	if (m_Port == -1)
		GetDefaultListenAddress(addr);
	else
		addr=CSocketAddress(m_Port, /*m_UseIPv6?IPv6:*/IPv4);
	
	PS_RESULT res=Bind(addr);
	if (res != PS_OK)
	{
		LOG(ERROR, LOG_CAT_NET, "CNetServer::JSI_Open(): Bind error: %s", res);
		return false;
	}

	return true;
}

PS_RESULT CNetServer::Bind(const CSocketAddress &address)
{
	PS_RESULT res=CServerSocket::Bind(address);
	if (res==PS_OK)
		m_ServerState=NSS_PreGame;
	return res;
}

void FillSetGameConfigCB(CStrW name, ISynchedJSProperty *prop, void *userdata)
{
	CSetGameConfig *pMsg=(CSetGameConfig *)userdata;
	size_t size=pMsg->m_Values.size();
	pMsg->m_Values.resize(size+1);
	pMsg->m_Values[size].m_Name=name;
	pMsg->m_Values[size].m_Value=prop->ToString();
}

void CNetServer::FillSetGameConfig(CSetGameConfig *pMsg)
{
	m_pGameAttributes->IterateSynchedProperties(FillSetGameConfigCB, pMsg);
}

void FillSetPlayerConfigCB(CStrW name, ISynchedJSProperty *prop, void *userdata)
{
	CSetPlayerConfig *pMsg=(CSetPlayerConfig *)userdata;
	size_t size=pMsg->m_Values.size();
	pMsg->m_Values.resize(size+1);
	pMsg->m_Values[size].m_Name=name;
	pMsg->m_Values[size].m_Value=prop->ToString();
}

void CNetServer::FillSetPlayerConfig(CSetPlayerConfig *pMsg, CPlayer *pPlayer)
{
	pMsg->m_PlayerID=pPlayer->GetPlayerID();
	pPlayer->IterateSynchedProperties(FillSetPlayerConfigCB, pMsg);
}

bool CNetServer::AddNewPlayer(CNetServerSession *pSession)
{
	CSetGameConfig *pMsg=new CSetGameConfig();
	FillSetGameConfig(pMsg);
	pSession->Push(pMsg);

	if (m_PlayerSessions.size() < m_NumPlayers-1)
	{
		// First two players are Gaia and Server player, so assign new player
		// ID's starting from 2
		uint newPlayerID=2+(uint)m_PlayerSessions.size();
		m_PlayerSessions.push_back(pSession);
		pSession->m_pPlayer=m_pGameAttributes->m_Players[newPlayerID];
		pSession->m_pPlayer->SetName(pSession->GetName());

		// Broadcast a message for the newly added player session
		CPlayerConnect *pMsg=new CPlayerConnect();
		pMsg->m_Players.resize(1);
		pMsg->m_Players[0].m_PlayerID=newPlayerID;
		pMsg->m_Players[0].m_Nick=pSession->GetName();
		Broadcast(pMsg);

		pMsg=new CPlayerConnect();
		
		// Server Player
		pMsg->m_Players.resize(1);
		pMsg->m_Players.back().m_PlayerID=1; // Server is always 1
		pMsg->m_Players.back().m_Nick=m_ServerPlayerName;
		
		// All the other players
		for (uint i=0;i<m_PlayerSessions.size()-1;i++)
		{
			if (m_PlayerSessions[i])
			{
				pMsg->m_Players.push_back(CPlayerConnect::S_m_Players());
				pMsg->m_Players.back().m_PlayerID=i+2;
				pMsg->m_Players.back().m_Nick=m_PlayerSessions[i]->GetName();
			}
		}
		pSession->Push(pMsg);

		// Sync other player's attributes
		for (uint i=0;i<m_PlayerSessions.size()-1;i++)
		{
			if (m_PlayerSessions[i])
			{
				CSetPlayerConfig *pMsg=new CSetPlayerConfig();
				FillSetPlayerConfig(pMsg, m_PlayerSessions[i]->GetPlayer());
				pSession->Push(pMsg);
			}
		}
		
		return true;
	}
	else
		return false;
}

void CNetServer::AttributeUpdate(CStrW name, CStrW newValue, void *userdata)
{
	CNetServer *pServer=(CNetServer *)userdata;
	g_Console->InsertMessage(L"AttributeUpdate: %ls = \"%ls\"", name.c_str(), newValue.c_str());

	if (name == CStrW(L"numPlayers"))
	{
		pServer->m_NumPlayers=newValue.ToUInt();
	}

	CSetGameConfig *pMsg=new CSetGameConfig;
	pMsg->m_Values.resize(1);
	pMsg->m_Values[0].m_Name=name;
	pMsg->m_Values[0].m_Value=newValue;

	pServer->Broadcast(pMsg);
}

void CNetServer::PlayerAttributeUpdate(CStrW name, CStrW newValue, CPlayer *pPlayer, void *userdata)
{
	CNetServer *pServer=(CNetServer *)userdata;
	g_Console->InsertMessage(L"PlayerAttributeUpdate(%d): %ls = \"%ls\"", pPlayer->GetPlayerID(), name.c_str(), newValue.c_str());

	CSetPlayerConfig *pMsg=new CSetPlayerConfig;
	pMsg->m_PlayerID=pPlayer->GetPlayerID();
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

	if (pSession->GetPlayer())
	{
		uint playerID=pSession->GetPlayer()->GetPlayerID();
		m_PlayerSessions[playerID-2]=NULL;
		CTurnManager::SetClientPipe(playerID-2, NULL);
	}
	
	// TODO Correct handling of observers
}

// Unfortunately, the message queueing model is made so that each message has
// to be copied once for each socket its sent over, messages are deleted when
// sent by CMessageSocket. We could ref-count, but that requires a lot of
// thread safety stuff => hard work
void CNetServer::Broadcast(CNetMessage *pMsg)
{
	if (m_Sessions.empty())
	{
		delete pMsg;
		return;
	}

	size_t i=0;
	for (;i<m_Sessions.size()-1;i++)
	{
		m_Sessions[i]->Push(pMsg->Copy());
	}
	m_Sessions[i]->Push(pMsg);
}

int CNetServer::StartGame()
{
	if (m_PlayerSessions.size() < m_pGameAttributes->m_NumPlayers-1)
	{
		return -1;
	}

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
	//IterateBatch(1, SendToObservers, this);
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

void CNetServer::OnChat(CStrW from, CStrW message)
{
	if (m_OnChat.Defined())
	{
		CChatEvent evt(from, message);
		m_OnChat.DispatchEvent(GetScript(), &evt);
	}
}
