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

#include "lib/timer.h"
#include "lib/sysdep/sysdep.h"
#include "lib/debug.h"

// udbg stubs.
// (Linux has a better implementation using BFD)

void* udbg_get_nth_caller(size_t UNUSED(n), void *UNUSED(context))
{
	return NULL;
}

LibError udbg_dump_stack(wchar_t* UNUSED(buf), size_t UNUSED(max_chars), size_t UNUSED(skip), void* UNUSED(context))
{
	return ERR::NOT_IMPLEMENTED;
}

LibError udbg_resolve_symbol(void* UNUSED(ptr_of_interest), char* UNUSED(sym_name), char* UNUSED(file), int* UNUSED(line))
{
	return ERR::NOT_IMPLEMENTED;
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

void debug_puts(const char* text)
{
	fputs(text, stdout);
	fflush(stdout);
}

// TODO: Do these properly. (I don't know what I'm doing; I just
// know that these functions are required in order to compile...)

int debug_WriteCrashlog(const char* UNUSED(file), wchar_t* UNUSED(header),
	void* UNUSED(context))
{
	abort();
}

int debug_IsPointerBogus(const void* UNUSED(p))
{
	return false;
}

void debug_heap_check()
{
}

// if <full_monty> is true or PARANOIA #defined, all possible checks are
// performed as often as possible. this is really slow (we are talking x100),
// but reports errors closer to where they occurred.
void debug_heap_enable(int UNUSED(heapChecks))
{
	// No-op until we find out if glibc has heap debugging
}

// disable all automatic checks until the next debug_heap_enable.
void debug_heap_disable()
{
	// No-op until we find out if glibc has heap debugging
}
