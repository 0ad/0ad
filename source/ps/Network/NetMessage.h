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
	inline CNetMessage(): m_Type(NMT_NONE)
	{}

	inline NetMessageType GetType() const
	{ return m_Type; }

	virtual uint GetSerializedLength() const;
	virtual void Serialize(u8 *buffer) const;

	static CNetMessage *DeserializeMessage(NetMessageType type, u8 *buffer, uint length);
};

class CNetMessage;
typedef CNetMessage * (*NetMessageDeserializer) (u8 *buffer, uint length);

struct SNetMessageDeserializerRegistration
{
	NetMessageType m_Type;
	NetMessageDeserializer m_pDeserializer;
};

#include "AllNetMessages.h"

#endif // #ifndef _NetMessage_H
