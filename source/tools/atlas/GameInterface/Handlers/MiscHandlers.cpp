#include "precompiled.h"

#include "MessageHandler.h"
#include "../MessagePasserImpl.h"

#include "ps/GameSetup/Config.h"
#include "ps/Util.h"

namespace AtlasMessage {

MESSAGEHANDLER(MessageTrace)
{
	((MessagePasserImpl<mCommand>*)g_MessagePasser_Command)->SetTrace(msg->enable);
	((MessagePasserImpl<mInput>*)g_MessagePasser_Input)->SetTrace(msg->enable);
}

MESSAGEHANDLER(Screenshot)
{
	// TODO: allow non-big screenshots too
	WriteBigScreenshot("bmp", msg->tiles);
}

}
