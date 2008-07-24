/**
 *-----------------------------------------------------------------------------
 *	FILE			: NetMessages.h
 *	PROJECT			: 0 A.D.
 *	DESCRIPTION		: The list of messages used by the network subsystem
 *-----------------------------------------------------------------------------
 */

#ifndef NETMESSAGES_H
#define NETMESSAGES_H

// INCLUDES
#include "ps/CStr.h"
#include "scripting/JSSerialization.h"
#include "simulation/EntityHandles.h"

// DEFINES
#define PS_PROTOCOL_MAGIC				0x5073013f		// 'P', 's', 0x01, '?'
#define PS_PROTOCOL_MAGIC_RESPONSE		0x50630121		// 'P', 'c', 0x01, '!'
#define PS_PROTOCOL_VERSION				0x01010002		// Arbitrary protocol
#define PS_DEFAULT_PORT					0x5073			// 'P', 's'

// Defines the list of message types. The order of the list must not change
// The message types having a negative value are used internally and not sent
// over the network. The message types used for network communication have 
// positive values.
enum NetMessageType
{
	NMT_ERROR = -256,		// Delivery of error states
	NMT_CONNECT_COMPLETE,	// Connection is complete
	NMT_CLOSE_REQUEST,		// Close connection request
	NMT_INVALID = 0,		// Invalid message
	NMT_SERVER_HANDSHAKE,	// Handshake stage
	NMT_CLIENT_HANDSHAKE,
	NMT_SERVER_HANDSHAKE_RESPONSE,
	NMT_AUTHENTICATE,		// Authentication stage
	NMT_AUTHENTICATE_RESULT,
	NMT_CHAT,		// Common chat message
	NMT_PLAYER_JOIN,		// Pre-game stage
	NMT_PLAYER_LEAVE,
	NMT_GAME_SETUP,
	NMT_ASSIGN_PLAYER_SLOT,
	NMT_PLAYER_CONFIG,
	NMT_FILES_REQUIRED,
	NMT_FILE_REQUEST,
	NMT_FILE_CHUNK,
	NMT_FILE_CHUNK_ACK,
	NMT_FILE_PROGRESS,
	NMT_GAME_START,
	NMT_END_COMMAND_BATCH,	// In-game stage
	NMT_GOTO,
	NMT_COMMAND_FIRST = NMT_GOTO,
	NMT_PATROL,
	NMT_ADD_WAYPOINT,
	NMT_GENERIC,
	NMT_PRODUCE,
	NMT_PLACE_OBJECT,
	NMT_RUN,
	NMT_SET_RALLY_POINT,
	NMT_SET_STANCE,
	NMT_NOTIFY_REQUEST,
	NMT_FORMATION_GOTO,
	NMT_FORMATION_GENERIC,
	NMT_COMMAND_LAST,
	NMT_LAST				// Last message in the list
};

// Authentication result codes
enum AuthenticateResultCode
{
	ARC_OK,
	ARC_PASSWORD_INVALID,
	ARC_NICK_TAKEN,
	ARC_NICK_INVALID,
};

enum
{
	CHAT_RECIPIENT_FIRST		=	0xFFFD,
	CHAT_RECIPIENT_ENEMIES		=	0xFFFD,
	CHAT_RECIPIENT_ALLIES		=	0xFFFE,
	CHAT_RECIPIENT_ALL			=	0xFFFF
};

enum
{
	ASSIGN_OPEN,
	ASSIGN_CLOSED,
	ASSIGN_AI,
	ASSIGN_SESSION
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

START_NMT_CLASS_(CliHandshake,NMT_CLIENT_HANDSHAKE)
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
	NMT_FIELD(CStrW, m_Name)
	//NMT_FIELD(CPasswordHash, m_Password)
	NMT_FIELD(CStrW, m_Password)
END_NMT_CLASS()

START_NMT_CLASS_(AuthenticateResult, NMT_AUTHENTICATE_RESULT)
	NMT_FIELD_INT(m_Code, u32, 4)
	NMT_FIELD_INT(m_SessionID, u32, 2)
	NMT_FIELD(CStrW, m_Message)
END_NMT_CLASS()

START_NMT_CLASS_(Chat, NMT_CHAT)
	NMT_FIELD(CStrW, m_Sender)
	NMT_FIELD_INT(m_Recipient, u32, 2)
	NMT_FIELD(CStrW, m_Message)
END_NMT_CLASS()

START_NMT_CLASS_(PlayerJoin, NMT_PLAYER_JOIN)
	NMT_START_ARRAY(m_Clients)
		NMT_FIELD_INT(m_SessionID, u32, 2)
		NMT_FIELD(CStr, m_Name)
	NMT_END_ARRAY()
END_NMT_CLASS()

START_NMT_CLASS_(PlayerLeave, NMT_PLAYER_LEAVE)
	NMT_FIELD_INT(m_SessionID, u32, 2)
END_NMT_CLASS()

START_NMT_CLASS_(GameSetup, NMT_GAME_SETUP)
	NMT_START_ARRAY(m_Values)
		NMT_FIELD(CStrW, m_Name)
		NMT_FIELD(CStrW, m_Value)
	NMT_END_ARRAY()
END_NMT_CLASS()

START_NMT_CLASS_(AssignPlayerSlot, NMT_ASSIGN_PLAYER_SLOT)
	NMT_FIELD_INT(m_SlotID, u32, 2)
	NMT_FIELD_INT(m_Assignment, u32, 1)
	NMT_FIELD_INT(m_SessionID, u32, 2) // Only applicable for PS_ASSIGN_SESSION
END_NMT_CLASS()

START_NMT_CLASS_(PlayerConfig, NMT_PLAYER_CONFIG)
	NMT_FIELD_INT(m_PlayerID, u32, 2)
	NMT_START_ARRAY(m_Values)
		NMT_FIELD(CStrW, m_Name)
		NMT_FIELD(CStrW, m_Value)
	NMT_END_ARRAY()
END_NMT_CLASS()

START_NMT_CLASS_(GameStart, NMT_GAME_START)
END_NMT_CLASS()

START_NMT_CLASS_(EndCommandBatch, NMT_END_COMMAND_BATCH)
	NMT_FIELD_INT(m_TurnLength, u32, 2)
END_NMT_CLASS()

START_NMT_CLASS_(Command, NMT_INVALID)
	NMT_FIELD(CEntityList, m_Entities)
	NMT_FIELD_INT(m_IsQueued, u32, 1)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(Command, Goto, NMT_GOTO)
	NMT_FIELD_INT(m_TargetX, u32, 2)
	NMT_FIELD_INT(m_TargetY, u32, 2)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(Command, Run, NMT_RUN)
	NMT_FIELD_INT(m_TargetX, u32, 2)
	NMT_FIELD_INT(m_TargetY, u32, 2)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(Command, Patrol, NMT_PATROL)
	NMT_FIELD_INT(m_TargetX, u32, 2)
	NMT_FIELD_INT(m_TargetY, u32, 2)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(Command, AddWaypoint, NMT_ADD_WAYPOINT)
	NMT_FIELD_INT(m_TargetX, u32, 2)
	NMT_FIELD_INT(m_TargetY, u32, 2)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(Command, SetRallyPoint, NMT_SET_RALLY_POINT)
	NMT_FIELD_INT(m_TargetX, u32, 2)
	NMT_FIELD_INT(m_TargetY, u32, 2)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(Command, SetStance, NMT_SET_STANCE)
	NMT_FIELD(CStrW, m_Stance)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(Command, Generic, NMT_GENERIC)
	NMT_FIELD(HEntity, m_Target)
	NMT_FIELD_INT(m_Action, u32, 4)
	NMT_FIELD_INT(m_Run, u32, 1)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(Command, Produce, NMT_PRODUCE)
	NMT_FIELD_INT(m_Type, u32, 4)
	NMT_FIELD(CStrW, m_Name)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(Command, PlaceObject, NMT_PLACE_OBJECT)
	NMT_FIELD(CStrW, m_Template)
	NMT_FIELD_INT(m_X, u32, 4)
	NMT_FIELD_INT(m_Y, u32, 4)
	NMT_FIELD_INT(m_Z, u32, 4)
	NMT_FIELD_INT(m_Angle, u32, 4)		// Orientation angle
END_NMT_CLASS()

DERIVE_NMT_CLASS_(Command, NotifyRequest, NMT_NOTIFY_REQUEST)
	NMT_FIELD(HEntity, m_Target)
	NMT_FIELD_INT(m_Action, u32, 4)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(Command, FormationGoto, NMT_FORMATION_GOTO)
	NMT_FIELD_INT(m_TargetX, u32, 2)
	NMT_FIELD_INT(m_TargetY, u32, 2)
END_NMT_CLASS()

DERIVE_NMT_CLASS_(Command, FormationGeneric, NMT_FORMATION_GENERIC)
	NMT_FIELD(HEntity, m_Target)
	NMT_FIELD_INT(m_Action, u32, 4)
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
