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

#include "NetSession.h"

#include "NetClient.h"
#include "NetMessage.h"
#include "NetServer.h"
#include "NetStats.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"
#include "scriptinterface/ScriptInterface.h"

constexpr int NETCLIENT_POLL_TIMEOUT = 50;

constexpr int CHANNEL_COUNT = 1;

CNetClientSession::CNetClientSession(CNetClient& client) :
	m_Client(client), m_FileTransferer(this), m_Host(nullptr), m_Server(nullptr),
	m_Stats(nullptr), m_IncomingMessages(16), m_OutgoingMessages(16),
	m_LoopRunning(false), m_ShouldShutdown(false), m_MeanRTT(0), m_LastReceivedTime(0)
{
}

CNetClientSession::~CNetClientSession()
{
	ENSURE(!m_LoopRunning);

	delete m_Stats;

	if (m_Host && m_Server)
	{
		// Disconnect immediately (we can't wait for acks)
		enet_peer_disconnect_now(m_Server, NDR_SERVER_SHUTDOWN);
		enet_host_destroy(m_Host);

		m_Host = NULL;
		m_Server = NULL;
	}
}

bool CNetClientSession::Connect(const CStr& server, const u16 port, ENetHost* enetClient)
{
	ENSURE(!m_LoopRunning);
	ENSURE(!m_Host);
	ENSURE(!m_Server);

	// Create ENet host
	ENetHost* host;
	if (enetClient != nullptr)
		host = enetClient;
	else
		host = enet_host_create(NULL, 1, CHANNEL_COUNT, 0, 0);

	if (!host)
		return false;

	// Bind to specified host
	ENetAddress addr;
	addr.port = port;
	if (enet_address_set_host(&addr, server.c_str()) < 0)
		return false;

	// Initiate connection to server
	ENetPeer* peer = enet_host_connect(host, &addr, CHANNEL_COUNT, 0);
	if (!peer)
		return false;

	m_Host = host;
	m_Server = peer;

	m_Stats = new CNetStatsTable(m_Server);
	if (CProfileViewer::IsInitialised())
		g_ProfileViewer.AddRootTable(m_Stats);

	return true;
}

void CNetClientSession::RunNetLoop(CNetClientSession* session)
{
	ENSURE(!session->m_LoopRunning);
	session->m_LoopRunning = true;

	debug_SetThreadName("NetClientSession loop");

	while (!session->m_ShouldShutdown)
	{
		ENSURE(session->m_Host && session->m_Server);

		session->m_FileTransferer.Poll();
		session->Poll();
		session->Flush();

		session->m_LastReceivedTime = enet_time_get() - session->m_Server->lastReceiveTime;
		session->m_MeanRTT = session->m_Server->roundTripTime;
	}

	session->m_LoopRunning = false;

	// Deleting the session is handled in this thread as it might outlive the CNetClient.
	SAFE_DELETE(session);
}

void CNetClientSession::Shutdown()
{
	m_ShouldShutdown = true;
}

void CNetClientSession::Poll()
{
	ENetEvent event;

	// Use the timeout to make the thread wait and save CPU time.
	if (enet_host_service(m_Host, &event, NETCLIENT_POLL_TIMEOUT) <= 0)
		return;

	if (event.type == ENET_EVENT_TYPE_CONNECT)
	{
		ENSURE(event.peer == m_Server);

		// Report the server address immediately.
		char hostname[256] = "(error)";
		enet_address_get_host_ip(&event.peer->address, hostname, ARRAY_SIZE(hostname));
		LOGMESSAGE("Net client: Connected to %s:%u", hostname, (unsigned int)event.peer->address.port);
		m_Connected = true;

		m_IncomingMessages.push(event);
	}
	else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
	{
		ENSURE(event.peer == m_Server);

		// Report immediately.
		LOGMESSAGE("Net client: Disconnected");
		m_Connected = false;

		m_IncomingMessages.push(event);
	}
	else if (event.type == ENET_EVENT_TYPE_RECEIVE)
		m_IncomingMessages.push(event);
}

void CNetClientSession::Flush()
{
	ENetPacket* packet;
	while (m_OutgoingMessages.pop(packet))
		if (enet_peer_send(m_Server, CNetHost::DEFAULT_CHANNEL, packet) < 0)
		{
			// Report the error, but do so silently if we know we are disconnected.
			if (m_Connected)
				LOGERROR("NetClient: Failed to send packet to server");
			else
				LOGMESSAGE("NetClient: Failed to send packet to server");
		}

	enet_host_flush(m_Host);
}

void CNetClientSession::ProcessPolledMessages()
{
	ENetEvent event;
	while(m_IncomingMessages.pop(event))
	{
		if (event.type == ENET_EVENT_TYPE_CONNECT)
			m_Client.HandleConnect();
		else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
		{
			// This deletes the session, so we must break;
			m_Client.HandleDisconnect(event.data);
			break;
		}
		else if (event.type == ENET_EVENT_TYPE_RECEIVE)
		{
			CNetMessage* msg = CNetMessageFactory::CreateMessage(event.packet->data, event.packet->dataLength, m_Client.GetScriptInterface());
			if (msg)
			{
				LOGMESSAGE("Net client: Received message %s of size %lu from server", msg->ToString().c_str(), (unsigned long)msg->GetSerializedLength());

				m_Client.HandleMessage(msg);
			}
			// Thread-safe
			enet_packet_destroy(event.packet);
		}
	}
}

bool CNetClientSession::SendMessage(const CNetMessage* message)
{
	ENSURE(m_Host && m_Server);

	// Thread-safe.
	ENetPacket* packet = CNetHost::CreatePacket(message);
	if (!packet)
		return false;

	if (!m_OutgoingMessages.push(packet))
	{
		LOGERROR("NetClient: Failed to push message on the outgoing queue.");
		return false;
	}

	return true;
}

u32 CNetClientSession::GetLastReceivedTime() const
{
	if (!m_Server)
		return 0;

	return m_LastReceivedTime;
}

u32 CNetClientSession::GetMeanRTT() const
{
	if (!m_Server)
		return 0;

	return m_MeanRTT;
}

CNetServerSession::CNetServerSession(CNetServerWorker& server, ENetPeer* peer) :
	m_Server(server), m_FileTransferer(this), m_Peer(peer), m_HostID(0), m_GUID(), m_UserName()
{
}

u32 CNetServerSession::GetIPAddress() const
{
	return m_Peer->address.host;
}

u32 CNetServerSession::GetLastReceivedTime() const
{
	if (!m_Peer)
		return 0;

	return enet_time_get() - m_Peer->lastReceiveTime;
}

u32 CNetServerSession::GetMeanRTT() const
{
	if (!m_Peer)
		return 0;

	return m_Peer->roundTripTime;
}

void CNetServerSession::Disconnect(NetDisconnectReason reason)
{
	if (reason == NDR_UNKNOWN)
		LOGWARNING("Disconnecting client without communicating the disconnect reason!");

	Update((uint)NMT_CONNECTION_LOST, NULL);

	enet_peer_disconnect(m_Peer, static_cast<enet_uint32>(reason));
}

void CNetServerSession::DisconnectNow(NetDisconnectReason reason)
{
	if (reason == NDR_UNKNOWN)
		LOGWARNING("Disconnecting client without communicating the disconnect reason!");

	enet_peer_disconnect_now(m_Peer, static_cast<enet_uint32>(reason));
}

bool CNetServerSession::SendMessage(const CNetMessage* message)
{
	return m_Server.SendMessage(m_Peer, message);
}
