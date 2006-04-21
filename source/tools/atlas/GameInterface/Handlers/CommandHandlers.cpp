#include "precompiled.h"

#include "MessageHandler.h"

#include "../CommandProc.h"

namespace AtlasMessage {


MESSAGEHANDLER(DoCommand)
{
	Command* c = NULL;
	cmdHandlers::const_iterator it = GetCmdHandlers().find("c" + *msg->name);
	if (it != GetCmdHandlers().end())
	{
		c = (it->second)(msg->data);
	}
	else
	{
		debug_warn("Unrecognised command");
		return;
	}

	GetCommandProc().Submit(c);
}

MESSAGEHANDLER(UndoCommand)
{
	UNUSED2(msg);
	GetCommandProc().Undo();
}

MESSAGEHANDLER(RedoCommand)
{
	UNUSED2(msg);
	GetCommandProc().Redo();
}

MESSAGEHANDLER(MergeCommand)
{
	UNUSED2(msg);
	GetCommandProc().Merge();
}


}
