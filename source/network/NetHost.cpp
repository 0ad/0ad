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

#include "lib/external_libraries/enet.h"
#include "network/NetMessage.h"
#include "ps/CLogger.h"

bool CNetHost::SendMessage(const CNetMessage* message, ENetPeer* peer, const char* peerName)
{
	ENetPacket* packet = CreatePacket(message);
	if (!packet)
		return false;

	LOGMESSAGE(L"Net: Sending message %hs of size %lu to %hs", message->ToString().c_str(), (unsigned long)packet->dataLength, peerName);

	// Let ENet send the message to peer
	if (enet_peer_send(peer, DEFAULT_CHANNEL, packet) < 0)
	{
		LOGERROR(L"Net: Failed to send packet to peer");
		return false;
	}

	// Don't call enet_host_flush now - let it queue up all the packets
	// and send them during the next frame
	//
	// TODO: we should flush explicitly at some appropriate point before the next frame

	return true;
}

ENetPacket* CNetHost::CreatePacket(const CNetMessage* message)
{
	size_t size = message->GetSerializedLength();

	ENSURE(size); // else we'll fail when accessing the 0th element

	// Adjust buffer for message
	std::vector<u8> buffer;
	buffer.resize(size);

	// Save message to internal buffer
	message->Serialize(&buffer[0]);

	// Create a reliable packet
	ENetPacket* packet = enet_packet_create(&buffer[0], size, ENET_PACKET_FLAG_RELIABLE);
	if (!packet)
		LOGERROR(L"Net: Failed to construct packet");

	return packet;
}

void CNetHost::Initialize()
{
	int ret = enet_initialize();
	ENSURE(ret == 0);
}

void CNetHost::Deinitialize()
{
	enet_deinitialize();
}
