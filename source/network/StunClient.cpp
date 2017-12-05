/* Copyright (C) 2017 Wildfire Games.
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

#include <chrono>
#include <cstdio>

#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#ifdef WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <sys/socket.h>
#  include <netdb.h>
#endif

#include <vector>

#include "lib/external_libraries/enet.h"

#if OS_WIN
#include "lib/sysdep/os/win/wposix/wtime.h"
#endif

#include "scriptinterface/ScriptInterface.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"

unsigned int m_StunServerIP;
int m_StunServerPort;

/**
 * These constants are defined in Section 6 of RFC 5389.
 */
const u32 m_MagicCookie = 0x2112A442;
const u32 m_MethodTypeBinding = 0x0001;
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

/**
 * Discovered STUN endpoint
 */
u32 m_IP;
u16 m_Port;

void AddUInt16(std::vector<u8>& buffer, const u16 value)
{
	buffer.push_back((value >> 8) & 0xff);
	buffer.push_back(value & 0xff);
}

void AddUInt32(std::vector<u8>& buffer, const u32 value)
{
	buffer.push_back((value >> 24) & 0xff);
	buffer.push_back((value >> 16) & 0xff);
	buffer.push_back((value >>  8) & 0xff);
	buffer.push_back( value        & 0xff);
}

template<typename T, size_t n>
bool GetFromBuffer(const std::vector<u8>& buffer, u32& offset, T& result)
{
	if (offset + n > buffer.size())
		return false;

	int a = n;
	offset += n;
	while (a--)
	{
		// Prevent shift count overflow if the type is u8
		if (n > 1)
			result <<= 8;

		result += buffer[offset - 1 - a];
	}
	return true;
}

/**
 * Creates a STUN request and sends it to a STUN server.
 * The request is sent through transactionHost, from which the answer
 * will be retrieved by ReceiveStunResponse and interpreted by ParseStunResponse.
 */
bool CreateStunRequest(ENetHost* transactionHost)
{
	ENSURE(transactionHost);

	CStr server_name;
	CFG_GET_VAL("lobby.stun.server", server_name);
	CFG_GET_VAL("lobby.stun.port", m_StunServerPort);

	debug_printf("GetPublicAddress: Using STUN server %s:%d\n", server_name.c_str(), m_StunServerPort);

	addrinfo hints;
	addrinfo* res;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	hints.ai_socktype = SOCK_STREAM;

	// Resolve the stun server name so we can send it a STUN request
	int status = getaddrinfo(server_name.c_str(), nullptr, &hints, &res);
	if (status != 0)
	{
#ifdef UNICODE
		LOGERROR("GetPublicAddress: Error in getaddrinfo: %s", utf8_from_wstring(gai_strerror(status)));
#else
		LOGERROR("GetPublicAddress: Error in getaddrinfo: %s", gai_strerror(status));
#endif
		return false;
	}

	ENSURE(res);

	// Documentation says it points to "one or more addrinfo structures"
	sockaddr_in* current_interface = (sockaddr_in*)(res->ai_addr);
	m_StunServerIP = ntohl(current_interface->sin_addr.s_addr);

	StunClient::SendStunRequest(transactionHost, m_StunServerIP, m_StunServerPort);

	freeaddrinfo(res);
	return true;
}

void StunClient::SendStunRequest(ENetHost* transactionHost, u32 targetIp, u16 targetPort)
{
	std::vector<u8> buffer;
	AddUInt16(buffer, m_MethodTypeBinding);
	AddUInt16(buffer, 0); // length
	AddUInt32(buffer, m_MagicCookie);

	for (std::size_t i = 0; i < sizeof(m_TransactionID); ++i)
	{
		u8 random_byte = rand() % 256;
		buffer.push_back(random_byte);
		m_TransactionID[i] = random_byte;
	}

	sockaddr_in to;
	int to_len = sizeof(to);
	memset(&to, 0, to_len);

	to.sin_family = AF_INET;
	to.sin_port = htons(targetPort);
	to.sin_addr.s_addr = htonl(targetIp);

	sendto(transactionHost->socket, (char*)(buffer.data()), (int)buffer.size(), 0, (sockaddr*)&to, to_len);
}

/**
 * Gets the response from the STUN server and checks it for its validity.
 */
bool ReceiveStunResponse(ENetHost* transactionHost, std::vector<u8>& buffer)
{
	ENSURE(transactionHost);

	// TransportAddress sender;
	const int LEN = 2048;
	char input_buffer[LEN];

	memset(input_buffer, 0, LEN);

	sockaddr_in addr;
	socklen_t from_len = sizeof(addr);

	int len = recvfrom(transactionHost->socket, input_buffer, LEN, 0, (sockaddr*)(&addr), &from_len);

	int delay = 200;
	CFG_GET_VAL("lobby.stun.delay", delay);

	// Wait to receive the message because enet sockets are non-blocking
	const int max_tries = 5;
	for (int count = 0; len < 0 && (count < max_tries || max_tries == -1); ++count)
	{
		usleep(delay * 1000);
		len = recvfrom(transactionHost->socket, input_buffer, LEN, 0, (sockaddr*)(&addr), &from_len);
	}

	if (len < 0)
	{
		LOGERROR("GetPublicAddress: recvfrom error (%d): %s", errno, strerror(errno));
		return false;
	}

	u32 sender_ip = ntohl((u32)(addr.sin_addr.s_addr));
	u16 sender_port = ntohs(addr.sin_port);

	if (sender_ip != m_StunServerIP)
		LOGERROR("GetPublicAddress: Received stun response from different address: %d:%d (%d.%d.%d.%d:%d) %s",
			addr.sin_addr.s_addr,
			addr.sin_port,
			(sender_ip >> 24) & 0xff,
			(sender_ip >> 16) & 0xff,
			(sender_ip >>  8) & 0xff,
			(sender_ip >>  0) & 0xff,
			sender_port,
			input_buffer);

	// Convert to network string.
	buffer.resize(len);
	memcpy(buffer.data(), (u8*)input_buffer, len);

	return true;
}

bool ParseStunResponse(const std::vector<u8>& buffer)
{
	u32 offset = 0;

	u16 responseType = 0;
	if (!GetFromBuffer<u16, 2>(buffer, offset, responseType) || responseType != m_BindingSuccessResponse)
	{
		LOGERROR("STUN response isn't a binding success response");
		return false;
	}

	// Ignore message size
	offset += 2;

	u32 cookie = 0;
	if (!GetFromBuffer<u32, 4>(buffer, offset, cookie) || cookie != m_MagicCookie)
	{
		LOGERROR("STUN response doesn't contain the magic cookie");
		return false;
	}

	for (std::size_t i = 0; i < sizeof(m_TransactionID); ++i)
	{
		u8 transactionChar = 0;
		if (!GetFromBuffer<u8, 1>(buffer, offset, transactionChar) || transactionChar != m_TransactionID[i])
		{
			LOGERROR("STUN response doesn't contain the transaction ID");
			return false;
		}
	}

	while (offset < buffer.size())
	{
		u16 type = 0;
		u16 size = 0;
		if (!GetFromBuffer<u16, 2>(buffer, offset, type) ||
		    !GetFromBuffer<u16, 2>(buffer, offset, size))
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
			if (!GetFromBuffer<u8, 1>(buffer, offset, ipFamily) || ipFamily != m_IPAddressFamilyIPv4)
			{
				LOGERROR("Unsupported address family, IPv4 is expected");
				return false;
			}

			u16 port = 0;
			u32 ip = 0;
			if (!GetFromBuffer<u16, 2>(buffer, offset, port) ||
			    !GetFromBuffer<u32, 4>(buffer, offset, ip))
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

			m_Port = port;
			m_IP = ip;

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

bool STUNRequestAndResponse(ENetHost* transactionHost)
{
	if (!CreateStunRequest(transactionHost))
		return false;

	std::vector<u8> buffer;
	return ReceiveStunResponse(transactionHost, buffer) &&
	       ParseStunResponse(buffer);
}

JS::Value StunClient::FindStunEndpointHost(const ScriptInterface& scriptInterface, int port)
{
	ENetAddress hostAddr{ENET_HOST_ANY, (u16)port};
	ENetHost* transactionHost = enet_host_create(&hostAddr, 1, 1, 0, 0);
	if (!transactionHost)
	{
		LOGERROR("FindStunEndpointHost: Failed to create enet host");
		return JS::UndefinedValue();
	}

	bool success = STUNRequestAndResponse(transactionHost);
	enet_host_destroy(transactionHost);
	if (!success)
		return JS::UndefinedValue();

	// Convert m_IP to string
	char ipStr[256] = "(error)";
	ENetAddress addr;
	addr.host = ntohl(m_IP);
	enet_address_get_host_ip(&addr, ipStr, ARRAY_SIZE(ipStr));

	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	JS::RootedValue stunEndpoint(cx);
	scriptInterface.Eval("({})", &stunEndpoint);
	scriptInterface.SetProperty(stunEndpoint, "ip", CStr(ipStr));
	scriptInterface.SetProperty(stunEndpoint, "port", m_Port);
	return stunEndpoint;
}

StunClient::StunEndpoint* StunClient::FindStunEndpointJoin(ENetHost* transactionHost)
{
	ENSURE(transactionHost);

	if (!STUNRequestAndResponse(transactionHost))
		return nullptr;

	// Convert m_IP to string
	char ipStr[256] = "(error)";
	ENetAddress addr;
	addr.host = ntohl(m_IP);
	enet_address_get_host_ip(&addr, ipStr, ARRAY_SIZE(ipStr));

	return new StunEndpoint({ m_IP, m_Port });
}

void StunClient::SendHolePunchingMessages(ENetHost* enetClient, const char* serverAddress, u16 serverPort)
{
	// Convert ip string to int64
	ENetAddress addr;
	addr.port = serverPort;
	enet_address_set_host(&addr, serverAddress);

	int delay = 200;
	CFG_GET_VAL("lobby.stun.delay", delay);

	// Send an UDP message from enet host to ip:port
	for (int i = 0; i < 3; ++i)
	{
		StunClient::SendStunRequest(enetClient, htonl(addr.host), serverPort);
		usleep(delay * 1000);
	}
}
