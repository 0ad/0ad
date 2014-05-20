/* Copyright (C) 2014 Wildfire Games.
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
#include "ps/ConfigDB.h"

#if CONFIG2_MINIUPNPC
#include <miniupnpc/miniwget.h>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>
#endif

#define	DEFAULT_SERVER_NAME			L"Unnamed Server"
#define DEFAULT_WELCOME_MESSAGE		L"Welcome"
#define MAX_CLIENTS					8

static const int CHANNEL_COUNT = 1;

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

/**
 * Async task for receiving the initial game state to be forwarded to another
 * client that is rejoining an in-progress network game.
 */
class CNetFileReceiveTask_ServerRejoin : public CNetFileReceiveTask
{
	NONCOPYABLE(CNetFileReceiveTask_ServerRejoin);
public:
	CNetFileReceiveTask_ServerRejoin(CNetServerWorker& server, u32 hostID)
		: m_Server(server), m_RejoinerHostID(hostID)
	{
	}

	virtual void OnComplete()
	{
		// We've received the game state from an existing player - now
		// we need to send it onwards to the newly rejoining player

		// Find the session corresponding to the rejoining host (if any)
		CNetServerSession* session = NULL;
		for (size_t i = 0; i < m_Server.m_Sessions.size(); ++i)
		{
			if (m_Server.m_Sessions[i]->GetHostID() == m_RejoinerHostID)
			{
				session = m_Server.m_Sessions[i];
				break;
			}
		}

		if (!session)
		{
			LOGMESSAGE(L"Net server: rejoining client disconnected before we sent to it");
			return;
		}

		// Store the received state file, and tell the client to start downloading it from us
		// TODO: this will get kind of confused if there's multiple clients downloading in parallel;
		// they'll race and get whichever happens to be the latest received by the server,
		// which should still work but isn't great
		m_Server.m_JoinSyncFile = m_Buffer;
		CJoinSyncStartMessage message;
		session->SendMessage(&message);
	}

private:
	CNetServerWorker& m_Server;
	u32 m_RejoinerHostID;
};

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
	m_Host = enet_host_create(&addr, MAX_CLIENTS, CHANNEL_COUNT, 0, 0);
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

#if CONFIG2_MINIUPNPC
	// Launch the UPnP thread
	ret = pthread_create(&m_UPnPThread, NULL, &SetupUPnP, NULL);
	ENSURE(ret == 0);
#endif

	return true;
}

#if CONFIG2_MINIUPNPC
void* CNetServerWorker::SetupUPnP(void*)
{
	// Values we want to set.
	char psPort[6];
	sprintf_s(psPort, ARRAY_SIZE(psPort), "%d", PS_DEFAULT_PORT);
	const char* leaseDuration = "0"; // Indefinite/permanent lease duration.
	const char* description = "0AD Multiplayer";
	const char* protocall = "UDP";
	char internalIPAddress[64];
	char externalIPAddress[40];
	// Variables to hold the values that actually get set.
	char intClient[40];
	char intPort[6];
	char duration[16];
	// Intermediate variables.
	struct UPNPUrls urls;
	struct IGDdatas data;
	struct UPNPDev* devlist = 0;

	// Cached root descriptor URL.
	std::string rootDescURL;
	CFG_GET_VAL("network.upnprootdescurl", String, rootDescURL);
	if (!rootDescURL.empty())
		LOGMESSAGE(L"Net server: attempting to use cached root descriptor URL: %hs", rootDescURL.c_str());

	// Init the return variable for UPNP_GetValidIGD to 1 so things behave when using cached URLs.
	int ret = 1;

	// If we have a cached URL, try that first, otherwise try getting a valid UPnP device for 10 seconds. We also get our LAN address here.
	if (!((!rootDescURL.empty() && UPNP_GetIGDFromUrl(rootDescURL.c_str(), &urls, &data, internalIPAddress, sizeof(internalIPAddress)))
	  || ((devlist = upnpDiscover(10000, 0, 0, 0, 0, 0)) != NULL && (ret = UPNP_GetValidIGD(devlist, &urls, &data, internalIPAddress, sizeof(internalIPAddress))) != 0)))
	{
		LOGMESSAGE(L"Net server: upnpDiscover failed and no working cached URL.");
		return NULL;
	}

	switch (ret)
	{
	case 1:
		LOGMESSAGE(L"Net server: found valid IGD = %hs", urls.controlURL);
		break;
	case 2:
		LOGMESSAGE(L"Net server: found a valid, not connected IGD = %hs, will try to continue anyway", urls.controlURL);
		break;
	case 3:
		LOGMESSAGE(L"Net server: found a UPnP device unrecognized as IGD = %hs, will try to continue anyway", urls.controlURL);
		break;
	default:
		debug_warn(L"Unrecognized return value from UPNP_GetValidIGD");
	}

	// Try getting our external/internet facing IP. TODO: Display this on the game-setup page for conviniance.
	ret = UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, externalIPAddress);
	if (ret != UPNPCOMMAND_SUCCESS)
	{
		LOGMESSAGE(L"Net server: GetExternalIPAddress failed with code %d (%hs)", ret, strupnperror(ret));
		return NULL;
	}
	LOGMESSAGE(L"Net server: ExternalIPAddress = %hs", externalIPAddress);

	// Try to setup port forwarding.
	ret = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype, psPort, psPort,
							internalIPAddress, description, protocall, 0, leaseDuration);
	if (ret != UPNPCOMMAND_SUCCESS)
	{
		LOGMESSAGE(L"Net server: AddPortMapping(%hs, %hs, %hs) failed with code %d (%hs)",
			   psPort, psPort, internalIPAddress, ret, strupnperror(ret));
		return NULL;
	}

	// Check that the port was actually forwarded.
	ret = UPNP_GetSpecificPortMappingEntry(urls.controlURL,
									 data.first.servicetype,
									 psPort, protocall,
#if MINIUPNPC_API_VERSION >= 10
									 NULL/*remoteHost*/,
#endif
									 intClient, intPort, NULL/*desc*/,
									 NULL/*enabled*/, duration);

	if (ret != UPNPCOMMAND_SUCCESS)
	{
		LOGMESSAGE(L"Net server: GetSpecificPortMappingEntry() failed with code %d (%hs)", ret, strupnperror(ret));
		return NULL;
	}

	LOGMESSAGE(L"Net server: External %hs:%hs %hs is redirected to internal %hs:%hs (duration=%hs)",
				   externalIPAddress, psPort, protocall, intClient, intPort, duration);

	// Cache root descriptor URL to try to avoid discovery next time.
	g_ConfigDB.SetValueString(CFG_USER, "network.upnprootdescurl", urls.controlURL);
	g_ConfigDB.WriteFile(CFG_USER);
	LOGMESSAGE(L"Net server: cached UPnP root descriptor URL as %hs", urls.controlURL);

	// Make sure everything is properly freed.
	FreeUPNPUrls(&urls);
	freeUPNPDevlist(devlist);

	return NULL;
}
#endif // CONFIG2_MINIUPNPC

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
	// The script runtime uses the profiler and therefore the thread must be registered before the runtime is created
	g_Profiler2.RegisterCurrentThread("Net server");
	
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

	// Clear roots before deleting their context
	m_GameAttributes = CScriptValRooted();
	m_SavedCommands.clear();

	SAFE_DELETE(m_ScriptInterface);
}

bool CNetServerWorker::RunStep()
{
	// Check for messages from the game thread.
	// (Do as little work as possible while the mutex is held open,
	// to avoid performance problems and deadlocks.)

	std::vector<std::pair<int, CStr> > newAssignPlayer;
	std::vector<bool> newStartGame;
	std::vector<std::pair<CStr, int> > newPlayerReady;
	std::vector<bool> newPlayerResetReady;
	std::vector<std::string> newGameAttributes;
	std::vector<u32> newTurnLength;

	{
		CScopeLock lock(m_WorkerMutex);

		if (m_Shutdown)
			return false;

		newStartGame.swap(m_StartGameQueue);
		newPlayerReady.swap(m_PlayerReadyQueue);
		newPlayerResetReady.swap(m_PlayerResetReadyQueue);
		newAssignPlayer.swap(m_AssignPlayerQueue);
		newGameAttributes.swap(m_GameAttributesQueue);
		newTurnLength.swap(m_TurnLengthQueue);
	}

	for (size_t i = 0; i < newAssignPlayer.size(); ++i)
		AssignPlayer(newAssignPlayer[i].first, newAssignPlayer[i].second);

	for (size_t i = 0; i < newPlayerReady.size(); ++i)
		SetPlayerReady(newPlayerReady[i].first, newPlayerReady[i].second);

	if (!newPlayerResetReady.empty())
		ClearAllPlayerReady();

	if (!newGameAttributes.empty())
		UpdateGameAttributes(GetScriptInterface().ParseJSON(newGameAttributes.back()));

	if (!newTurnLength.empty())
		SetTurnLength(newTurnLength.back());

	// Do StartGame last, so we have the most up-to-date game attributes when we start
	if (!newStartGame.empty())
		StartGame();

	// Perform file transfers
	for (size_t i = 0; i < m_Sessions.size(); ++i)
		m_Sessions[i]->GetFileTransferer().Poll();

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
		LOGMESSAGE(L"Net server: Received connection from %hs:%u", hostname, (unsigned int)event.peer->address.port);

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
	// Handle non-FSM messages first
	Status status = session->GetFileTransferer().HandleMessageReceive(message);
	if (status != INFO::SKIPPED)
		return;

	if (message->GetType() == NMT_FILE_TRANSFER_REQUEST)
	{
		CFileTransferRequestMessage* reqMessage = (CFileTransferRequestMessage*)message;

		// Rejoining client got our JoinSyncStart after we received the state from
		// another client, and has now requested that we forward it to them

		ENSURE(!m_JoinSyncFile.empty());
		session->GetFileTransferer().StartResponse(reqMessage->m_RequestID, m_JoinSyncFile);

		return;
	}

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
	session->AddTransition(NSS_PREGAME, (uint)NMT_READY, NSS_PREGAME, (void*)&OnReady, context);
	session->AddTransition(NSS_PREGAME, (uint)NMT_LOADED_GAME, NSS_INGAME, (void*)&OnLoadedGame, context);

	session->AddTransition(NSS_JOIN_SYNCING, (uint)NMT_LOADED_GAME, NSS_INGAME, (void*)&OnJoinSyncingLoadedGame, context);

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
	// Find all player IDs in active use; we mustn't give them to a second player (excluding the unassigned ID: -1)
	std::set<i32> usedIDs;
	for (PlayerAssignmentMap::iterator it = m_PlayerAssignments.begin(); it != m_PlayerAssignments.end(); ++it)
		if (it->second.m_Enabled && it->second.m_PlayerID != -1)
			usedIDs.insert(it->second.m_PlayerID);

	// If the player is rejoining after disconnecting, try to give them
	// back their old player ID

	i32 playerID = -1;

	// Try to match GUID first
	for (PlayerAssignmentMap::iterator it = m_PlayerAssignments.begin(); it != m_PlayerAssignments.end(); ++it)
	{
		if (!it->second.m_Enabled && it->first == guid && usedIDs.find(it->second.m_PlayerID) == usedIDs.end())
		{
			playerID = it->second.m_PlayerID;
			m_PlayerAssignments.erase(it); // delete the old mapping, since we've got a new one now
			goto found;
		}
	}

	// Try to match username next
	for (PlayerAssignmentMap::iterator it = m_PlayerAssignments.begin(); it != m_PlayerAssignments.end(); ++it)
	{
		if (!it->second.m_Enabled && it->second.m_Name == name && usedIDs.find(it->second.m_PlayerID) == usedIDs.end())
		{
			playerID = it->second.m_PlayerID;
			m_PlayerAssignments.erase(it); // delete the old mapping, since we've got a new one now
			goto found;
		}
	}

	// Otherwise leave the player ID as -1 (observer) and let gamesetup change it as needed.

found:
	PlayerAssignment assignment;
	assignment.m_Enabled = true;
	assignment.m_Name = name;
	assignment.m_PlayerID = playerID;
	assignment.m_Status = 0;
	m_PlayerAssignments[guid] = assignment;

	// Send the new assignments to all currently active players
	// (which does not include the one that's just joining)
	SendPlayerAssignments();
}

void CNetServerWorker::RemovePlayer(const CStr& guid)
{
	m_PlayerAssignments[guid].m_Enabled = false;

	SendPlayerAssignments();
}

void CNetServerWorker::SetPlayerReady(const CStr& guid, const int ready)
{
	m_PlayerAssignments[guid].m_Status = ready;

	SendPlayerAssignments();
}

void CNetServerWorker::ClearAllPlayerReady()
{
	for (PlayerAssignmentMap::iterator it = m_PlayerAssignments.begin(); it != m_PlayerAssignments.end(); ++it)
		it->second.m_Status = 0;

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
		if (!it->second.m_Enabled)
			continue;

		CPlayerAssignmentMessage::S_m_Hosts h;
		h.m_GUID = it->first;
		h.m_Name = it->second.m_Name;
		h.m_PlayerID = it->second.m_PlayerID;
		h.m_Status = it->second.m_Status;
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

	CAuthenticateMessage* message = (CAuthenticateMessage*)event->GetParamRef();

	CStrW username = server.DeduplicatePlayerName(SanitisePlayerName(message->m_Name));

	bool isRejoining = false;

	if (server.m_State != SERVER_STATE_PREGAME)
	{
// 		isRejoining = true; // uncomment this to test rejoining even if the player wasn't connected previously

		// Search for an old disconnected player of the same name
		// (TODO: if GUIDs were stable, we should use them instead)
		for (PlayerAssignmentMap::iterator it = server.m_PlayerAssignments.begin(); it != server.m_PlayerAssignments.end(); ++it)
		{
			if (!it->second.m_Enabled && it->second.m_Name == username)
			{
				isRejoining = true;
				break;
			}
		}

		// Players who weren't already in the game are not allowed to join now that it's started
		if (!isRejoining)
		{
			LOGMESSAGE(L"Refused connection after game start from not-previously-known user \"%ls\"", username.c_str());
			session->Disconnect(NDR_SERVER_ALREADY_IN_GAME);
			return true;
		}
	}

	// TODO: check server password etc?

	u32 newHostID = server.m_NextHostID++;

	session->SetUserName(username);
	session->SetGUID(message->m_GUID);
	session->SetHostID(newHostID);

	CAuthenticateResultMessage authenticateResult;
	authenticateResult.m_Code = isRejoining ? ARC_OK_REJOINING : ARC_OK;
	authenticateResult.m_HostID = newHostID;
	authenticateResult.m_Message = L"Logged in";
	session->SendMessage(&authenticateResult);

	server.OnUserJoin(session);

	if (isRejoining)
	{
		// Request a copy of the current game state from an existing player,
		// so we can send it on to the new player

		// Assume session 0 is most likely the local player, so they're
		// the most efficient client to request a copy from
		CNetServerSession* sourceSession = server.m_Sessions.at(0);
		sourceSession->GetFileTransferer().StartTask(
			shared_ptr<CNetFileReceiveTask>(new CNetFileReceiveTask_ServerRejoin(server, newHostID))
		);

		session->SetNextState(NSS_JOIN_SYNCING);
	}

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

		// Save all the received commands
		if (server.m_SavedCommands.size() < simMessage->m_Turn + 1)
			server.m_SavedCommands.resize(simMessage->m_Turn + 1);
		server.m_SavedCommands[simMessage->m_Turn].push_back(*simMessage);

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

bool CNetServerWorker::OnReady(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_READY);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	CReadyMessage* message = (CReadyMessage*)event->GetParamRef();

	message->m_GUID = session->GetGUID();

	server.Broadcast(message);

	return true;
}

bool CNetServerWorker::OnLoadedGame(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_LOADED_GAME);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	// We're in the loading state, so wait until every player has loaded before
	// starting the game
	ENSURE(server.m_State == SERVER_STATE_LOADING);
	server.CheckGameLoadStatus(session);

	return true;
}

bool CNetServerWorker::OnJoinSyncingLoadedGame(void* context, CFsmEvent* event)
{
	// A client rejoining an in-progress game has now finished loading the
	// map and deserialized the initial state.
	// The simulation may have progressed since then, so send any subsequent
	// commands to them and set them as an active player so they can participate
	// in all future turns.
	// 
	// (TODO: if it takes a long time for them to receive and execute all these
	// commands, the other players will get frozen for that time and may be unhappy;
	// we could try repeating this process a few times until the client converges
	// on the up-to-date state, before setting them as active.)

	ENSURE(event->GetType() == (uint)NMT_LOADED_GAME);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	CLoadedGameMessage* message = (CLoadedGameMessage*)event->GetParamRef();

	u32 turn = message->m_CurrentTurn;
	u32 readyTurn = server.m_ServerTurnManager->GetReadyTurn();

	// Send them all commands received since their saved state,
	// and turn-ended messages for any turns that have already been processed
	for (size_t i = turn + 1; i < std::max(readyTurn+1, (u32)server.m_SavedCommands.size()); ++i)
	{
		if (i < server.m_SavedCommands.size())
			for (size_t j = 0; j < server.m_SavedCommands[i].size(); ++j)
				session->SendMessage(&server.m_SavedCommands[i][j]);

		if (i <= readyTurn)
		{
			CEndCommandBatchMessage endMessage;
			endMessage.m_Turn = i;
			endMessage.m_TurnLength = server.m_ServerTurnManager->GetSavedTurnLength(i);
			session->SendMessage(&endMessage);
		}
	}

	// Tell the turn manager to expect commands from this new client
	server.m_ServerTurnManager->InitialiseClient(session->GetHostID(), readyTurn);

	// Tell the client that everything has finished loading and it should start now
	CLoadedGameMessage loaded;
	loaded.m_CurrentTurn = readyTurn;
	session->SendMessage(&loaded);

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
	loaded.m_CurrentTurn = 0;
	Broadcast(&loaded);

	m_State = SERVER_STATE_INGAME;
}

void CNetServerWorker::StartGame()
{
	m_ServerTurnManager = new CNetServerTurnManager(*this);

	for (size_t i = 0; i < m_Sessions.size(); ++i)
		m_ServerTurnManager->InitialiseClient(m_Sessions[i]->GetHostID(), 0); // TODO: only for non-observers

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

void CNetServer::SetPlayerReady(const CStr& guid, int ready)
{
	CScopeLock lock(m_Worker->m_WorkerMutex);
	m_Worker->m_PlayerReadyQueue.push_back(std::make_pair(guid, ready));
}

void CNetServer::ClearAllPlayerReady()
{
	CScopeLock lock(m_Worker->m_WorkerMutex);
	m_Worker->m_PlayerResetReadyQueue.push_back(false);
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
