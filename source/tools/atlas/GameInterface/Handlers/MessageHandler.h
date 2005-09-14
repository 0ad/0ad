#include "../Messages.h"

#include <map>

namespace AtlasMessage
{

// (Random note: Be careful not to give handler .cpp files the same name
// as any other file in the project, because it makes everything very confused)

typedef void     (*msgHandler)(IMessage*);
typedef std::map<std::string, msgHandler> msgHandlers;
extern msgHandlers& GetMsgHandlers();

#define MESSAGEHANDLER(t) \
	void f##t(m##t*); \
	namespace register_handler_##t { \
		void wrapper(IMessage* msg) { \
			f##t (static_cast<m##t*>(msg)); \
		} \
		struct init { init() { \
			bool notAlreadyRegisted = GetMsgHandlers().insert(std::pair<std::string, msgHandler>(#t, &wrapper)).second; \
			debug_assert(notAlreadyRegisted); \
		} } init; \
	}; \
	void f##t(m##t* msg)

#define MESSAGEHANDLER_STR(t) \
	void fCommandString_##t(); \
	namespace register_handler_##t { \
		void wrapper(IMessage*) { \
			fCommandString_##t (); \
		} \
		struct init { init() { \
			bool notAlreadyRegisted = GetMsgHandlers().insert(std::pair<std::string, msgHandler>("CommandString_"#t, &wrapper)).second; \
			debug_assert(notAlreadyRegisted); \
		} } init; \
	}; \
	void fCommandString_##t()

}
