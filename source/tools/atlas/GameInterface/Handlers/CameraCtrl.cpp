#include "precompiled.h"

#include "MessageHandler.h"
#include "../GameLoop.h"

namespace AtlasMessage {


void fScrollConstant(IMessage* msg)
{
	mScrollConstant* cmd = static_cast<mScrollConstant*>(msg);

	if (cmd->dir < 0 || cmd->dir > 3)
	{
		debug_warn("ScrollConstant: invalid direction");
	}
	else
	{
		g_GameLoop->input.scrollSpeed[cmd->dir] = cmd->speed;
	}
}
REGISTER(ScrollConstant);

}