/* Copyright (C) 2009 Wildfire Games.
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
 *	FILE			: NetMessage.h
 *	PROJECT			: 0 A.D.
 *	DESCRIPTION		: Defines the basic interface for network messages
 *-----------------------------------------------------------------------------
 */
 
#ifndef NETMESSAGE_H
#define NETMESSAGE_H

// INCLUDES
#include "Serialization.h"
#include "ps/Vector2D.h"

#include <map>

// We need the enum from NetMessages.h, but we can't create any classes in
// NetMessages.h, since they in turn require CNetMessage to be defined
#define ALLNETMSGS_DONT_CREATE_NMTS
#include "NetMessages.h"
#undef ALLNETMSGS_DONT_CREATE_NMTS

/*
	CLASS			: CNetMessage
	DESCRIPTION		: CNetMessage is the base class for all network messages
					  exchanged within the game.
	NOTES			:
 */

class CNetMessage : public ISerializable
{
	NONCOPYABLE(CNetMessage);

	friend class CNetSession;

public:

	CNetMessage( void );
	CNetMessage( NetMessageType type );
	virtual ~CNetMessage( void );

	/**
	 * Retrieves the message header. If changes are made on header,SetDirty 
	 * must be called on the header's message
	 *
	 * @return							Message header
	 */
	//const CNetMessageHeader& GetHeader( void ) const { return m_Header; }
	NetMessageType GetType( void ) const { return m_Type; }

	/**
	 * Returns whether the message has changed since its last use
	 *
	 * @return							true if it changed or false otherwise
	 */
	bool GetDirty( void ) const	{ return m_Dirty; }

	/**
	 * Specify the message has changed since its last use
	 *
	 */
	void SetDirty( void ) { m_Dirty = true; }

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
	virtual u8* Serialize( u8* pBuffer ) const;

	/**
	 * Deserializes the message from the specified buffer.
	 *
	 * @param pStart				Message start within the serialized buffer
	 * @param pEnd					Message end within the serialized buffer
	 * @return						The position in the buffer right after the
	 *								message or NULL if an error occured
	 */
	virtual const u8* Deserialize( const u8* pStart, const u8* pEnd );

	/**
	 * Deserializes the specified message from the specified buffer using 
	 * registered deserializers.
	 *
	 * @param messageType				Message type
	 * @param pBuffer					Buffer from which to deserialize
	 * @param bufferSize				The size in bytes of the buffer
	 * @return							A pointer to a newly created
	 *									CNetMessage, or NULL if the message was
	 *									not correctly deserialized.
	 */
	static CNetMessage* Deserialize( 
									NetMessageType type,
									const u8* pBuffer,
									uint bufferSize );
	//static CNetMessage* Deserialize(ENetMessageType type, u8 *buffer, uint length);

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
	virtual CStr ToString( void ) const;

private:
	bool			m_Dirty;			// Message has been modified
	NetMessageType	m_Type;				// Message type

public:

	/**
	 * Register a selection of message types as JS constants.
	 * The constant's names will be the same as those of the enums
	 */
	static void ScriptingInit( void );
	
	/*static CCommandMessage* CommandFromJSArgs(
											  const CEntityList &entities,
											  JSContext* cx,
											  uintN argc,
											  jsval* argv,
											  bool isQueued );*/

	static CNetMessage* CommandFromJSArgs(
											  const CEntityList &entities,
											  JSContext* cx,
											  uintN argc,
											  jsval* argv,
											  bool isQueued );

	static CNetMessage* CreatePositionMessage(
											  const CEntityList& entities,
											  const int type,
											  CVector2D pos );

	static CNetMessage* CreateEntityIntMessage(
											   const CEntityList& entities,
											   const int type,
											   HEntity& target,
											   int action );

	static CNetMessage* CreateProduceMessage( 
											 const CEntityList& entities,
											 const int type,
											 int proType,
											 const CStrW& name );
};

/*
	CLASS			: CNetMessageFactory
	DESCRIPTION		: Creates messages from data received through the network
	NOTES			: It follows the factory method pattern implementation
*/
class CNetMessageFactory
{
public:

	/**
	 * Factory method which creates a message object based on the given data
	 *
	 * @param pData						Data buffer
	 * @param dataSize					Size of data buffer
	 * @return							The new message created
	 */
	static CNetMessage* CreateMessage( const void* pData, size_t dataSize );

protected:

private:

	// Not implemented
	CNetMessageFactory( void );
	~CNetMessageFactory( void );
	CNetMessageFactory( const CNetMessageFactory& );
	CNetMessageFactory& operator=( const CNetMessageFactory& );
};

/**
 * Special message type for simulation commands.
 * These commands are exposed as arbitrary JS objects, associated with a specific player.
 */
class CSimulationMessage : public CNetMessage
{
public:
	CSimulationMessage(ScriptInterface& scriptInterface);
	CSimulationMessage(ScriptInterface& scriptInterface, u32 client, i32 player, u32 turn, jsval data);
	virtual u8* Serialize(u8* pBuffer) const;
	virtual const u8* Deserialize(const u8* pStart, const u8* pEnd);
	virtual size_t GetSerializedLength() const;
	virtual CStr ToString() const;

	u32 m_Client;
	i32 m_Player;
	u32 m_Turn;
	CScriptValRooted m_Data;
private:
	ScriptInterface& m_ScriptInterface;
};

// This time, the classes are created
#include "NetMessages.h"

#endif // NETMESSAGE_H
