/* Copyright (C) 2018 Wildfire Games.
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
#include "NetServer.h"
#include "NetMessage.h"
#include "NetStats.h"
#include "lib/external_libraries/enet.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Profile.h"
#include "scriptinterface/ScriptInterface.h"

const u32 NETWORK_WARNING_TIMEOUT = 2000;

const u32 MAXIMUM_HOST_TIMEOUT = std::numeric_limits<u32>::max();

static const int CHANNEL_COUNT = 1;

// Only disable long timeouts after a packet from the remote enet peer has been processed.
// Otherwise a long timeout can still be in progress when disabling it here.
void SetEnetLongTimeout(ENetPeer* peer, bool isLocalClient, bool enabled)
{
#if (ENET_VERSION >= ENET_VERSION_CREATE(1, 3, 4))
	if (!peer || isLocalClient)
		return;

	if (enabled)
	{
		u32 timeout;
		CFG_GET_VAL("network.gamestarttimeout", timeout);
		enet_peer_timeout(peer, 0, timeout, timeout);
	}
	else
		enet_peer_timeout(peer, 0, 0, 0);
#endif
}

CNetClientSession::CNetClientSession(CNetClient& client) :
	m_Client(client), m_FileTransferer(this), m_Host(nullptr), m_Server(nullptr), m_Stats(nullptr), m_IsLocalClient(false)
{
}

CNetClientSession::~CNetClientSession()
{
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

bool CNetClientSession::Connect(const CStr& server, const u16 port, const bool isLocalClient, ENetHost* enetClient)
{
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
	m_IsLocalClient = isLocalClient;

	// Prevent the local client of the host from timing out too quickly.
#if (ENET_VERSION >= ENET_VERSION_CREATE(1, 3, 4))
	if (isLocalClient)
		enet_peer_timeout(peer, 1, MAXIMUM_HOST_TIMEOUT, MAXIMUM_HOST_TIMEOUT);
#endif

	m_Stats = new CNetStatsTable(m_Server);
	if (CProfileViewer::IsInitialised())
		g_ProfileViewer.AddRootTable(m_Stats);

	return true;
}

void CNetClientSession::Disconnect(u32 reason)
{
	ENSURE(m_Host && m_Server);

	// TODO: ought to do reliable async disconnects, probably
	enet_peer_disconnect_now(m_Server, reason);
	enet_host_destroy(m_Host);

	m_Host = NULL;
	m_Server = NULL;

	SAFE_DELETE(m_Stats);
}

void CNetClientSession::Poll()
{
	PROFILE3("net client poll");

	ENSURE(m_Host && m_Server);

	m_FileTransferer.Poll();

	ENetEvent event;
	while (enet_host_service(m_Host, &event, 0) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
		{
			ENSURE(event.peer == m_Server);

			// Report the server address
			char hostname[256] = "(error)";
			enet_address_get_host_ip(&event.peer->address, hostname, ARRAY_SIZE(hostname));
			LOGMESSAGE("Net client: Connected to %s:%u", hostname, (unsigned int)event.peer->address.port);

			m_Client.HandleConnect();

			break;
		}

		case ENET_EVENT_TYPE_DISCONNECT:
		{
			ENSURE(event.peer == m_Server);

			LOGMESSAGE("Net client: Disconnected");
			m_Client.HandleDisconnect(event.data);
			return;
		}

		case ENET_EVENT_TYPE_RECEIVE:
		{
			CNetMessage* msg = CNetMessageFactory::CreateMessage(event.packet->data, event.packet->dataLength, m_Client.GetScriptInterface());
			if (msg)
			{
				LOGMESSAGE("Net client: Received message %s of size %lu from server", msg->ToString().c_str(), (unsigned long)msg->GetSerializedLength());

				m_Client.HandleMessage(msg);

				delete msg;
			}

			enet_packet_destroy(event.packet);

			break;
		}

		case ENET_EVENT_TYPE_NONE:
			break;
		}
	}

}

void CNetClientSession::Flush()
{
	PROFILE3("net client flush");

	ENSURE(m_Host && m_Server);

	enet_host_flush(m_Host);
}

bool CNetClientSession::SendMessage(const CNetMessage* message)
{
	ENSURE(m_Host && m_Server);

	return CNetHost::SendMessage(message, m_Server, "server");
}

u32 CNetClientSession::GetLastReceivedTime() const
{
	if (!m_Server)
		return 0;

	return enet_time_get() - m_Server->lastReceiveTime;
}

u32 CNetClientSession::GetMeanRTT() const
{
	if (!m_Server)
		return 0;

	return m_Server->roundTripTime;
}

void CNetClientSession::SetLongTimeout(bool enabled)
{
	SetEnetLongTimeout(m_Server, m_IsLocalClient, enabled);
}

CNetServerSession::CNetServerSession(CNetServerWorker& server, ENetPeer* peer) :
	m_Server(server), m_FileTransferer(this), m_Peer(peer), m_IsLocalClient(false), m_HostID(0), m_GUID(), m_UserName()
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

void CNetServerSession::Disconnect(u32 reason)
{
	Update((uint)NMT_CONNECTION_LOST, NULL);

	enet_peer_disconnect(m_Peer, reason);
}

void CNetServerSession::DisconnectNow(u32 reason)
{
	enet_peer_disconnect_now(m_Peer, reason);
}

bool CNetServerSession::SendMessage(const CNetMessage* message)
{
	return m_Server.SendMessage(m_Peer, message);
}

bool CNetServerSession::IsLocalClient() const
{
	return m_IsLocalClient;
}

void CNetServerSession::SetLocalClient(bool isLocalClient)
{
	m_IsLocalClient = isLocalClient;

	if (!isLocalClient)
		return;

	// Prevent the local client of the host from timing out too quickly
#if (ENET_VERSION >= ENET_VERSION_CREATE(1, 3, 4))
	enet_peer_timeout(m_Peer, 0, MAXIMUM_HOST_TIMEOUT, MAXIMUM_HOST_TIMEOUT);
#endif
}

void CNetServerSession::SetLongTimeout(bool enabled)
{
	SetEnetLongTimeout(m_Peer, m_IsLocalClient, enabled);
}
