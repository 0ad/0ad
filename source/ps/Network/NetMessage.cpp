#include "precompiled.h"

#include "posix.h"
#include "lib.h"
#include <stdio.h>
#include <map>

#include "Entity.h"

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

const u8 *CNetMessage::Deserialize(const u8* pos, const u8* UNUSED(end))
{
	return pos;
}

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

void CNetMessage::ScriptingInit()
{
#define def(_msg) g_ScriptingHost.DefineConstant(#_msg, _msg)
	
	def(NMT_Goto);
	def(NMT_Run);
	def(NMT_Patrol);
	def(NMT_AddWaypoint);
	def(NMT_Generic);
	def(NMT_Produce);
	def(NMT_NotifyRequest);
}

CNetCommand *CNetMessage::CommandFromJSArgs(const CEntityList &entities, JSContext *cx, uintN argc, jsval *argv)
{
	debug_assert(argc >= 1);

	int msgType;

	try
	{
		msgType = ToPrimitive<int>( argv[0] );
	}
	catch(PSERROR_Scripting_ConversionFailed)
	{
		JS_ReportError(cx, "Invalid order type");
		return NULL;
	}
	
	#define ArgumentCountError() STMT(\
			JS_ReportError(cx, "Too few parameters!"); \
			return NULL; )
	#define ArgumentTypeError() STMT(\
			JS_ReportError(cx, "Parameter type error!"); \
			return NULL; )
	#define ReadPosition(_msg, _field) \
		try { \
			if (argIndex+2 > argc) \
				ArgumentCountError();\
			if (!JSVAL_IS_INT(argv[argIndex]) || !JSVAL_IS_INT(argv[argIndex+1])) \
				ArgumentTypeError(); \
			_msg->_field ## X = ToPrimitive<int>(argv[argIndex++]); \
			_msg->_field ## Y = ToPrimitive<int>(argv[argIndex++]); \
		} catch (PSERROR_Scripting_ConversionFailed) { \
			JS_ReportError(cx, "Invalid location"); \
			return NULL; \
		}
	#define ReadEntity(_msg, _field) \
		STMT(\
			if (argIndex+1 > argc) \
				ArgumentCountError(); \
			if (!JSVAL_IS_OBJECT(argv[argIndex])) \
				ArgumentTypeError(); \
			CEntity *ent=ToNative<CEntity>(argv[argIndex++]); \
			if (!ent) \
			{ \
				JS_ReportError(cx, "Invalid entity parameter"); \
				return NULL; \
			} \
			_msg->_field=ent->me; \
		)
	#define ReadInt(_msg, _field) \
		STMT(\
			if (argIndex+1 > argc) \
				ArgumentCountError(); \
			if (!JSVAL_IS_INT(argv[argIndex])) \
				ArgumentTypeError(); \
			int val=ToPrimitive<int>(argv[argIndex++]); \
			_msg->_field=val; \
		)
	#define ReadString(_msg, _field) \
		STMT(\
			if (argIndex+1 > argc) \
				ArgumentCountError(); \
			if (!JSVAL_IS_STRING(argv[argIndex])) \
				ArgumentTypeError(); \
			CStrW val=ToPrimitive<CStrW>(argv[argIndex++]); \
			_msg->_field=val; \
		)
	
	#define PositionMessage(_msg) \
		case NMT_ ## _msg: \
		{ \
			C##_msg *msg = new C##_msg(); \
			msg->m_Entities = entities; \
			ReadPosition(msg, m_Target); \
			return msg; \
		}
	
	#define EntityMessage(_msg) \
		case NMT_ ## _msg: \
		{ \
			C##_msg *msg = new C##_msg(); \
			msg->m_Entities = entities; \
			ReadEntity(msg, m_Target); \
			return msg; \
		}

	#define EntityIntMessage(_msg) \
		case NMT_ ## _msg: \
		{ \
			C##_msg *msg = new C##_msg(); \
			msg->m_Entities = entities; \
			ReadEntity(msg, m_Target); \
			ReadInt(msg, m_Action); \
			return msg; \
		}
	
	#define ProduceMessage(_msg) \
		case NMT_ ## _msg: \
		{ \
			C##_msg *msg = new C##_msg(); \
			msg->m_Entities = entities; \
			ReadInt(msg, m_Type); \
			ReadString(msg, m_Name); \
			return msg; \
		}

	// argIndex, incremented by reading macros. We have already "eaten" the
	// first argument (message type)
	uint argIndex = 1;
	switch (msgType)
	{
		// NMT_Goto, targetX, targetY
		PositionMessage(Goto)
		PositionMessage(Run)
		PositionMessage(Patrol)
		PositionMessage(AddWaypoint)

		// NMT_Generic, target, action
		EntityIntMessage(Generic)
		EntityIntMessage(NotifyRequest)

		// NMT_Produce, type, name
		ProduceMessage(Produce)

		default:
			JS_ReportError(cx, "Invalid order type");
			return NULL;
	}
}
