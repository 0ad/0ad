#include "../Messages.h"

#include <map>

namespace AtlasMessage
{

// (Random note: Be careful not to give handler .cpp files the same name
// as any other file in the project, because it makes everything very confused)

typedef void (*msgHandler)(IMessage*);
typedef std::map<std::string, msgHandler> msgHandlers;
extern msgHandlers& GetMsgHandlers();

#define THINGHANDLER(prefix, expectedtype, t) \
	void f##t(prefix##t*); \
	namespace register_handler_##t { \
		void wrapper(IMessage* msg) { \
			debug_assert(msg->GetType() == IMessage::expectedtype); \
			f##t (static_cast<prefix##t*>(msg)); \
		} \
		struct init { init() { \
			bool notAlreadyRegisted = GetMsgHandlers().insert(std::pair<std::string, msgHandler>(#t, &wrapper)).second; \
			debug_assert(notAlreadyRegisted); \
		} } init; \
	}; \
	void f##t(prefix##t* msg)

#define MESSAGEHANDLER(t) THINGHANDLER(m, Message, t)
#define QUERYHANDLER(t) THINGHANDLER(q, Query, t)

#define MESSAGEHANDLER_STR(t) \
	void fCommandString_##t(); \
	namespace register_handler_##t { \
		void wrapper(IMessage* msg) { \
			debug_assert(msg->GetType() == IMessage::Message); \
			fCommandString_##t (); \
		} \
		struct init { init() { \
			bool notAlreadyRegisted = GetMsgHandlers().insert(std::pair<std::string, msgHandler>("CommandString_"#t, &wrapper)).second; \
			debug_assert(notAlreadyRegisted); \
		} } init; \
	}; \
	void fCommandString_##t()

}
