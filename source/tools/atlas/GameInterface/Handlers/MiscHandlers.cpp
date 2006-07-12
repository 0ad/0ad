#include "precompiled.h"

#include "MessageHandler.h"
#include "../MessagePasserImpl.h"

#include "ps/GameSetup/Config.h"
#include "ps/Util.h"

namespace AtlasMessage {

MESSAGEHANDLER(MessageTrace)
{
	((MessagePasserImpl*)g_MessagePasser)->SetTrace(msg->enable);
}

MESSAGEHANDLER(Screenshot)
{
	// TODO: allow non-big screenshots too
	WriteBigScreenshot("bmp", msg->tiles);
}

QUERYHANDLER(Ping)
{
	UNUSED2(msg);
}

}
