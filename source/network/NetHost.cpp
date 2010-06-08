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
#include "NetHost.h"
#include "NetSession.h"
#include "NetMessage.h"

#include "ps/CLogger.h"
#include "simulation2/Simulation2.h"

#include <enet/enet.h>

static const int ENET_DEFAULT_CHANNEL = 0;
static const int CONNECT_TIMEOUT = 5000;
static const int DISCONNECT_TIMEOUT = 1000;

//-----------------------------------------------------------------------------
// Name: CNetHost()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetHost::CNetHost(ScriptInterface& scriptInterface) :
	m_ScriptInterface(scriptInterface)
{
	m_Host = NULL;
}

//-----------------------------------------------------------------------------
// Name: ~CNetHost()
// Desc: Destructor
//-----------------------------------------------------------------------------
CNetHost::~CNetHost()
{
//	Shutdown(); // TODO: should do something like this except don't call HandleDisconnect()
}

//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Creates a client host
//-----------------------------------------------------------------------------
bool CNetHost::Create()
{
	debug_assert(!m_Host);

	// Create ENet host
	m_Host = enet_host_create(NULL, 1, 0, 0);
	if (!m_Host)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Creates a server host
//-----------------------------------------------------------------------------
bool CNetHost::Create(u16 port, size_t maxPeers)
{
	ENetAddress addr;

	// Bind to default host
	addr.host = ENET_HOST_ANY;
	addr.port = port;

	// Create ENet server
	m_Host = enet_host_create(&addr, maxPeers, 0, 0);
	if (!m_Host)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Name: Shutdown()
// Desc: Shuts down network server and releases any resources
//-----------------------------------------------------------------------------
void CNetHost::Shutdown()
{
	// Disconnect and release each peer
	PeerSessionList::iterator it = m_PeerSessions.begin();
	for (; it != m_PeerSessions.end(); it++)
	{
		if (!it->pSession)
			continue;

		Disconnect(it->pSession);

		delete it->pSession;
	}

	m_PeerSessions.clear();

	// Destroy server
	if (m_Host)
		enet_host_destroy(m_Host);

	m_Host = NULL;
}

//-----------------------------------------------------------------------------
// Name: Connect()
// Desc: Connects to the specified remote host
// Note: Only clients use this method for connection to server
//-----------------------------------------------------------------------------
bool CNetHost::Connect(const CStr& host, u16 port)
{
	debug_assert(m_Host);

	// Bind to specified host
	ENetAddress addr;
	addr.port = port;
	if (enet_address_set_host(&addr, host.c_str()) < 0)
		return false;

	// Initiate connection, allocate one channel
	ENetPeer* pPeer = enet_host_connect(m_Host, &addr, 1);
	if (!pPeer)
		return false;

	// Wait a few seconds for the connection to succeed
	// TODO: we ought to poll asynchronously so we can update the GUI while waiting
	ENetEvent event;
	if (enet_host_service(m_Host, &event, CONNECT_TIMEOUT) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
	{
		// Connection succeeded
		CNetSession* pNewSession = new CNetSession(this, event.peer);

		if (!SetupSession(pNewSession))
		{
			delete pNewSession;
			return false;
		}

		LOGMESSAGE(L"Net: Successfully connected to server %hs:%d", host.c_str(), port);

		// Successfully handled?
		if (!HandleConnect(pNewSession))
		{
			delete pNewSession;
			return false;
		}

		// Store the only server session
		PeerSession item;
		item.pPeer = event.peer;
		item.pSession = pNewSession;
		m_PeerSessions.push_back(item);

		return true;
	}

	LOGERROR(L"Net: Connection to server %hs:%d failed", host.c_str(), port);

	// Timed out or a host was disconnected
	enet_peer_reset(pPeer);

	return false;
}

//-----------------------------------------------------------------------------
// Name: ConnectAsync()
// Desc: Connects to the specified remote host
// Note: Only clients use this method for connection to server
//-----------------------------------------------------------------------------
bool CNetHost::ConnectAsync(const CStr& host, u16 port)
{
	debug_assert(m_Host);

	// Bind to specified host
	ENetAddress addr;
	addr.port = port;
	if (enet_address_set_host(&addr, host.c_str()) < 0)
		return false;

	// Initiate connection, allocate one channel
	ENetPeer* pPeer = enet_host_connect(m_Host, &addr, 1);
	if (!pPeer)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Name: Disconnect()
// Desc: Disconnects the specified session from the host
//-----------------------------------------------------------------------------
bool CNetHost::Disconnect(CNetSession* pSession)
{
	// Validate parameters
	if (!pSession)
		return false;

	debug_assert(m_Host);
	debug_assert(pSession->m_Peer);

	// Disconnect peer
	enet_peer_disconnect(pSession->m_Peer, 0);

	// Allow a few seconds for the disconnect to succeed
	ENetEvent event;
	while (enet_host_service(m_Host, &event, DISCONNECT_TIMEOUT) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_RECEIVE:

			// Drop any received packets
			enet_packet_destroy(event.packet);
			break;

		case ENET_EVENT_TYPE_DISCONNECT:

			// Disconnect received for peer
			if (!HandleDisconnect(pSession))
				return false;
			break;
		}
	}

	// Disconnect attempt didn't succeed, force connection down
	enet_peer_reset(pSession->m_Peer);

	return true;
}

//-----------------------------------------------------------------------------
// Name: ProcessEvents()
// Desc: Wait for events and shuttles packets between the host and its peers
//-----------------------------------------------------------------------------
void CNetHost::Poll()
{
	debug_assert(m_Host);

	// Poll host for events
	ENetEvent event;
	while (enet_host_service(m_Host, &event, 0) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
		{
			// A new client has connected, handle it
			CNetSession* pSession = new CNetSession(this, event.peer);

			// Setup new session
			if (!SetupSession(pSession))
			{
				delete pSession;
				break;
			}

			LOGMESSAGE(L"Net: A new client connected from %x:%u", event.peer->address.host, event.peer->address.port);

			// Successfully handled?
			if (!HandleConnect(pSession))
			{
				delete pSession;
				break;
			}

			event.peer->data = pSession;

			// Add new item to internal list
			PeerSession item;
			item.pPeer = event.peer;
			item.pSession = pSession;
			m_PeerSessions.push_back( item );

			break;
		}

		case ENET_EVENT_TYPE_DISCONNECT:
		{
			// Client has disconnected, handle it
			PeerSessionList::iterator it = m_PeerSessions.begin();
			for (; it != m_PeerSessions.end(); it++)
			{
				// Is this our session?
				if (it->pPeer == event.peer)
				{
					LOGMESSAGE(L"Net: %p disconnected", event.peer->data);

					// Successfully handled?
					if (HandleDisconnect(it->pSession))
						m_PeerSessions.erase(it);
				}
			}

			break;
		}

		case ENET_EVENT_TYPE_RECEIVE:
		{
			// A new data packet was received from client, handle message
			PeerSessionList::iterator it = m_PeerSessions.begin();
			for (; it != m_PeerSessions.end(); it++)
			{
				// Is this our session?
				if (it->pPeer == event.peer)
				{
					bool ok = false;

					// Create message from raw data
					CNetMessage* pNewMessage = CNetMessageFactory::CreateMessage(event.packet->data, event.packet->dataLength, m_ScriptInterface);
					if (pNewMessage)
					{
						LOGMESSAGE(L"Message %hs of size %lu was received from %p", pNewMessage->ToString().c_str(), (unsigned long)pNewMessage->GetSerializedLength(), event.peer->data);

						ok = HandleMessageReceive(pNewMessage, it->pSession);

						delete pNewMessage;
					}

					// Done using the packet
					enet_packet_destroy(event.packet);

					// TODO: what should we do if ok is false?
					// For now, just carry on as if nothing bad happened
				}
			}

			break;
		}
		}
	}
}

//-----------------------------------------------------------------------------
// Name: Broadcast()
// Desc: Broadcast the specified message to connected clients
//-----------------------------------------------------------------------------
void CNetHost::Broadcast(const CNetMessage* pMessage)
{
	// Validate parameters
	if (!pMessage)
		return;

	// Loop through the list of sessions and send the message to each
	for (uint i = 0; i < GetSessionCount(); i++)
	{
		CNetSession* pCurrSession = GetSession(i);
		if (!pCurrSession)
			continue;

		SendMessage(pCurrSession, pMessage);
	}
}

//-----------------------------------------------------------------------------
// Name: SendMessage()
// Desc: Sends the specified message to peer
//-----------------------------------------------------------------------------
bool CNetHost::SendMessage(const CNetSession* pSession, const CNetMessage* pMessage)
{
	// Validate parameters
	if (!pMessage || !pSession)
		return false;

	debug_assert(pSession->m_Peer);
	debug_assert(m_Host);

	size_t size = pMessage->GetSerializedLength();

	debug_assert(size); // else we'll fail when accessing the 0th element

	// Adjust buffer for message
	std::vector<u8> buffer;
	buffer.resize(size);

	// Save message to internal buffer
	pMessage->Serialize(&buffer[0]);

	// Create a reliable packet
	ENetPacket* pPacket = enet_packet_create(&buffer[0], size, ENET_PACKET_FLAG_RELIABLE);
	if (!pPacket)
		return false;

	// Let ENet send the message to peer
	if (enet_peer_send(pSession->m_Peer, ENET_DEFAULT_CHANNEL, pPacket) < 0)
	{
		// ENet failed to send the packet
		LOGERROR(L"Net: Failed to send ENet packet to peer");

		return false;
	}
	else
	{
		LOGMESSAGE(L"Net: Message %hs of size %lu was sent to %p",
				pMessage->ToString().c_str(), (unsigned long)size, pSession->m_Peer->data);
	}

	// Don't call enet_host_flush - let it queue up all the packets
	// and send them during the next frame

	return true;
}

//-----------------------------------------------------------------------------
// Name: ReceiveMessage()
// Desc: Receives a message from client if incoming packets are available
//-----------------------------------------------------------------------------
CNetMessage* CNetHost::ReceiveMessage(const CNetSession* pSession)
{
	// Validate parameters
	if (!pSession)
		return NULL;

	debug_assert(pSession->m_Peer);

	// Let ENet receive a message from peer
	ENetPacket* pPacket = enet_peer_receive(pSession->m_Peer, ENET_DEFAULT_CHANNEL);
	if (!pPacket)
		return NULL;

	// Create new message
	return CNetMessageFactory::CreateMessage(pPacket->data, pPacket->dataLength, m_ScriptInterface);
}

//-----------------------------------------------------------------------------
// Name: HandleMessageReceive()
// Desc: Allow application to handle message recive
//-----------------------------------------------------------------------------
bool CNetHost::HandleMessageReceive(CNetMessage* pMessage, CNetSession* pSession)
{
	// Validate parameters
	if (!pSession || !pMessage)
		return false;

	// Update FSM
	return pSession->Update(pMessage->GetType(), pMessage);
}

//-----------------------------------------------------------------------------
// Name: GetSessionCount()
// Desc: Returns the number of sessions the host manages
//-----------------------------------------------------------------------------
size_t CNetHost::GetSessionCount() const
{
	return m_PeerSessions.size();
}

//-----------------------------------------------------------------------------
// Name: GetSession()
// Desc: Rteurns the session for the index
//-----------------------------------------------------------------------------
CNetSession* CNetHost::GetSession(size_t index)
{
	// Validate parameter
	if (index >= GetSessionCount())
		return NULL;

	return m_PeerSessions[index].pSession;
}
