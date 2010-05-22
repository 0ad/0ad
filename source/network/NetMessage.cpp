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

/**
 *-----------------------------------------------------------------------------
 *	FILE			: NetMessage.cpp
 *	PROJECT			: 0 A.D.
 *	DESCRIPTION		:
 *-----------------------------------------------------------------------------
 */

// INCLUDES
#include "precompiled.h"
#include "ps/CLogger.h"
#include "Network.h"
#include "NetMessage.h"

#include "ps/Game.h"
#include "simulation2/Simulation2.h"

#undef ALLNETMSGS_DONT_CREATE_NMTS
#define ALLNETMSGS_IMPLEMENT
#include "NetMessages.h"

// DEFINES
#define LOG_CATEGORY L"net"

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

	size_t size = GetSerializedLength();
	Serialize_int_1( pBuffer, m_Type );
	Serialize_int_2( pBuffer, size );

	return pBuffer;
}

//-----------------------------------------------------------------------------
// Name: Deserialize()
// Desc: Loads this message from the specified buffer parameter
//-----------------------------------------------------------------------------
const u8* CNetMessage::Deserialize( const u8* pStart, const u8* pEnd )
{
	// Validate parameters
	if ( !pStart || !pEnd )	return NULL;

	debug_assert( pStart + 3 <= pEnd );

	const u8* pBuffer = pStart;

	int type;
	size_t size;
	Deserialize_int_1( pBuffer, type );
	Deserialize_int_2( pBuffer, size );
	m_Type = (NetMessageType)type;

	debug_assert( pStart + size == pEnd );

	return pBuffer;
}

//-----------------------------------------------------------------------------
// Name: GetSerializedLength()
// Desc: Returns the size of the serialized message
//-----------------------------------------------------------------------------
size_t CNetMessage::GetSerializedLength( void ) const
{
	// By default, return header size
	return 3;
}

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

	case NMT_SYNC_CHECK:
		pNewMessage = new CSyncCheckMessage;
		break;

	case NMT_SYNC_ERROR:
		pNewMessage = new CSyncErrorMessage;
		break;

	case NMT_CHAT:
		pNewMessage = new CChatMessage;
		break;

	case NMT_SIMULATION_COMMAND:
		pNewMessage = new CSimulationMessage(g_Game->GetSimulation2()->GetScriptInterface());
		break;

	default:
		LOG(CLogger::Error, LOG_CATEGORY, L"CNetMessageFactory::CreateMessage(): Unknown message received" );
		break;
	}

	if ( pNewMessage )
		pNewMessage->Deserialize( ( const u8* )pData, ( const u8* )pData + dataSize );

	return pNewMessage;
}
