#ifndef _NetMessage_H
#define _NetMessage_H

#include "types.h"

#define ALLNETMSGS_DONT_CREATE_NMTS
#include "AllNetMessages.h"
#undef ALLNETMSGS_DONT_CREATE_NMTS


class CNetMessage
{
	NetMessageType m_Type;
protected:
	inline CNetMessage(NetMessageType type):
		m_Type(type)
	{}

public:
	inline NetMessageType GetType() const
	{ return m_Type; }

	/**
	 * @returns The length of the message when serialized.
	 */
	virtual uint GetSerializedLength() const;
	/**
	 * Serialize the message into the buffer. The buffer will have the size
	 * returned from the last call to GetSerializedLength()
	 */
	virtual void Serialize(u8 *buffer) const;

	/**
	 * Deserialize a net message, using the globally registered deserializers.
	 *
	 * @param type The NetMessageType of the message
	 * @param buffer A pointer to the buffer holding the message data
	 * @param length The length of the message data
	 *
	 * @returns a pointer to a newly created CNetMessage subclass, or NULL if
	 * there was an error in data format.
	 */
	static CNetMessage *DeserializeMessage(NetMessageType type, u8 *buffer, uint length);
};

class CNetMessage;
typedef CNetMessage * (*NetMessageDeserializer) (const u8 *buffer, uint length);

#include "Entity.h"

struct SNetMessageDeserializerRegistration
{
	NetMessageType m_Type;
	NetMessageDeserializer m_pDeserializer;
};

#include "AllNetMessages.h"

#endif // #ifndef _NetMessage_H
