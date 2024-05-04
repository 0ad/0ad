/* Copyright (C) 2024 Wildfire Games.
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

#include "NetClient.h"

#include "NetClientTurnManager.h"
#include "NetEnet.h"
#include "NetMessage.h"
#include "NetSession.h"

#include "lib/byte_order.h"
#include "lib/external_libraries/enet.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/sysdep/sysdep.h"
#include "lobby/IXmppClient.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/Compress.h"
#include "ps/CStr.h"
#include "ps/Game.h"
#include "ps/Hashing.h"
#include "ps/Loader.h"
#include "ps/Profile.h"
#include "ps/Threading.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/JSON.h"
#include "simulation2/Simulation2.h"
#include "network/StunClient.h"

/**
 * Once ping goes above turn length * command delay,
 * the game will start 'freezing' for other clients while we catch up.
 * Since commands are sent client -> server -> client, divide by 2.
 * (duplicated in NetServer.cpp to avoid having to fetch the constants in a header file)
 */
constexpr u32 NETWORK_BAD_PING = DEFAULT_TURN_LENGTH * COMMAND_DELAY_MP / 2;

CNetClient *g_NetClient = NULL;

CNetClient::CNetClient(CGame* game) :
	m_Session(NULL),
	m_UserName(L"anonymous"),
	m_HostID((u32)-1), m_ClientTurnManager(NULL), m_Game(game),
	m_LastConnectionCheck(0),
	m_ServerAddress(),
	m_ServerPort(0),
	m_Rejoin(false)
{
	m_Game->SetTurnManager(NULL); // delete the old local turn manager so we don't accidentally use it

	JS_AddExtraGCRootsTracer(GetScriptInterface().GetGeneralJSContext(), CNetClient::Trace, this);

	// Set up transitions for session
	AddTransition(NCS_UNCONNECTED, (uint)NMT_CONNECT_COMPLETE, NCS_CONNECT, &OnConnect, this);

	AddTransition(NCS_CONNECT, (uint)NMT_SERVER_HANDSHAKE, NCS_HANDSHAKE, &OnHandshake, this);

	AddTransition(NCS_HANDSHAKE, (uint)NMT_SERVER_HANDSHAKE_RESPONSE, NCS_AUTHENTICATE, &OnHandshakeResponse, this);

	AddTransition(NCS_AUTHENTICATE, (uint)NMT_AUTHENTICATE, NCS_AUTHENTICATE, &OnAuthenticateRequest, this);
	AddTransition(NCS_AUTHENTICATE, (uint)NMT_AUTHENTICATE_RESULT, NCS_PREGAME, &OnAuthenticate, this);

	AddTransition(NCS_PREGAME, (uint)NMT_CHAT, NCS_PREGAME, &OnChat, this);
	AddTransition(NCS_PREGAME, (uint)NMT_READY, NCS_PREGAME, &OnReady, this);
	AddTransition(NCS_PREGAME, (uint)NMT_GAME_SETUP, NCS_PREGAME, &OnGameSetup, this);
	AddTransition(NCS_PREGAME, (uint)NMT_PLAYER_ASSIGNMENT, NCS_PREGAME, &OnPlayerAssignment, this);
	AddTransition(NCS_PREGAME, (uint)NMT_KICKED, NCS_PREGAME, &OnKicked, this);
	AddTransition(NCS_PREGAME, (uint)NMT_CLIENT_TIMEOUT, NCS_PREGAME, &OnClientTimeout, this);
	AddTransition(NCS_PREGAME, (uint)NMT_CLIENT_PERFORMANCE, NCS_PREGAME, &OnClientPerformance, this);
	AddTransition(NCS_PREGAME, (uint)NMT_GAME_START, NCS_LOADING, &OnGameStart, this);
	AddTransition(NCS_PREGAME, (uint)NMT_JOIN_SYNC_START, NCS_JOIN_SYNCING, &OnJoinSyncStart, this);

	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_CHAT, NCS_JOIN_SYNCING, &OnChat, this);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_GAME_SETUP, NCS_JOIN_SYNCING, &OnGameSetup, this);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_PLAYER_ASSIGNMENT, NCS_JOIN_SYNCING, &OnPlayerAssignment, this);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_KICKED, NCS_JOIN_SYNCING, &OnKicked, this);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_CLIENT_TIMEOUT, NCS_JOIN_SYNCING, &OnClientTimeout, this);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_CLIENT_PERFORMANCE, NCS_JOIN_SYNCING, &OnClientPerformance, this);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_GAME_START, NCS_JOIN_SYNCING, &OnGameStart, this);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_SIMULATION_COMMAND, NCS_JOIN_SYNCING, &OnInGame, this);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_END_COMMAND_BATCH, NCS_JOIN_SYNCING, &OnJoinSyncEndCommandBatch, this);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_LOADED_GAME, NCS_INGAME, &OnLoadedGame, this);

	AddTransition(NCS_LOADING, (uint)NMT_CHAT, NCS_LOADING, &OnChat, this);
	AddTransition(NCS_LOADING, (uint)NMT_GAME_SETUP, NCS_LOADING, &OnGameSetup, this);
	AddTransition(NCS_LOADING, (uint)NMT_PLAYER_ASSIGNMENT, NCS_LOADING, &OnPlayerAssignment, this);
	AddTransition(NCS_LOADING, (uint)NMT_KICKED, NCS_LOADING, &OnKicked, this);
	AddTransition(NCS_LOADING, (uint)NMT_CLIENT_TIMEOUT, NCS_LOADING, &OnClientTimeout, this);
	AddTransition(NCS_LOADING, (uint)NMT_CLIENT_PERFORMANCE, NCS_LOADING, &OnClientPerformance, this);
	AddTransition(NCS_LOADING, (uint)NMT_CLIENTS_LOADING, NCS_LOADING, &OnClientsLoading, this);
	AddTransition(NCS_LOADING, (uint)NMT_LOADED_GAME, NCS_INGAME, &OnLoadedGame, this);

	AddTransition(NCS_INGAME, (uint)NMT_REJOINED, NCS_INGAME, &OnRejoined, this);
	AddTransition(NCS_INGAME, (uint)NMT_KICKED, NCS_INGAME, &OnKicked, this);
	AddTransition(NCS_INGAME, (uint)NMT_CLIENT_TIMEOUT, NCS_INGAME, &OnClientTimeout, this);
	AddTransition(NCS_INGAME, (uint)NMT_CLIENT_PERFORMANCE, NCS_INGAME, &OnClientPerformance, this);
	AddTransition(NCS_INGAME, (uint)NMT_CLIENTS_LOADING, NCS_INGAME, &OnClientsLoading, this);
	AddTransition(NCS_INGAME, (uint)NMT_CLIENT_PAUSED, NCS_INGAME, &OnClientPaused, this);
	AddTransition(NCS_INGAME, (uint)NMT_CHAT, NCS_INGAME, &OnChat, this);
	AddTransition(NCS_INGAME, (uint)NMT_GAME_SETUP, NCS_INGAME, &OnGameSetup, this);
	AddTransition(NCS_INGAME, (uint)NMT_PLAYER_ASSIGNMENT, NCS_INGAME, &OnPlayerAssignment, this);
	AddTransition(NCS_INGAME, (uint)NMT_SIMULATION_COMMAND, NCS_INGAME, &OnInGame, this);
	AddTransition(NCS_INGAME, (uint)NMT_SYNC_ERROR, NCS_INGAME, &OnInGame, this);
	AddTransition(NCS_INGAME, (uint)NMT_END_COMMAND_BATCH, NCS_INGAME, &OnInGame, this);

	// Set first state
	SetFirstState(NCS_UNCONNECTED);
}

CNetClient::~CNetClient()
{
	// Try to flush messages before dying (probably fails).
	if (m_ClientTurnManager)
		m_ClientTurnManager->OnDestroyConnection();

	DestroyConnection();
	JS_RemoveExtraGCRootsTracer(GetScriptInterface().GetGeneralJSContext(), CNetClient::Trace, this);
}

void CNetClient::TraceMember(JSTracer *trc)
{
	for (JS::Heap<JS::Value>& guiMessage : m_GuiMessageQueue)
		JS::TraceEdge(trc, &guiMessage, "m_GuiMessageQueue");
}

void CNetClient::SetUserName(const CStrW& username)
{
	ENSURE(!m_Session); // must be called before we start the connection

	m_UserName = username;
}

void CNetClient::SetHostJID(const CStr& jid)
{
	m_HostJID = jid;
}

void CNetClient::SetGamePassword(const CStr& hashedPassword)
{
	// Hash on top with the user's name, to make sure not all
	// hashing data is in control of the host.
	m_Password = HashCryptographically(hashedPassword, m_UserName.ToUTF8());
}

void CNetClient::SetControllerSecret(const std::string& secret)
{
	m_ControllerSecret = secret;
}


bool CNetClient::SetupConnection(ENetHost* enetClient)
{
	CNetClientSession* session = new CNetClientSession(*this);
	bool ok = session->Connect(m_ServerAddress, m_ServerPort, enetClient);
	SetAndOwnSession(session);
	if (ok)
		m_PollingThread = std::thread(Threading::HandleExceptions<CNetClientSession::RunNetLoop>::Wrapper, m_Session);
	return ok;
}

void CNetClient::SetupConnectionViaLobby()
{
	g_XmppClient->SendIqGetConnectionData(m_HostJID, m_Password, m_UserName.ToUTF8(), false);
}

void CNetClient::SetupServerData(CStr address, u16 port, bool stun)
{
	ENSURE(!m_Session);

	m_ServerAddress = address;
	m_ServerPort = port;
	m_UseSTUN = stun;
}

void CNetClient::HandleGetServerDataFailed(const CStr& error)
{
	if (m_Session)
		return;

	PushGuiMessage(
		"type", "serverdata",
		"status", "failed",
		"reason", error
	);
}

bool CNetClient::TryToConnect(const CStr& hostJID, bool localNetwork)
{
	if (m_Session)
		return false;

	if (m_ServerAddress.empty())
	{
		PushGuiMessage(
			"type", "netstatus",
			"status", "disconnected",
			"reason", static_cast<i32>(NDR_SERVER_REFUSED));
		return false;
	}

	ENetAddress hostAddr{ ENET_HOST_ANY, ENET_PORT_ANY };
	ENetHost* enetClient = PS::Enet::CreateHost(&hostAddr, 1, 1);

	if (!enetClient)
	{
		PushGuiMessage(
			"type", "netstatus",
			"status", "disconnected",
			"reason", static_cast<i32>(NDR_STUN_PORT_FAILED));
		return false;
	}

	CStr ip;
	u16 port = 0;
	if (g_XmppClient && m_UseSTUN)
	{
		if (!StunClient::FindPublicIP(*enetClient, ip, port))
		{
			PushGuiMessage(
				"type", "netstatus",
				"status", "disconnected",
				"reason", static_cast<i32>(NDR_STUN_ENDPOINT_FAILED));
			return false;
		}

		// If the host is on the same network, we risk failing to connect
		// on routers that don't support NAT hairpinning/NAT loopback.
		// To work around that, send again a connection data request, but for internal IP this time.
		if (ip == m_ServerAddress)
		{
			g_XmppClient->SendIqGetConnectionData(m_HostJID, m_Password, m_UserName.ToUTF8(), true);
			// Return true anyways - we're on a success path here.
			return true;
		}
	}
	else if (g_XmppClient && localNetwork)
	{
		// We may need to punch a hole through the local firewall, so fetch our local IP.
		// NB: we'll ignore failures here, and hope that the firewall will be open to connection
		// if we fail to fetch the local IP (which is unlikely anyways).
		if (!StunClient::FindLocalIP(ip))
			ip = "";
		// Check if we're hosting on localhost, and if so, explicitly use that
		// (this circumvents, at least, the 'block all incoming connections' setting
		// on the MacOS firewall).
		if (ip == m_ServerAddress)
		{
			m_ServerAddress = "127.0.0.1";
			ip = "";
		}
		port = enetClient->address.port;
	}

	LOGMESSAGE("NetClient: connecting to server at %s:%i", m_ServerAddress, m_ServerPort);

	if (!ip.empty())
	{
		// UDP hole-punching
		// Step 0: send a message, via XMPP, to the server with our external IP & port.
		g_XmppClient->SendStunEndpointToHost(ip, port, hostJID);

		// Step 1b: Wait some time - we need the host to receive the stun endpoint and start punching a hole themselves before
		// we try to establish the connection below.
		SDL_Delay(1000);

		// Step 2: Send a message ourselves to the server so that the NAT, if any, routes incoming trafic correctly.
		// TODO: verify if this step is necessary, since we'll try and connect anyways below.
		StunClient::SendHolePunchingMessages(*enetClient, m_ServerAddress, m_ServerPort);
	}

	if (!g_NetClient->SetupConnection(enetClient))
	{
		PushGuiMessage(
			"type", "netstatus",
			"status", "disconnected",
			"reason", static_cast<i32>(NDR_UNKNOWN));
		return false;
	}

	return true;
}


void CNetClient::SetAndOwnSession(CNetClientSession* session)
{
	delete m_Session;
	m_Session = session;
}

void CNetClient::DestroyConnection()
{
	if (m_Session)
		m_Session->Shutdown();

	if (m_PollingThread.joinable())
		// Use detach() over join() because we don't want to wait for the session
		// (which may be polling or trying to send messages).
		m_PollingThread.detach();

	// The polling thread will cleanup the session on its own,
	// mark it as nullptr here so we know we're done using it.
	m_Session = nullptr;
}

void CNetClient::Poll()
{
	if (!m_Session)
		return;

	PROFILE3("NetClient::poll");

	CheckServerConnection();
	m_Session->ProcessPolledMessages();
}

void CNetClient::CheckServerConnection()
{
	// Trigger local warnings if the connection to the server is bad.
	// At most once per second.
	std::time_t now = std::time(nullptr);
	if (now <= m_LastConnectionCheck)
		return;

	m_LastConnectionCheck = now;

	// Report if we are losing the connection to the server
	u32 lastReceived = m_Session->GetLastReceivedTime();
	if (lastReceived > NETWORK_WARNING_TIMEOUT)
	{
		PushGuiMessage(
			"type", "netwarn",
			"warntype", "server-timeout",
			"lastReceivedTime", lastReceived);
		return;
	}

	// Report if we have a bad ping to the server.
	u32 meanRTT = m_Session->GetMeanRTT();
	if (meanRTT > NETWORK_BAD_PING)
	{
		PushGuiMessage(
			"type", "netwarn",
			"warntype", "server-latency",
			"meanRTT", meanRTT);
	}
}

void CNetClient::GuiPoll(JS::MutableHandleValue ret)
{
	if (m_GuiMessageQueue.empty())
	{
		ret.setUndefined();
		return;
	}

	ret.set(m_GuiMessageQueue.front());
	m_GuiMessageQueue.pop_front();
}

std::string CNetClient::TestReadGuiMessages()
{
	ScriptRequest rq(GetScriptInterface());

	std::string r;
	JS::RootedValue msg(rq.cx);
	while (true)
	{
		GuiPoll(&msg);
		if (msg.isUndefined())
			break;
		r += Script::ToString(rq, &msg) + "\n";
	}
	return r;
}

const ScriptInterface& CNetClient::GetScriptInterface()
{
	return m_Game->GetSimulation2()->GetScriptInterface();
}

void CNetClient::PostPlayerAssignmentsToScript()
{
	ScriptRequest rq(GetScriptInterface());

	JS::RootedValue newAssignments(rq.cx);
	Script::CreateObject(rq, &newAssignments);

	for (const std::pair<const CStr, PlayerAssignment>& p : m_PlayerAssignments)
	{
		JS::RootedValue assignment(rq.cx);

		Script::CreateObject(
			rq,
			&assignment,
			"name", p.second.m_Name,
			"player", p.second.m_PlayerID,
			"status", p.second.m_Status);

		Script::SetProperty(rq, newAssignments, p.first.c_str(), assignment);
	}

	PushGuiMessage(
		"type", "players",
		"newAssignments", newAssignments);
}

bool CNetClient::SendMessage(const CNetMessage* message)
{
	if (!m_Session)
		return false;

	return m_Session->SendMessage(message);
}

void CNetClient::HandleConnect()
{
	Update((uint)NMT_CONNECT_COMPLETE, NULL);
}

void CNetClient::HandleDisconnect(u32 reason)
{
	PushGuiMessage(
		"type", "netstatus",
		"status", "disconnected",
		"reason", reason);

	DestroyConnection();

	// Update the state immediately to UNCONNECTED (don't bother with FSM transitions since
	// we'd need one for every single state, and we don't need to use per-state actions)
	SetCurrState(NCS_UNCONNECTED);
}

void CNetClient::SendGameSetupMessage(JS::MutableHandleValue attrs, const ScriptInterface& scriptInterface)
{
	CGameSetupMessage gameSetup(scriptInterface);
	gameSetup.m_Data = attrs;
	SendMessage(&gameSetup);
}

void CNetClient::SendAssignPlayerMessage(const int playerID, const CStr& guid)
{
	CAssignPlayerMessage assignPlayer;
	assignPlayer.m_PlayerID = playerID;
	assignPlayer.m_GUID = guid;
	SendMessage(&assignPlayer);
}

void CNetClient::SendChatMessage(const std::wstring& text)
{
	CChatMessage chat;
	chat.m_Message = text;
	SendMessage(&chat);
}

void CNetClient::SendReadyMessage(const int status)
{
	CReadyMessage readyStatus;
	readyStatus.m_Status = status;
	SendMessage(&readyStatus);
}

void CNetClient::SendClearAllReadyMessage()
{
	CClearAllReadyMessage clearAllReady;
	SendMessage(&clearAllReady);
}

void CNetClient::SendStartGameMessage(const CStr& initAttribs)
{
	CGameStartMessage gameStart;
	gameStart.m_InitAttributes = initAttribs;
	SendMessage(&gameStart);
}

void CNetClient::SendRejoinedMessage()
{
	CRejoinedMessage rejoinedMessage;
	SendMessage(&rejoinedMessage);
}

void CNetClient::SendKickPlayerMessage(const CStrW& playerName, bool ban)
{
	CKickedMessage kickPlayer;
	kickPlayer.m_Name = playerName;
	kickPlayer.m_Ban = ban;
	SendMessage(&kickPlayer);
}

void CNetClient::SendPausedMessage(bool pause)
{
	CClientPausedMessage pausedMessage;
	pausedMessage.m_Pause = pause;
	SendMessage(&pausedMessage);
}

bool CNetClient::HandleMessage(CNetMessage* message)
{
	// Handle non-FSM messages first

	Status status = m_Session->GetFileTransferer().HandleMessageReceive(*message);
	if (status == INFO::OK)
		return true;
	if (status != INFO::SKIPPED)
		return false;

	if (message->GetType() == NMT_FILE_TRANSFER_REQUEST)
	{
		CFileTransferRequestMessage* reqMessage = static_cast<CFileTransferRequestMessage*>(message);

		// TODO: we should support different transfer request types, instead of assuming
		// it's always requesting the simulation state

		std::stringstream stream;

		LOGMESSAGERENDER("Serializing game at turn %u for rejoining player", m_ClientTurnManager->GetCurrentTurn());
		u32 turn = to_le32(m_ClientTurnManager->GetCurrentTurn());
		stream.write((char*)&turn, sizeof(turn));

		bool ok = m_Game->GetSimulation2()->SerializeState(stream);
		ENSURE(ok);

		// Compress the content with zlib to save bandwidth
		// (TODO: if this is still too large, compressing with e.g. LZMA works much better)
		std::string compressed;
		CompressZLib(stream.str(), compressed, true);

		m_Session->GetFileTransferer().StartResponse(reqMessage->m_RequestID, compressed);

		return true;
	}

	// Update FSM
	bool ok = Update(message->GetType(), message);
	if (!ok)
		LOGERROR("Net client: Error running FSM update (type=%d state=%d)", (int)message->GetType(), (int)GetCurrState());
	return ok;
}

void CNetClient::LoadFinished()
{
	if (!m_JoinSyncBuffer.empty())
	{
		// We're rejoining a game, and just finished loading the initial map,
		// so deserialize the saved game state now

		std::string state;
		DecompressZLib(m_JoinSyncBuffer, state, true);

		std::stringstream stream(state);

		u32 turn;
		stream.read((char*)&turn, sizeof(turn));
		turn = to_le32(turn);

		LOGMESSAGE("Rejoining client deserializing state at turn %u\n", turn);

		bool ok = m_Game->GetSimulation2()->DeserializeState(stream);
		ENSURE(ok);

		m_ClientTurnManager->ResetState(turn, turn);

		PushGuiMessage(
			"type", "netstatus",
			"status", "join_syncing");
	}
	else
	{
		// Connecting at the start of a game, so we'll wait for other players to finish loading
		PushGuiMessage(
			"type", "netstatus",
			"status", "waiting_for_players");
	}

	CLoadedGameMessage loaded;
	loaded.m_CurrentTurn = m_ClientTurnManager->GetCurrentTurn();
	SendMessage(&loaded);
}

void CNetClient::SendAuthenticateMessage()
{
	CAuthenticateMessage authenticate;
	authenticate.m_Name = m_UserName;
	authenticate.m_Password = m_Password;
	authenticate.m_ControllerSecret = m_ControllerSecret;
	SendMessage(&authenticate);
}

bool CNetClient::OnConnect(CNetClient* client, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_CONNECT_COMPLETE);

	client->PushGuiMessage(
		"type", "netstatus",
		"status", "connected");

	return true;
}

bool CNetClient::OnHandshake(CNetClient* client, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_SERVER_HANDSHAKE);

	CCliHandshakeMessage handshake;
	handshake.m_MagicResponse = PS_PROTOCOL_MAGIC_RESPONSE;
	handshake.m_ProtocolVersion = PS_PROTOCOL_VERSION;
	handshake.m_SoftwareVersion = PS_PROTOCOL_VERSION;
	client->SendMessage(&handshake);

	return true;
}

bool CNetClient::OnHandshakeResponse(CNetClient* client, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_SERVER_HANDSHAKE_RESPONSE);

	CSrvHandshakeResponseMessage* message = static_cast<CSrvHandshakeResponseMessage*>(event->GetParamRef());

	client->m_GUID = message->m_GUID;

	if (message->m_Flags & PS_NETWORK_FLAG_REQUIRE_LOBBYAUTH)
	{
		if (g_XmppClient && !client->m_HostJID.empty())
			g_XmppClient->SendIqLobbyAuth(client->m_HostJID, client->m_GUID);
		else
		{
			client->PushGuiMessage(
				"type", "netstatus",
				"status", "disconnected",
				"reason", static_cast<i32>(NDR_LOBBY_AUTH_FAILED));

			LOGMESSAGE("Net client: Couldn't send lobby auth xmpp message");
		}
		return true;
	}

	client->SendAuthenticateMessage();
	return true;
}

bool CNetClient::OnAuthenticateRequest(CNetClient* client, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_AUTHENTICATE);

	client->SendAuthenticateMessage();
	return true;
}

bool CNetClient::OnAuthenticate(CNetClient* client, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_AUTHENTICATE_RESULT);

	CAuthenticateResultMessage* message = static_cast<CAuthenticateResultMessage*>(event->GetParamRef());

	LOGMESSAGE("Net: Authentication result: host=%u, %s", message->m_HostID, utf8_from_wstring(message->m_Message));

	client->m_HostID = message->m_HostID;
	client->m_Rejoin = message->m_Code == ARC_OK_REJOINING;
	client->m_IsController = message->m_IsController;

	client->PushGuiMessage(
		"type", "netstatus",
		"status", "authenticated",
		"rejoining", client->m_Rejoin);

	return true;
}

bool CNetClient::OnChat(CNetClient* client, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_CHAT);

	CChatMessage* message = static_cast<CChatMessage*>(event->GetParamRef());

	client->PushGuiMessage(
		"type", "chat",
		"guid", message->m_GUID,
		"text", message->m_Message);

	return true;
}

bool CNetClient::OnReady(CNetClient* client, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_READY);

	CReadyMessage* message = static_cast<CReadyMessage*>(event->GetParamRef());

	client->PushGuiMessage(
		"type", "ready",
		"guid", message->m_GUID,
		"status", message->m_Status);

	return true;
}

bool CNetClient::OnGameSetup(CNetClient* client, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_GAME_SETUP);

	CGameSetupMessage* message = static_cast<CGameSetupMessage*>(event->GetParamRef());

	client->PushGuiMessage(
		"type", "gamesetup",
		"data", message->m_Data);

	return true;
}

bool CNetClient::OnPlayerAssignment(CNetClient* client, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_PLAYER_ASSIGNMENT);

	CPlayerAssignmentMessage* message = static_cast<CPlayerAssignmentMessage*>(event->GetParamRef());

	// Unpack the message
	PlayerAssignmentMap newPlayerAssignments;
	for (size_t i = 0; i < message->m_Hosts.size(); ++i)
	{
		PlayerAssignment assignment;
		assignment.m_Enabled = true;
		assignment.m_Name = message->m_Hosts[i].m_Name;
		assignment.m_PlayerID = message->m_Hosts[i].m_PlayerID;
		assignment.m_Status = message->m_Hosts[i].m_Status;
		newPlayerAssignments[message->m_Hosts[i].m_GUID] = assignment;
	}

	client->m_PlayerAssignments.swap(newPlayerAssignments);

	client->PostPlayerAssignmentsToScript();

	return true;
}

// This is called either when the host clicks the StartGame button or
// if this client rejoins and finishes the download of the simstate.
bool CNetClient::OnGameStart(CNetClient* client, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_GAME_START);

	CGameStartMessage* message = static_cast<CGameStartMessage*>(event->GetParamRef());

	// Find the player assigned to our GUID
	int player = -1;
	if (client->m_PlayerAssignments.find(client->m_GUID) != client->m_PlayerAssignments.end())
		player = client->m_PlayerAssignments[client->m_GUID].m_PlayerID;

	client->m_ClientTurnManager = new CNetClientTurnManager(
			*client->m_Game->GetSimulation2(), *client, client->m_HostID, client->m_Game->GetReplayLogger());

	// Parse init attributes.
	const ScriptInterface& scriptInterface = client->m_Game->GetSimulation2()->GetScriptInterface();
	ScriptRequest rq(scriptInterface);
	JS::RootedValue initAttribs(rq.cx);
	Script::ParseJSON(rq, message->m_InitAttributes, &initAttribs);

	client->m_Game->SetPlayerID(player);
	client->m_Game->StartGame(&initAttribs, "");

	client->PushGuiMessage("type", "start",
						   "initAttributes", initAttribs);

	return true;
}

bool CNetClient::OnJoinSyncStart(CNetClient* client, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_JOIN_SYNC_START);

	CJoinSyncStartMessage* joinSyncStartMessage = (CJoinSyncStartMessage*)event->GetParamRef();

	// The server wants us to start downloading the game state from it, so do so
	client->m_Session->GetFileTransferer().StartTask(
		[client, initAttributes = std::move(joinSyncStartMessage->m_InitAttributes)](std::string buffer)
			mutable
		{
			// We've received the game state from the server.

			// Save it so we can use it after the map has finished loading.
			client->m_JoinSyncBuffer = std::move(buffer);

			// Pretend the server told us to start the game.
			CGameStartMessage start;
			start.m_InitAttributes = std::move(initAttributes);
			client->HandleMessage(&start);
		});

	return true;
}

bool CNetClient::OnJoinSyncEndCommandBatch(CNetClient* client, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_END_COMMAND_BATCH);

	CEndCommandBatchMessage* endMessage = (CEndCommandBatchMessage*)event->GetParamRef();

	client->m_ClientTurnManager->FinishedAllCommands(endMessage->m_Turn, endMessage->m_TurnLength);

	// Execute all the received commands for the latest turn
	client->m_ClientTurnManager->UpdateFastForward();

	return true;
}

bool CNetClient::OnRejoined(CNetClient* client, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_REJOINED);

	CRejoinedMessage* message = static_cast<CRejoinedMessage*>(event->GetParamRef());

	client->PushGuiMessage(
		"type", "rejoined",
		"guid", message->m_GUID);

	return true;
}

bool CNetClient::OnKicked(CNetClient* client, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_KICKED);

	CKickedMessage* message = static_cast<CKickedMessage*>(event->GetParamRef());

	client->PushGuiMessage(
		"username", message->m_Name,
		"type", "kicked",
		"banned", message->m_Ban != 0);

	return true;
}

bool CNetClient::OnClientTimeout(CNetClient* client, CFsmEvent* event)
{
	// Report the timeout of some other client

	ENSURE(event->GetType() == (uint)NMT_CLIENT_TIMEOUT);

	CClientTimeoutMessage* message = static_cast<CClientTimeoutMessage*>(event->GetParamRef());

	client->PushGuiMessage(
		"type", "netwarn",
		"warntype", "client-timeout",
		"guid", message->m_GUID,
		"lastReceivedTime", message->m_LastReceivedTime);

	return true;
}

bool CNetClient::OnClientPerformance(CNetClient* client, CFsmEvent* event)
{
	// Performance statistics for one or multiple clients

	ENSURE(event->GetType() == (uint)NMT_CLIENT_PERFORMANCE);

	CClientPerformanceMessage* message = static_cast<CClientPerformanceMessage*>(event->GetParamRef());

	// Display warnings for other clients with bad ping
	for (size_t i = 0; i < message->m_Clients.size(); ++i)
	{
		if (message->m_Clients[i].m_MeanRTT < NETWORK_BAD_PING || message->m_Clients[i].m_GUID == client->m_GUID)
			continue;

		client->PushGuiMessage(
			"type", "netwarn",
			"warntype", "client-latency",
			"guid", message->m_Clients[i].m_GUID,
			"meanRTT", message->m_Clients[i].m_MeanRTT);
	}

	return true;
}

bool CNetClient::OnClientsLoading(CNetClient* client, CFsmEvent *event)
{
	ENSURE(event->GetType() == (uint)NMT_CLIENTS_LOADING);

	CClientsLoadingMessage* message = static_cast<CClientsLoadingMessage*>(event->GetParamRef());

	std::vector<CStr> guids;
	guids.reserve(message->m_Clients.size());
	for (const CClientsLoadingMessage::S_m_Clients& mClient : message->m_Clients)
		guids.push_back(mClient.m_GUID);

	client->PushGuiMessage(
		"type", "clients-loading",
		"guids", guids);
	return true;
}

bool CNetClient::OnClientPaused(CNetClient* client, CFsmEvent *event)
{
	ENSURE(event->GetType() == (uint)NMT_CLIENT_PAUSED);

	CClientPausedMessage* message = static_cast<CClientPausedMessage*>(event->GetParamRef());

	client->PushGuiMessage(
		"type", "paused",
		"pause", message->m_Pause != 0,
		"guid", message->m_GUID);

	return true;
}

bool CNetClient::OnLoadedGame(CNetClient* client, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_LOADED_GAME);

	// All players have loaded the game - start running the turn manager
	// so that the game begins
	client->m_Game->SetTurnManager(client->m_ClientTurnManager);

	client->PushGuiMessage(
		"type", "netstatus",
		"status", "active");

	// If we have rejoined an in progress game, send the rejoined message to the server.
	if (client->m_Rejoin)
		client->SendRejoinedMessage();

	return true;
}

bool CNetClient::OnInGame(CNetClient* client, CFsmEvent* event)
{
	// TODO: should split each of these cases into a separate method

	CNetMessage* message = static_cast<CNetMessage*>(event->GetParamRef());

	if (message)
	{
		if (message->GetType() == NMT_SIMULATION_COMMAND)
		{
			CSimulationMessage* simMessage = static_cast<CSimulationMessage*> (message);
			client->m_ClientTurnManager->OnSimulationMessage(simMessage);
		}
		else if (message->GetType() == NMT_SYNC_ERROR)
		{
			CSyncErrorMessage* syncMessage = static_cast<CSyncErrorMessage*> (message);
			client->m_ClientTurnManager->OnSyncError(syncMessage->m_Turn, syncMessage->m_HashExpected, syncMessage->m_PlayerNames);
		}
		else if (message->GetType() == NMT_END_COMMAND_BATCH)
		{
			CEndCommandBatchMessage* endMessage = static_cast<CEndCommandBatchMessage*> (message);
			client->m_ClientTurnManager->FinishedAllCommands(endMessage->m_Turn, endMessage->m_TurnLength);
		}
	}

	return true;
}
