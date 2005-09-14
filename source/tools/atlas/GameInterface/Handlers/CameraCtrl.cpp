#include "precompiled.h"

#include "MessageHandler.h"
#include "../GameLoop.h"

#include <assert.h>

namespace AtlasMessage {


MESSAGEHANDLER(ScrollConstant)
{
	if (msg->dir < 0 || msg->dir > 3)
	{
		debug_warn("ScrollConstant: invalid direction");
	}
	else
	{
		g_GameLoop->input.scrollSpeed[msg->dir] = msg->speed;
	}
}

}
