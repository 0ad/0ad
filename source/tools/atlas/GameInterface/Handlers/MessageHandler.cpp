#include "precompiled.h"

#include "MessageHandler.h"

namespace AtlasMessage
{

handlers& GetHandlers()
{
	// Make sure this is initialised when it's first required, rather than
	// hoping to be lucky with static initialisation order.
	// (TODO: But is it safe to be sticking things into STL containers during
	// static initialisation?)
	static handlers h;
	return h;
}

}
