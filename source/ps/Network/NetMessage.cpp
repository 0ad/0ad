#include "posix.h"
#include "lib.h"
#include "misc.h"
#include <stdio.h>

#define ALLNETMSGS_IMPLEMENT

#include "NetMessage.h"
#include <map>

// NEVER modify the deserializer map outside the ONCE-block in DeserializeMessage
typedef std::map <ENetMessageType, NetMessageDeserializer> MessageDeserializerMap;
MessageDeserializerMap g_DeserializerMap;

u8 *CNetMessage::Serialize(u8 *pos) const
{ return pos; }

uint CNetMessage::GetSerializedLength() const
{
	return 0;
}

CStr CNetMessage::GetString() const
{ return CStr(); }

const u8 *CNetMessage::Deserialize(const u8 *pos, const u8 *end)
{ return pos; }

CNetMessage *CNetMessage::DeserializeMessage(ENetMessageType type, u8 *buffer, uint length)
{
	{
	ONCE(
		SNetMessageDeserializerRegistration *pReg=&g_DeserializerRegistrations[0];
		for (;pReg->m_pDeserializer;pReg++)
		{
			g_DeserializerMap.insert(std::make_pair(pReg->m_Type, pReg->m_pDeserializer));
		}
	);
	}
	
	printf("DeserializeMessage: Finding for MT %d\n", type);
	MessageDeserializerMap::const_iterator dEntry=g_DeserializerMap.find(type);
	if (dEntry == g_DeserializerMap.end())
		return NULL;
	NetMessageDeserializer pDes=dEntry->second;
	return (pDes)(buffer, length);
}

