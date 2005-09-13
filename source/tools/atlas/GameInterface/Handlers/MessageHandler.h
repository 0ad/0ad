#include "../Messages.h"

#include <map>

namespace AtlasMessage
{

// (Random note: Be careful not to give handler .cpp files the same name
// as any other file in the project, because it makes everything very confused)

typedef void     (*msgHandler)(IMessage*);
typedef std::map<std::string, msgHandler> msgHandlers;
extern msgHandlers& GetMsgHandlers();

#define CAT1(a,b) a##b
#define CAT2(a,b) CAT1(a,b)

// TODO quite urgently: Fix this, because it's broken and not very helpful anyway
#define REGISTER(t) namespace CAT2(hndlr_, __LINE__) { struct init { init() { \
	bool notAlreadyRegisted = GetMsgHandlers().insert(std::pair<std::string, msgHandler>(#t, &f##t)).second; \
	assert(notAlreadyRegisted); \
  } } init; };

}
