#include "../Messages.h"

namespace AtlasMessage
{

typedef void (*handler)(IMessage*);
typedef std::map<std::string, handler> handlers;
extern handlers& GetHandlers();

#define CAT1(a,b) a##b
#define CAT2(a,b) CAT1(a,b)

#define REGISTER(t) namespace CAT2(hndlr_, __LINE__) { struct init { init() { \
	bool notAlreadyRegisted = GetHandlers().insert(std::pair<std::string, handler>(#t, &f##t)).second; \
	assert(notAlreadyRegisted); \
  } } init; };

}