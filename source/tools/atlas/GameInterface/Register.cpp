#include "precompiled.h"

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
	extern void f##name##__wrapper(AtlasMessage::IMessage*); \
	AtlasMessage::GetMsgHandlers().insert(std::pair<std::string, AtlasMessage::msgHandler>(#name, &f##name##__wrapper));

#define QUERY(name, in_vals, out_vals) \
	extern void f##name##__wrapper(AtlasMessage::IMessage*); \
	AtlasMessage::GetMsgHandlers().insert(std::pair<std::string, AtlasMessage::msgHandler>(#name, &f##name##__wrapper));

#define COMMAND(name, merge, vals) \
	extern cmdHandler c##name##__create(); \
	GetCmdHandlers().insert(std::pair<std::string, cmdHandler>("c"#name, c##name##__create()));

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

