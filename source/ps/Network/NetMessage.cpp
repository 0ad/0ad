#include "precompiled.h"

#include "posix.h"
#include "lib.h"
#include <stdio.h>
#include <map>

#define ALLNETMSGS_IMPLEMENT

#include "NetMessage.h"

#include "CLogger.h"

#define LOG_CAT_NET "net"

// NEVER modify the deserializer map outside the ONCE-block in DeserializeMessage
typedef std::map <ENetMessageType, NetMessageDeserializer> MessageDeserializerMap;
MessageDeserializerMap g_DeserializerMap;

CNetMessage::~CNetMessage()
{
	m_Type=NMT_NONE;
}

u8 *CNetMessage::Serialize(u8 *pos) const
{ return pos; }

uint CNetMessage::GetSerializedLength() const
{
	return 0;
}

CStr CNetMessage::GetString() const
{
	if (m_Type==NMT_NONE)
		return "NMT_NONE { Invalid Message }";
	else
		return CStr("Unknown Message ")+CStr(m_Type);
}

const u8 *CNetMessage::Deserialize(const u8 *pos, const u8 *end)
{ return pos; }

CNetMessage *CNetMessage::Copy() const
{
	LOG(ERROR, LOG_CAT_NET, "CNetMessage::Copy(): Attempting to copy non-copyable message!");
	return new CNetMessage(NMT_NONE);
}

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
	
	MessageDeserializerMap::const_iterator dEntry=g_DeserializerMap.find(type);
	if (dEntry == g_DeserializerMap.end())
	{
		LOG(WARNING, LOG_CAT_NET, "Unknown message received on socket: type 0x%04x, length %u", type, length);
		return NULL;
	}
	NetMessageDeserializer pDes=dEntry->second;
	return (pDes)(buffer, length);
}

