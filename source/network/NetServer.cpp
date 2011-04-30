/* Copyright (C) 2010 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "NetServer.h"

#include "NetClient.h"
#include "NetMessage.h"
#include "NetSession.h"
#include "NetStats.h"
#include "NetTurnManager.h"

#include "lib/external_libraries/enet.h"
#include "ps/CLogger.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"

#define	DEFAULT_SERVER_NAME			L"Unnamed Server"
#define DEFAULT_WELCOME_MESSAGE		L"Welcome"
#define MAX_CLIENTS					8

/**
 * enet_host_service timeout (msecs).
 * Smaller numbers may hurt performance; larger numbers will
 * hurt latency responding to messages from game thread.
 */
static const int HOST_SERVICE_TIMEOUT = 50;

CNetServer* g_NetServer = NULL;

static CStr DebugName(CNetServerSession* session)
{
	if (session == NULL)
		return "[unknown host]";
	if (session->GetGUID().empty())
		return "[unauthed host]";
	return "[" + session->GetGUID().substr(0, 8) + "...]";
}

/*
 * XXX: We use some non-threadsafe functions from the worker thread.
 * See http://trac.wildfiregames.com/ticket/654
 */

CNetServerWorker::CNetServerWorker(int autostartPlayers) :
	m_AutostartPlayers(autostartPlayers),
	m_Shutdown(false),
	m_ScriptInterface(NULL),
	m_NextHostID(1), m_Host(NULL), m_Stats(NULL)
{
	m_State = SERVER_STATE_UNCONNECTED;

	m_ServerTurnManager = NULL;

	m_ServerName = DEFAULT_SERVER_NAME;
	m_WelcomeMessage = DEFAULT_WELCOME_MESSAGE;
}

CNetServerWorker::~CNetServerWorker()
{
	if (m_State != SERVER_STATE_UNCONNECTED)
	{
		// Tell the thread to shut down
		{
			CScopeLock lock(m_WorkerMutex);
			m_Shutdown = true;
		}

		// Wait for it to shut down cleanly
		pthread_join(m_WorkerThread, NULL);
	}

	// Clean up resources

	delete m_Stats;

	for (size_t i = 0; i < m_Sessions.size(); ++i)
	{
		m_Sessions[i]->DisconnectNow(NDR_UNEXPECTED_SHUTDOWN);
		delete m_Sessions[i];
	}

	if (m_Host)
	{
		enet_host_destroy(m_Host);
	}

	delete m_ServerTurnManager;
}

bool CNetServerWorker::SetupConnection()
{
	ENSURE(m_State == SERVER_STATE_UNCONNECTED);
	ENSURE(!m_Host);

	// Bind to default host
	ENetAddress addr;
	addr.host = ENET_HOST_ANY;
	addr.port = PS_DEFAULT_PORT;

	// Create ENet server
	m_Host = enet_host_create(&addr, MAX_CLIENTS, 0, 0);
	if (!m_Host)
	{
		LOGERROR(L"Net server: enet_host_create failed");
		return false;
	}

	m_Stats = new CNetStatsTable();
	if (CProfileViewer::IsInitialised())
		g_ProfileViewer.AddRootTable(m_Stats);

	m_State = SERVER_STATE_PREGAME;

	// Launch the worker thread
	int ret = pthread_create(&m_WorkerThread, NULL, &RunThread, this);
	ENSURE(ret == 0);

	return true;
}

bool CNetServerWorker::SendMessage(ENetPeer* peer, const CNetMessage* message)
{
	ENSURE(m_Host);

	CNetServerSession* session = static_cast<CNetServerSession*>(peer->data);

	return CNetHost::SendMessage(message, peer, DebugName(session).c_str());
}

bool CNetServerWorker::Broadcast(const CNetMessage* message)
{
	ENSURE(m_Host);

	bool ok = true;

	// Send to all sessions that are active and has finished authentication
	for (size_t i = 0; i < m_Sessions.size(); ++i)
	{
		if (m_Sessions[i]->GetCurrState() == NSS_PREGAME || m_Sessions[i]->GetCurrState() == NSS_INGAME)
		{
			if (!m_Sessions[i]->SendMessage(message))
				ok = false;

			// TODO: this does lots of repeated message serialisation if we have lots
			// of remote peers; could do it more efficiently if that's a real problem
		}
	}

	return ok;
}

void* CNetServerWorker::RunThread(void* data)
{
	debug_SetThreadName("NetServer");

	static_cast<CNetServerWorker*>(data)->Run();

	return NULL;
}

void CNetServerWorker::Run()
{
	// To avoid the need for JS_SetContextThread, we create and use and destroy
	// the script interface entirely within this network thread
	m_ScriptInterface = new ScriptInterface("Engine", "Net server", ScriptInterface::CreateRuntime());

	while (true)
	{
		if (!RunStep())
			break;

		// Implement autostart mode
		if (m_State == SERVER_STATE_PREGAME && (int)m_PlayerAssignments.size() == m_AutostartPlayers)
			StartGame();

		// Update profiler stats
		m_Stats->LatchHostState(m_Host);
	}

	// Clear root before deleting its context
	m_GameAttributes = CScriptValRooted();

	SAFE_DELETE(m_ScriptInterface);
}

bool CNetServerWorker::RunStep()
{
	// Check for messages from the game thread.
	// (Do as little work as possible while the mutex is held open,
	// to avoid performance problems and deadlocks.)

	std::vector<std::pair<int, CStr> > newAssignPlayer;
	std::vector<bool> newStartGame;
	std::vector<std::string> newGameAttributes;
	std::vector<u32> newTurnLength;

	{
		CScopeLock lock(m_WorkerMutex);

		if (m_Shutdown)
			return false;

		newStartGame.swap(m_StartGameQueue);
		newAssignPlayer.swap(m_AssignPlayerQueue);
		newGameAttributes.swap(m_GameAttributesQueue);
		newTurnLength.swap(m_TurnLengthQueue);
	}

	for (size_t i = 0; i < newAssignPlayer.size(); ++i)
		AssignPlayer(newAssignPlayer[i].first, newAssignPlayer[i].second);

	if (!newGameAttributes.empty())
		UpdateGameAttributes(GetScriptInterface().ParseJSON(newGameAttributes.back()));

	if (!newTurnLength.empty())
		SetTurnLength(newTurnLength.back());

	// Do StartGame last, so we have the most up-to-date game attributes when we start
	if (!newStartGame.empty())
		StartGame();

	// Process network events:

	ENetEvent event;
	int status = enet_host_service(m_Host, &event, HOST_SERVICE_TIMEOUT);
	if (status < 0)
	{
		LOGERROR(L"CNetServerWorker: enet_host_service failed (%d)", status);
		// TODO: notify game that the server has shut down
		return false;
	}

	if (status == 0)
	{
		// Reached timeout with no events - try again
		return true;
	}

	// Process the event:

	switch (event.type)
	{
	case ENET_EVENT_TYPE_CONNECT:
	{
		// Report the client address
		char hostname[256] = "(error)";
		enet_address_get_host_ip(&event.peer->address, hostname, ARRAY_SIZE(hostname));
		LOGMESSAGE(L"Net server: Received connection from %hs:%u", hostname, event.peer->address.port);

		if (m_State != SERVER_STATE_PREGAME)
		{
			enet_peer_disconnect(event.peer, NDR_SERVER_ALREADY_IN_GAME);
			break;
		}

		// Set up a session object for this peer

		CNetServerSession* session = new CNetServerSession(*this, event.peer);

		m_Sessions.push_back(session);

		SetupSession(session);

		ENSURE(event.peer->data == NULL);
		event.peer->data = session;

		HandleConnect(session);

		break;
	}

	case ENET_EVENT_TYPE_DISCONNECT:
	{
		// If there is an active session with this peer, then reset and delete it

		CNetServerSession* session = static_cast<CNetServerSession*>(event.peer->data);
		if (session)
		{
			LOGMESSAGE(L"Net server: Disconnected %hs", DebugName(session).c_str());

			// Remove the session first, so we won't send player-update messages to it
			// when updating the FSM
			m_Sessions.erase(remove(m_Sessions.begin(), m_Sessions.end(), session), m_Sessions.end());

			session->Update((uint)NMT_CONNECTION_LOST, NULL);

			delete session;
			event.peer->data = NULL;
		}

		break;
	}

	case ENET_EVENT_TYPE_RECEIVE:
	{
		// If there is an active session with this peer, then process the message

		CNetServerSession* session = static_cast<CNetServerSession*>(event.peer->data);
		if (session)
		{
			// Create message from raw data
			CNetMessage* msg = CNetMessageFactory::CreateMessage(event.packet->data, event.packet->dataLength, GetScriptInterface());
			if (msg)
			{
				LOGMESSAGE(L"Net server: Received message %hs of size %lu from %hs", msg->ToString().c_str(), (unsigned long)msg->GetSerializedLength(), DebugName(session).c_str());

				HandleMessageReceive(msg, session);

				delete msg;
			}
		}

		// Done using the packet
		enet_packet_destroy(event.packet);

		break;
	}

	case ENET_EVENT_TYPE_NONE:
		break;
	}

	return true;
}

void CNetServerWorker::HandleMessageReceive(const CNetMessage* message, CNetServerSession* session)
{
	// Update FSM
	bool ok = session->Update(message->GetType(), (void*)message);
	if (!ok)
		LOGERROR(L"Net server: Error running FSM update (type=%d state=%d)", (int)message->GetType(), (int)session->GetCurrState());
}

void CNetServerWorker::SetupSession(CNetServerSession* session)
{
	void* context = session;

	// Set up transitions for session

	session->AddTransition(NSS_UNCONNECTED, (uint)NMT_CONNECTION_LOST, NSS_UNCONNECTED);

	session->AddTransition(NSS_HANDSHAKE, (uint)NMT_CONNECTION_LOST, NSS_UNCONNECTED);
	session->AddTransition(NSS_HANDSHAKE, (uint)NMT_CLIENT_HANDSHAKE, NSS_AUTHENTICATE, (void*)&OnClientHandshake, context);

	session->AddTransition(NSS_AUTHENTICATE, (uint)NMT_CONNECTION_LOST, NSS_UNCONNECTED);
	session->AddTransition(NSS_AUTHENTICATE, (uint)NMT_AUTHENTICATE, NSS_PREGAME, (void*)&OnAuthenticate, context);

	session->AddTransition(NSS_PREGAME, (uint)NMT_CONNECTION_LOST, NSS_UNCONNECTED, (void*)&OnDisconnect, context);
	session->AddTransition(NSS_PREGAME, (uint)NMT_CHAT, NSS_PREGAME, (void*)&OnChat, context);
	session->AddTransition(NSS_PREGAME, (uint)NMT_LOADED_GAME, NSS_INGAME, (void*)&OnLoadedGame, context);

	session->AddTransition(NSS_INGAME, (uint)NMT_CONNECTION_LOST, NSS_UNCONNECTED, (void*)&OnDisconnect, context);
	session->AddTransition(NSS_INGAME, (uint)NMT_CHAT, NSS_INGAME, (void*)&OnChat, context);
	session->AddTransition(NSS_INGAME, (uint)NMT_SIMULATION_COMMAND, NSS_INGAME, (void*)&OnInGame, context);
	session->AddTransition(NSS_INGAME, (uint)NMT_SYNC_CHECK, NSS_INGAME, (void*)&OnInGame, context);
	session->AddTransition(NSS_INGAME, (uint)NMT_END_COMMAND_BATCH, NSS_INGAME, (void*)&OnInGame, context);

	// Set first state
	session->SetFirstState(NSS_HANDSHAKE);
}

bool CNetServerWorker::HandleConnect(CNetServerSession* session)
{
	CSrvHandshakeMessage handshake;
	handshake.m_Magic = PS_PROTOCOL_MAGIC;
	handshake.m_ProtocolVersion = PS_PROTOCOL_VERSION;
	handshake.m_SoftwareVersion = PS_PROTOCOL_VERSION;
	return session->SendMessage(&handshake);
}

void CNetServerWorker::OnUserJoin(CNetServerSession* session)
{
	AddPlayer(session->GetGUID(), session->GetUserName());

	CGameSetupMessage gameSetupMessage(GetScriptInterface());
	gameSetupMessage.m_Data = m_GameAttributes;
	session->SendMessage(&gameSetupMessage);

	CPlayerAssignmentMessage assignMessage;
	ConstructPlayerAssignmentMessage(assignMessage);
	session->SendMessage(&assignMessage);
}

void CNetServerWorker::OnUserLeave(CNetServerSession* session)
{
	RemovePlayer(session->GetGUID());

	if (m_ServerTurnManager)
		m_ServerTurnManager->UninitialiseClient(session->GetHostID()); // TODO: only for non-observers

	// TODO: ought to switch the player controlled by that client
	// back to AI control, or something?
}

void CNetServerWorker::AddPlayer(const CStr& guid, const CStrW& name)
{
	// Find the first free player ID

	std::set<i32> usedIDs;
	for (PlayerAssignmentMap::iterator it = m_PlayerAssignments.begin(); it != m_PlayerAssignments.end(); ++it)
		usedIDs.insert(it->second.m_PlayerID);

	i32 playerID;
	for (playerID = 1; usedIDs.find(playerID) != usedIDs.end(); ++playerID)
	{
		// (do nothing)
	}

	PlayerAssignment assignment;
	assignment.m_Name = name;
	assignment.m_PlayerID = playerID;
	m_PlayerAssignments[guid] = assignment;

	// Send the new assignments to all currently active players
	// (which does not include the one that's just joining)
	SendPlayerAssignments();
}

void CNetServerWorker::RemovePlayer(const CStr& guid)
{
	m_PlayerAssignments.erase(guid);

	SendPlayerAssignments();
}

void CNetServerWorker::AssignPlayer(int playerID, const CStr& guid)
{
	// Remove anyone who's already assigned to this player
	for (PlayerAssignmentMap::iterator it = m_PlayerAssignments.begin(); it != m_PlayerAssignments.end(); ++it)
	{
		if (it->second.m_PlayerID == playerID)
			it->second.m_PlayerID = -1;
	}

	// Update this host's assignment if it exists
	if (m_PlayerAssignments.find(guid) != m_PlayerAssignments.end())
		m_PlayerAssignments[guid].m_PlayerID = playerID;

	SendPlayerAssignments();
}

void CNetServerWorker::ConstructPlayerAssignmentMessage(CPlayerAssignmentMessage& message)
{
	for (PlayerAssignmentMap::iterator it = m_PlayerAssignments.begin(); it != m_PlayerAssignments.end(); ++it)
	{
		CPlayerAssignmentMessage::S_m_Hosts h;
		h.m_GUID = it->first;
		h.m_Name = it->second.m_Name;
		h.m_PlayerID = it->second.m_PlayerID;
		message.m_Hosts.push_back(h);
	}
}

void CNetServerWorker::SendPlayerAssignments()
{
	CPlayerAssignmentMessage message;
	ConstructPlayerAssignmentMessage(message);
	Broadcast(&message);
}

ScriptInterface& CNetServerWorker::GetScriptInterface()
{
	return *m_ScriptInterface;
}

void CNetServerWorker::SetTurnLength(u32 msecs)
{
	if (m_ServerTurnManager)
		m_ServerTurnManager->SetTurnLength(msecs);
}

bool CNetServerWorker::OnClientHandshake(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_CLIENT_HANDSHAKE);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	if (server.m_State != SERVER_STATE_PREGAME)
	{
		session->Disconnect(NDR_SERVER_ALREADY_IN_GAME);
		return false;
	}

	CCliHandshakeMessage* message = (CCliHandshakeMessage*)event->GetParamRef();
	if (message->m_ProtocolVersion != PS_PROTOCOL_VERSION)
	{
		session->Disconnect(NDR_INCORRECT_PROTOCOL_VERSION);
		return false;
	}

	CSrvHandshakeResponseMessage handshakeResponse;
	handshakeResponse.m_UseProtocolVersion = PS_PROTOCOL_VERSION;
	handshakeResponse.m_Message = server.m_WelcomeMessage;
	handshakeResponse.m_Flags = 0;
	session->SendMessage(&handshakeResponse);

	return true;
}

bool CNetServerWorker::OnAuthenticate(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_AUTHENTICATE);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	if (server.m_State != SERVER_STATE_PREGAME)
	{
		session->Disconnect(NDR_SERVER_ALREADY_IN_GAME);
		return false;
	}

	CAuthenticateMessage* message = (CAuthenticateMessage*)event->GetParamRef();

	// TODO: check server password etc?

	u32 newHostID = server.m_NextHostID++;

	CStrW username = server.DeduplicatePlayerName(SanitisePlayerName(message->m_Name));

	session->SetUserName(username);
	session->SetGUID(message->m_GUID);
	session->SetHostID(newHostID);

	CAuthenticateResultMessage authenticateResult;
	authenticateResult.m_Code = ARC_OK;
	authenticateResult.m_HostID = newHostID;
	authenticateResult.m_Message = L"Logged in";
	session->SendMessage(&authenticateResult);

	server.OnUserJoin(session);

	return true;
}

bool CNetServerWorker::OnInGame(void* context, CFsmEvent* event)
{
	// TODO: should split each of these cases into a separate method

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	CNetMessage* message = (CNetMessage*)event->GetParamRef();
	if (message->GetType() == (uint)NMT_SIMULATION_COMMAND)
	{
		CSimulationMessage* simMessage = static_cast<CSimulationMessage*> (message);

		// Send it back to all clients immediately
		server.Broadcast(simMessage);

		// TODO: we should do some validation of ownership (clients can't send commands on behalf of opposing players)

		// TODO: we shouldn't send the message back to the client that first sent it
	}
	else if (message->GetType() == (uint)NMT_SYNC_CHECK)
	{
		CSyncCheckMessage* syncMessage = static_cast<CSyncCheckMessage*> (message);
		server.m_ServerTurnManager->NotifyFinishedClientUpdate(session->GetHostID(), syncMessage->m_Turn, syncMessage->m_Hash);
	}
	else if (message->GetType() == (uint)NMT_END_COMMAND_BATCH)
	{
		CEndCommandBatchMessage* endMessage = static_cast<CEndCommandBatchMessage*> (message);
		server.m_ServerTurnManager->NotifyFinishedClientCommands(session->GetHostID(), endMessage->m_Turn);
	}

	return true;
}

bool CNetServerWorker::OnChat(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_CHAT);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	CChatMessage* message = (CChatMessage*)event->GetParamRef();

	message->m_GUID = session->GetGUID();

	server.Broadcast(message);

	return true;
}

bool CNetServerWorker::OnLoadedGame(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_LOADED_GAME);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	server.CheckGameLoadStatus(session);

	return true;
}

bool CNetServerWorker::OnDisconnect(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_CONNECTION_LOST);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	server.OnUserLeave(session);

	return true;
}

void CNetServerWorker::CheckGameLoadStatus(CNetServerSession* changedSession)
{
	for (size_t i = 0; i < m_Sessions.size(); ++i)
	{
		if (m_Sessions[i] != changedSession && m_Sessions[i]->GetCurrState() != NSS_INGAME)
			return;
	}

	CLoadedGameMessage loaded;
	Broadcast(&loaded);

	m_State = SERVER_STATE_INGAME;
}

void CNetServerWorker::StartGame()
{
	m_ServerTurnManager = new CNetServerTurnManager(*this);

	for (size_t i = 0; i < m_Sessions.size(); ++i)
		m_ServerTurnManager->InitialiseClient(m_Sessions[i]->GetHostID()); // TODO: only for non-observers

	m_State = SERVER_STATE_LOADING;

	// Send the final setup state to all clients
	UpdateGameAttributes(m_GameAttributes);
	SendPlayerAssignments();

	CGameStartMessage gameStart;
	Broadcast(&gameStart);
}

void CNetServerWorker::UpdateGameAttributes(const CScriptValRooted& attrs)
{
	m_GameAttributes = attrs;

	if (!m_Host)
		return;

	CGameSetupMessage gameSetupMessage(GetScriptInterface());
	gameSetupMessage.m_Data = m_GameAttributes;
	Broadcast(&gameSetupMessage);
}

CStrW CNetServerWorker::SanitisePlayerName(const CStrW& original)
{
	const size_t MAX_LENGTH = 32;

	CStrW name = original;
	name.Replace(L"[", L"{"); // remove GUI tags
	name.Replace(L"]", L"}"); // remove for symmetry

	// Restrict the length
	if (name.length() > MAX_LENGTH)
		name = name.Left(MAX_LENGTH);

	// Don't allow surrounding whitespace
	name.Trim(PS_TRIM_BOTH);

	// Don't allow empty name
	if (name.empty())
		name = L"Anonymous";

	return name;
}

CStrW CNetServerWorker::DeduplicatePlayerName(const CStrW& original)
{
	CStrW name = original;

	// Try names "Foo", "Foo (2)", "Foo (3)", etc
	size_t id = 2;
	while (true)
	{
		bool unique = true;
		for (size_t i = 0; i < m_Sessions.size(); ++i)
		{
			if (m_Sessions[i]->GetUserName() == name)
			{
				unique = false;
				break;
			}
		}

		if (unique)
			return name;

		name = original + L" (" + CStrW::FromUInt(id++) + L")";
	}
}




CNetServer::CNetServer(int autostartPlayers) :
	m_Worker(new CNetServerWorker(autostartPlayers))
{
}

CNetServer::~CNetServer()
{
	delete m_Worker;
}

bool CNetServer::SetupConnection()
{
	return m_Worker->SetupConnection();
}

void CNetServer::AssignPlayer(int playerID, const CStr& guid)
{
	CScopeLock lock(m_Worker->m_WorkerMutex);
	m_Worker->m_AssignPlayerQueue.push_back(std::make_pair(playerID, guid));
}

void CNetServer::StartGame()
{
	CScopeLock lock(m_Worker->m_WorkerMutex);
	m_Worker->m_StartGameQueue.push_back(true);
}

void CNetServer::UpdateGameAttributes(const CScriptVal& attrs, ScriptInterface& scriptInterface)
{
	// Pass the attributes as JSON, since that's the easiest safe
	// cross-thread way of passing script data
	std::string attrsJSON = scriptInterface.StringifyJSON(attrs.get(), false);

	CScopeLock lock(m_Worker->m_WorkerMutex);
	m_Worker->m_GameAttributesQueue.push_back(attrsJSON);
}

void CNetServer::SetTurnLength(u32 msecs)
{
	CScopeLock lock(m_Worker->m_WorkerMutex);
	m_Worker->m_TurnLengthQueue.push_back(msecs);
}
