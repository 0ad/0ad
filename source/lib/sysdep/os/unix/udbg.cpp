/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* udbg.cpp

This file contains debug helpers that are common for all unix systems. See
linux/ldbg.cpp for the linux-specific stuff (Using BFD and backtrace() for
symbol lookups and backtraces)
*/

#include "precompiled.h"

#include <cstdio>
#include <sys/types.h>
#include <signal.h>

#include "lib/timer.h"
#include "lib/sysdep/sysdep.h"
#include "lib/debug.h"
#include "lib/utf8.h"


Status debug_CaptureContext(void* UNUSED(context))
{
	// (not needed unless/until we support stack traces)
	return INFO::SKIPPED;
}

void debug_break()
{
	kill(getpid(), SIGTRAP);
}

#define DEBUGGER_WAIT 3
#define DEBUGGER_CMD "gdb"
#define DEBUGGER_ARG_FORMAT "--pid=%d"
#define DEBUGGER_BREAK_AFTER_WAIT 0

/*
Start the debugger and tell it to attach to the current process/thread
(called by display_error)
*/
void udbg_launch_debugger()
{
	pid_t orgpid=getpid();
	pid_t ret=fork();
	if (ret == 0)
	{
		// Child Process: exec() gdb (Debugger), set to attach to old fork
		char buf[16];
		snprintf(buf, 16, DEBUGGER_ARG_FORMAT, orgpid);

		int ret=execlp(DEBUGGER_CMD, DEBUGGER_CMD, buf, NULL);
		// In case of success, we should never get here anyway, though...
		if (ret != 0)
		{
			perror("Debugger launch failed");
		}
	}
	else if (ret > 0)
	{
		// Parent (original) fork:
		debug_printf("Sleeping until debugger attaches.\nPlease wait.\n");
		sleep(DEBUGGER_WAIT);
	}
	else // fork error, ret == -1
	{
		perror("Debugger launch: fork failed");
	}
}

#if OS_ANDROID

#include <android/log.h>

void debug_puts(const char* text)
{
	__android_log_print(ANDROID_LOG_WARN, "pyrogenesis", "%s", text);
}

#else

void debug_puts(const char* text)
{
	printf("%s", text);
	fflush(stdout);
}

#endif

int debug_IsPointerBogus(const void* UNUSED(p))
{
	// TODO: maybe this should do some checks
	return false;
}
