/* Copyright (C) 2021 Wildfire Games.
 * Copyright (C) 2013-2016 SuperTuxKart-Team.
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

#include "StunClient.h"

#include "lib/byte_order.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/CStr.h"

#include "lib/external_libraries/enet.h"

#include <chrono>
#include <vector>
#include <thread>

namespace StunClient
{

/**
 * These constants are defined in Section 6 of RFC 5389.
 */
const u32 m_MagicCookie = 0x2112A442;
const u16 m_MethodTypeBinding = 0x01;
const u32 m_BindingSuccessResponse = 0x0101;

/**
 * Bit determining whether comprehension of an attribute is optional.
 * Described in Section 15 of RFC 5389.
 */
const u16 m_ComprehensionOptional = 0x1 << 15;

/**
 * Bit determining whether the bit was assigned by IETF Review.
 * Described in section 18.1. of  RFC 5389.
 */
const u16 m_IETFReview = 0x1 << 14;

/**
 * These constants are defined in Section 15.1 of RFC 5389.
 */
const u8 m_IPAddressFamilyIPv4 = 0x01;

/**
 * These constants are defined in Section 18.2 of RFC 5389.
 */
const u16 m_AttrTypeMappedAddress = 0x001;
const u16 m_AttrTypeXORMappedAddress = 0x0020;

/**
 * Described in section 3 of RFC 5389.
 */
u8 m_TransactionID[12];

ENetAddress m_StunServer;

/**
 * Public IP + port discovered via the STUN transaction.
 */
ENetAddress m_PublicAddress;

/**
 * Push POD data to a network-byte-order buffer.
 * TODO: this should be optimised & moved to byte_order.h
 */
template<typename T, size_t n = sizeof(T)>
void AddToBuffer(std::vector<u8>& buffer, const T value)
{
	static_assert(std::is_pod_v<T>, "T must be POD");
	buffer.reserve(buffer.size() + n);
	// std::byte* can alias anything so this is legal.
	const std::byte* ptr = reinterpret_cast<const std::byte*>(&value);
	for (size_t a = 0; a < n; ++a)
#if BYTE_ORDER == LITTLE_ENDIAN
		buffer.push_back(static_cast<u8>(*(ptr + n - 1 - a)));
#else
		buffer.push_back(static_cast<u8>(*(ptr + a)));
#endif
}

/**
 * Read POD data from a network-byte-order buffer.
 * TODO: this should be optimised & moved to byte_order.h
 */
template<typename T, size_t n = sizeof(T)>
bool GetFromBuffer(const std::vector<u8>& buffer, u32& offset, T& result)
{
	static_assert(std::is_pod_v<T>, "T must be POD");
	if (offset + n > buffer.size())
		return false;

	// std::byte* can alias anything so this is legal.
	std::byte* ptr = reinterpret_cast<std::byte*>(&result);
	for (size_t a = 0; a < n; ++a)
#if BYTE_ORDER == LITTLE_ENDIAN
		*ptr++ = static_cast<std::byte>(buffer[offset + n - 1 - a]);
#else
		*ptr++ = static_cast<std::byte>(buffer[offset + a]);
#endif

	offset += n;
	return true;
}

void SendStunRequest(ENetHost& transactionHost, ENetAddress addr)
{
	std::vector<u8> buffer;
	AddToBuffer<u16>(buffer, m_MethodTypeBinding);
	AddToBuffer<u16>(buffer, 0); // length
	AddToBuffer<u32>(buffer, m_MagicCookie);

	for (std::size_t i = 0; i < sizeof(m_TransactionID); ++i)
	{
		u8 random_byte = rand() % 256;
		buffer.push_back(random_byte);
		m_TransactionID[i] = random_byte;
	}

	ENetBuffer enetBuffer;
	enetBuffer.data = buffer.data();
	enetBuffer.dataLength = buffer.size();
	enet_socket_send(transactionHost.socket, &addr, &enetBuffer, 1);
}

/**
 * Creates a STUN request and sends it to a STUN server.
 * The request is sent through transactionHost, from which the answer
 * will be retrieved by ReceiveStunResponse and interpreted by ParseStunResponse.
 */
bool CreateStunRequest(ENetHost& transactionHost)
{
	CStr server_name;
	int port;
	CFG_GET_VAL("lobby.stun.server", server_name);
	CFG_GET_VAL("lobby.stun.port", port);

	LOGMESSAGE("StunClient: Using STUN server %s:%d\n", server_name.c_str(), port);

	ENetAddress addr;
	addr.port = port;
	if (enet_address_set_host(&addr, server_name.c_str()) == -1)
		return false;

	m_StunServer = addr;

	StunClient::SendStunRequest(transactionHost, addr);

	return true;
}

/**
 * Gets the response from the STUN server and checks it for its validity.
 */
bool ReceiveStunResponse(ENetHost& transactionHost, std::vector<u8>& buffer)
{
	// TransportAddress sender;
	const int LEN = 2048;
	char input_buffer[LEN];

	memset(input_buffer, 0, LEN);

	ENetBuffer enetBuffer;
	enetBuffer.data = input_buffer;
	enetBuffer.dataLength = LEN;

	ENetAddress sender = m_StunServer;
	int len = enet_socket_receive(transactionHost.socket, &sender, &enetBuffer, 1);

	int delay = 200;
	CFG_GET_VAL("lobby.stun.delay", delay);

	// Wait to receive the message because enet sockets are non-blocking
	const int max_tries = 5;
	for (int count = 0; len <= 0 && (count < max_tries || max_tries == -1); ++count)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
		len = enet_socket_receive(transactionHost.socket, &sender, &enetBuffer, 1);
	}

	if (len <= 0)
	{
		LOGERROR("ReceiveStunResponse: recvfrom error (%d): %s", errno, strerror(errno));
		return false;
	}

	if (memcmp(&sender, &m_StunServer, sizeof(m_StunServer)) != 0)
		LOGERROR("ReceiveStunResponse: Received stun response from different address: %d.%d.%d.%d:%d %s",
			(sender.host >> 24) & 0xff,
			(sender.host >> 16) & 0xff,
			(sender.host >>  8) & 0xff,
			(sender.host >>  0) & 0xff,
			sender.port,
			input_buffer);

	// Convert to network string.
	buffer.resize(len);
	memcpy(buffer.data(), reinterpret_cast<u8*>(input_buffer), len);

	return true;
}

bool ParseStunResponse(const std::vector<u8>& buffer)
{
	u32 offset = 0;

	u16 responseType = 0;
	if (!GetFromBuffer(buffer, offset, responseType) || responseType != m_BindingSuccessResponse)
	{
		LOGERROR("STUN response isn't a binding success response");
		return false;
	}

	// Ignore message size
	offset += 2;

	u32 cookie = 0;
	if (!GetFromBuffer(buffer, offset, cookie) || cookie != m_MagicCookie)
	{
		LOGERROR("STUN response doesn't contain the magic cookie");
		return false;
	}

	for (std::size_t i = 0; i < sizeof(m_TransactionID); ++i)
	{
		u8 transactionChar = 0;
		if (!GetFromBuffer(buffer, offset, transactionChar) || transactionChar != m_TransactionID[i])
		{
			LOGERROR("STUN response doesn't contain the transaction ID");
			return false;
		}
	}

	while (offset < buffer.size())
	{
		u16 type = 0;
		u16 size = 0;
		if (!GetFromBuffer(buffer, offset, type) ||
		    !GetFromBuffer(buffer, offset, size))
		{
			LOGERROR("STUN response contains invalid attribute");
			return false;
		}

		// The first two bits are irrelevant to the type
		type &= ~(m_ComprehensionOptional | m_IETFReview);

		switch (type)
		{
		case m_AttrTypeMappedAddress:
		case m_AttrTypeXORMappedAddress:
		{
			if (size != 8)
			{
				LOGERROR("Invalid STUN Mapped Address length");
				return false;
			}

			// Ignore the first byte as mentioned in Section 15.1 of RFC 5389.
			++offset;

			u8 ipFamily = 0;
			if (!GetFromBuffer(buffer, offset, ipFamily) || ipFamily != m_IPAddressFamilyIPv4)
			{
				LOGERROR("Unsupported address family, IPv4 is expected");
				return false;
			}

			u16 port = 0;
			u32 ip = 0;
			if (!GetFromBuffer(buffer, offset, port) ||
			    !GetFromBuffer(buffer, offset, ip))
			{
				LOGERROR("Mapped address doesn't contain IP and port");
				return false;
			}

			// Obfuscation is described in Section 15.2 of RFC 5389.
			if (type == m_AttrTypeXORMappedAddress)
			{
				port ^= m_MagicCookie >> 16;
				ip ^= m_MagicCookie;
			}

			// ENetAddress takes a host byte-order port and network byte-order IP.
			// Network byte order is big endian, so convert appropriately.
			m_PublicAddress.host = to_be32(ip);
			m_PublicAddress.port = port;

			break;
		}
		default:
		{
			// We don't care about other attributes at all

			// Skip attribute
			offset += size;

			// Skip padding
			int padding = size % 4;
			if (padding)
				offset += 4 - padding;
			break;
		}
		}
	}

	return true;
}

bool STUNRequestAndResponse(ENetHost& transactionHost)
{
	if (!CreateStunRequest(transactionHost))
		return false;

	std::vector<u8> buffer;
	return ReceiveStunResponse(transactionHost, buffer) &&
	       ParseStunResponse(buffer);
}

bool FindPublicIP(ENetHost& transactionHost, CStr& ip, u16& port)
{
	if (!STUNRequestAndResponse(transactionHost))
		return false;

	// Convert m_IP to string
	char ipStr[256] = "(error)";
	enet_address_get_host_ip(&m_PublicAddress, ipStr, ARRAY_SIZE(ipStr));

	ip = ipStr;
	port = m_PublicAddress.port;

	LOGMESSAGE("StunClient: external IP address is %s:%i", ip.c_str(), port);

	return true;
}

void SendHolePunchingMessages(ENetHost& enetClient, const std::string& serverAddress, u16 serverPort)
{
	// Convert ip string to int64
	ENetAddress addr;
	addr.port = serverPort;
	enet_address_set_host(&addr, serverAddress.c_str());

	int delay = 200;
	CFG_GET_VAL("lobby.stun.delay", delay);

	// Send an UDP message from enet host to ip:port
	for (int i = 0; i < 3; ++i)
	{
		SendStunRequest(enetClient, addr);
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
	}
}

bool FindLocalIP(CStr& ip)
{
	// Open an UDP socket.
	ENetSocket socket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);

	ENetAddress addr;
	addr.port = 9; // Use the debug port (which we pick does not matter).
	// Connect to a random address. It does not need to be valid, only to not be the loopback address.
	if (enet_address_set_host(&addr, "100.0.100.0") == -1)
		return false;

	// Connect the socket. Being UDP, there is no actual outgoing traffic, this just binds it
	// to a valid port locally, allowing us to get the local IP of the machine.
	if (enet_socket_connect(socket, &addr) == -1)
		return false;

	// Fetch the local port & IP.
	if (enet_socket_get_address(socket, &addr) == -1)
		return false;

	enet_socket_destroy(socket);

	// Convert to a human readable string.
	char buf[50];
	if (enet_address_get_host_ip(&addr, buf, ARRAY_SIZE(buf)) == -1)
		return false;

	ip = buf;

	return true;
}
}
