/**
 *-----------------------------------------------------------------------------
 *	FILE			: NetMessage.cpp
 *	PROJECT			: 0 A.D.
 *	DESCRIPTION		:
 *-----------------------------------------------------------------------------
 */

// INCLUDES
#include "precompiled.h"
#include "simulation/Entity.h"
#include "ps/Vector2D.h"
#define ALLNETMSGS_IMPLEMENT
#include "NetMessage.h"
#include "ps/CLogger.h"
#include "Network.h"

#include <stdio.h>
#include <map>

// DEFINES

// Please don't modify the deserializer map outside the ONCE-block in DeserializeMessage
//typedef std::map< NetMessageType, NetMessageDeserializer > MessageDeserializerMap;
//MessageDeserializerMap g_DeserializerMap;

//-----------------------------------------------------------------------------
// Name: CNetMessage()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetMessage::CNetMessage( void )
{
	m_Type	= NMT_INVALID;
	m_Dirty	= false;
}

//-----------------------------------------------------------------------------
// Name: CNetMessage()
// Desc: Constructor
//-----------------------------------------------------------------------------
CNetMessage::CNetMessage( NetMessageType type )
{
	m_Type  = type;
	m_Dirty	= false;
}

//-----------------------------------------------------------------------------
// Name: ~CNetMessage()
// Desc: Destructor
//-----------------------------------------------------------------------------
CNetMessage::~CNetMessage( void )
{
	m_Dirty	= false;
}

//-----------------------------------------------------------------------------
// Name: Serialize()
// Desc: Serializes the message into the buffer parameter
//-----------------------------------------------------------------------------
u8* CNetMessage::Serialize( u8* pBuffer ) const
{
	// Validate parameters
	if ( !pBuffer ) return NULL;

//	Serialize_int_1( pBuffer, m_Type );
//	Serialize_int_2( pBuffer, m_SerializeSize );

	// Serialize message type and its size
	*( ( NetMessageType* )pBuffer ) = m_Type;
	*( ( size_t* )( pBuffer + sizeof( NetMessageType ) ) ) = GetSerializedLength();

	return pBuffer + sizeof( NetMessageType ) + sizeof( uint );
}

//-----------------------------------------------------------------------------
// Name: Deserialize()
// Desc: Loads this message from the specifie buffer parameter
//-----------------------------------------------------------------------------
const u8* CNetMessage::Deserialize( const u8* pStart, const u8* pEnd )
{
	// Validate parameters
	if ( !pStart || !pEnd )	return NULL;

	// Deserialize message type and its size

//	Deserialize_int_1( pStart, m_Type );
//	Deserialize_int_2( pStart, m_SerializeSize );

	m_Type = *( ( NetMessageType* )pStart );
	uint size = *( ( uint* )( pStart + sizeof( NetMessageType ) ) );

	assert( pStart + size == pEnd );

	return pStart + sizeof( NetMessageType ) + sizeof( size );
}

//-----------------------------------------------------------------------------
// Name: Deserialize()
// Desc: Constructs a CNetMessage object from the specified buffer parameter
// Note: It uses the registered desrializers to create the CNetMessage object
//-----------------------------------------------------------------------------
/*CNetMessage* CNetMessage::Deserialize( 
									  NetMessageType type,
									  const u8* pBuffer,
									  uint bufferSize )
{
	ONCE
	(
		SNetMessageDeserializer* pDeserializer = &g_DeserializerRegistrations[ 0 ];
		for ( ; pDeserializer->Deserializer; pDeserializer++ )
		{
			g_DeserializerMap.insert( std::make_pair( 
													 pDeserializer->Type,
													 pDeserializer->Deserializer ) );
		}
	);
	
	MessageDeserializerMap::const_iterator it = g_DeserializerMap.find( type );
	if ( it == g_DeserializerMap.end() )
	{
		LOG(WARNING, LOG_CAT_NET, "Unknown message received on socket: type 0x%04x, length %u", type, length);
	
		return NULL;
	}

	pfnNetMessageDeserializer pDeserializer = it->second;
	if ( pDeserializer )
	{
		// Call deserializer
		return ( pDeserializer )( pBuffer, bufferSize );
	}

	return NULL;
}*/

//-----------------------------------------------------------------------------
// Name: GetSerializedLength()
// Desc: Returns the size of the serialized message
//-----------------------------------------------------------------------------
size_t CNetMessage::GetSerializedLength( void ) const
{
	// By default, return header size
	return ( sizeof( m_Type ) + sizeof( uint ) );
}

//-----------------------------------------------------------------------------
// Name: operator ( ENetPacket* )()
// Desc: Cast to an ENetPacket structure
//-----------------------------------------------------------------------------
/*void CNetMessage::operator ENetPacket*( void )
{
	// Did we already serialized the message?
	if ( m_Packet )
	{
		// Dirty message?
		if ( m_Dirty )
		{
			// Make room for the new message content
			enet_packet_resize( m_Packet, GetSerializedLength() );

			// Serialize into buffer
			Serialize( m_Packet->data );
		}
	}
	else
	{
		// Serialize message into temporary buffer
		u8* pBuffer = new u8[ GetSerializedLength() ];
		if ( !pBuffer ) return NULL;

		Serialize( pBuffer );

		// Create ENet packet for this message
		m_Packet = enet_packet_create( pBuffer, GetSerializedLength(), ENET_PACKET_FLAG_RELIABLE );

		delete [] pBuffer;
	}

	return m_Packet;
}*/

//-----------------------------------------------------------------------------
// Name: ToString()
// Desc: Returns a string representation for the message
//-----------------------------------------------------------------------------
CStr CNetMessage::ToString( void ) const
{
	CStr ret;

	// Not defined yet?
	if ( GetType() == NMT_INVALID )
	{
		ret = "MESSAGE_TYPE_NONE { Undefined Message }";
	}
	else
	{
		ret = " Unknown Message " + CStr( GetType() );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Name: ScriptingInit()
// Desc:
//-----------------------------------------------------------------------------
void CNetMessage::ScriptingInit()
{
	g_ScriptingHost.DefineConstant( "NMT_GOTO", NMT_GOTO );
	g_ScriptingHost.DefineConstant( "NMT_RUN", NMT_RUN );
	g_ScriptingHost.DefineConstant( "NMT_PATROL", NMT_PATROL );
	g_ScriptingHost.DefineConstant( "NMT_ADD_WAYPOINT", NMT_ADD_WAYPOINT );
	g_ScriptingHost.DefineConstant( "NMT_CONTACT_ACTION", NMT_CONTACT_ACTION );
	g_ScriptingHost.DefineConstant( "NMT_PRODUCE", NMT_PRODUCE );
	g_ScriptingHost.DefineConstant( "NMT_PLACE_OBJECT", NMT_PLACE_OBJECT );
	g_ScriptingHost.DefineConstant( "NMT_REMOVE_OBJECT", NMT_REMOVE_OBJECT );
	g_ScriptingHost.DefineConstant( "NMT_SET_RALLY_POINT", NMT_SET_RALLY_POINT );
	g_ScriptingHost.DefineConstant( "NMT_SET_STANCE", NMT_SET_STANCE );
	g_ScriptingHost.DefineConstant( "NMT_NOTIFY_REQUEST", NMT_NOTIFY_REQUEST );
	g_ScriptingHost.DefineConstant( "NMT_FORMATION_GOTO", NMT_FORMATION_GOTO );
	g_ScriptingHost.DefineConstant( "NMT_FORMATION_CONTACT_ACTION", NMT_FORMATION_CONTACT_ACTION );
}

//-----------------------------------------------------------------------------
// Name: CommandFromJSArgs()
// Desc:
//-----------------------------------------------------------------------------
CNetMessage* CNetMessage::CommandFromJSArgs(
											const CEntityList &entities,
											JSContext* pContext,
											uintN argc,
											jsval* argv,
											bool isQueued )
{
	uint idx = 0;
	uint messageType;

	// Validate parameters
	if ( argv == 0 ) return NULL;

	try
	{
		messageType = ToPrimitive< uint >( argv[ idx++ ] );
	}
	catch ( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( pContext, "Invalid order type" );
		return NULL;
	}
	
	switch ( messageType )
	{
	case NMT_GOTO:
		{
			CGotoMessage* pMessage = new CGotoMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_IsQueued = isQueued;
			pMessage->m_Entities = entities;

			try
			{
				if ( idx + 2 > argc )
				{
					JS_ReportError( pContext, "Too few parameters!" );
					return NULL;
				}

				if ( !JSVAL_IS_INT( argv[ idx ] ) ||
					 !JSVAL_IS_INT( argv[ idx + 1 ] ) )
				{
					JS_ReportError( pContext, "Parameter type error!" );
					return NULL;
				}

				pMessage->m_TargetX  = ToPrimitive< int >( argv[ idx++ ] );
				pMessage->m_TargetY = ToPrimitive< int >( argv[ idx++ ] );
			}
			catch ( PSERROR_Scripting_ConversionFailed )
			{
				JS_ReportError( pContext, "Invalid location" );
				return NULL;
			}

			return pMessage;
		}

	case NMT_RUN:
		{
			CRunMessage* pMessage = new CRunMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_IsQueued = isQueued;
			pMessage->m_Entities = entities;

			try
			{
				if ( idx + 2 > argc )
				{
					JS_ReportError( pContext, "Too few parameters!" );
					return NULL; 
				}

				if ( !JSVAL_IS_INT( argv[ idx ] ) ||
					 !JSVAL_IS_INT( argv[ idx + 1 ] ) )
				{
					JS_ReportError( pContext, "Parameter type error!" );
					return NULL;
				}

				pMessage->m_TargetX = ToPrimitive< int >( argv[ idx++ ] );
				pMessage->m_TargetY = ToPrimitive< int >( argv[ idx++ ] );
			}
			catch ( PSERROR_Scripting_ConversionFailed )
			{
				JS_ReportError( pContext, "Invalid location" );
				return NULL;
			}

			return pMessage;
		}

	case NMT_PATROL:
		{
			CPatrolMessage* pMessage = new CPatrolMessage;
			if ( pMessage ) return NULL;

			pMessage->m_IsQueued = isQueued;
			pMessage->m_Entities = entities;

			try
			{
				if ( idx + 2 > argc )
				{
					JS_ReportError( pContext, "Too few parameters!" );
					return NULL;
				}

				if ( !JSVAL_IS_INT( argv[ idx ] ) ||
					 !JSVAL_IS_INT( argv[ idx + 1 ] ) )
				{
					JS_ReportError( pContext, "Parameter type error!" );
					return NULL;
				}

				pMessage->m_TargetX = ToPrimitive< int >( argv[ idx++ ] );
				pMessage->m_TargetY = ToPrimitive< int >( argv[ idx++ ] );
			}
			catch ( PSERROR_Scripting_ConversionFailed )
			{
				JS_ReportError( pContext, "Invalid location" );
				return NULL;
			}

			return pMessage;
		}

	case NMT_ADD_WAYPOINT:
		{
			CAddWaypointMessage* pMessage = new CAddWaypointMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_IsQueued = isQueued;
			pMessage->m_Entities = entities;

			try
			{
				if ( idx + 2 > argc )
				{
					JS_ReportError( pContext, "Too few parameters!" );
					return NULL;
				}
				
				if ( !JSVAL_IS_INT( argv[ idx ] ) ||
					 !JSVAL_IS_INT( argv[ idx + 1 ] ) )
				{
					JS_ReportError( pContext, "Parameter type error!" );
					return NULL;
				}

				pMessage->m_TargetX = ToPrimitive< int >( argv[ idx++ ] );
				pMessage->m_TargetY = ToPrimitive< int >( argv[ idx++ ] );
			}
			catch ( PSERROR_Scripting_ConversionFailed )
			{
				JS_ReportError( pContext, "Invalid location" );
				return NULL;
			}

			return pMessage;
		}

	case NMT_SET_RALLY_POINT:
		{
			CSetRallyPointMessage* pMessage = new CSetRallyPointMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_IsQueued = isQueued;
			pMessage->m_Entities = entities;

			try
			{
				if ( idx + 2 > argc )
				{
					JS_ReportError( pContext, "Too few parameters!" );
					return NULL; 
				}

				if ( !JSVAL_IS_INT( argv[ idx ] ) ||
					 !JSVAL_IS_INT( argv[ idx + 1 ] ) )
				{
					JS_ReportError( pContext, "Parameter type error!" );
					return NULL;
				}

				pMessage->m_TargetX = ToPrimitive< int >( argv[ idx++ ] );
				pMessage->m_TargetY = ToPrimitive< int >( argv[ idx++ ] );
			}
			catch ( PSERROR_Scripting_ConversionFailed )
			{
				JS_ReportError( pContext, "Invalid location" );
				return NULL;
			}

			return pMessage;
		}

	case NMT_SET_STANCE:
		{
			CSetStanceMessage* pMessage = new CSetStanceMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_IsQueued = isQueued;
			pMessage->m_Entities = entities;

			try
			{
				if ( idx + 1 > argc )
				{
					JS_ReportError( pContext, "Too few parameters!" );
					return NULL; 
				}

				if ( !JSVAL_IS_STRING( argv[ idx ] ) )
				{
					JS_ReportError( pContext, "Parameter type error!" );
					return NULL;
				}

				pMessage->m_Stance = ToPrimitive< CStrW >( argv[ idx++ ] );
			}
			catch ( PSERROR_Scripting_ConversionFailed )
			{
				JS_ReportError( pContext, "Invalid location" );
				return NULL;
			}

			return pMessage;
		}

	case NMT_FORMATION_GOTO:
		{
			CFormationGotoMessage* pMessage = new CFormationGotoMessage;
			if ( !pMessage ) return NULL;
	
			pMessage->m_IsQueued = isQueued;
			pMessage->m_Entities = entities;

			try
			{
				if ( idx + 2 > argc )
				{
					JS_ReportError( pContext, "Too few parameters!" );
					return NULL;
				}

				if ( !JSVAL_IS_INT( argv[ idx ] ) ||
					 !JSVAL_IS_INT( argv[ idx + 1 ] ) )
				{
					JS_ReportError( pContext, "Parameter type error!" );
					return NULL;
				}

				pMessage->m_TargetX = ToPrimitive< int >( argv[ idx++ ] );
				pMessage->m_TargetY = ToPrimitive< int >( argv[ idx++ ] );
			}
			catch ( PSERROR_Scripting_ConversionFailed )
			{
				JS_ReportError( pContext, "Invalid location" );
				return NULL;
			}

			return pMessage;
		}

	case NMT_CONTACT_ACTION:
		{
			CContactActionMessage* pMessage = new CContactActionMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_IsQueued = isQueued;
			pMessage->m_Entities = entities;

			if ( idx + 3 > argc )
			{
				JS_ReportError( pContext, "Too few parameters!" );
				return NULL;
			}

			if ( !JSVAL_IS_OBJECT( argv[ idx ] ) )
			{
				JS_ReportError( pContext, "Parameter type error!" );
				return NULL;
			}

			CEntity* pEntity = ToNative< CEntity >( argv[ idx++ ] );
			if ( !pEntity )
			{
				JS_ReportError( pContext, "Invalid entity parameter" );
				return NULL;
			}

			pMessage->m_Target = pEntity->me;

			if ( !JSVAL_IS_INT( argv[ idx ] ) )
			{
				JS_ReportError( pContext, "Parameter type error!" );
				return NULL;
			}
		
			pMessage->m_Action = ToPrimitive< int >( argv[ idx++ ] );

			if ( !JSVAL_IS_BOOLEAN( argv[ idx ] ) )
			{
				JS_ReportError( pContext, "Parameter type error!" );
				return NULL;
			}
		
			pMessage->m_Run = ToPrimitive< bool >( argv[ idx++ ] );

			return pMessage;
		}

	case NMT_NOTIFY_REQUEST:
		{
			CNotifyRequestMessage* pMessage = new CNotifyRequestMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_IsQueued = isQueued;
			pMessage->m_Entities = entities;

			if ( idx + 1 > argc )
			{

				JS_ReportError( pContext, "Too few parameters!" );
				return NULL;
			}
			
			if ( !JSVAL_IS_OBJECT( argv[ idx ] ) )
			{
				JS_ReportError( pContext, "Parameter type error!" );
				return NULL;
			}
			
			CEntity* pEntity = ToNative< CEntity >( argv[ idx++ ] );
			if ( !pEntity )
			{
				JS_ReportError( pContext, "Invalid entity parameter" );
				return NULL;
			}
			
			pMessage->m_Target = pEntity->me;

			return pMessage;
		}

	case NMT_FORMATION_CONTACT_ACTION:
		{
			CFormationContactActionMessage* pMessage = new CFormationContactActionMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_IsQueued = isQueued;
			pMessage->m_Entities = entities;

			if ( idx + 1 > argc )
			{
				JS_ReportError( pContext, "Too few parameters!" );\
				return NULL;
			}
			
			if ( !JSVAL_IS_OBJECT( argv[ idx ] ) )
			{
				JS_ReportError( pContext, "Parameter type error!" );
				return NULL;
			}
			
			CEntity* pEntity = ToNative< CEntity >( argv[ idx++ ] );
			if ( !pEntity )
			{
				JS_ReportError( pContext, "Invalid entity parameter" );
				return NULL;
			}
			
			pMessage->m_Target = pEntity->me;
			
			return pMessage;
		}

	case NMT_PRODUCE:
		{
			CProduceMessage* pMessage = new CProduceMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_IsQueued = isQueued;
			pMessage->m_Entities = entities;

			if ( idx + 1 > argc )
			{
				JS_ReportError( pContext, "Too few parameters!" );
				return NULL;
			}
			
			if ( !JSVAL_IS_INT( argv[ idx ] ) )
			{
				JS_ReportError( pContext, "Parameter type error!" );
				return NULL;
			}

			pMessage->m_Type = ToPrimitive< int >( argv[ idx++ ] );

			if ( idx + 1 > argc )
			{
				JS_ReportError( pContext, "Too few parameters!" );
				return NULL;
			}
            
			if ( !JSVAL_IS_STRING( argv[ idx ] ) )
			{
				JS_ReportError( pContext, "Parameter type error!" );
				return NULL; 
			}
			
			pMessage->m_Name = ToPrimitive< CStrW >( argv[ idx++ ] );

			return pMessage;
		}

	case NMT_PLACE_OBJECT:
		{
			CPlaceObjectMessage* pMessage = new CPlaceObjectMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_IsQueued = isQueued;
			pMessage->m_Entities = entities;

			if ( idx + 1 > argc )
			{
				JS_ReportError( pContext, "Too few parameters!" );
				return NULL;
			}
			
			if ( !JSVAL_IS_STRING( argv[ idx ] ) )
			{
				JS_ReportError( pContext, "Parameter type error!" );
				return NULL;
			}
				
			pMessage->m_Template = ToPrimitive< CStrW >( argv[ idx++ ] );

			if ( idx + 1 > argc )
			{
				JS_ReportError( pContext, "Too few parameters!" );
				return NULL;
			}
			
			if ( !JSVAL_IS_INT( argv[ idx ] ) )
			{
				JS_ReportError( pContext, "Parameter type error!" );
				return NULL;
			}
			
			pMessage->m_X = ToPrimitive< int >( argv[ idx++ ] );

			if ( idx + 1 > argc )
			{
				JS_ReportError( pContext, "Too few parameters!" );
				return NULL;
			}
			
			if ( !JSVAL_IS_INT( argv[ idx ] ) )
			{
				JS_ReportError( pContext, "Parameter type error!" );
				return NULL;
			}

			pMessage->m_Y = ToPrimitive< int >( argv[ idx++ ] );

			if ( idx + 1 > argc )
			{
				JS_ReportError( pContext, "Too few parameters!" );
				return NULL;
			}
			if ( !JSVAL_IS_INT( argv[ idx ] ) )
			{
				JS_ReportError( pContext, "Parameter type error!" );
				return NULL;
			}

			pMessage->m_Z = ToPrimitive< int >( argv[ idx++ ] );

			if ( idx + 1 > argc )
			{
				JS_ReportError( pContext, "Too few parameters!" );
				return NULL;
			}

			if ( !JSVAL_IS_INT( argv[ idx ] ) )
			{
				JS_ReportError( pContext, "Parameter type error!" );
				return NULL;
			}
			
			pMessage->m_Angle = ToPrimitive< int >( argv[ idx++ ] );
					
			return pMessage;
		}

	case NMT_REMOVE_OBJECT:
		{
			CRemoveObjectMessage* pMessage = new CRemoveObjectMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_IsQueued = isQueued;
			pMessage->m_Entities = entities;
					
			return pMessage;
		}

	default:

		JS_ReportError( pContext, "Invalid order type" );
		break;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Name: CreatePositionMessage()
// Desc:
//-----------------------------------------------------------------------------
CNetMessage* CNetMessage::CreatePositionMessage( 
												const CEntityList& entities,
												const int type,
												CVector2D pos )
{
	switch ( type )
	{
	case NMT_GOTO:
		{
			CGotoMessage *pMessage = new CGotoMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_Entities = entities;
			pMessage->m_TargetX = pos.x;
			pMessage->m_TargetY = pos.y;

			return pMessage;
		}

	case NMT_RUN:
		{
			CRunMessage *pMessage = new CRunMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_Entities = entities;
			pMessage->m_TargetX = pos.x;
			pMessage->m_TargetY = pos.y;

			return pMessage;
		}

	case NMT_PATROL:
		{
			CPatrolMessage *pMessage = new CPatrolMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_Entities = entities;
			pMessage->m_TargetX = pos.x;
			pMessage->m_TargetY = pos.y;

			return pMessage;
		}

	case NMT_ADD_WAYPOINT:
		{
			CAddWaypointMessage *pMessage = new CAddWaypointMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_Entities = entities;
			pMessage->m_TargetX = pos.x;
			pMessage->m_TargetY = pos.y;

			return pMessage;
		}

	case NMT_SET_RALLY_POINT:
		{
			CSetRallyPointMessage *pMessage = new CSetRallyPointMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_Entities = entities;
			pMessage->m_TargetX = pos.x;
			pMessage->m_TargetY = pos.y;

			return pMessage;
		}

	case NMT_FORMATION_GOTO:
		{
			CFormationGotoMessage *pMessage = new CFormationGotoMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_Entities = entities;
			pMessage->m_TargetX = pos.x;
			pMessage->m_TargetY = pos.y;

			return pMessage;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Name: CreateEntityIntMessage()
// Desc: 
//-----------------------------------------------------------------------------
CNetMessage* CNetMessage::CreateEntityIntMessage( 
												 const CEntityList& entities,
												 const int type,
												 HEntity& target,
												 int action )
{
	switch ( type )
	{
	case NMT_CONTACT_ACTION:
		{
			CContactActionMessage *pMessage = new CContactActionMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_Entities = entities;
			pMessage->m_Target   = target;
			pMessage->m_Action   = action;
			pMessage->m_Run      = false;
			
			return pMessage;
		}

	case NMT_NOTIFY_REQUEST:
		{
			CNotifyRequestMessage *pMessage = new CNotifyRequestMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_Entities = entities;
			pMessage->m_Target   = target;
			pMessage->m_Action   = action;

			return pMessage;
		}

	case NMT_FORMATION_CONTACT_ACTION:
		{
			CFormationContactActionMessage *pMessage = new CFormationContactActionMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_Entities = entities;
			pMessage->m_Target   = target;
			pMessage->m_Action   = action;

			return pMessage;
		}
	}

	return NULL;
}
CNetMessage* CNetMessage::CreateProduceMessage( const CEntityList& entities, const int type, int proType, const CStrW& name )
{
	switch ( type )
	{
		case NMT_PRODUCE:
		{
			CProduceMessage* pMessage = new CProduceMessage;
			if ( !pMessage ) return NULL;

			pMessage->m_Entities = entities;
			pMessage->m_Type	 = proType;
			pMessage->m_Name	 = name;

			return pMessage;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Name: CreateMessage()
// Desc: Creates the appropriate message based on the given data
//-----------------------------------------------------------------------------
CNetMessage* CNetMessageFactory::CreateMessage(const void* pData,
											   size_t dataSize )
{
	CNetMessage*	pNewMessage = NULL;
	CNetMessage		header;

	// Validate parameters
	if ( !pData ) return NULL;

	// Figure out message type
	header.Deserialize( ( const u8* )pData, ( const u8* )pData  + dataSize );

	// This is what we want
	//pNewMessage = m_Pool.GetMessage( header.GetType() );

	switch ( header.GetType() )
	{
	case NMT_GAME_SETUP:
		pNewMessage = new CGameSetupMessage;
		break;

	case NMT_ASSIGN_PLAYER_SLOT:
		pNewMessage = new CAssignPlayerSlotMessage;
		break;

	case NMT_PLAYER_CONFIG:
		pNewMessage = new CPlayerConfigMessage;
		break;

	case NMT_PLAYER_JOIN:
		pNewMessage = new CPlayerJoinMessage;
		break;

	case NMT_SERVER_HANDSHAKE:
		pNewMessage = new CSrvHandshakeMessage;
		break;

	case NMT_SERVER_HANDSHAKE_RESPONSE:
		pNewMessage = new CSrvHandshakeResponseMessage;
		break;

	case NMT_CONNECT_COMPLETE:
		pNewMessage = new CConnectCompleteMessage;
		break;

	case NMT_ERROR:
		pNewMessage = new CErrorMessage;
		break;

	case NMT_CLIENT_HANDSHAKE:
		pNewMessage = new CCliHandshakeMessage;
		break;

	case NMT_AUTHENTICATE:
		pNewMessage = new CAuthenticateMessage;
		break;

	case NMT_AUTHENTICATE_RESULT:
		pNewMessage = new CAuthenticateResultMessage;
		break;

	case NMT_GAME_START:
		pNewMessage = new CGameStartMessage;
		break;

	case NMT_END_COMMAND_BATCH:
		pNewMessage = new CEndCommandBatchMessage;
		break;
	
	case NMT_CHAT:
		pNewMessage = new CChatMessage;
		break;

	case NMT_GOTO:
		pNewMessage = new CGotoMessage;
		break;

	case NMT_PATROL:
		pNewMessage = new CPatrolMessage;
		break;

	case NMT_ADD_WAYPOINT:
		pNewMessage = new CAddWaypointMessage;
		break;

	case NMT_CONTACT_ACTION:
		pNewMessage = new CContactActionMessage;
		break;

	case NMT_PRODUCE:
		pNewMessage = new CProduceMessage;
		break;

	case NMT_PLACE_OBJECT:
		pNewMessage = new CPlaceObjectMessage;
		break;

	case NMT_REMOVE_OBJECT:
		pNewMessage = new CRemoveObjectMessage;
		break;

	case NMT_RUN:
		pNewMessage = new CRunMessage;
		break;

	case NMT_SET_RALLY_POINT:
		pNewMessage = new CSetRallyPointMessage;
		break;

	case NMT_SET_STANCE:
		pNewMessage = new CSetStanceMessage;
		break;

	case NMT_NOTIFY_REQUEST:
		pNewMessage = new CNotifyRequestMessage;
		break;

	case NMT_FORMATION_GOTO:
		pNewMessage = new CFormationGotoMessage;
		break;

	case NMT_FORMATION_CONTACT_ACTION:
		pNewMessage = new CFormationContactActionMessage;
		break;

	default:
		LOG(CLogger::Error, LOG_CAT_NET, "CNetMessageFactory::CreateMessage(): Unknown message received" );
		break;
	}

	if ( pNewMessage )
		pNewMessage->Deserialize( ( const u8* )pData, ( const u8* )pData + dataSize );

	return pNewMessage;
}
