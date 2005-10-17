#include "precompiled.h"

#include "MessageHandler.h"
#include "../MessagePasserImpl.h"

namespace AtlasMessage {

MESSAGEHANDLER(MessageTrace)
{
	((MessagePasserImpl<mCommand>*)g_MessagePasser_Command)->SetTrace(msg->enable);
	((MessagePasserImpl<mInput>*)g_MessagePasser_Input)->SetTrace(msg->enable);
}

}
