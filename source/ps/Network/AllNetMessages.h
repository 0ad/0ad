#ifndef _AllNetMessages_H
#define _AllNetMessages_H

#include "lib/types.h"
#include "CStr.h"
#include "scripting/JSSerialization.h"

enum ENetMessageType
{
	/*
		All Message Types should be put here. Never change the order of this
		list.
		First, all negative types are only for internal/local use and may never
		be sent over the network.
		After that, all "real" network messages are listed, giving them a value
		from 0 and upwards (*unique* value)
	*/
	/**
	 * A special message that contains a PS_RESULT code, used for delivery of
	 * OOB error status messages from a CMessageSocket
	 */
	NMT_ERROR=-256,
	/**
	 * The message socket connect attempt is complete
	 */
	NMT_CONNECT_COMPLETE,
	/**
	 * Close the message socket
	 */
	NMT_CLOSE_REQUEST,
	/**
	 * An invalid message type, representing an uninitialized message.
	 */
	NMT_NONE=0,
	
	/* Handshake Stage */
	NMT_ServerHandshake,
	NMT_ClientHandshake,
	NMT_ServerHandshakeResponse,
	/* Authentication Stage */
	NMT_Authenticate,
	NMT_AuthenticationResult,
	/* Common Messages, stage 3-5 */
	NMT_ChatMessage,
	/* Pre-Game Stage */
	NMT_ClientConnect,
	NMT_ClientDisconnect,
	NMT_SetGameConfig,
	NMT_AssignPlayerSlot,
	NMT_SetPlayerConfig,
	NMT_FilesRequired,
	NMT_FileRequest,
	NMT_FileChunk,
	NMT_FileChunkAck,
	NMT_FileProgress,
	NMT_StartGame,
	/* In-Game Stage */
	NMT_EndCommandBatch,
	NMT_Goto, NMT_COMMAND_FIRST=NMT_Goto,
	NMT_Patrol,
	NMT_AddWaypoint,
	NMT_AttackMelee,
	NMT_Gather,
	NMT_Heal,
	NMT_Generic,
	NMT_Run,
	NMT_COMMAND_LAST,
	/* Post-Game Stage */

	/**
	 * One higher than the highest value of any message type
	 */
	NMT_LAST // Always put this last in the list
};

/**
	Only applies to NMT_AuthenticationResult (as of now).
*/
enum ENetResultCodes
{
	NRC_OK,
	NRC_PasswordInvalid,
	NRC_NickTaken,
	NRC_NickInvalid,
};

// These constants (magic numbers) are highly arbitrary, but have been chosen
// such that they should be unlikely to stumble across randomly.

// in network byte order: 'P', 's', 0x01, '?'
#define PS_PROTOCOL_MAGIC 0x5073013f
// in network byte order: 'P', 'c', 0x01, '!'
#define PS_PROTOCOL_MAGIC_RESPONSE 0x50630121

// At a later date, there should be a standard for assigning protocol version
// numbers. For now, this also highly arbitrary number will hold its place
// 1.1.2
#define PS_PROTOCOL_VERSION 0x01010002

// 0x5073 = decimal 20595
// The "symbolism" is that the two bytes of the port number in network byte
// order is 'P' 's'
#define PS_DEFAULT_PORT 0x5073

// Chat Recipient Constants
enum
{
	// Any chat recipients lower than this one is an actual session ID
	PS_CHAT_RCP_FIRST_SPECIAL=0xfffd,
	PS_CHAT_RCP_ENEMIES=0xfffd,
	PS_CHAT_RCP_ALLIES=0xfffe,
	PS_CHAT_RCP_ALL=0xffff,
};

enum
{
	PS_ASSIGN_OPEN,
	PS_ASSIGN_CLOSED,
	PS_ASSIGN_AI,
	PS_ASSIGN_SESSION
};

#endif // #ifndef _AllNetMessage_H

#ifdef CREATING_NMT

#define ALLNETMSGS_DONT_CREATE_NMTS
#define START_NMT_CLASS_(_nm) START_NMT_CLASS(C ## _nm, NMT_ ## _nm)
#define DERIVE_NMT_CLASS_(_base, _nm) START_NMT_CLASS_DERIVED(C ## _base, C ## _nm, NMT_ ## _nm)

START_NMTS()

START_NMT_CLASS_(ServerHandshake)
	NMT_FIELD_INT(m_Magic, u32, 4)
	NMT_FIELD_INT(m_ProtocolVersion, u32, 4)
	NMT_FIELD_INT(m_SoftwareVersion, u32, 4)
END_NMT_CLASS()

START_NMT_CLASS_(ClientHandshake)
	NMT_FIELD_INT(m_MagicResponse, u32, 4)
	NMT_FIELD_INT(m_ProtocolVersion, u32, 4)
	NMT_FIELD_INT(m_SoftwareVersion, u32, 4)
END_NMT_CLASS()

START_NMT_CLASS_(ServerHandshakeResponse)
	NMT_FIELD_INT(m_UseProtocolVersion, u32, 4)
	NMT_FIELD_INT(m_Flags, u32, 4)
	NMT_FIELD(CStrW, m_Message)
END_NMT_CLASS()

START_NMT_CLASS_(AuthenticationResult)
	NMT_FIELD_INT(m_Code, u32, 4)
	NMT_FIELD_INT(m_SessionID, u32, 2)
	NMT_FIELD(CStrW, m_Message)
END_NMT_CLASS()

START_NMT_CLASS_(Authenticate)
	NMT_FIELD(CStrW, m_Name)
	//NMT_FIELD(CPasswordHash, m_Password)
	NMT_FIELD(CStrW, m_Password)
END_NMT_CLASS()

START_NMT_CLASS_(ChatMessage)
	NMT_FIELD(CStrW, m_Sender)
	NMT_FIELD_INT(m_Recipient, u32, 2)
	NMT_FIELD(CStrW, m_Message)
END_NMT_CLASS()

START_NMT_CLASS_(ClientConnect)
	NMT_START_ARRAY(m_Clients)
		NMT_FIELD_INT(m_SessionID, u32, 2)
		NMT_FIELD(CStrW, m_Name)
	NMT_END_ARRAY()
END_NMT_CLASS()

START_NMT_CLASS_(ClientDisconnect)
	NMT_FIELD_INT(m_SessionID, u32, 2)
END_NMT_CLASS()

START_NMT_CLASS_(SetGameConfig)
	NMT_START_ARRAY(m_Values)
		NMT_FIELD(CStrW, m_Name)
		NMT_FIELD(CStrW, m_Value)
	NMT_END_ARRAY()
END_NMT_CLASS()

START_NMT_CLASS_(AssignPlayerSlot)
	NMT_FIELD_INT(m_SlotID, u32, 2)
	NMT_FIELD_INT(m_Assignment, u32, 1)
	NMT_FIELD_INT(m_SessionID, u32, 2) // Only applicable for PS_ASSIGN_SESSION
END_NMT_CLASS()

START_NMT_CLASS_(SetPlayerConfig)
	NMT_FIELD_INT(m_PlayerID, u32, 2)
	NMT_START_ARRAY(m_Values)
		NMT_FIELD(CStrW, m_Name)
		NMT_FIELD(CStrW, m_Value)
	NMT_END_ARRAY()
END_NMT_CLASS()

START_NMT_CLASS_(StartGame)
END_NMT_CLASS()

START_NMT_CLASS_(EndCommandBatch)
	NMT_FIELD_INT(m_TurnLength, u32, 2)
END_NMT_CLASS()

START_NMT_CLASS(CNetCommand, NMT_NONE)
	NMT_FIELD(CEntityList, m_Entities)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(NetCommand, Goto)
	NMT_FIELD_INT(m_TargetX, u32, 2)
	NMT_FIELD_INT(m_TargetY, u32, 2)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(NetCommand, Run)
	NMT_FIELD_INT(m_TargetX, u32, 2)
	NMT_FIELD_INT(m_TargetY, u32, 2)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(NetCommand, Patrol)
	NMT_FIELD_INT(m_TargetX, u32, 2)
	NMT_FIELD_INT(m_TargetY, u32, 2)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(NetCommand, AddWaypoint)
	NMT_FIELD_INT(m_TargetX, u32, 2)
	NMT_FIELD_INT(m_TargetY, u32, 2)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(NetCommand, AttackMelee)
	NMT_FIELD(HEntity, m_Target)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(NetCommand, Gather)
	NMT_FIELD(HEntity, m_Target)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(NetCommand, Heal)
	NMT_FIELD(HEntity, m_Target)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(NetCommand, Generic)
	NMT_FIELD(HEntity, m_Target)
	NMT_FIELD_INT(m_Action, u32, 4)
END_NMT_CLASS()

END_NMTS()

#else
#ifndef ALLNETMSGS_DONT_CREATE_NMTS

# ifdef ALLNETMSGS_IMPLEMENT
#  define NMT_CREATOR_IMPLEMENT
# endif

# define NMT_CREATE_HEADER_NAME "AllNetMessages.h"
# include "NMTCreator.h"

#endif // #ifndef ALLNETMSGS_DONT_CREATE_NMTS
#endif // #ifdef CREATING_NMT
