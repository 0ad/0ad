#include "precompiled.h"

#include <stdio.h>
#include <map>

#include "simulation/Entity.h"
#include "ps/Vector2D.h"
#define ALLNETMSGS_IMPLEMENT

#include "NetMessage.h"

#include "ps/CLogger.h"

#define LOG_CAT_NET "net"

// Please don't modify the deserializer map outside the ONCE-block in DeserializeMessage
typedef std::map <ENetMessageType, NetMessageDeserializer> MessageDeserializerMap;
MessageDeserializerMap g_DeserializerMap;

CNetMessage::~CNetMessage()
{
	m_Type=NMT_NONE;
}

u8 *CNetMessage::Serialize(u8 *pos) const
{ return pos; }

size_t CNetMessage::GetSerializedLength() const
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
	LOG(CLogger::Error, LOG_CAT_NET, "CNetMessage::Copy(): Attempting to copy non-copyable message!");
	return new CNetMessage(NMT_NONE);
}

CNetMessage *CNetMessage::DeserializeMessage(ENetMessageType type, u8 *buffer, size_t length)
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
		LOG(CLogger::Warning, LOG_CAT_NET, "Unknown message received on socket: type 0x%04x, length %u", type, length);
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
	def(NMT_PlaceObject);
	def(NMT_NotifyRequest);
	def(NMT_FormationGoto);
	def(NMT_FormationGeneric);

#undef def
}

CNetCommand *CNetMessage::CommandFromJSArgs(const CEntityList &entities, JSContext *cx, uintN argc, jsval *argv, bool isQueued)
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
			msg->m_IsQueued = isQueued; \
			msg->m_Entities = entities; \
			ReadPosition(msg, m_Target); \
			return msg; \
		}
	
	#define EntityMessage(_msg) \
		case NMT_ ## _msg: \
		{ \
			C##_msg *msg = new C##_msg(); \
			msg->m_IsQueued = isQueued; \
			msg->m_Entities = entities; \
			ReadEntity(msg, m_Target); \
			return msg; \
		}

	#define EntityIntMessage(_msg) \
		case NMT_ ## _msg: \
		{ \
			C##_msg *msg = new C##_msg(); \
			msg->m_IsQueued = isQueued; \
			msg->m_Entities = entities; \
			ReadEntity(msg, m_Target); \
			ReadInt(msg, m_Action); \
			return msg; \
		}
	
	#define ProduceMessage(_msg) \
		case NMT_ ## _msg: \
		{ \
			C##_msg *msg = new C##_msg(); \
			msg->m_IsQueued = isQueued; \
			msg->m_Entities = entities; \
			ReadInt(msg, m_Type); \
			ReadString(msg, m_Name); \
			return msg; \
		}

	#define PlaceObjectMessage(_msg) \
		case NMT_ ## _msg: \
		{ \
			C##_msg *msg = new C##_msg(); \
			msg->m_IsQueued = isQueued; \
			msg->m_Entities = entities; \
			ReadString(msg, m_Template); \
			ReadInt(msg, m_X); \
			ReadInt(msg, m_Y); \
			ReadInt(msg, m_Z); \
			ReadInt(msg, m_Angle); \
			return msg; \
		}

	// argIndex, incremented by reading macros. We have already "eaten" the
	// first argument (message type)
	size_t argIndex = 1;
	switch (msgType)
	{
		// NMT_Goto, targetX, targetY
		PositionMessage(Goto)
		PositionMessage(Run)
		PositionMessage(Patrol)
		PositionMessage(AddWaypoint)
		PositionMessage(FormationGoto)

		// NMT_Generic, target, action
		EntityIntMessage(Generic)
		EntityIntMessage(NotifyRequest)
		EntityIntMessage(FormationGeneric)

		// NMT_Produce, type, name
		ProduceMessage(Produce)

		// NMT_PlaceObject, template, x, y, z, angle, action
		PlaceObjectMessage(PlaceObject)

		default:
			JS_ReportError(cx, "Invalid order type");
			return NULL;
	}
}

CNetMessage *CNetMessage::CreatePositionMessage( const CEntityList& entities, const int type, CVector2D pos )
{
	#define PosMessage(_msg) \
		case NMT_ ## _msg: \
		{ \
			C##_msg* msg = new C##_msg(); \
			msg->m_Entities = entities; \
			msg->m_TargetX = pos.x; \
			msg->m_TargetY = pos.y; \
			return msg; \
		}
	
	switch (type)
	{
		PosMessage(Goto)
		PosMessage(Run)
		PosMessage(Patrol)
		PosMessage(AddWaypoint)
		PosMessage(FormationGoto)
	
	default:
			return NULL;
	}
}
CNetMessage *CNetMessage::CreateEntityIntMessage( const CEntityList& entities, const int type, HEntity& target, int action )
{
	#define EntMessage(_msg) \
		case NMT_ ## _msg: \
		{ \
			C##_msg* msg = new C##_msg(); \
			msg->m_Entities = entities; \
			msg->m_Target = target; \
			msg->m_Action = action; \
			return msg; \
		}

	switch (type)
	{
		EntMessage(Generic)
		EntMessage(NotifyRequest)
		EntMessage(FormationGeneric)

	default:
		return NULL;
	}
}
CNetMessage *CNetMessage::CreateProduceMessage( const CEntityList& entities, const int type, int proType, const CStrW& name )
{
	#define ProMessage(_msg)\
		case NMT_ ## _msg: \
		{ \
			C##_msg* msg = new C##_msg(); \
			msg->m_Entities = entities; \
			msg->m_Type = proType; \
			msg->m_Name = name; \
			return msg; \
		}

	switch (type)
	{
		ProMessage(Produce)

	default:
		return NULL;
	}
}

		

	
