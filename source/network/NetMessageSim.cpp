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

#include "NetMessage.h"

#include "scriptinterface/ScriptInterface.h"
#include "simulation2/serialization/BinarySerializer.h"
#include "simulation2/serialization/StdDeserializer.h"

/**
 * Serializer instance that writes directly to a buffer (which must be long enough).
 */
class CBufferBinarySerializer : public CBinarySerializer
{
public:
	CBufferBinarySerializer(ScriptInterface& scriptInterface, u8* buffer) :
		CBinarySerializer(scriptInterface), m_Buffer(buffer)
	{
	}

	void Put(const char* UNUSED(name), const u8* data, size_t len)
	{
		memcpy(m_Buffer, data, len);
		m_Buffer += len;
	}

	u8* m_Buffer;
};

/**
 * Serializer instance that simply counts how many bytes would be written.
 */
class CLengthBinarySerializer : public CBinarySerializer
{
public:
	CLengthBinarySerializer(ScriptInterface& scriptInterface) :
		CBinarySerializer(scriptInterface), m_Length(0)
	{
	}

	void Put(const char* UNUSED(name), const u8* UNUSED(data), size_t len)
	{
		m_Length += len;
	}

	size_t m_Length;
};

CSimulationMessage::CSimulationMessage(ScriptInterface& scriptInterface) :
	CNetMessage(NMT_SIMULATION_COMMAND), m_ScriptInterface(scriptInterface)
{
}

CSimulationMessage::CSimulationMessage(ScriptInterface& scriptInterface, u32 client, i32 player, u32 turn, jsval data) :
	CNetMessage(NMT_SIMULATION_COMMAND), m_ScriptInterface(scriptInterface),
	m_Client(client), m_Player(player), m_Turn(turn), m_Data(scriptInterface.GetContext(), data)
{
}

u8* CSimulationMessage::Serialize(u8* pBuffer) const
{
	// TODO: ought to handle serialization exceptions
	// TODO: ought to represent common commands more efficiently

	u8* pos = CNetMessage::Serialize(pBuffer);
	CBufferBinarySerializer serializer(m_ScriptInterface, pos);
	serializer.NumberU32_Unbounded("client", m_Client);
	serializer.NumberI32_Unbounded("player", m_Player);
	serializer.NumberU32_Unbounded("turn", m_Turn);
	serializer.ScriptVal("command", m_Data);
	return serializer.m_Buffer;
}

const u8* CSimulationMessage::Deserialize(const u8* pStart, const u8* pEnd)
{
	// TODO: ought to handle serialization exceptions
	// TODO: ought to represent common commands more efficiently

	const u8* pos = CNetMessage::Deserialize(pStart, pEnd);
	std::istringstream stream(std::string(pos, pEnd));
	CStdDeserializer deserializer(m_ScriptInterface, stream);
	deserializer.NumberU32_Unbounded(m_Client);
	deserializer.NumberI32_Unbounded(m_Player);
	deserializer.NumberU32_Unbounded(m_Turn);
	deserializer.ScriptVal(m_Data);
	return pEnd;
}

size_t CSimulationMessage::GetSerializedLength() const
{
	// TODO: serializing twice is stupidly inefficient - we should just
	// do it once, store the result, and use it here and in Serialize

	CLengthBinarySerializer serializer(m_ScriptInterface);
	serializer.NumberU32_Unbounded("client", m_Client);
	serializer.NumberI32_Unbounded("player", m_Player);
	serializer.NumberU32_Unbounded("turn", m_Turn);
	serializer.ScriptVal("command", m_Data);
	return CNetMessage::GetSerializedLength() + serializer.m_Length;
}

CStr CSimulationMessage::ToString() const
{
	std::string source;

	if (!m_ScriptInterface.CallFunction(m_Data.get(), "toSource", source))
		source = "ERROR";

	std::stringstream stream;
	stream << "CSimulationMessage { m_Client: " << m_Client << ", m_Player: " << m_Player << ", m_Turn: " << m_Turn << ", m_Data: " << source << " }";
	return stream.str();
}
