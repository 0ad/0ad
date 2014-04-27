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

#include "precompiled.h"
#include "NetMessage.h"

#include "ps/CLogger.h"

#include "ps/Game.h"
#include "simulation2/Simulation2.h"

#undef ALLNETMSGS_DONT_CREATE_NMTS
#define ALLNETMSGS_IMPLEMENT
#include "NetMessages.h"

CNetMessage::CNetMessage()
{
	m_Type	= NMT_INVALID;
}

CNetMessage::CNetMessage(NetMessageType type)
{
	m_Type  = type;
}

CNetMessage::~CNetMessage()
{
}

u8* CNetMessage::Serialize(u8* pBuffer) const
{
	size_t size = GetSerializedLength();
	Serialize_int_1(pBuffer, m_Type);
	Serialize_int_2(pBuffer, size);

	return pBuffer;
}

const u8* CNetMessage::Deserialize(const u8* pStart, const u8* pEnd)
{
	if (pStart + 3 > pEnd)
	{
		LOGERROR(L"CNetMessage: Corrupt packet (smaller than header)");
		return NULL;
	}

	const u8* pBuffer = pStart;

	int type;
	size_t size;
	Deserialize_int_1(pBuffer, type);
	Deserialize_int_2(pBuffer, size);
	m_Type = (NetMessageType)type;

	if (pStart + size != pEnd)
	{
		LOGERROR(L"CNetMessage: Corrupt packet (incorrect size)");
		return NULL;
	}

	return pBuffer;
}

size_t CNetMessage::GetSerializedLength() const
{
	// By default, return header size
	return 3;
}

CStr CNetMessage::ToString() const
{
	// This is called only when the subclass doesn't override it

	if (GetType() == NMT_INVALID)
		return "MESSAGE_TYPE_NONE { Undefined Message }";
	else
		return "Unknown Message " + CStr::FromInt(GetType());
}

CNetMessage* CNetMessageFactory::CreateMessage(const void* pData,
											   size_t dataSize,
											   ScriptInterface& scriptInterface)
{
	CNetMessage* pNewMessage = NULL;
	CNetMessage header;

	// Figure out message type
	header.Deserialize((const u8*)pData, (const u8*)pData + dataSize);

	switch (header.GetType())
	{
	case NMT_GAME_SETUP:
		pNewMessage = new CGameSetupMessage(scriptInterface);
		break;

	case NMT_PLAYER_ASSIGNMENT:
		pNewMessage = new CPlayerAssignmentMessage;
		break;

	case NMT_FILE_TRANSFER_REQUEST:
		pNewMessage = new CFileTransferRequestMessage;
		break;

	case NMT_FILE_TRANSFER_RESPONSE:
		pNewMessage = new CFileTransferResponseMessage;
		break;

	case NMT_FILE_TRANSFER_DATA:
		pNewMessage = new CFileTransferDataMessage;
		break;

	case NMT_FILE_TRANSFER_ACK:
		pNewMessage = new CFileTransferAckMessage;
		break;

	case NMT_JOIN_SYNC_START:
		pNewMessage = new CJoinSyncStartMessage;
		break;

	case NMT_LOADED_GAME:
		pNewMessage = new CLoadedGameMessage;
		break;

	case NMT_SERVER_HANDSHAKE:
		pNewMessage = new CSrvHandshakeMessage;
		break;

	case NMT_SERVER_HANDSHAKE_RESPONSE:
		pNewMessage = new CSrvHandshakeResponseMessage;
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
	
	case NMT_READY:
		pNewMessage = new CReadyMessage;
		break;

	case NMT_SIMULATION_COMMAND:
		pNewMessage = new CSimulationMessage(scriptInterface);
		break;

	default:
		LOGERROR(L"CNetMessageFactory::CreateMessage(): Unknown message type '%d' received", header.GetType());
		break;
	}

	if (pNewMessage)
		pNewMessage->Deserialize((const u8*)pData, (const u8*)pData + dataSize);

	return pNewMessage;
}
