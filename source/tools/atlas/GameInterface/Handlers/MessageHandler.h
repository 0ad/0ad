#include "../Messages.h"

#include <map>
#include <string>

namespace AtlasMessage
{

typedef void (*msgHandler)(IMessage*);
typedef std::map<std::string, msgHandler> msgHandlers;
extern msgHandlers& GetMsgHandlers();

#define THINGHANDLER(prefix, expectedtype, t) \
	void f##t(prefix##t*); \
	void f##t##_wrapper(IMessage* msg) { \
		debug_assert(msg->GetType() == IMessage::expectedtype); \
		f##t (static_cast<prefix##t*>(msg)); \
	} \
	void f##t(prefix##t* msg)

#define MESSAGEHANDLER(t) THINGHANDLER(m, Message, t)
#define QUERYHANDLER(t) THINGHANDLER(q, Query, t)

}
