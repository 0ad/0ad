#ifndef _AllNetMessages_H
#define _AllNetMessages_H

#include "types.h"
#include "CStr.h"

enum ENetMessageType
{
	/*
		All Message Types should be put here. Never change the order of this
		list.
		First, all negative types are only for internal/local use and may never
		be sent over the network.
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
	
	/* Handshake State */
	NMT_ServerHandshake,
	NMT_ClientHandshake,
	NMT_ServerHandshakeResponse,
	/* Common Messages, state 2-5 */
	NMT_Result,
	/* Authentication State */
	NMT_Authenticate,
	/* Common Messages, state 3-5 */
	NMT_Chat,
	/* Pre-Game State */
	NMT_PlayerConnect,
	NMT_SelectCiv,
	NMT_FilesRequired,
	NMT_FileRequest,
	NMT_FileChunk,
	NMT_FileChunkAck,
	NMT_FileProgress,
	/* In-Game State */
	NMT_StartCommandBatch,
	NMT_EndCommandBatch,
	/* Post-Game State */

	/* Game event messages */

	NMT_Event_IveGotAnInteger,

	/**
	 * One higher than the highest value of any message type
	 */
	NMT_LAST // Always put this last in the list
};

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
// 1.1.1.1
#define PS_PROTOCOL_VERSION 0x01010101

#endif // #ifndef _AllNetMessage_H

#ifdef CREATING_NMT

#define ALLNETMSGS_DONT_CREATE_NMTS
#define START_NMT_CLASS_(_nm) START_NMT_CLASS(C ## _nm, NMT_ ## _nm)

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
	NMT_FIELD(CStr, m_Message)
END_NMT_CLASS()

START_NMT_CLASS_(Authenticate)
	NMT_FIELD(CStr, m_Nick)
	//NMT_FIELD(CPasswordHash, m_Password)
	NMT_FIELD(CStr, m_Password)
END_NMT_CLASS()

START_NMT_CLASS_(Chat)
	NMT_FIELD_INT(m_Recipient, u32, 2)
	NMT_FIELD(CStr, m_Message)
END_NMT_CLASS()

START_NMT_CLASS_(PlayerConnect)
	NMT_START_ARRAY(m_Players)
		NMT_FIELD_INT(m_PlayerID, u32, 2)
		NMT_FIELD(CStr, m_Nick)
	NMT_END_ARRAY()
END_NMT_CLASS()

#include "../EventTypes.h"

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
