#include "precompiled.h"

#include "Messages.h"

// We want to include Messages.h again below, with some different definitions,
// so cheat and undefine its include-guard
#undef MESSAGES_H__

#include "SharedTypes.h"
#include "Shareable.h"

namespace AtlasMessage
{

struct IMessage;

typedef void (*msgHandler)(IMessage*);
typedef std::map<std::string, msgHandler> msgHandlers;
extern msgHandlers& GetMsgHandlers();

struct Command;
typedef Command* (*cmdHandler)(const void*);
typedef std::map<std::string, cmdHandler> cmdHandlers;
extern cmdHandlers& GetCmdHandlers();

#define MESSAGE(name, vals) \
	extern void f##name##_wrapper(AtlasMessage::IMessage*); \
	AtlasMessage::GetMsgHandlers().insert(std::pair<std::string, AtlasMessage::msgHandler>(#name, &f##name##_wrapper));

#define QUERY(name, in_vals, out_vals) \
	extern void f##name##_wrapper(AtlasMessage::IMessage*); \
	AtlasMessage::GetMsgHandlers().insert(std::pair<std::string, AtlasMessage::msgHandler>(#name, &f##name##_wrapper));

#define COMMAND(name, merge, vals) \
	extern cmdHandler c##name##_create(); \
	GetCmdHandlers().insert(std::pair<std::string, cmdHandler>("c"#name, c##name##_create()));

#define FUNCTION(def)

#undef SHAREABLE_STRUCT
#define SHAREABLE_STRUCT(name)

void RegisterHandlers()
{
	MESSAGE(DoCommand, );
	MESSAGE(UndoCommand, );
	MESSAGE(RedoCommand, );
	MESSAGE(MergeCommand, );
	
	#define MESSAGES_SKIP_SETUP
	#include "Messages.h"
}

}
