/* Copyright (C) 2011 Wildfire Games.
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

/**
 * @file
 * The list of messages used by the network subsystem.
 */

#ifndef NETMESSAGES_H
#define NETMESSAGES_H

#include "ps/CStr.h"
#include "scriptinterface/ScriptVal.h"

#define PS_PROTOCOL_MAGIC				0x5073013f		// 'P', 's', 0x01, '?'
#define PS_PROTOCOL_MAGIC_RESPONSE		0x50630121		// 'P', 'c', 0x01, '!'
#define PS_PROTOCOL_VERSION				0x01010005		// Arbitrary protocol
#define PS_DEFAULT_PORT					0x5073			// 'P', 's'

// Defines the list of message types. The order of the list must not change.
// The message types having a negative value are used internally and not sent
// over the network. The message types used for network communication have 
// positive values.
enum NetMessageType
{
	NMT_CONNECT_COMPLETE = -256,	// Connection is complete
	NMT_CONNECTION_LOST,
	NMT_INVALID = 0,		// Invalid message
	NMT_SERVER_HANDSHAKE,	// Handshake stage
	NMT_CLIENT_HANDSHAKE,
	NMT_SERVER_HANDSHAKE_RESPONSE,
	NMT_AUTHENTICATE,		// Authentication stage
	NMT_AUTHENTICATE_RESULT,
	NMT_CHAT,		// Common chat message
	NMT_READY,
	NMT_GAME_SETUP,
	NMT_PLAYER_ASSIGNMENT,

	NMT_FILE_TRANSFER_REQUEST,
	NMT_FILE_TRANSFER_RESPONSE,
	NMT_FILE_TRANSFER_DATA,
	NMT_FILE_TRANSFER_ACK,

	NMT_JOIN_SYNC_START,

	NMT_LOADED_GAME,
	NMT_GAME_START,
	NMT_END_COMMAND_BATCH,
	NMT_SYNC_CHECK,	// OOS-detection hash checking
	NMT_SYNC_ERROR,	// OOS-detection error
	NMT_SIMULATION_COMMAND,
	NMT_LAST				// Last message in the list
};

// Authentication result codes
enum AuthenticateResultCode
{
	ARC_OK,
	ARC_OK_REJOINING,
	ARC_PASSWORD_INVALID,
};

#endif //	NETMESSAGES_H

#ifdef CREATING_NMT

#define ALLNETMSGS_DONT_CREATE_NMTS
#define START_NMT_CLASS_(_nm, _message) START_NMT_CLASS(C##_nm##Message, _message)
#define DERIVE_NMT_CLASS_(_base, _nm, _message) START_NMT_CLASS_DERIVED(C ## _base ## Message, C ## _nm ## Message, _message)

START_NMTS()

START_NMT_CLASS_(SrvHandshake, NMT_SERVER_HANDSHAKE)
	NMT_FIELD_INT(m_Magic, u32, 4)
	NMT_FIELD_INT(m_ProtocolVersion, u32, 4)
	NMT_FIELD_INT(m_SoftwareVersion, u32, 4)
END_NMT_CLASS()

START_NMT_CLASS_(CliHandshake, NMT_CLIENT_HANDSHAKE)
	NMT_FIELD_INT(m_MagicResponse, u32, 4)
	NMT_FIELD_INT(m_ProtocolVersion, u32, 4)
	NMT_FIELD_INT(m_SoftwareVersion, u32, 4)
END_NMT_CLASS()

START_NMT_CLASS_(SrvHandshakeResponse, NMT_SERVER_HANDSHAKE_RESPONSE)
	NMT_FIELD_INT(m_UseProtocolVersion, u32, 4)
	NMT_FIELD_INT(m_Flags, u32, 4)
	NMT_FIELD(CStrW, m_Message)
END_NMT_CLASS()

START_NMT_CLASS_(Authenticate, NMT_AUTHENTICATE)
	NMT_FIELD(CStr8, m_GUID)
	NMT_FIELD(CStrW, m_Name)
	NMT_FIELD(CStrW, m_Password)
END_NMT_CLASS()

START_NMT_CLASS_(AuthenticateResult, NMT_AUTHENTICATE_RESULT)
	NMT_FIELD_INT(m_Code, u32, 4)
	NMT_FIELD_INT(m_HostID, u32, 2)
	NMT_FIELD(CStrW, m_Message)
END_NMT_CLASS()

START_NMT_CLASS_(Chat, NMT_CHAT)
	NMT_FIELD(CStr8, m_GUID) // ignored when client->server, valid when server->client
	NMT_FIELD(CStrW, m_Message)
END_NMT_CLASS()

START_NMT_CLASS_(Ready, NMT_READY)
	NMT_FIELD(CStr8, m_GUID)
	NMT_FIELD_INT(m_Status, u8, 1)
END_NMT_CLASS()

START_NMT_CLASS_(PlayerAssignment, NMT_PLAYER_ASSIGNMENT)
	NMT_START_ARRAY(m_Hosts)
		NMT_FIELD(CStr8, m_GUID)
		NMT_FIELD(CStrW, m_Name)
		NMT_FIELD_INT(m_PlayerID, i8, 1)
		NMT_FIELD_INT(m_Status, u8, 1)
	NMT_END_ARRAY()
END_NMT_CLASS()

START_NMT_CLASS_(FileTransferRequest, NMT_FILE_TRANSFER_REQUEST)
	NMT_FIELD_INT(m_RequestID, u32, 4)
END_NMT_CLASS()

START_NMT_CLASS_(FileTransferResponse, NMT_FILE_TRANSFER_RESPONSE)
	NMT_FIELD_INT(m_RequestID, u32, 4)
	NMT_FIELD_INT(m_Length, u32, 4)
END_NMT_CLASS()

START_NMT_CLASS_(FileTransferData, NMT_FILE_TRANSFER_DATA)
	NMT_FIELD_INT(m_RequestID, u32, 4)
	NMT_FIELD(CStr8, m_Data)
END_NMT_CLASS()

START_NMT_CLASS_(FileTransferAck, NMT_FILE_TRANSFER_ACK)
	NMT_FIELD_INT(m_RequestID, u32, 4)
	NMT_FIELD_INT(m_NumPackets, u32, 4)
END_NMT_CLASS()

START_NMT_CLASS_(JoinSyncStart, NMT_JOIN_SYNC_START)
END_NMT_CLASS()

START_NMT_CLASS_(LoadedGame, NMT_LOADED_GAME)
	NMT_FIELD_INT(m_CurrentTurn, u32, 4)
END_NMT_CLASS()

START_NMT_CLASS_(GameStart, NMT_GAME_START)
END_NMT_CLASS()

START_NMT_CLASS_(EndCommandBatch, NMT_END_COMMAND_BATCH)
	NMT_FIELD_INT(m_Turn, u32, 4)
	NMT_FIELD_INT(m_TurnLength, u32, 2)
END_NMT_CLASS()

START_NMT_CLASS_(SyncCheck, NMT_SYNC_CHECK)
	NMT_FIELD_INT(m_Turn, u32, 4)
	NMT_FIELD(CStr, m_Hash)
END_NMT_CLASS()

START_NMT_CLASS_(SyncError, NMT_SYNC_ERROR)
	NMT_FIELD_INT(m_Turn, u32, 4)
	NMT_FIELD(CStr, m_HashExpected)
END_NMT_CLASS()

END_NMTS()

#else
#ifndef ALLNETMSGS_DONT_CREATE_NMTS

# ifdef ALLNETMSGS_IMPLEMENT
#  define NMT_CREATOR_IMPLEMENT
# endif

# define NMT_CREATE_HEADER_NAME "NetMessages.h"
# include "NMTCreator.h"

#endif // #ifndef ALLNETMSGS_DONT_CREATE_NMTS
#endif // #ifdef CREATING_NMT
