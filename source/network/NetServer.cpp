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
#include "NetTurnManager.h"

#include "ps/CLogger.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"

#include <enet/enet.h>

#define	DEFAULT_SERVER_NAME			L"Unnamed Server"
#define DEFAULT_WELCOME_MESSAGE		L"Welcome"
#define MAX_CLIENTS					8

CNetServer* g_NetServer = NULL;

static CStr DebugName(CNetServerSession* session)
{
	if (session == NULL)
		return "[unknown host]";
	if (session->GetGUID().empty())
		return "[unauthed host]";
	return "[" + session->GetGUID().substr(0, 8) + "...]";
}

CNetServer::CNetServer() :
	m_ScriptInterface(new ScriptInterface("Engine")), m_NextHostID(1), m_Host(NULL)
{
	m_State = SERVER_STATE_UNCONNECTED;

	m_ServerTurnManager = NULL;

	m_Port = PS_DEFAULT_PORT;
	m_ServerName = DEFAULT_SERVER_NAME;
	m_WelcomeMessage = DEFAULT_WELCOME_MESSAGE;
}

CNetServer::~CNetServer()
{
	for (size_t i = 0; i < m_Sessions.size(); ++i)
	{
		m_Sessions[i]->Disconnect();
		delete m_Sessions[i];
	}

	if (m_Host)
	{
		enet_host_destroy(m_Host);
	}

	m_GameAttributes = CScriptValRooted(); // clear root before deleting its context
	delete m_ScriptInterface;
}

bool CNetServer::SetupConnection()
{
	debug_assert(m_State == SERVER_STATE_UNCONNECTED);
	debug_assert(!m_Host);

	// Bind to default host
	ENetAddress addr;
	addr.host = ENET_HOST_ANY;
	addr.port = m_Port;

	// Create ENet server
	m_Host = enet_host_create(&addr, MAX_CLIENTS, 0, 0);
	if (!m_Host)
	{
		LOGERROR(L"Net server: enet_host_create failed");
		return false;
	}

	m_State = SERVER_STATE_PREGAME;

	return true;
}

bool CNetServer::SendMessage(ENetPeer* peer, const CNetMessage* message)
{
	debug_assert(m_Host);

	CNetServerSession* session = static_cast<CNetServerSession*>(peer->data);

	return CNetHost::SendMessage(message, peer, DebugName(session).c_str());
}

bool CNetServer::Broadcast(const CNetMessage* message)
{
	debug_assert(m_Host);

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

void CNetServer::Poll()
{
	debug_assert(m_Host);

	for (size_t i = 0; i < m_LocalMessageQueue.size(); ++i)
	{
		CNetMessage* msg = m_LocalMessageQueue[i].second;
		CNetServerSession* session = m_LocalMessageQueue[i].first;

		LOGMESSAGE(L"Net server: Received local message %hs of size %lu from %hs", msg->ToString().c_str(), (unsigned long)msg->GetSerializedLength(), DebugName(session).c_str());

		bool ok = HandleMessageReceive(msg, session);
		debug_assert(ok); // TODO

		delete msg;
	}
	m_LocalMessageQueue.clear();

	// Poll host for events
	ENetEvent event;
	while (enet_host_service(m_Host, &event, 0) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
		{
			// If this is a new client to our server, save the peer reference
			if (std::find(m_Peers.begin(), m_Peers.end(), event.peer) == m_Peers.end())
				m_Peers.push_back(event.peer);

			// Report the client address
			char hostname[256] = "(error)";
			enet_address_get_host_ip(&event.peer->address, hostname, ARRAY_SIZE(hostname));
			LOGMESSAGE(L"Net server: Received connection from %hs:%u", hostname, event.peer->address.port);

			// Set up a session object for this peer

			CNetServerSession* session = new CNetServerSessionRemote(*this, event.peer);

			SetupSession(session);

			if (!HandleConnect(session))
			{
				delete session;
				break;
			}

			debug_assert(event.peer->data == NULL);
			event.peer->data = session;

			break;
		}

		case ENET_EVENT_TYPE_DISCONNECT:
		{
			// Delete from our peer list
			m_Peers.erase(remove(m_Peers.begin(), m_Peers.end(), event.peer), m_Peers.end());

			// If there is an active session with this peer, then reset and delete it

			CNetServerSession* session = static_cast<CNetServerSession*>(event.peer->data);
			if (session)
			{
				LOGMESSAGE(L"Net server: %hs disconnected", DebugName(session).c_str());

				session->Update((uint)NMT_CONNECTION_LOST, NULL);

				HandleDisconnect(session);

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

					bool ok = HandleMessageReceive(msg, session);
					debug_assert(ok); // TODO

					delete msg;
				}
			}

			// Done using the packet
			enet_packet_destroy(event.packet);

			break;
		}
		}
	}
}

void CNetServer::AddLocalClientSession(CNetClientSessionLocal& clientSession)
{
	LOGMESSAGE(L"Net server: Received local connection");
	CNetServerSessionLocal* session = new CNetServerSessionLocal(*this, clientSession);
	clientSession.SetServerSession(session);
	SetupSession(session);
	HandleConnect(session);
}

void CNetServer::SendLocalMessage(CNetClientSessionLocal& clientSession, const CNetMessage* message)
{
	CNetMessage* clonedMessage = CNetMessageFactory::CloneMessage(message, GetScriptInterface());
	if (!clonedMessage)
		return;
	m_LocalMessageQueue.push_back(std::make_pair(clientSession.GetServerSession(), clonedMessage));
}

bool CNetServer::HandleMessageReceive(const CNetMessage* message, CNetServerSession* session)
{
	// Update FSM
	bool ok = session->Update(message->GetType(), (void*)message);
	if (!ok)
		LOGERROR(L"Net server: Error running FSM update (type=%d state=%d)", (int)message->GetType(), (int)session->GetCurrState());
	return ok;
}

void CNetServer::SetupSession(CNetServerSession* session)
{
	void* context = session;

	// Set up transitions for session
	session->AddTransition(NSS_HANDSHAKE, (uint)NMT_CONNECTION_LOST, NSS_UNCONNECTED, (void*)&OnDisconnect, context);
	session->AddTransition(NSS_HANDSHAKE, (uint)NMT_CLIENT_HANDSHAKE, NSS_AUTHENTICATE, (void*)&OnClientHandshake, context);

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

bool CNetServer::HandleConnect(CNetServerSession* session)
{
	m_Sessions.push_back(session);

	// Player joined the game, start authentication
	CSrvHandshakeMessage handshake;
	handshake.m_Magic = PS_PROTOCOL_MAGIC;
	handshake.m_ProtocolVersion = PS_PROTOCOL_VERSION;
	handshake.m_SoftwareVersion = PS_PROTOCOL_VERSION;
	return session->SendMessage(&handshake);
}

bool CNetServer::HandleDisconnect(CNetServerSession* session)
{
	m_Sessions.erase(remove(m_Sessions.begin(), m_Sessions.end(), session), m_Sessions.end());

	return true;
}

void CNetServer::OnUserJoin(CNetServerSession* session)
{
	AddPlayer(session->GetGUID(), session->GetUserName());

	CGameSetupMessage gameSetupMessage(GetScriptInterface());
	gameSetupMessage.m_Data = m_GameAttributes;
	session->SendMessage(&gameSetupMessage);

	CPlayerAssignmentMessage assignMessage;
	ConstructPlayerAssignmentMessage(assignMessage);
	session->SendMessage(&assignMessage);

	OnAddPlayer();
}

void CNetServer::OnUserLeave(CNetServerSession* session)
{
	RemovePlayer(session->GetGUID());

	OnRemovePlayer();
}

void CNetServer::AddPlayer(const CStr& guid, const CStrW& name)
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

void CNetServer::RemovePlayer(const CStr& guid)
{
	m_PlayerAssignments.erase(guid);

	SendPlayerAssignments();

	OnRemovePlayer();
}

void CNetServer::AssignPlayer(int playerID, const CStr& guid)
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

void CNetServer::ConstructPlayerAssignmentMessage(CPlayerAssignmentMessage& message)
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

void CNetServer::SendPlayerAssignments()
{
	CPlayerAssignmentMessage message;
	ConstructPlayerAssignmentMessage(message);
	Broadcast(&message);
}

ScriptInterface& CNetServer::GetScriptInterface()
{
	return *m_ScriptInterface;
}

bool CNetServer::OnClientHandshake(void* context, CFsmEvent* event)
{
	debug_assert(event->GetType() == (uint)NMT_CLIENT_HANDSHAKE);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServer& server = session->GetServer();

	CCliHandshakeMessage* message = (CCliHandshakeMessage*)event->GetParamRef();
	if (message->m_ProtocolVersion != PS_PROTOCOL_VERSION)
	{
		// TODO: probably should report some error message (either locally or to the client)
		session->Disconnect();
	}
	else
	{
		CSrvHandshakeResponseMessage handshakeResponse;
		handshakeResponse.m_UseProtocolVersion = PS_PROTOCOL_VERSION;
		handshakeResponse.m_Message = server.m_WelcomeMessage;
		handshakeResponse.m_Flags = 0;
		session->SendMessage(&handshakeResponse);
	}

	return true;
}

bool CNetServer::OnAuthenticate(void* context, CFsmEvent* event)
{
	debug_assert(event->GetType() == (uint)NMT_AUTHENTICATE);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServer& server = session->GetServer();

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

bool CNetServer::OnInGame(void* context, CFsmEvent* event)
{
	// TODO: should split each of these cases into a separate method

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServer& server = session->GetServer();

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

bool CNetServer::OnChat(void* context, CFsmEvent* event)
{
	debug_assert(event->GetType() == (uint)NMT_CHAT);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServer& server = session->GetServer();

	CChatMessage* message = (CChatMessage*)event->GetParamRef();

	message->m_Sender = session->GetUserName();

	server.Broadcast(message);

	return true;
}

bool CNetServer::OnLoadedGame(void* context, CFsmEvent* event)
{
	debug_assert(event->GetType() == (uint)NMT_LOADED_GAME);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServer& server = session->GetServer();

	server.CheckGameLoadStatus(session);

	return true;
}

bool CNetServer::OnDisconnect(void* context, CFsmEvent* event)
{
	debug_assert(event->GetType() == (uint)NMT_CONNECTION_LOST);

	CNetServerSession* session = (CNetServerSession*)context;
	CNetServer& server = session->GetServer();

	// If the user had authenticated, we need to handle their leaving
	if (session->GetCurrState() == NSS_PREGAME || session->GetCurrState() == NSS_INGAME)
		server.OnUserLeave(session);

	return true;
}

void CNetServer::CheckGameLoadStatus(CNetServerSession* changedSession)
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

void CNetServer::StartGame()
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

void CNetServer::UpdateGameAttributes(const CScriptValRooted& attrs)
{
	m_GameAttributes = attrs;

	if (!m_Host)
		return;

	CGameSetupMessage gameSetupMessage(GetScriptInterface());
	gameSetupMessage.m_Data = m_GameAttributes;
	Broadcast(&gameSetupMessage);
}

CStrW CNetServer::SanitisePlayerName(const CStrW& original)
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

CStrW CNetServer::DeduplicatePlayerName(const CStrW& original)
{
	CStrW name = original;

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

		name = original + L" (" + CStrW(id++) + L")";
	}
}
