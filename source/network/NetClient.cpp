/* Copyright (C) 2019 Wildfire Games.
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
#include "NetMessage.h"
#include "NetSession.h"

#include "lib/byte_order.h"
#include "lib/external_libraries/enet.h"
#include "lib/sysdep/sysdep.h"
#include "lobby/IXmppClient.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/Compress.h"
#include "ps/CStr.h"
#include "ps/Game.h"
#include "ps/Loader.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"

CNetClient *g_NetClient = NULL;

/**
 * Async task for receiving the initial game state when rejoining an
 * in-progress network game.
 */
class CNetFileReceiveTask_ClientRejoin : public CNetFileReceiveTask
{
	NONCOPYABLE(CNetFileReceiveTask_ClientRejoin);
public:
	CNetFileReceiveTask_ClientRejoin(CNetClient& client)
		: m_Client(client)
	{
	}

	virtual void OnComplete()
	{
		// We've received the game state from the server

		// Save it so we can use it after the map has finished loading
		m_Client.m_JoinSyncBuffer = m_Buffer;

		// Pretend the server told us to start the game
		CGameStartMessage start;
		m_Client.HandleMessage(&start);
	}

private:
	CNetClient& m_Client;
};

CNetClient::CNetClient(CGame* game, bool isLocalClient) :
	m_Session(NULL),
	m_UserName(L"anonymous"),
	m_HostID((u32)-1), m_ClientTurnManager(NULL), m_Game(game),
	m_GameAttributes(game->GetSimulation2()->GetScriptInterface().GetContext()),
	m_IsLocalClient(isLocalClient),
	m_LastConnectionCheck(0),
	m_Rejoin(false)
{
	m_Game->SetTurnManager(NULL); // delete the old local turn manager so we don't accidentally use it

	void* context = this;

	JS_AddExtraGCRootsTracer(GetScriptInterface().GetJSRuntime(), CNetClient::Trace, this);

	// Set up transitions for session
	AddTransition(NCS_UNCONNECTED, (uint)NMT_CONNECT_COMPLETE, NCS_CONNECT, (void*)&OnConnect, context);

	AddTransition(NCS_CONNECT, (uint)NMT_SERVER_HANDSHAKE, NCS_HANDSHAKE, (void*)&OnHandshake, context);

	AddTransition(NCS_HANDSHAKE, (uint)NMT_SERVER_HANDSHAKE_RESPONSE, NCS_AUTHENTICATE, (void*)&OnHandshakeResponse, context);

	AddTransition(NCS_AUTHENTICATE, (uint)NMT_AUTHENTICATE, NCS_AUTHENTICATE, (void*)&OnAuthenticateRequest, context);
	AddTransition(NCS_AUTHENTICATE, (uint)NMT_AUTHENTICATE_RESULT, NCS_INITIAL_GAMESETUP, (void*)&OnAuthenticate, context);

	AddTransition(NCS_INITIAL_GAMESETUP, (uint)NMT_GAME_SETUP, NCS_PREGAME, (void*)&OnGameSetup, context);

	AddTransition(NCS_PREGAME, (uint)NMT_CHAT, NCS_PREGAME, (void*)&OnChat, context);
	AddTransition(NCS_PREGAME, (uint)NMT_READY, NCS_PREGAME, (void*)&OnReady, context);
	AddTransition(NCS_PREGAME, (uint)NMT_GAME_SETUP, NCS_PREGAME, (void*)&OnGameSetup, context);
	AddTransition(NCS_PREGAME, (uint)NMT_PLAYER_ASSIGNMENT, NCS_PREGAME, (void*)&OnPlayerAssignment, context);
	AddTransition(NCS_PREGAME, (uint)NMT_KICKED, NCS_PREGAME, (void*)&OnKicked, context);
	AddTransition(NCS_PREGAME, (uint)NMT_CLIENT_TIMEOUT, NCS_PREGAME, (void*)&OnClientTimeout, context);
	AddTransition(NCS_PREGAME, (uint)NMT_CLIENT_PERFORMANCE, NCS_PREGAME, (void*)&OnClientPerformance, context);
	AddTransition(NCS_PREGAME, (uint)NMT_GAME_START, NCS_LOADING, (void*)&OnGameStart, context);
	AddTransition(NCS_PREGAME, (uint)NMT_JOIN_SYNC_START, NCS_JOIN_SYNCING, (void*)&OnJoinSyncStart, context);

	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_CHAT, NCS_JOIN_SYNCING, (void*)&OnChat, context);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_GAME_SETUP, NCS_JOIN_SYNCING, (void*)&OnGameSetup, context);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_PLAYER_ASSIGNMENT, NCS_JOIN_SYNCING, (void*)&OnPlayerAssignment, context);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_KICKED, NCS_JOIN_SYNCING, (void*)&OnKicked, context);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_CLIENT_TIMEOUT, NCS_JOIN_SYNCING, (void*)&OnClientTimeout, context);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_CLIENT_PERFORMANCE, NCS_JOIN_SYNCING, (void*)&OnClientPerformance, context);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_GAME_START, NCS_JOIN_SYNCING, (void*)&OnGameStart, context);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_SIMULATION_COMMAND, NCS_JOIN_SYNCING, (void*)&OnInGame, context);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_END_COMMAND_BATCH, NCS_JOIN_SYNCING, (void*)&OnJoinSyncEndCommandBatch, context);
	AddTransition(NCS_JOIN_SYNCING, (uint)NMT_LOADED_GAME, NCS_INGAME, (void*)&OnLoadedGame, context);

	AddTransition(NCS_LOADING, (uint)NMT_CHAT, NCS_LOADING, (void*)&OnChat, context);
	AddTransition(NCS_LOADING, (uint)NMT_GAME_SETUP, NCS_LOADING, (void*)&OnGameSetup, context);
	AddTransition(NCS_LOADING, (uint)NMT_PLAYER_ASSIGNMENT, NCS_LOADING, (void*)&OnPlayerAssignment, context);
	AddTransition(NCS_LOADING, (uint)NMT_KICKED, NCS_LOADING, (void*)&OnKicked, context);
	AddTransition(NCS_LOADING, (uint)NMT_CLIENT_TIMEOUT, NCS_LOADING, (void*)&OnClientTimeout, context);
	AddTransition(NCS_LOADING, (uint)NMT_CLIENT_PERFORMANCE, NCS_LOADING, (void*)&OnClientPerformance, context);
	AddTransition(NCS_LOADING, (uint)NMT_CLIENTS_LOADING, NCS_LOADING, (void*)&OnClientsLoading, context);
	AddTransition(NCS_LOADING, (uint)NMT_LOADED_GAME, NCS_INGAME, (void*)&OnLoadedGame, context);

	AddTransition(NCS_INGAME, (uint)NMT_REJOINED, NCS_INGAME, (void*)&OnRejoined, context);
	AddTransition(NCS_INGAME, (uint)NMT_KICKED, NCS_INGAME, (void*)&OnKicked, context);
	AddTransition(NCS_INGAME, (uint)NMT_CLIENT_TIMEOUT, NCS_INGAME, (void*)&OnClientTimeout, context);
	AddTransition(NCS_INGAME, (uint)NMT_CLIENT_PERFORMANCE, NCS_INGAME, (void*)&OnClientPerformance, context);
	AddTransition(NCS_INGAME, (uint)NMT_CLIENTS_LOADING, NCS_INGAME, (void*)&OnClientsLoading, context);
	AddTransition(NCS_INGAME, (uint)NMT_CLIENT_PAUSED, NCS_INGAME, (void*)&OnClientPaused, context);
	AddTransition(NCS_INGAME, (uint)NMT_CHAT, NCS_INGAME, (void*)&OnChat, context);
	AddTransition(NCS_INGAME, (uint)NMT_GAME_SETUP, NCS_INGAME, (void*)&OnGameSetup, context);
	AddTransition(NCS_INGAME, (uint)NMT_PLAYER_ASSIGNMENT, NCS_INGAME, (void*)&OnPlayerAssignment, context);
	AddTransition(NCS_INGAME, (uint)NMT_SIMULATION_COMMAND, NCS_INGAME, (void*)&OnInGame, context);
	AddTransition(NCS_INGAME, (uint)NMT_SYNC_ERROR, NCS_INGAME, (void*)&OnInGame, context);
	AddTransition(NCS_INGAME, (uint)NMT_END_COMMAND_BATCH, NCS_INGAME, (void*)&OnInGame, context);

	// Set first state
	SetFirstState(NCS_UNCONNECTED);
}

CNetClient::~CNetClient()
{
	DestroyConnection();
	JS_RemoveExtraGCRootsTracer(GetScriptInterface().GetJSRuntime(), CNetClient::Trace, this);
}

void CNetClient::TraceMember(JSTracer *trc)
{
	for (JS::Heap<JS::Value>& guiMessage : m_GuiMessageQueue)
		JS_CallValueTracer(trc, &guiMessage, "m_GuiMessageQueue");
}

void CNetClient::SetUserName(const CStrW& username)
{
	ENSURE(!m_Session); // must be called before we start the connection

	m_UserName = username;
}

void CNetClient::SetHostingPlayerName(const CStr& hostingPlayerName)
{
	m_HostingPlayerName = hostingPlayerName;
}

bool CNetClient::SetupConnection(const CStr& server, const u16 port, ENetHost* enetClient)
{
	CNetClientSession* session = new CNetClientSession(*this);
	bool ok = session->Connect(server, port, m_IsLocalClient, enetClient);
	SetAndOwnSession(session);
	return ok;
}

void CNetClient::SetAndOwnSession(CNetClientSession* session)
{
	delete m_Session;
	m_Session = session;
}

void CNetClient::DestroyConnection()
{
	// Attempt to send network messages from the current frame before connection is destroyed.
	if (m_ClientTurnManager)
	{
		m_ClientTurnManager->OnDestroyConnection();
		Flush();
	}
	SAFE_DELETE(m_Session);
}

void CNetClient::Poll()
{
	if (!m_Session)
		return;

	CheckServerConnection();
	m_Session->Poll();
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

	// Report if we have a bad ping to the server
	u32 meanRTT = m_Session->GetMeanRTT();
	if (meanRTT > DEFAULT_TURN_LENGTH_MP)
	{
		PushGuiMessage(
			"type", "netwarn",
			"warntype", "server-latency",
			"meanRTT", meanRTT);
	}
}

void CNetClient::Flush()
{
	if (m_Session)
		m_Session->Flush();
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
	JSContext* cx = GetScriptInterface().GetContext();
	JSAutoRequest rq(cx);

	std::string r;
	JS::RootedValue msg(cx);
	while (true)
	{
		GuiPoll(&msg);
		if (msg.isUndefined())
			break;
		r += GetScriptInterface().ToString(&msg) + "\n";
	}
	return r;
}

const ScriptInterface& CNetClient::GetScriptInterface()
{
	return m_Game->GetSimulation2()->GetScriptInterface();
}

void CNetClient::PostPlayerAssignmentsToScript()
{
	JSContext* cx = GetScriptInterface().GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue newAssignments(cx);
	ScriptInterface::CreateObject(cx, &newAssignments);

	for (const std::pair<CStr, PlayerAssignment>& p : m_PlayerAssignments)
	{
		JS::RootedValue assignment(cx);

		ScriptInterface::CreateObject(
			cx,
			&assignment,
			"name", p.second.m_Name,
			"player", p.second.m_PlayerID,
			"status", p.second.m_Status);

		GetScriptInterface().SetProperty(newAssignments, p.first.c_str(), assignment);
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

	SAFE_DELETE(m_Session);

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

void CNetClient::SendStartGameMessage()
{
	CGameStartMessage gameStart;
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
	authenticate.m_Password = L""; // TODO
	authenticate.m_IsLocalClient = m_IsLocalClient;
	SendMessage(&authenticate);
}

bool CNetClient::OnConnect(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_CONNECT_COMPLETE);

	CNetClient* client = static_cast<CNetClient*>(context);

	client->PushGuiMessage(
		"type", "netstatus",
		"status", "connected");

	return true;
}

bool CNetClient::OnHandshake(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_SERVER_HANDSHAKE);

	CNetClient* client = static_cast<CNetClient*>(context);

	CCliHandshakeMessage handshake;
	handshake.m_MagicResponse = PS_PROTOCOL_MAGIC_RESPONSE;
	handshake.m_ProtocolVersion = PS_PROTOCOL_VERSION;
	handshake.m_SoftwareVersion = PS_PROTOCOL_VERSION;
	client->SendMessage(&handshake);

	return true;
}

bool CNetClient::OnHandshakeResponse(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_SERVER_HANDSHAKE_RESPONSE);

	CNetClient* client = static_cast<CNetClient*>(context);
	CSrvHandshakeResponseMessage* message = static_cast<CSrvHandshakeResponseMessage*>(event->GetParamRef());

	client->m_GUID = message->m_GUID;

	if (message->m_Flags & PS_NETWORK_FLAG_REQUIRE_LOBBYAUTH)
	{
		if (g_XmppClient && !client->m_HostingPlayerName.empty())
			g_XmppClient->SendIqLobbyAuth(client->m_HostingPlayerName, client->m_GUID);
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

bool CNetClient::OnAuthenticateRequest(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_AUTHENTICATE);

	CNetClient* client = static_cast<CNetClient*>(context);
	client->SendAuthenticateMessage();
	return true;
}

bool CNetClient::OnAuthenticate(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_AUTHENTICATE_RESULT);

	CNetClient* client = static_cast<CNetClient*>(context);
	CAuthenticateResultMessage* message = static_cast<CAuthenticateResultMessage*>(event->GetParamRef());

	LOGMESSAGE("Net: Authentication result: host=%u, %s", message->m_HostID, utf8_from_wstring(message->m_Message));

	client->m_HostID = message->m_HostID;
	client->m_Rejoin = message->m_Code == ARC_OK_REJOINING;

	client->PushGuiMessage(
		"type", "netstatus",
		"status", "authenticated",
		"rejoining", client->m_Rejoin);

	return true;
}

bool CNetClient::OnChat(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_CHAT);

	CNetClient* client = static_cast<CNetClient*>(context);
	CChatMessage* message = static_cast<CChatMessage*>(event->GetParamRef());

	client->PushGuiMessage(
		"type", "chat",
		"guid", message->m_GUID,
		"text", message->m_Message);

	return true;
}

bool CNetClient::OnReady(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_READY);

	CNetClient* client = static_cast<CNetClient*>(context);
	CReadyMessage* message = static_cast<CReadyMessage*>(event->GetParamRef());

	client->PushGuiMessage(
		"type", "ready",
		"guid", message->m_GUID,
		"status", message->m_Status);

	return true;
}

bool CNetClient::OnGameSetup(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_GAME_SETUP);

	CNetClient* client = static_cast<CNetClient*>(context);
	CGameSetupMessage* message = static_cast<CGameSetupMessage*>(event->GetParamRef());

	client->m_GameAttributes = message->m_Data;

	client->PushGuiMessage(
		"type", "gamesetup",
		"data", message->m_Data);

	return true;
}

bool CNetClient::OnPlayerAssignment(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_PLAYER_ASSIGNMENT);

	CNetClient* client = static_cast<CNetClient*>(context);
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
bool CNetClient::OnGameStart(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_GAME_START);

	CNetClient* client = static_cast<CNetClient*>(context);

	client->m_Session->SetLongTimeout(true);

	// Find the player assigned to our GUID
	int player = -1;
	if (client->m_PlayerAssignments.find(client->m_GUID) != client->m_PlayerAssignments.end())
		player = client->m_PlayerAssignments[client->m_GUID].m_PlayerID;

	client->m_ClientTurnManager = new CNetClientTurnManager(
			*client->m_Game->GetSimulation2(), *client, client->m_HostID, client->m_Game->GetReplayLogger());

	client->m_Game->SetPlayerID(player);
	client->m_Game->StartGame(&client->m_GameAttributes, "");

	client->PushGuiMessage("type", "start");

	return true;
}

bool CNetClient::OnJoinSyncStart(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_JOIN_SYNC_START);

	CNetClient* client = static_cast<CNetClient*>(context);

	// The server wants us to start downloading the game state from it, so do so
	client->m_Session->GetFileTransferer().StartTask(
		shared_ptr<CNetFileReceiveTask>(new CNetFileReceiveTask_ClientRejoin(*client))
	);

	return true;
}

bool CNetClient::OnJoinSyncEndCommandBatch(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_END_COMMAND_BATCH);

	CNetClient* client = static_cast<CNetClient*>(context);

	CEndCommandBatchMessage* endMessage = (CEndCommandBatchMessage*)event->GetParamRef();

	client->m_ClientTurnManager->FinishedAllCommands(endMessage->m_Turn, endMessage->m_TurnLength);

	// Execute all the received commands for the latest turn
	client->m_ClientTurnManager->UpdateFastForward();

	return true;
}

bool CNetClient::OnRejoined(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_REJOINED);

	CNetClient* client = static_cast<CNetClient*>(context);
	CRejoinedMessage* message = static_cast<CRejoinedMessage*>(event->GetParamRef());

	client->PushGuiMessage(
		"type", "rejoined",
		"guid", message->m_GUID);

	return true;
}

bool CNetClient::OnKicked(void *context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_KICKED);

	CNetClient* client = static_cast<CNetClient*>(context);
	CKickedMessage* message = static_cast<CKickedMessage*>(event->GetParamRef());

	client->PushGuiMessage(
		"username", message->m_Name,
		"type", "kicked",
		"banned", message->m_Ban != 0);

	return true;
}

bool CNetClient::OnClientTimeout(void *context, CFsmEvent* event)
{
	// Report the timeout of some other client

	ENSURE(event->GetType() == (uint)NMT_CLIENT_TIMEOUT);

	CNetClient* client = static_cast<CNetClient*>(context);
	CClientTimeoutMessage* message = static_cast<CClientTimeoutMessage*>(event->GetParamRef());

	client->PushGuiMessage(
		"type", "netwarn",
		"warntype", "client-timeout",
		"guid", message->m_GUID,
		"lastReceivedTime", message->m_LastReceivedTime);

	return true;
}

bool CNetClient::OnClientPerformance(void *context, CFsmEvent* event)
{
	// Performance statistics for one or multiple clients

	ENSURE(event->GetType() == (uint)NMT_CLIENT_PERFORMANCE);

	CNetClient* client = static_cast<CNetClient*>(context);
	CClientPerformanceMessage* message = static_cast<CClientPerformanceMessage*>(event->GetParamRef());

	// Display warnings for other clients with bad ping
	for (size_t i = 0; i < message->m_Clients.size(); ++i)
	{
		if (message->m_Clients[i].m_MeanRTT < DEFAULT_TURN_LENGTH_MP || message->m_Clients[i].m_GUID == client->m_GUID)
			continue;

		client->PushGuiMessage(
			"type", "netwarn",
			"warntype", "client-latency",
			"guid", message->m_Clients[i].m_GUID,
			"meanRTT", message->m_Clients[i].m_MeanRTT);
	}

	return true;
}

bool CNetClient::OnClientsLoading(void *context, CFsmEvent *event)
{
	ENSURE(event->GetType() == (uint)NMT_CLIENTS_LOADING);

	CNetClient* client = static_cast<CNetClient*>(context);
	CClientsLoadingMessage* message = static_cast<CClientsLoadingMessage*>(event->GetParamRef());

	bool finished = true;
	std::vector<CStr> guids;
	guids.reserve(message->m_Clients.size());
	for (const CClientsLoadingMessage::S_m_Clients& mClient : message->m_Clients)
	{
		if (client->m_GUID == mClient.m_GUID)
			finished = false;

		guids.push_back(mClient.m_GUID);
	}

	// Disable the timeout here after processing the enet message, so as to ensure that the connection isn't currently
	// timing out (as it is when just leaving the loading screen in LoadFinished).
	if (finished)
		client->m_Session->SetLongTimeout(false);

	client->PushGuiMessage(
		"type", "clients-loading",
		"guids", guids);
	return true;
}

bool CNetClient::OnClientPaused(void *context, CFsmEvent *event)
{
	ENSURE(event->GetType() == (uint)NMT_CLIENT_PAUSED);

	CNetClient* client = static_cast<CNetClient*>(context);
	CClientPausedMessage* message = static_cast<CClientPausedMessage*>(event->GetParamRef());

	client->PushGuiMessage(
		"type", "paused",
		"pause", message->m_Pause != 0,
		"guid", message->m_GUID);

	return true;
}

bool CNetClient::OnLoadedGame(void* context, CFsmEvent* event)
{
	ENSURE(event->GetType() == (uint)NMT_LOADED_GAME);

	CNetClient* client = static_cast<CNetClient*>(context);

	// All players have loaded the game - start running the turn manager
	// so that the game begins
	client->m_Game->SetTurnManager(client->m_ClientTurnManager);

	client->PushGuiMessage(
		"type", "netstatus",
		"status", "active");

	// If we have rejoined an in progress game, send the rejoined message to the server.
	if (client->m_Rejoin)
		client->SendRejoinedMessage();

	// The last client to leave the loading screen didn't receive the CClientsLoadingMessage, so disable here.
	client->m_Session->SetLongTimeout(false);

	return true;
}

bool CNetClient::OnInGame(void *context, CFsmEvent* event)
{
	// TODO: should split each of these cases into a separate method

	CNetClient* client = static_cast<CNetClient*>(context);
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
