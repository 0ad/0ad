#include "precompiled.h"

#include "sysdep/sysdep.h"

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef OS_LINUX
# define DEBUGGER_WAIT 3
# define DEBUGGER_CMD "gdb"
# define DEBUGGER_ARG_FORMAT "--pid=%d"
# define DEBUGGER_BREAK_AFTER_WAIT 0
#else
# error "port"
#endif

void unix_debug_break()
{
	kill(getpid(), SIGTRAP);
}

/*
Start the debugger and tell it to attach to the current process/thread
*/
int unix_launch_debugger()
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
		return 2;
	}
	else if (ret > 0)
	{
		// Parent (original) fork:
		sleep(DEBUGGER_WAIT);
#if DEBUGGER_BREAK_AFTER_WAIT
		unix_debug_break();
#endif
	}
	else // fork error, ret == -1
	{
		perror("Debugger launch: fork failed");
		return 2;
	}
	return 0;
}

/*
 * return values:
 *   0 - continue
 *   1 - suppress
 *   2 - break
 */
int debug_assert_failed(const char *file, int line, const char *expr)
{
	printf("%s:%d: Assertion `%s' failed.\n", file, line, expr);
	do
	{
		printf("(B)reak, Launch (D)ebugger, (C)ontinue, (S)uppress or (A)bort? ");
		// TODO Should have some kind of timeout here.. in case you're unable to
		// access the controlling terminal (As might be the case if launched
		// from an xterm and in full-screen mode)
		int c=getchar();
		if (c == EOF) // I/O Error
			return 2;
		c=tolower(c);
		switch (c)
		{
			case 'b':
				return 2;
			case 'd':
				return unix_launch_debugger();
			case 'c':
				return 0;
			case 's':
				return 1;
			case 'a':
				abort();
			default:
				continue;
		}
	} while (false);
}
