/* Copyright (C) 2022 Wildfire Games.
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
#include "NetServerTurnManager.h"
#include "NetStats.h"

#include "lib/external_libraries/enet.h"
#include "lib/types.h"
#include "network/StunClient.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/GUID.h"
#include "ps/Hashing.h"
#include "ps/Profile.h"
#include "ps/Threading.h"
#include "scriptinterface/ScriptContext.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/JSON.h"
#include "simulation2/Simulation2.h"
#include "simulation2/system/TurnManager.h"

#if CONFIG2_MINIUPNPC
#include <miniupnpc/miniwget.h>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>
#endif

#include <string>

/**
 * Number of peers to allocate for the enet host.
 * Limited by ENET_PROTOCOL_MAXIMUM_PEER_ID (4096).
 *
 * At most 8 players, 32 observers and 1 temporary connection to send the "server full" disconnect-reason.
 */
#define MAX_CLIENTS 41

#define	DEFAULT_SERVER_NAME			L"Unnamed Server"

constexpr int CHANNEL_COUNT = 1;
constexpr int FAILED_PASSWORD_TRIES_BEFORE_BAN = 3;

/**
 * enet_host_service timeout (msecs).
 * Smaller numbers may hurt performance; larger numbers will
 * hurt latency responding to messages from game thread.
 */
static const int HOST_SERVICE_TIMEOUT = 50;

/**
 * Once ping goes above turn length * command delay,
 * the game will start 'freezing' for other clients while we catch up.
 * Since commands are sent client -> server -> client, divide by 2.
 * (duplicated in NetServer.cpp to avoid having to fetch the constants in a header file)
 */
constexpr u32 NETWORK_BAD_PING = DEFAULT_TURN_LENGTH * COMMAND_DELAY_MP / 2;

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
		for (CNetServerSession* serverSession : m_Server.m_Sessions)
		{
			if (serverSession->GetHostID() == m_RejoinerHostID)
			{
				session = serverSession;
				break;
			}
		}

		if (!session)
		{
			LOGMESSAGE("Net server: rejoining client disconnected before we sent to it");
			return;
		}

		// Store the received state file, and tell the client to start downloading it from us
		// TODO: this will get kind of confused if there's multiple clients downloading in parallel;
		// they'll race and get whichever happens to be the latest received by the server,
		// which should still work but isn't great
		m_Server.m_JoinSyncFile = m_Buffer;

		// Send the init attributes alongside - these should be correct since the game should be started.
		CJoinSyncStartMessage message;
		message.m_InitAttributes = Script::StringifyJSON(ScriptRequest(m_Server.GetScriptInterface()), &m_Server.m_InitAttributes);
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

CNetServerWorker::CNetServerWorker(bool useLobbyAuth) :
	m_LobbyAuth(useLobbyAuth),
	m_Shutdown(false),
	m_ScriptInterface(NULL),
	m_NextHostID(1), m_Host(NULL), m_ControllerGUID(), m_Stats(NULL),
	m_LastConnectionCheck(0)
{
	m_State = SERVER_STATE_UNCONNECTED;

	m_ServerTurnManager = NULL;

	m_ServerName = DEFAULT_SERVER_NAME;
}

CNetServerWorker::~CNetServerWorker()
{
	if (m_State != SERVER_STATE_UNCONNECTED)
	{
		// Tell the thread to shut down
		{
			std::lock_guard<std::mutex> lock(m_WorkerMutex);
			m_Shutdown = true;
		}

		// Wait for it to shut down cleanly
		m_WorkerThread.join();
	}

#if CONFIG2_MINIUPNPC
	if (m_UPnPThread.joinable())
		m_UPnPThread.detach();
#endif

	// Clean up resources

	delete m_Stats;

	for (CNetServerSession* session : m_Sessions)
	{
		session->DisconnectNow(NDR_SERVER_SHUTDOWN);
		delete session;
	}

	if (m_Host)
		enet_host_destroy(m_Host);

	delete m_ServerTurnManager;
}

void CNetServerWorker::SetPassword(const CStr& hashedPassword)
{
	m_Password = hashedPassword;
}


void CNetServerWorker::SetControllerSecret(const std::string& secret)
{
	m_ControllerSecret = secret;
}


bool CNetServerWorker::CheckPassword(const std::string& password, const std::string& salt) const
{
	return HashCryptographically(m_Password, salt) == password;
}


bool CNetServerWorker::SetupConnection(const u16 port)
{
	ENSURE(m_State == SERVER_STATE_UNCONNECTED);
	ENSURE(!m_Host);

	// Bind to default host
	ENetAddress addr;
	addr.host = ENET_HOST_ANY;
	addr.port = port;

	// Create ENet server
	m_Host = enet_host_create(&addr, MAX_CLIENTS, CHANNEL_COUNT, 0, 0);
	if (!m_Host)
	{
		LOGERROR("Net server: enet_host_create failed");
		return false;
	}

	m_Stats = new CNetStatsTable();
	if (CProfileViewer::IsInitialised())
		g_ProfileViewer.AddRootTable(m_Stats);

	m_State = SERVER_STATE_PREGAME;

	// Launch the worker thread
	m_WorkerThread = std::thread(Threading::HandleExceptions<RunThread>::Wrapper, this);

#if CONFIG2_MINIUPNPC
	// Launch the UPnP thread
	m_UPnPThread = std::thread(Threading::HandleExceptions<SetupUPnP>::Wrapper);
#endif

	return true;
}

#if CONFIG2_MINIUPNPC
void CNetServerWorker::SetupUPnP()
{
	debug_SetThreadName("UPnP");

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
	bool allocatedUrls = false;
	struct UPNPUrls urls;
	struct IGDdatas data;
	struct UPNPDev* devlist = NULL;

	// Make sure everything is properly freed.
	std::function<void()> freeUPnP = [&allocatedUrls, &urls, &devlist]()
	{
		if (allocatedUrls)
			FreeUPNPUrls(&urls);
		freeUPNPDevlist(devlist);
		// IGDdatas does not need to be freed according to UPNP_GetIGDFromUrl
	};

	// Cached root descriptor URL.
	std::string rootDescURL;
	CFG_GET_VAL("network.upnprootdescurl", rootDescURL);
	if (!rootDescURL.empty())
		LOGMESSAGE("Net server: attempting to use cached root descriptor URL: %s", rootDescURL.c_str());

	int ret = 0;

	// Try a cached URL first
	if (!rootDescURL.empty() && UPNP_GetIGDFromUrl(rootDescURL.c_str(), &urls, &data, internalIPAddress, sizeof(internalIPAddress)))
	{
		LOGMESSAGE("Net server: using cached IGD = %s", urls.controlURL);
		ret = 1;
	}
	// No cached URL, or it did not respond. Try getting a valid UPnP device for 10 seconds.
#if defined(MINIUPNPC_API_VERSION) && MINIUPNPC_API_VERSION >= 14
	else if ((devlist = upnpDiscover(10000, 0, 0, 0, 0, 2, 0)) != NULL)
#else
	else if ((devlist = upnpDiscover(10000, 0, 0, 0, 0, 0)) != NULL)
#endif
	{
		ret = UPNP_GetValidIGD(devlist, &urls, &data, internalIPAddress, sizeof(internalIPAddress));
		allocatedUrls = ret != 0; // urls is allocated on non-zero return values
	}
	else
	{
		LOGMESSAGE("Net server: upnpDiscover failed and no working cached URL.");
		freeUPnP();
		return;
	}

	switch (ret)
	{
	case 0:
		LOGMESSAGE("Net server: No IGD found");
		break;
	case 1:
		LOGMESSAGE("Net server: found valid IGD = %s", urls.controlURL);
		break;
	case 2:
		LOGMESSAGE("Net server: found a valid, not connected IGD = %s, will try to continue anyway", urls.controlURL);
		break;
	case 3:
		LOGMESSAGE("Net server: found a UPnP device unrecognized as IGD = %s, will try to continue anyway", urls.controlURL);
		break;
	default:
		debug_warn(L"Unrecognized return value from UPNP_GetValidIGD");
	}

	// Try getting our external/internet facing IP. TODO: Display this on the game-setup page for conviniance.
	ret = UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, externalIPAddress);
	if (ret != UPNPCOMMAND_SUCCESS)
	{
		LOGMESSAGE("Net server: GetExternalIPAddress failed with code %d (%s)", ret, strupnperror(ret));
		freeUPnP();
		return;
	}
	LOGMESSAGE("Net server: ExternalIPAddress = %s", externalIPAddress);

	// Try to setup port forwarding.
	ret = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype, psPort, psPort,
							internalIPAddress, description, protocall, 0, leaseDuration);
	if (ret != UPNPCOMMAND_SUCCESS)
	{
		LOGMESSAGE("Net server: AddPortMapping(%s, %s, %s) failed with code %d (%s)",
			   psPort, psPort, internalIPAddress, ret, strupnperror(ret));
		freeUPnP();
		return;
	}

	// Check that the port was actually forwarded.
	ret = UPNP_GetSpecificPortMappingEntry(urls.controlURL,
									 data.first.servicetype,
									 psPort, protocall,
#if defined(MINIUPNPC_API_VERSION) && MINIUPNPC_API_VERSION >= 10
									 NULL/*remoteHost*/,
#endif
									 intClient, intPort, NULL/*desc*/,
									 NULL/*enabled*/, duration);

	if (ret != UPNPCOMMAND_SUCCESS)
	{
		LOGMESSAGE("Net server: GetSpecificPortMappingEntry() failed with code %d (%s)", ret, strupnperror(ret));
		freeUPnP();
		return;
	}

	LOGMESSAGE("Net server: External %s:%s %s is redirected to internal %s:%s (duration=%s)",
				   externalIPAddress, psPort, protocall, intClient, intPort, duration);

	// Cache root descriptor URL to try to avoid discovery next time.
	g_ConfigDB.SetValueString(CFG_USER, "network.upnprootdescurl", urls.controlURL);
	g_ConfigDB.WriteValueToFile(CFG_USER, "network.upnprootdescurl", urls.controlURL);
	LOGMESSAGE("Net server: cached UPnP root descriptor URL as %s", urls.controlURL);

	freeUPnP();
}
#endif // CONFIG2_MINIUPNPC

bool CNetServerWorker::SendMessage(ENetPeer* peer, const CNetMessage* message)
{
	ENSURE(m_Host);

	CNetServerSession* session = static_cast<CNetServerSession*>(peer->data);

	return CNetHost::SendMessage(message, peer, DebugName(session).c_str());
}

bool CNetServerWorker::Broadcast(const CNetMessage* message, const std::vector<NetServerSessionState>& targetStates)
{
	ENSURE(m_Host);

	bool ok = true;

	// TODO: this does lots of repeated message serialisation if we have lots
	// of remote peers; could do it more efficiently if that's a real problem

	for (CNetServerSession* session : m_Sessions)
		if (std::find(targetStates.begin(), targetStates.end(), static_cast<NetServerSessionState>(session->GetCurrState())) != targetStates.end() &&
		    !session->SendMessage(message))
			ok = false;

	return ok;
}

void CNetServerWorker::RunThread(CNetServerWorker* data)
{
	debug_SetThreadName("NetServer");

	data->Run();
}

void CNetServerWorker::Run()
{
	// The script context uses the profiler and therefore the thread must be registered before the context is created
	g_Profiler2.RegisterCurrentThread("Net server");

	// We create a new ScriptContext for this network thread, with a single ScriptInterface.
	std::shared_ptr<ScriptContext> netServerContext = ScriptContext::CreateContext();
	m_ScriptInterface = new ScriptInterface("Engine", "Net server", netServerContext);
	m_InitAttributes.init(m_ScriptInterface->GetGeneralJSContext(), JS::UndefinedValue());

	while (true)
	{
		if (!RunStep())
			break;

		// Update profiler stats
		m_Stats->LatchHostState(m_Host);
	}

	// Clear roots before deleting their context
	m_SavedCommands.clear();

	SAFE_DELETE(m_ScriptInterface);
}

bool CNetServerWorker::RunStep()
{
	// Check for messages from the game thread.
	// (Do as little work as possible while the mutex is held open,
	// to avoid performance problems and deadlocks.)

	m_ScriptInterface->GetContext()->MaybeIncrementalGC(0.5f);

	ScriptRequest rq(m_ScriptInterface);

	std::vector<bool> newStartGame;
	std::vector<std::string> newGameAttributes;
	std::vector<std::pair<CStr, CStr>> newLobbyAuths;
	std::vector<u32> newTurnLength;

	{
		std::lock_guard<std::mutex> lock(m_WorkerMutex);

		if (m_Shutdown)
			return false;

		newStartGame.swap(m_StartGameQueue);
		newGameAttributes.swap(m_InitAttributesQueue);
		newLobbyAuths.swap(m_LobbyAuthQueue);
		newTurnLength.swap(m_TurnLengthQueue);
	}

	if (!newGameAttributes.empty())
	{
		if (m_State != SERVER_STATE_UNCONNECTED && m_State != SERVER_STATE_PREGAME)
			LOGERROR("NetServer: Init Attributes cannot be changed after the server starts loading.");
		else
		{
			JS::RootedValue gameAttributesVal(rq.cx);
			Script::ParseJSON(rq, newGameAttributes.back(), &gameAttributesVal);
			m_InitAttributes = gameAttributesVal;
		}
	}

	if (!newTurnLength.empty())
		SetTurnLength(newTurnLength.back());

	while (!newLobbyAuths.empty())
	{
		const std::pair<CStr, CStr>& auth = newLobbyAuths.back();
		ProcessLobbyAuth(auth.first, auth.second);
		newLobbyAuths.pop_back();
	}

	// Perform file transfers
	for (CNetServerSession* session : m_Sessions)
		session->GetFileTransferer().Poll();

	CheckClientConnections();

	// Process network events:

	ENetEvent event;
	int status = enet_host_service(m_Host, &event, HOST_SERVICE_TIMEOUT);
	if (status < 0)
	{
		LOGERROR("CNetServerWorker: enet_host_service failed (%d)", status);
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
		LOGMESSAGE("Net server: Received connection from %s:%u", hostname, (unsigned int)event.peer->address.port);

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
			LOGMESSAGE("Net server: Disconnected %s", DebugName(session).c_str());

			// Remove the session first, so we won't send player-update messages to it
			// when updating the FSM
			m_Sessions.erase(remove(m_Sessions.begin(), m_Sessions.end(), session), m_Sessions.end());

			session->Update((uint)NMT_CONNECTION_LOST, NULL);

			delete session;
			event.peer->data = NULL;
		}

		if (m_State == SERVER_STATE_LOADING)
			CheckGameLoadStatus(NULL);

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
				LOGMESSAGE("Net server: Received message %s of size %lu from %s", msg->ToString().c_str(), (unsigned long)msg->GetSerializedLength(), DebugName(session).c_str());

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

void CNetServerWorker::CheckClientConnections()
{
	// Send messages at most once per second
	std::time_t now = std::time(nullptr);
	if (now <= m_LastConnectionCheck)
		return;

	m_LastConnectionCheck = now;

	for (size_t i = 0; i < m_Sessions.size(); ++i)
	{
		u32 lastReceived = m_Sessions[i]->GetLastReceivedTime();
		u32 meanRTT = m_Sessions[i]->GetMeanRTT();

		CNetMessage* message = nullptr;

		// Report if we didn't hear from the client since few seconds
		if (lastReceived > NETWORK_WARNING_TIMEOUT)
		{
			CClientTimeoutMessage* msg = new CClientTimeoutMessage();
			msg->m_GUID = m_Sessions[i]->GetGUID();
			msg->m_LastReceivedTime = lastReceived;
			message = msg;
		}
		// Report if the client has bad ping
		else if (meanRTT > NETWORK_BAD_PING)
		{
			CClientPerformanceMessage* msg = new CClientPerformanceMessage();
			CClientPerformanceMessage::S_m_Clients client;
			client.m_GUID = m_Sessions[i]->GetGUID();
			client.m_MeanRTT = meanRTT;
			msg->m_Clients.push_back(client);
			message = msg;
		}

		// Send to all clients except the affected one
		// (since that will show the locally triggered warning instead).
		// Also send it to clients that finished the loading screen while
		// the game is still waiting for other clients to finish the loading screen.
		if (message)
			for (size_t j = 0; j < m_Sessions.size(); ++j)
			{
				if (i != j && (
				    (m_Sessions[j]->GetCurrState() == NSS_PREGAME && m_State == SERVER_STATE_PREGAME) ||
				    m_Sessions[j]->GetCurrState() == NSS_INGAME))
				{
					m_Sessions[j]->SendMessage(message);
				}
			}

		SAFE_DELETE(message);
	}
}

void CNetServerWorker::HandleMessageReceive(const CNetMessage* message, CNetServerSession* session)
{
	// Handle non-FSM messages first
	Status status = session->GetFileTransferer().HandleMessageReceive(*message);
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
	if (!session->Update(message->GetType(), (void*)message))
		LOGERROR("Net server: Error running FSM update (type=%d state=%d)", (int)message->GetType(), (int)session->GetCurrState());
}

void CNetServerWorker::SetupSession(CNetServerSession* session)
{
	void* context = session;

	// Set up transitions for session

	session->AddTransition(NSS_UNCONNECTED, (uint)NMT_CONNECTION_LOST, NSS_UNCONNECTED);

	session->AddTransition(NSS_HANDSHAKE, (uint)NMT_CONNECTION_LOST, NSS_UNCONNECTED);
	session->AddTransition(NSS_HANDSHAKE, (uint)NMT_CLIENT_HANDSHAKE, NSS_AUTHENTICATE, (void*)&OnClientHandshake, context);

	session->AddTransition(NSS_LOBBY_AUTHENTICATE, (uint)NMT_CONNECTION_LOST, NSS_UNCONNECTED);
	session->AddTransition(NSS_LOBBY_AUTHENTICATE, (uint)NMT_AUTHENTICATE, NSS_PREGAME, (void*)&OnAuthenticate, context);

	session->AddTransition(NSS_AUTHENTICATE, (uint)NMT_CONNECTION_LOST, NSS_UNCONNECTED);
	session->AddTransition(NSS_AUTHENTICATE, (uint)NMT_AUTHENTICATE, NSS_PREGAME, (void*)&OnAuthenticate, context);

	session->AddTransition(NSS_PREGAME, (uint)NMT_CONNECTION_LOST, NSS_UNCONNECTED, (void*)&OnDisconnect, context);
	session->AddTransition(NSS_PREGAME, (uint)NMT_CHAT, NSS_PREGAME, (void*)&OnChat, context);
	session->AddTransition(NSS_PREGAME, (uint)NMT_READY, NSS_PREGAME, (void*)&OnReady, context);
	session->AddTransition(NSS_PREGAME, (uint)NMT_CLEAR_ALL_READY, NSS_PREGAME, (void*)&OnClearAllReady, context);
	session->AddTransition(NSS_PREGAME, (uint)NMT_GAME_SETUP, NSS_PREGAME, (void*)&OnGameSetup, context);
	session->AddTransition(NSS_PREGAME, (uint)NMT_ASSIGN_PLAYER, NSS_PREGAME, (void*)&OnAssignPlayer, context);
	session->AddTransition(NSS_PREGAME, (uint)NMT_KICKED, NSS_PREGAME, (void*)&OnKickPlayer, context);
	session->AddTransition(NSS_PREGAME, (uint)NMT_GAME_START, NSS_PREGAME, (void*)&OnGameStart, context);
	session->AddTransition(NSS_PREGAME, (uint)NMT_LOADED_GAME, NSS_INGAME, (void*)&OnLoadedGame, context);

	session->AddTransition(NSS_JOIN_SYNCING, (uint)NMT_KICKED, NSS_JOIN_SYNCING, (void*)&OnKickPlayer, context);
	session->AddTransition(NSS_JOIN_SYNCING, (uint)NMT_CONNECTION_LOST, NSS_UNCONNECTED, (void*)&OnDisconnect, context);
	session->AddTransition(NSS_JOIN_SYNCING, (uint)NMT_LOADED_GAME, NSS_INGAME, (void*)&OnJoinSyncingLoadedGame, context);

	session->AddTransition(NSS_INGAME, (uint)NMT_REJOINED, NSS_INGAME, (void*)&OnRejoined, context);
	session->AddTransition(NSS_INGAME, (uint)NMT_KICKED, NSS_INGAME, (void*)&OnKickPlayer, context);
	session->AddTransition(NSS_INGAME, (uint)NMT_CLIENT_PAUSED, NSS_INGAME, (void*)&OnClientPaused, context);
	session->AddTransition(NSS_INGAME, (uint)NMT_CONNECTION_LOST, NSS_UNCONNECTED, (void*)&OnDisconnect, context);
	session->AddTransition(NSS_INGAME, (uint)NMT_CHAT, NSS_INGAME, (void*)&OnChat, context);
	session->AddTransition(NSS_INGAME, (uint)NMT_SIMULATION_COMMAND, NSS_INGAME, (void*)&OnSimulationCommand, context);
	session->AddTransition(NSS_INGAME, (uint)NMT_SYNC_CHECK, NSS_INGAME, (void*)&OnSyncCheck, context);
	session->AddTransition(NSS_INGAME, (uint)NMT_END_COMMAND_BATCH, NSS_INGAME, (void*)&OnEndCommandBatch, context);

	// Set first state
	session->SetFirstState(NSS_HANDSHAKE);
}

bool CNetServerWorker::HandleConnect(CNetServerSession* session)
{
	if (std::find(m_BannedIPs.begin(), m_BannedIPs.end(), session->GetIPAddress()) != m_BannedIPs.end())
	{
		session->Disconnect(NDR_BANNED);
		return false;
	}

	CSrvHandshakeMessage handshake;
	handshake.m_Magic = PS_PROTOCOL_MAGIC;
	handshake.m_ProtocolVersion = PS_PROTOCOL_VERSION;
	handshake.m_SoftwareVersion = PS_PROTOCOL_VERSION;
	return session->SendMessage(&handshake);
}

void CNetServerWorker::OnUserJoin(CNetServerSession* session)
{
	AddPlayer(session->GetGUID(), session->GetUserName());

	CPlayerAssignmentMessage assignMessage;
	ConstructPlayerAssignmentMessage(assignMessage);
	session->SendMessage(&assignMessage);
}

void CNetServerWorker::OnUserLeave(CNetServerSession* session)
{
	std::vector<CStr>::iterator pausing = std::find(m_PausingPlayers.begin(), m_PausingPlayers.end(), session->GetGUID());
	if (pausing != m_PausingPlayers.end())
		m_PausingPlayers.erase(pausing);

	RemovePlayer(session->GetGUID());

	if (m_ServerTurnManager && session->GetCurrState() != NSS_JOIN_SYNCING)
		m_ServerTurnManager->UninitialiseClient(session->GetHostID());

	// TODO: ought to switch the player controlled by that client
	// back to AI control, or something?
}

void CNetServerWorker::AddPlayer(const CStr& guid, const CStrW& name)
{
	// Find all player IDs in active use; we mustn't give them to a second player (excluding the unassigned ID: -1)
	std::set<i32> usedIDs;
	for (const std::pair<const CStr, PlayerAssignment>& p : m_PlayerAssignments)
		if (p.second.m_Enabled && p.second.m_PlayerID != -1)
			usedIDs.insert(p.second.m_PlayerID);

	// If the player is rejoining after disconnecting, try to give them
	// back their old player ID. Don't do this in pregame however,
	// as that ID might be invalid for various reasons.

	i32 playerID = -1;

	if (m_State != SERVER_STATE_UNCONNECTED && m_State != SERVER_STATE_PREGAME)
	{
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
	}

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

void CNetServerWorker::ClearAllPlayerReady()
{
	for (std::pair<const CStr, PlayerAssignment>& p : m_PlayerAssignments)
		if (p.second.m_Status != 2)
			p.second.m_Status = 0;

	SendPlayerAssignments();
}

void CNetServerWorker::KickPlayer(const CStrW& playerName, const bool ban)
{
	// Find the user with that name
	std::vector<CNetServerSession*>::iterator it = std::find_if(m_Sessions.begin(), m_Sessions.end(),
		[&](CNetServerSession* session) { return session->GetUserName() == playerName; });

	// and return if no one or the host has that name
	if (it == m_Sessions.end() || (*it)->GetGUID() == m_ControllerGUID)
		return;

	if (ban)
	{
		// Remember name
		if (std::find(m_BannedPlayers.begin(), m_BannedPlayers.end(), playerName) == m_BannedPlayers.end())
			m_BannedPlayers.push_back(m_LobbyAuth ? CStrW(playerName.substr(0, playerName.find(L" ("))) : playerName);

		// Remember IP address
		u32 ipAddress = (*it)->GetIPAddress();
		if (std::find(m_BannedIPs.begin(), m_BannedIPs.end(), ipAddress) == m_BannedIPs.end())
			m_BannedIPs.push_back(ipAddress);
	}

	// Disconnect that user
	(*it)->Disconnect(ban ? NDR_BANNED : NDR_KICKED);

	// Send message notifying other clients
	CKickedMessage kickedMessage;
	kickedMessage.m_Name = playerName;
	kickedMessage.m_Ban = ban;
	Broadcast(&kickedMessage, { NSS_PREGAME, NSS_JOIN_SYNCING, NSS_INGAME });
}

void CNetServerWorker::AssignPlayer(int playerID, const CStr& guid)
{
	// Remove anyone who's already assigned to this player
	for (std::pair<const CStr, PlayerAssignment>& p : m_PlayerAssignments)
	{
		if (p.second.m_PlayerID == playerID)
			p.second.m_PlayerID = -1;
	}

	// Update this host's assignment if it exists
	if (m_PlayerAssignments.find(guid) != m_PlayerAssignments.end())
		m_PlayerAssignments[guid].m_PlayerID = playerID;

	SendPlayerAssignments();
}

void CNetServerWorker::ConstructPlayerAssignmentMessage(CPlayerAssignmentMessage& message)
{
	for (const std::pair<const CStr, PlayerAssignment>& p : m_PlayerAssignments)
	{
		if (!p.second.m_Enabled)
			continue;

		CPlayerAssignmentMessage::S_m_Hosts h;
		h.m_GUID = p.first;
		h.m_Name = p.second.m_Name;
		h.m_PlayerID = p.second.m_PlayerID;
		h.m_Status = p.second.m_Status;
		message.m_Hosts.push_back(h);
	}
}

void CNetServerWorker::SendPlayerAssignments()
{
	CPlayerAssignmentMessage message;
	ConstructPlayerAssignmentMessage(message);
	Broadcast(&message, { NSS_PREGAME, NSS_JOIN_SYNCING, NSS_INGAME });
}

const ScriptInterface& CNetServerWorker::GetScriptInterface()
{
	return *m_ScriptInterface;
}

void CNetServerWorker::SetTurnLength(u32 msecs)
{
	if (m_ServerTurnManager)
		m_ServerTurnManager->SetTurnLength(msecs);
}

void CNetServerWorker::ProcessLobbyAuth(const CStr& name, const CStr& token)
{
	LOGMESSAGE("Net Server: Received lobby auth message from %s with %s", name, token);
	// Find the user with that guid
	std::vector<CNetServerSession*>::iterator it = std::find_if(m_Sessions.begin(), m_Sessions.end(),
		[&](CNetServerSession* session)
		{ return session->GetGUID() == token; });

	if (it == m_Sessions.end())
		return;

	(*it)->SetUserName(name.FromUTF8());
	// Send an empty message to request the authentication message from the client
	// after its identity has been confirmed via the lobby
	CAuthenticateMessage emptyMessage;
	(*it)->SendMessage(&emptyMessage);
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

	CStr guid = ps_generate_guid();
	int count = 0;
	// Ensure unique GUID
	while(std::find_if(
		server.m_Sessions.begin(), server.m_Sessions.end(),
		[&guid] (const CNetServerSession* session)
		{ return session->GetGUID() == guid; }) != server.m_Sessions.end())
	{
		if (++count > 100)
		{
			session->Disconnect(NDR_GUID_FAILED);
			return true;
		}
		guid = ps_generate_guid();
	}

	session->SetGUID(guid);

	CSrvHandshakeResponseMessage handshakeResponse;
	handshakeResponse.m_UseProtocolVersion = PS_PROTOCOL_VERSION;
	handshakeResponse.m_GUID = guid;
	handshakeResponse.m_Flags = 0;

	if (server.m_LobbyAuth)
	{
		handshakeResponse.m_Flags |= PS_NETWORK_FLAG_REQUIRE_LOBBYAUTH;
		session->SetNextState(NSS_LOBBY_AUTHENTICATE);
	}

	session->SendMessage(&handshakeResponse);

	return true;
}

bool CNetServerWorker::OnAuthenticate(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_AUTHENTICATE);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	// Prohibit joins while the game is loading
	if (server.m_State == SERVER_STATE_LOADING)
	{
		LOGMESSAGE("Refused connection while the game is loading");
		session->Disconnect(NDR_SERVER_LOADING);
		return true;
	}

	CAuthenticateMessage* message = (CAuthenticateMessage*)event->GetParamRef();
	CStrW username = SanitisePlayerName(message->m_Name);
	CStrW usernameWithoutRating(username.substr(0, username.find(L" (")));

	// Compare the lowercase names as specified by https://xmpp.org/extensions/xep-0029.html#sect-idm139493404168176
	// "[...] comparisons will be made in case-normalized canonical form."
	if (server.m_LobbyAuth && usernameWithoutRating.LowerCase() != session->GetUserName().LowerCase())
	{
		LOGERROR("Net server: lobby auth: %s tried joining as %s",
			session->GetUserName().ToUTF8(),
			usernameWithoutRating.ToUTF8());
		session->Disconnect(NDR_LOBBY_AUTH_FAILED);
		return true;
	}

	// Check the password before anything else.
	// NB: m_Name must match the client's salt, @see CNetClient::SetGamePassword
	if (!server.CheckPassword(message->m_Password, message->m_Name.ToUTF8()))
	{
		// Noisy logerror because players are not supposed to be able to get the IP,
		// so this might be someone targeting the host for some reason
		// (or TODO a dedicated server and we do want to log anyways)
		LOGERROR("Net server: user %s tried joining with the wrong password",
				 session->GetUserName().ToUTF8());
		session->Disconnect(NDR_SERVER_REFUSED);
		return true;
	}

	// Either deduplicate or prohibit join if name is in use
	bool duplicatePlayernames = false;
	CFG_GET_VAL("network.duplicateplayernames", duplicatePlayernames);
	// If lobby authentication is enabled, the clients playername has already been registered.
	// There also can't be any duplicated names.
	if (!server.m_LobbyAuth && duplicatePlayernames)
		username = server.DeduplicatePlayerName(username);
	else
	{
		std::vector<CNetServerSession*>::iterator it = std::find_if(
			server.m_Sessions.begin(), server.m_Sessions.end(),
			[&username] (const CNetServerSession* session)
			{ return session->GetUserName() == username; });

		if (it != server.m_Sessions.end() && (*it) != session)
		{
			session->Disconnect(NDR_PLAYERNAME_IN_USE);
			return true;
		}
	}

	// Disconnect banned usernames
	if (std::find(server.m_BannedPlayers.begin(), server.m_BannedPlayers.end(), server.m_LobbyAuth ? usernameWithoutRating : username) != server.m_BannedPlayers.end())
	{
		session->Disconnect(NDR_BANNED);
		return true;
	}

	int maxObservers = 0;
	CFG_GET_VAL("network.observerlimit", maxObservers);

	bool isRejoining = false;
	bool serverFull = false;
	if (server.m_State == SERVER_STATE_PREGAME)
	{
		// Don't check for maxObservers in the gamesetup, as we don't know yet who will be assigned
		serverFull = server.m_Sessions.size() >= MAX_CLIENTS;
	}
	else
	{
		bool isObserver = true;
		int disconnectedPlayers = 0;
		int connectedPlayers = 0;
		// (TODO: if GUIDs were stable, we should use them instead)
		for (const std::pair<const CStr, PlayerAssignment>& p : server.m_PlayerAssignments)
		{
			const PlayerAssignment& assignment = p.second;

			if (!assignment.m_Enabled && assignment.m_Name == username)
			{
				isObserver = assignment.m_PlayerID == -1;
				isRejoining = true;
			}

			if (assignment.m_PlayerID == -1)
				continue;

			if (assignment.m_Enabled)
				++connectedPlayers;
			else
				++disconnectedPlayers;
		}

		// Optionally allow everyone or only buddies to join after the game has started
		if (!isRejoining)
		{
			CStr observerLateJoin;
			CFG_GET_VAL("network.lateobservers", observerLateJoin);

			if (observerLateJoin == "everyone")
			{
				isRejoining = true;
			}
			else if (observerLateJoin == "buddies")
			{
				CStr buddies;
				CFG_GET_VAL("lobby.buddies", buddies);
				std::wstringstream buddiesStream(wstring_from_utf8(buddies));
				CStrW buddy;
				while (std::getline(buddiesStream, buddy, L','))
				{
					if (buddy == usernameWithoutRating)
					{
						isRejoining = true;
						break;
					}
				}
			}
		}

		if (!isRejoining)
		{
			LOGMESSAGE("Refused connection after game start from not-previously-known user \"%s\"", utf8_from_wstring(username));
			session->Disconnect(NDR_SERVER_ALREADY_IN_GAME);
			return true;
		}

		// Ensure all players will be able to rejoin
		serverFull = isObserver && (
			(int) server.m_Sessions.size() - connectedPlayers > maxObservers ||
			(int) server.m_Sessions.size() + disconnectedPlayers >= MAX_CLIENTS);
	}

	if (serverFull)
	{
		session->Disconnect(NDR_SERVER_FULL);
		return true;
	}

	u32 newHostID = server.m_NextHostID++;

	session->SetUserName(username);
	session->SetHostID(newHostID);

	CAuthenticateResultMessage authenticateResult;
	authenticateResult.m_Code = isRejoining ? ARC_OK_REJOINING : ARC_OK;
	authenticateResult.m_HostID = newHostID;
	authenticateResult.m_Message = L"Logged in";
	authenticateResult.m_IsController = 0;

	if (message->m_ControllerSecret == server.m_ControllerSecret)
	{
		if (server.m_ControllerGUID.empty())
		{
			server.m_ControllerGUID = session->GetGUID();
			authenticateResult.m_IsController = 1;
		}
		// TODO: we could probably handle having several controllers, or swapping?
	}

	session->SendMessage(&authenticateResult);

	server.OnUserJoin(session);

	if (isRejoining)
	{
		ENSURE(server.m_State != SERVER_STATE_UNCONNECTED && server.m_State != SERVER_STATE_PREGAME);

		// Request a copy of the current game state from an existing player,
		// so we can send it on to the new player

		// Assume session 0 is most likely the local player, so they're
		// the most efficient client to request a copy from
		CNetServerSession* sourceSession = server.m_Sessions.at(0);

		sourceSession->GetFileTransferer().StartTask(
			std::shared_ptr<CNetFileReceiveTask>(new CNetFileReceiveTask_ServerRejoin(server, newHostID))
		);

		session->SetNextState(NSS_JOIN_SYNCING);
	}

	return true;
}
bool CNetServerWorker::OnSimulationCommand(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_SIMULATION_COMMAND);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	CSimulationMessage* message = (CSimulationMessage*)event->GetParamRef();

	// Ignore messages sent by one player on behalf of another player
	// unless cheating is enabled
	bool cheatsEnabled = false;
	const ScriptInterface& scriptInterface = server.GetScriptInterface();
	ScriptRequest rq(scriptInterface);
	JS::RootedValue settings(rq.cx);
	Script::GetProperty(rq, server.m_InitAttributes, "settings", &settings);
	if (Script::HasProperty(rq, settings, "CheatsEnabled"))
		Script::GetProperty(rq, settings, "CheatsEnabled", cheatsEnabled);

	PlayerAssignmentMap::iterator it = server.m_PlayerAssignments.find(session->GetGUID());
	// When cheating is disabled, fail if the player the message claims to
	// represent does not exist or does not match the sender's player name
	if (!cheatsEnabled && (it == server.m_PlayerAssignments.end() || it->second.m_PlayerID != message->m_Player))
		return true;

	// Send it back to all clients that have finished
	// the loading screen (and the synchronization when rejoining)
	server.Broadcast(message, { NSS_INGAME });

	// Save all the received commands
	if (server.m_SavedCommands.size() < message->m_Turn + 1)
		server.m_SavedCommands.resize(message->m_Turn + 1);
	server.m_SavedCommands[message->m_Turn].push_back(*message);

	// TODO: we shouldn't send the message back to the client that first sent it
	return true;
}

bool CNetServerWorker::OnSyncCheck(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_SYNC_CHECK);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	CSyncCheckMessage* message = (CSyncCheckMessage*)event->GetParamRef();

	server.m_ServerTurnManager->NotifyFinishedClientUpdate(*session, message->m_Turn, message->m_Hash);
	return true;
}

bool CNetServerWorker::OnEndCommandBatch(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_END_COMMAND_BATCH);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	CEndCommandBatchMessage* message = (CEndCommandBatchMessage*)event->GetParamRef();

	// The turn-length field is ignored
	server.m_ServerTurnManager->NotifyFinishedClientCommands(*session, message->m_Turn);
	return true;
}

bool CNetServerWorker::OnChat(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_CHAT);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	CChatMessage* message = (CChatMessage*)event->GetParamRef();

	message->m_GUID = session->GetGUID();

	server.Broadcast(message, { NSS_PREGAME, NSS_INGAME });

	return true;
}

bool CNetServerWorker::OnReady(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_READY);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	// Occurs if a client presses not-ready
	// in the very last moment before the hosts starts the game
	if (server.m_State == SERVER_STATE_LOADING)
		return true;

	CReadyMessage* message = (CReadyMessage*)event->GetParamRef();
	message->m_GUID = session->GetGUID();
	server.Broadcast(message, { NSS_PREGAME });

	server.m_PlayerAssignments[message->m_GUID].m_Status = message->m_Status;

	return true;
}

bool CNetServerWorker::OnClearAllReady(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_CLEAR_ALL_READY);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	if (session->GetGUID() == server.m_ControllerGUID)
		server.ClearAllPlayerReady();

	return true;
}

bool CNetServerWorker::OnGameSetup(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_GAME_SETUP);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	// Changing the settings after gamestart is not implemented and would cause an Out-of-sync error.
	// This happened when doubleclicking on the startgame button.
	if (server.m_State != SERVER_STATE_PREGAME)
		return true;

	// Only the controller is allowed to send game setup updates.
	// TODO: it would be good to allow other players to request changes to some settings,
	// e.g. their civilisation.
	// Possibly this should use another message, to enforce a single source of truth.
	if (session->GetGUID() == server.m_ControllerGUID)
	{
		CGameSetupMessage* message = (CGameSetupMessage*)event->GetParamRef();
		server.Broadcast(message, { NSS_PREGAME });
	}
	return true;
}

bool CNetServerWorker::OnAssignPlayer(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_ASSIGN_PLAYER);
	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	if (session->GetGUID() == server.m_ControllerGUID)
	{
		CAssignPlayerMessage* message = (CAssignPlayerMessage*)event->GetParamRef();
		server.AssignPlayer(message->m_PlayerID, message->m_GUID);
	}
	return true;
}

bool CNetServerWorker::OnGameStart(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_GAME_START);
	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	if (session->GetGUID() != server.m_ControllerGUID)
		return true;

	CGameStartMessage* message = (CGameStartMessage*)event->GetParamRef();
	server.StartGame(message->m_InitAttributes);
	return true;
}

bool CNetServerWorker::OnLoadedGame(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_LOADED_GAME);

	CNetServerSession* loadedSession = (CNetServerSession*)context;
	CNetServerWorker& server = loadedSession->GetServer();

	// We're in the loading state, so wait until every client has loaded
	// before starting the game
	ENSURE(server.m_State == SERVER_STATE_LOADING);
	if (server.CheckGameLoadStatus(loadedSession))
		return true;

	CClientsLoadingMessage message;
	// We always send all GUIDs of clients in the loading state
	// so that we don't have to bother about switching GUI pages
	for (CNetServerSession* session : server.m_Sessions)
		if (session->GetCurrState() != NSS_INGAME && loadedSession->GetGUID() != session->GetGUID())
		{
			CClientsLoadingMessage::S_m_Clients client;
			client.m_GUID = session->GetGUID();
			message.m_Clients.push_back(client);
		}

	// Send to the client who has loaded the game but did not reach the NSS_INGAME state yet
	loadedSession->SendMessage(&message);
	server.Broadcast(&message, { NSS_INGAME });

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
	// Special case: the controller shouldn't be treated as an observer in any case.
	bool isObserver = server.m_PlayerAssignments[session->GetGUID()].m_PlayerID == -1 && server.m_ControllerGUID != session->GetGUID();
	server.m_ServerTurnManager->InitialiseClient(session->GetHostID(), readyTurn, isObserver);

	// Tell the client that everything has finished loading and it should start now
	CLoadedGameMessage loaded;
	loaded.m_CurrentTurn = readyTurn;
	session->SendMessage(&loaded);

	return true;
}

bool CNetServerWorker::OnRejoined(void* context, CFsmEvent* event)
{
	// A client has finished rejoining and the loading screen disappeared.
	ENSURE(event->GetType() == (uint)NMT_REJOINED);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	// Inform everyone of the client having rejoined
	CRejoinedMessage* message = (CRejoinedMessage*)event->GetParamRef();
	message->m_GUID = session->GetGUID();
	server.Broadcast(message, { NSS_INGAME });

	// Send all pausing players to the rejoined client.
	for (const CStr& guid : server.m_PausingPlayers)
	{
		CClientPausedMessage pausedMessage;
		pausedMessage.m_GUID = guid;
		pausedMessage.m_Pause = true;
		session->SendMessage(&pausedMessage);
	}

	return true;
}

bool CNetServerWorker::OnKickPlayer(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_KICKED);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	if (session->GetGUID() == server.m_ControllerGUID)
	{
		CKickedMessage* message = (CKickedMessage*)event->GetParamRef();
		server.KickPlayer(message->m_Name, message->m_Ban);
	}
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

bool CNetServerWorker::OnClientPaused(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_CLIENT_PAUSED);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServerWorker& server = session->GetServer();

	CClientPausedMessage* message = (CClientPausedMessage*)event->GetParamRef();

	message->m_GUID = session->GetGUID();

	// Update the list of pausing players.
	std::vector<CStr>::iterator player = std::find(server.m_PausingPlayers.begin(), server.m_PausingPlayers.end(), session->GetGUID());

	if (message->m_Pause)
	{
		if (player != server.m_PausingPlayers.end())
			return true;

		server.m_PausingPlayers.push_back(session->GetGUID());
	}
	else
	{
		if (player == server.m_PausingPlayers.end())
			return true;

		server.m_PausingPlayers.erase(player);
	}

	// Send messages to clients that are in game, and are not the client who paused.
	for (CNetServerSession* netSession : server.m_Sessions)
		if (netSession->GetCurrState() == NSS_INGAME && message->m_GUID != netSession->GetGUID())
			netSession->SendMessage(message);

	return true;
}

bool CNetServerWorker::CheckGameLoadStatus(CNetServerSession* changedSession)
{
	for (const CNetServerSession* session : m_Sessions)
		if (session != changedSession && session->GetCurrState() != NSS_INGAME)
			return false;

	// Inform clients that everyone has loaded the map and that the game can start
	CLoadedGameMessage loaded;
	loaded.m_CurrentTurn = 0;

	// Notice the changedSession is still in the NSS_PREGAME state
	Broadcast(&loaded, { NSS_PREGAME, NSS_INGAME });

	m_State = SERVER_STATE_INGAME;
	return true;
}

void CNetServerWorker::StartGame(const CStr& initAttribs)
{
	for (std::pair<const CStr, PlayerAssignment>& player : m_PlayerAssignments)
		if (player.second.m_Enabled && player.second.m_PlayerID != -1 && player.second.m_Status == 0)
		{
			LOGERROR("Tried to start the game without player \"%s\" being ready!", utf8_from_wstring(player.second.m_Name).c_str());
			return;
		}

	m_ServerTurnManager = new CNetServerTurnManager(*this);

	for (CNetServerSession* session : m_Sessions)
	{
		// Special case: the controller shouldn't be treated as an observer in any case.
		bool isObserver = m_PlayerAssignments[session->GetGUID()].m_PlayerID == -1 && m_ControllerGUID != session->GetGUID();
		m_ServerTurnManager->InitialiseClient(session->GetHostID(), 0, isObserver);
	}

	m_State = SERVER_STATE_LOADING;

	// Remove players and observers that are not present when the game starts
	for (PlayerAssignmentMap::iterator it = m_PlayerAssignments.begin(); it != m_PlayerAssignments.end();)
		if (it->second.m_Enabled)
			++it;
		else
			it = m_PlayerAssignments.erase(it);

	SendPlayerAssignments();

	// Update init attributes. They should no longer change.
	Script::ParseJSON(ScriptRequest(m_ScriptInterface), initAttribs, &m_InitAttributes);

	CGameStartMessage gameStart;
	gameStart.m_InitAttributes = initAttribs;
	Broadcast(&gameStart, { NSS_PREGAME });
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
		for (const CNetServerSession* session : m_Sessions)
		{
			if (session->GetUserName() == name)
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

void CNetServerWorker::SendHolePunchingMessage(const CStr& ipStr, u16 port)
{
	if (m_Host)
		StunClient::SendHolePunchingMessages(*m_Host, ipStr, port);
}




CNetServer::CNetServer(bool useLobbyAuth) :
	m_Worker(new CNetServerWorker(useLobbyAuth)),
	m_LobbyAuth(useLobbyAuth), m_UseSTUN(false), m_PublicIp(""), m_PublicPort(20595), m_Password()
{
}

CNetServer::~CNetServer()
{
	delete m_Worker;
}

bool CNetServer::GetUseSTUN() const
{
	return m_UseSTUN;
}

bool CNetServer::UseLobbyAuth() const
{
	return m_LobbyAuth;
}

bool CNetServer::SetupConnection(const u16 port)
{
	return m_Worker->SetupConnection(port);
}

CStr CNetServer::GetPublicIp() const
{
	return m_PublicIp;
}

u16 CNetServer::GetPublicPort() const
{
	return m_PublicPort;
}

u16 CNetServer::GetLocalPort() const
{
	std::lock_guard<std::mutex> lock(m_Worker->m_WorkerMutex);
	if (!m_Worker->m_Host)
		return 0;
	return m_Worker->m_Host->address.port;
}

void CNetServer::SetConnectionData(const CStr& ip, const u16 port)
{
	m_PublicIp = ip;
	m_PublicPort = port;
	m_UseSTUN = false;
}

bool CNetServer::SetConnectionDataViaSTUN()
{
	m_UseSTUN = true;
	std::lock_guard<std::mutex> lock(m_Worker->m_WorkerMutex);
	if (!m_Worker->m_Host)
		return false;
	return StunClient::FindPublicIP(*m_Worker->m_Host, m_PublicIp, m_PublicPort);
}

bool CNetServer::CheckPasswordAndIncrement(const std::string& username, const std::string& password, const std::string& salt)
{
	std::unordered_map<std::string, int>::iterator it = m_FailedAttempts.find(username);
	if (m_Worker->CheckPassword(password, salt))
	{
		if (it != m_FailedAttempts.end())
			it->second = 0;
		return true;
	}
	if (it == m_FailedAttempts.end())
		m_FailedAttempts.emplace(username, 1);
	else
		it->second++;
	return false;
}

bool CNetServer::IsBanned(const std::string& username) const
{
	std::unordered_map<std::string, int>::const_iterator it = m_FailedAttempts.find(username);
	return it != m_FailedAttempts.end() && it->second >= FAILED_PASSWORD_TRIES_BEFORE_BAN;
}

void CNetServer::SetPassword(const CStr& password)
{
	m_Password = password;
	std::lock_guard<std::mutex> lock(m_Worker->m_WorkerMutex);
	m_Worker->SetPassword(password);
}

void CNetServer::SetControllerSecret(const std::string& secret)
{
	std::lock_guard<std::mutex> lock(m_Worker->m_WorkerMutex);
	m_Worker->SetControllerSecret(secret);
}

void CNetServer::StartGame()
{
	std::lock_guard<std::mutex> lock(m_Worker->m_WorkerMutex);
	m_Worker->m_StartGameQueue.push_back(true);
}

void CNetServer::UpdateInitAttributes(JS::MutableHandleValue attrs, const ScriptRequest& rq)
{
	// Pass the attributes as JSON, since that's the easiest safe
	// cross-thread way of passing script data
	std::string attrsJSON = Script::StringifyJSON(rq, attrs, false);

	std::lock_guard<std::mutex> lock(m_Worker->m_WorkerMutex);
	m_Worker->m_InitAttributesQueue.push_back(attrsJSON);
}

void CNetServer::OnLobbyAuth(const CStr& name, const CStr& token)
{
	std::lock_guard<std::mutex> lock(m_Worker->m_WorkerMutex);
	m_Worker->m_LobbyAuthQueue.push_back(std::make_pair(name, token));
}

void CNetServer::SetTurnLength(u32 msecs)
{
	std::lock_guard<std::mutex> lock(m_Worker->m_WorkerMutex);
	m_Worker->m_TurnLengthQueue.push_back(msecs);
}

void CNetServer::SendHolePunchingMessage(const CStr& ip, u16 port)
{
	m_Worker->SendHolePunchingMessage(ip, port);
}
