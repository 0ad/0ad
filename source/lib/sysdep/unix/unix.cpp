#include "precompiled.h"

#include "sysdep/sysdep.h"

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

void unix_debug_break()
{
	kill(getpid(), SIGTRAP);
}
