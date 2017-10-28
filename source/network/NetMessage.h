/* Copyright (C) 2017 Wildfire Games.
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

#ifndef NETMESSAGE_H
#define NETMESSAGE_H

#include "Serialization.h"

// We need the enum from NetMessages.h, but we can't create any classes in
// NetMessages.h, since they in turn require CNetMessage to be defined
#define ALLNETMSGS_DONT_CREATE_NMTS
#include "NetMessages.h"
#undef ALLNETMSGS_DONT_CREATE_NMTS

/**
 * The base class for all network messages exchanged within the game.
 */
class CNetMessage : public ISerializable
{
	friend class CNetSession;

public:

	CNetMessage();
	CNetMessage(NetMessageType type);
	virtual ~CNetMessage();

	/**
	 * Retrieves the message type.
	 * @return						Message type
	 */
	NetMessageType GetType() const { return m_Type; }

	/**
	 * Serialize the message into the specified buffer parameter. The size
	 * required by the buffer parameter can be found by a call to
	 * GetSerializedLength method. The information contained within the message
	 * must be serialized before the message is sent. By default only the
	 * message type and its size are serialized in the buffer parameter.
	 *
	 * @param pBuffer				Buffer where to serialize the message
	 * @return						The position in the buffer right after the
	 *								serialized message
	 */
	virtual u8* Serialize(u8* pBuffer) const;

	/**
	 * Deserializes the message from the specified buffer.
	 *
	 * @param pStart				Message start within the serialized buffer
	 * @param pEnd					Message end within the serialized buffer
	 * @return						The position in the buffer right after the
	 *								message or NULL if an error occurred
	 */
	virtual const u8* Deserialize(const u8* pStart, const u8* pEnd);

	/**
	 * Retrieves the size in bytes of the serialized message. Before calling
	 * Serialize, the memory size for the buffer where to serialize the message
	 * object can be found by calling this method.
	 *
	 * @return							The size of serialized message
	 */
	virtual size_t GetSerializedLength() const;

	/**
	 * Returns a string representation for the message
	 *
	 * @return							The message as a string
	 */
	virtual CStr ToString() const;

private:
	NetMessageType	m_Type;				// Message type
};

/**
 * Creates messages from data received through the network.
 */
class CNetMessageFactory
{
public:

	/**
	 * Factory method which creates a message object based on the given data
	 *
	 * @param pData						Data buffer
	 * @param dataSize					Size of data buffer
	 * @param scriptInterface			Script instance to use when constructing scripted messages
	 * @return							The new message created
	 */
	static CNetMessage* CreateMessage(const void* pData, size_t dataSize, const ScriptInterface& scriptInterface);
};

/**
 * Special message type for simulation commands.
 * These commands are exposed as arbitrary JS objects, associated with a specific player.
 */
class CSimulationMessage : public CNetMessage
{
public:
	CSimulationMessage(const ScriptInterface& scriptInterface);
	CSimulationMessage(const ScriptInterface& scriptInterface, u32 client, i32 player, u32 turn, JS::HandleValue data);

	/** The compiler can't create a copy constructor because of the PersistentRooted member,
	 * so we have to write it manually.
	 * NOTE: It doesn't clone the m_Data member and the copy will reference the same JS::Value!
	 */
	CSimulationMessage(const CSimulationMessage& orig);

	virtual u8* Serialize(u8* pBuffer) const;
	virtual const u8* Deserialize(const u8* pStart, const u8* pEnd);
	virtual size_t GetSerializedLength() const;
	virtual CStr ToString() const;

	u32 m_Client;
	i32 m_Player;
	u32 m_Turn;
	JS::PersistentRooted<JS::Value> m_Data;
private:
	const ScriptInterface& m_ScriptInterface;
};

/**
 * Special message type for updated to game startup settings.
 */
class CGameSetupMessage : public CNetMessage
{
	NONCOPYABLE(CGameSetupMessage);
public:
	CGameSetupMessage(const ScriptInterface& scriptInterface);
	CGameSetupMessage(const ScriptInterface& scriptInterface, JS::HandleValue data);
	virtual u8* Serialize(u8* pBuffer) const;
	virtual const u8* Deserialize(const u8* pStart, const u8* pEnd);
	virtual size_t GetSerializedLength() const;
	virtual CStr ToString() const;

	JS::PersistentRootedValue m_Data;
private:
	const ScriptInterface& m_ScriptInterface;
};

// This time, the classes are created
#include "NetMessages.h"

#endif // NETMESSAGE_H
