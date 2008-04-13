#include "precompiled.h"

#include "Server.h"
#include "ServerSession.h"
#include "Network.h"
#include "JSEvents.h"

#include "scripting/ScriptableObject.h"

#include "ps/Game.h"
#include "simulation/Simulation.h"
#include "ps/Player.h"
#include "ps/CLogger.h"
#include "ps/CConsole.h"
#include "ps/ThreadUtil.h"

#define LOG_CAT_NET "net"

CNetServer *g_NetServer=NULL;

using namespace std;

// NOTE: Called in network thread
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

// NOTE: Called in network thread
void CNetServer::OnAccept(const CSocketAddress &addr)
{
	LOG(CLogger::Normal,  LOG_CAT_NET, "CNetServer::OnAccept(): Accepted connection from %s port %d", addr.GetString().c_str(), addr.GetPort());

	CSocketInternal *pInt=Accept();
	CNetServerSession *pSession=CreateSession(pInt);
	UNUSED2(pSession);
}

CNetServer::CNetServer(CGame *pGame, CGameAttributes *pGameAttribs):
	m_JSI_Sessions(&m_Sessions),
	m_pGame(pGame),
	m_pGameAttributes(pGameAttribs),
	m_MaxObservers(5),
	m_LastSessionID(1),
	m_ServerPlayerName(L"Noname Server Player"),
	m_ServerName(L"Noname Server"),
	m_WelcomeMessage(L"Noname Server Welcome Message"),
	m_Port(-1)
{
	m_pGameAttributes->SetUpdateCallback(AttributeUpdate, this);
	m_pGameAttributes->SetPlayerUpdateCallback(PlayerAttributeUpdate, this);
	m_pGameAttributes->SetPlayerSlotAssignmentCallback(PlayerSlotAssignmentCallback, this);

	m_pGame->GetSimulation()->SetTurnManager(this);

	// Set an incredibly long turn length for debugging - less command batch spam that way
	for (int i=0; i<3; i++)
		CTurnManager::SetTurnLength(i, CTurnManager::DEFAULT_TURN_LENGTH);

	g_ScriptingHost.SetGlobal("g_NetServer", OBJECT_TO_JSVAL(GetScript()));
}

CNetServer::~CNetServer()
{
	g_ScriptingHost.SetGlobal("g_NetServer", JSVAL_NULL);
	
	while (m_Sessions.size() > 0)
	{
		SessionMap::iterator it=m_Sessions.begin();
		delete it->second;
		m_Sessions.erase(it);
	}
}

void CNetServer::ScriptingInit()
{
	CJSMap<SessionMap>::ScriptingInit("NetServer_SessionMap");

	AddMethod<bool, &CNetServer::JSI_Open>("open", 0);

	AddProperty(L"sessions", &CNetServer::m_JSI_Sessions);

	AddProperty(L"serverPlayerName", &CNetServer::m_ServerPlayerName);
	AddProperty(L"serverName", &CNetServer::m_ServerName);
	AddProperty(L"welcomeMessage", &CNetServer::m_WelcomeMessage);
	
	AddProperty(L"port", &CNetServer::m_Port);
	
	AddProperty(L"onChat", &CNetServer::m_OnChat);
	AddProperty(L"onClientConnect", &CNetServer::m_OnClientConnect);
	AddProperty(L"onClientDisconnect", &CNetServer::m_OnClientDisconnect);

	CJSObject<CNetServer>::ScriptingInit("NetServer");
}

bool CNetServer::JSI_Open(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	CSocketAddress addr;
	if (m_Port == -1)
		GetDefaultListenAddress(addr);
	else
		addr=CSocketAddress(m_Port, /* m_UseIPv6 ? IPv6 : */ IPv4);
	
	PS_RESULT res=Bind(addr);
	if (res != PS_OK)
	{
		LOG(CLogger::Error, LOG_CAT_NET, "CNetServer::JSI_Open(): Bind error: %s", res);
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

void FillSetGameConfigCB(const CStrW& name, ISynchedJSProperty *prop, void *userdata)
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

void FillSetPlayerConfigCB(const CStrW& name, ISynchedJSProperty *prop, void *userdata)
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

void CNetServer::AssignSessionID(CNetServerSession *pSession)
{
	int newID=++m_LastSessionID;
	pSession->SetID(newID);
	m_Sessions[newID]=pSession;
}

void CNetServer::AddSession(CNetServerSession *pSession)
{
	{
		CSetGameConfig *pMsg=new CSetGameConfig();
		FillSetGameConfig(pMsg);
		pSession->Push(pMsg);
	}

	// Broadcast a message for the newly added player session
	CClientConnect *pMsg=new CClientConnect();
	pMsg->m_Clients.resize(1);
	pMsg->m_Clients[0].m_SessionID=pSession->GetID();
	pMsg->m_Clients[0].m_Name=pSession->GetName();
	Broadcast(pMsg);

	pMsg=new CClientConnect();

	// Server "client"
	pMsg->m_Clients.resize(1);
	pMsg->m_Clients.back().m_SessionID=1; // Server is always 1
	pMsg->m_Clients.back().m_Name=m_ServerPlayerName;

	// All the other clients
	SessionMap::iterator it=m_Sessions.begin();
	for (;it!=m_Sessions.end();++it)
	{
		pMsg->m_Clients.push_back(CClientConnect::S_m_Clients());
		pMsg->m_Clients.back().m_SessionID=it->second->GetID();
		pMsg->m_Clients.back().m_Name=it->second->GetName();
	}
	pSession->Push(pMsg);

	// Sync player slot assignments and player attributes
	for (uint i=0;i<m_pGameAttributes->GetSlotCount();i++)
	{
		CPlayerSlot *pSlot=m_pGameAttributes->GetSlot(i);
	
		pSession->Push(CreatePlayerSlotAssignmentMessage(pSlot));
	
		if (pSlot->GetAssignment() == SLOT_SESSION)
		{
			CSetPlayerConfig *pMsg=new CSetPlayerConfig();
			FillSetPlayerConfig(pMsg, pSlot->GetPlayer());
			pSession->Push(pMsg);
		}
	}
	
	OnClientConnect(pSession);
}

void CNetServer::AttributeUpdate(const CStrW& name, const CStrW& newValue, void *userdata)
{
	CNetServer *pServer=(CNetServer *)userdata;
	g_Console->InsertMessage(L"AttributeUpdate: %ls = \"%ls\"", name.c_str(), newValue.c_str());

	CSetGameConfig *pMsg=new CSetGameConfig;
	pMsg->m_Values.resize(1);
	pMsg->m_Values[0].m_Name=name;
	pMsg->m_Values[0].m_Value=newValue;

	pServer->Broadcast(pMsg);
}

void CNetServer::PlayerAttributeUpdate(const CStrW& name, const CStrW& newValue, CPlayer *pPlayer, void *userdata)
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

CNetMessage *CNetServer::CreatePlayerSlotAssignmentMessage(CPlayerSlot *pSlot)
{
	CAssignPlayerSlot *pMsg=new CAssignPlayerSlot();
	pMsg->m_SlotID=pSlot->GetSlotID();
	pMsg->m_SessionID=pSlot->GetSessionID();
	switch (pSlot->GetAssignment())
	{
#define CASE(_a, _b) case _a: pMsg->m_Assignment=_b; break;
		CASE(SLOT_CLOSED, PS_ASSIGN_CLOSED)
		CASE(SLOT_OPEN, PS_ASSIGN_OPEN)
		CASE(SLOT_SESSION, PS_ASSIGN_SESSION)
		//CASE(SLOT_AI, PS_ASSIGN_AI)
	}
	return pMsg;
}

void CNetServer::PlayerSlotAssignmentCallback(void *userdata, CPlayerSlot *pSlot)
{
	CNetServer *pInstance=(CNetServer *)userdata;
	if (pSlot->GetAssignment() == SLOT_SESSION)
		pSlot->GetSession()->SetPlayerSlot(pSlot);
	CNetMessage *pMsg=CreatePlayerSlotAssignmentMessage(pSlot);
	g_Console->InsertMessage(L"Player Slot Assignment: %hs\n", pMsg->GetString().c_str());
	pInstance->Broadcast(pMsg);
}

bool CNetServer::AllowObserver(CNetServerSession* UNUSED(pSession))
{
	return m_Observers.size() < m_MaxObservers;
}

void CNetServer::RemoveSession(CNetServerSession *pSession)
{
	SessionMap::iterator it=m_Sessions.find(pSession->GetID());
	if (it != m_Sessions.end())
		m_Sessions.erase(it);

	/*
	 * Player sessions require some extra care:
	 *
	 * Pre-Game: dissociate the slot that was used by the session and
	 * synchronize the disconnection of the client.
	 *
	 * In-Game: Revert all player's entities to Gaia control, awaiting the
	 * client's reconnect attempts [if/when we implement that]
	 *
	 * Post-Game: Just sync disconnection - we don't have any players anymore
	 * and all is fine.
	 *
	 * After this is done, call the JS callback if it's been set.
	 */
	if (pSession->GetPlayer())
	{
		if (m_ServerState == NSS_PreGame)
		{
			pSession->GetPlayerSlot()->AssignClosed();
		}
		else if (m_ServerState == NSS_InGame)
		{
			// TODO Reassign entities to Gaia control
			// TODO Set everything up for re-connect and resume
			SetClientPipe(pSession->GetPlayerSlot()->GetSlotID(), NULL);
			pSession->GetPlayerSlot()->AssignClosed();
		}
	}
		
	CClientDisconnect *pMsg=new CClientDisconnect();
	pMsg->m_SessionID=pSession->GetID();
	Broadcast(pMsg);

	OnClientDisconnect(pSession);

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

	SessionMap::iterator it=m_Sessions.begin();
	// Skip one session
	++it;
	// Send a *copy* to all remaining sessions
	for (;it != m_Sessions.end();++it)
	{
		it->second->Push(pMsg->Copy());
	}
	// Now send to the first session, *not* copying the message
	m_Sessions.begin()->second->Push(pMsg);
}

int CNetServer::StartGame()
{
	if (m_pGame->StartGame(m_pGameAttributes) != PSRETURN_OK)
	{
		return -1;
	}
	else
	{
		CTurnManager::Initialize(m_pGameAttributes->GetSlotCount());
		for (uint i=0;i<m_pGameAttributes->GetSlotCount();i++)
		{
			CPlayerSlot *pSlot=m_pGameAttributes->GetSlot(i);
			if (pSlot->GetAssignment() == SLOT_SESSION)
				CTurnManager::SetClientPipe(i, pSlot->GetSession());
		}
		m_ServerState=NSS_InGame;

		SessionMap::iterator it=m_Sessions.begin();
		while (it != m_Sessions.end())
		{
			it->second->StartGame();
			++it;
		}
		
		debug_printf("Server StartGame\n");
		Broadcast(new CStartGame());	
		
		// This is the signal for everyone to start their simulations.
		//SendBatch(1);

		return 0;
	}
}

void CNetServer::GetDefaultListenAddress(CSocketAddress &address)
{
	address=CSocketAddress(PS_DEFAULT_PORT, IPv4);
}

bool CNetServer::NewTurnReady()
{
	// Wait for all clients to check in
	SessionMap::iterator it = m_Sessions.begin();
	for (; it != m_Sessions.end(); ++it)
	{
		if (!it->second->IsReadyForTurn())
			return false;
	}
	return true;
}

void CNetServer::NewTurn()
{
	CScopeLock lock(m_Mutex);
	
	// Clear ready flags on clients
	SessionMap::iterator it = m_Sessions.begin();
	for (; it != m_Sessions.end(); ++it)
	{
		it->second->SetReadyForTurn(false);
	}
	
	RecordBatch(2);

	RotateBatches();
	ClearBatch(2);

	IterateBatch(1, CSimulation::GetMessageMask, m_pGame->GetSimulation());
	//debug_printf("In NewTurn - sending batch\n");
	SendBatch(1);
	//IterateBatch(1, SendToObservers, this);
}

void CNetServer::QueueLocalCommand(CNetMessage *pMsg)
{
	//debug_printf("Queueing command from server\n");
	QueueIncomingCommand(pMsg);
}

void CNetServer::QueueIncomingCommand(CNetMessage *pMsg)
{
	CScopeLock lock(m_Mutex);
	LOG(CLogger::Normal,  LOG_CAT_NET, "CNetServer::QueueIncomingCommand(): %s.", pMsg->GetString().c_str());
	debug_printf("Got a command! queueing it to 2 turns from now\n");
	QueueMessage(2, pMsg);
}

void CNetServer::OnChat(const CStrW& from, const CStrW& message)
{
	if (m_OnChat.Defined())
	{
		CChatEvent evt(from, message);
		m_OnChat.DispatchEvent(GetScript(), &evt);
	}
}

void CNetServer::OnClientConnect(CNetServerSession *pSession)
{
	if (m_OnClientConnect.Defined())
	{
		CClientConnectEvent evt(pSession);
		m_OnClientConnect.DispatchEvent(GetScript(), &evt);
	}
}

void CNetServer::OnClientDisconnect(CNetServerSession *pSession)
{
	if (m_OnClientDisconnect.Defined())
	{
		CClientDisconnectEvent evt(pSession->GetID(), pSession->GetName());
		m_OnClientDisconnect.DispatchEvent(GetScript(), &evt);
	}
}
