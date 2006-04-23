#include "precompiled.h"

#include "MessageHandler.h"

namespace AtlasMessage
{

msgHandlers& GetMsgHandlers()
{
	// Make sure this is initialised when it's first required, rather than
	// hoping to be lucky with static initialisation order.
	// (TODO: But is it safe to be sticking things into STL containers during
	// static initialisation?)
	static msgHandlers h;
	return h;
}

}
