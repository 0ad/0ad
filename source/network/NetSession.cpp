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
#include "NetSession.h"
#include "NetClient.h"
#include "NetServer.h"
#include "NetMessage.h"
#include "ps/CLogger.h"
#include "scriptinterface/ScriptInterface.h"

#include <enet/enet.h>

static const int CHANNEL_COUNT = 1;


CNetClientSession::CNetClientSession(CNetClient& client) :
	m_Client(client)
{
}

CNetClientSession::~CNetClientSession()
{
}



CNetClientSessionRemote::CNetClientSessionRemote(CNetClient& client) :
	CNetClientSession(client), m_Host(NULL), m_Server(NULL)
{
}

CNetClientSessionRemote::~CNetClientSessionRemote()
{

}

bool CNetClientSessionRemote::Connect(u16 port, const CStr& server)
{
	debug_assert(!m_Host);
	debug_assert(!m_Server);

	// Create ENet host
	ENetHost* host = enet_host_create(NULL, 1, 0, 0);
	if (!host)
		return false;

	// Bind to specified host
	ENetAddress addr;
	addr.port = port;
	if (enet_address_set_host(&addr, server.c_str()) < 0)
		return false;

	// Initiate connection to server
	ENetPeer* peer = enet_host_connect(host, &addr, CHANNEL_COUNT);
	if (!peer)
		return false;

	m_Host = host;
	m_Server = peer;

	return true;
}

void CNetClientSessionRemote::Disconnect()
{
	debug_assert(m_Host && m_Server);

	// TODO: ought to do reliable async disconnects, probably
	enet_peer_disconnect_now(m_Server, 0);
	enet_host_destroy(m_Host);

	m_Host = NULL;
	m_Server = NULL;
}

void CNetClientSessionRemote::Poll()
{
	debug_assert(m_Host && m_Server);

	ENetEvent event;
	while (enet_host_service(m_Host, &event, 0) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
		{
			debug_assert(event.peer == m_Server);

			// Report the server address
			char hostname[256] = "(error)";
			enet_address_get_host_ip(&event.peer->address, hostname, ARRAY_SIZE(hostname));
			LOGMESSAGE(L"Net client: Connected to %hs:%u", hostname, event.peer->address.port);

			GetClient().HandleConnect();

			break;
		}

		case ENET_EVENT_TYPE_DISCONNECT:
		{
			debug_assert(event.peer == m_Server);

			GetClient().HandleDisconnect();

			break;
		}

		case ENET_EVENT_TYPE_RECEIVE:
		{
			CNetMessage* msg = CNetMessageFactory::CreateMessage(event.packet->data, event.packet->dataLength, GetClient().GetScriptInterface());
			if (msg)
			{
				LOGMESSAGE(L"Net client: Received message %hs of size %lu from server", msg->ToString().c_str(), (unsigned long)msg->GetSerializedLength());

				bool ok = GetClient().HandleMessage(msg);
				debug_assert(ok); // TODO

				delete msg;
			}

			enet_packet_destroy(event.packet);

			break;
		}
		}
	}

}

bool CNetClientSessionRemote::SendMessage(const CNetMessage* message)
{
	debug_assert(m_Host && m_Server);

	return CNetHost::SendMessage(message, m_Server, "server");
}



CNetClientSessionLocal::CNetClientSessionLocal(CNetClient& client, CNetServer& server) :
	CNetClientSession(client), m_Server(server), m_ServerSession(NULL)
{
	server.AddLocalClientSession(*this);
	client.HandleConnect();
}

void CNetClientSessionLocal::Poll()
{
	for (size_t i = 0; i < m_LocalMessageQueue.size(); ++i)
	{
		CNetMessage* msg = m_LocalMessageQueue[i];

		LOGMESSAGE(L"Net client: Received local message %hs of size %lu from server", msg->ToString().c_str(), (unsigned long)msg->GetSerializedLength());

		bool ok = GetClient().HandleMessage(msg);
		debug_assert(ok); // TODO

		delete msg;
	}
	m_LocalMessageQueue.clear();
}

void CNetClientSessionLocal::Disconnect()
{
	// TODO
}

bool CNetClientSessionLocal::SendMessage(const CNetMessage* message)
{
	LOGMESSAGE(L"Net client: Sending local message %hs to server", message->ToString().c_str());
	m_Server.SendLocalMessage(*this, message);
	return true;
}

void CNetClientSessionLocal::AddLocalMessage(const CNetMessage* message)
{
	// Clone into the client's script context
	CNetMessage* clonedMessage = CNetMessageFactory::CloneMessage(message, GetClient().GetScriptInterface());
	if (!clonedMessage)
		return;
	m_LocalMessageQueue.push_back(clonedMessage);
}



CNetServerSession::CNetServerSession(CNetServer& server) :
	m_Server(server)
{
}

CNetServerSession::~CNetServerSession()
{
}



CNetServerSessionRemote::CNetServerSessionRemote(CNetServer& server, ENetPeer* peer) :
	CNetServerSession(server), m_Peer(peer)
{

}

void CNetServerSessionRemote::Disconnect()
{
	// TODO
}

bool CNetServerSessionRemote::SendMessage(const CNetMessage* message)
{
	return GetServer().SendMessage(m_Peer, message);
}



CNetServerSessionLocal::CNetServerSessionLocal(CNetServer& server, CNetClientSessionLocal& clientSession) :
	CNetServerSession(server), m_ClientSession(clientSession)
{
}

void CNetServerSessionLocal::Disconnect()
{
	// TODO
}

bool CNetServerSessionLocal::SendMessage(const CNetMessage* message)
{
	LOGMESSAGE(L"Net server: Sending local message %hs to %p", message->ToString().c_str(), &m_ClientSession);
	m_ClientSession.AddLocalMessage(message);
	return true;
}
