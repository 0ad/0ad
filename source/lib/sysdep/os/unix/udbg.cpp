/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/* udbg.cpp

This file contains debug helpers that are common for all unix systems. See
linux/ldbg.cpp for the linux-specific stuff (Using BFD and backtrace() for
symbol lookups and backtraces)
*/

#include "precompiled.h"

#include <cstdio>

#include "lib/timer.h"
#include "lib/sysdep/sysdep.h"
#include "lib/debug.h"


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
		debug_printf(L"Sleeping until debugger attaches.\nPlease wait.\n");
		sleep(DEBUGGER_WAIT);
	}
	else // fork error, ret == -1
	{
		perror("Debugger launch: fork failed");
	}
}

void debug_puts(const wchar_t* text)
{
	wprintf(L"%ls", text);
	fflush(stdout);
}

int debug_IsPointerBogus(const void* UNUSED(p))
{
	// TODO: maybe this should do some checks
	return false;
}
