#include "posix.h"
#include "misc.h"
#include <stdio.h>

#define ALLNETMSGS_IMPLEMENT

#include "NetMessage.h"
#include <map>

// NEVER modify the deserializer map outside the ONCE-block in DeserializeMessage
typedef std::map <NetMessageType, NetMessageDeserializer> MessageDeserializerMap;
MessageDeserializerMap g_DeserializerMap;

void CNetMessage::Serialize(u8 *) const
{}

uint CNetMessage::GetSerializedLength() const
{
	return 0;
}

CNetMessage *CNetMessage::DeserializeMessage(NetMessageType type, u8 *buffer, uint length)
{
	{
	ONCE(
		SNetMessageDeserializerRegistration *pReg=&g_DeserializerRegistrations[0];
		for (;pReg->m_pDeserializer;pReg++)
		{
			g_DeserializerMap.insert(std::make_pair(pReg->m_Type, pReg->m_pDeserializer));
		}
	)
	}
	
	printf("DeserializeMessage: Finding for MT %d\n", type);
	MessageDeserializerMap::const_iterator dEntry=g_DeserializerMap.find(type);
	if (dEntry == g_DeserializerMap.end())
		return NULL;
	NetMessageDeserializer pDes=dEntry->second;
	return (pDes)(buffer, length);
}
