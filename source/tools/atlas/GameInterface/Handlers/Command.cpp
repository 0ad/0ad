#include "precompiled.h"

#include "MessageHandler.h"

#include "../CommandProc.h"

namespace AtlasMessage {


void fDoCommand(IMessage* msg)
{
	mDoCommand* cmd = static_cast<mDoCommand*>(msg);

	Command* c = NULL;
	cmdHandlers::const_iterator it = GetCmdHandlers().find("c" + cmd->name);
	if (it != GetCmdHandlers().end())
	{
		c = (it->second)(cmd->data);
	}
	else
	{
		debug_warn("Unrecognised command");
		return;
	}

	GetCommandProc().Submit(c);
}
REGISTER(DoCommand);


void fUndoCommand(IMessage*)
{
	GetCommandProc().Undo();
}
REGISTER(UndoCommand);


void fRedoCommand(IMessage*)
{
	GetCommandProc().Redo();
}
REGISTER(RedoCommand);

void fMergeCommand(IMessage*)
{
	GetCommandProc().Merge();
}
REGISTER(MergeCommand);

}
