/* Copyright (c) 2010 Wildfire Games
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

#include "precompiled.h"

#include <unistd.h>
#include <stdio.h>
#include <wchar.h>

#include "lib/external_libraries/sdl.h"
#include "lib/utf8.h"
#include "lib/sysdep/sysdep.h"
#include "lib/sysdep/cursor.h"
#include "udbg.h"

#include <boost/algorithm/string/replace.hpp>

#define GNU_SOURCE
#include <dlfcn.h>

#include <sys/wait.h>

#if OS_MACOSX
#define URL_OPEN_COMMAND "open"
#else
#define URL_OPEN_COMMAND "xdg-open"
#endif

// these are basic POSIX-compatible backends for the sysdep.h functions.
// Win32 has better versions which override these.

void sys_display_msg(const wchar_t* caption, const wchar_t* msg)
{
	fprintf(stderr, "%ls: %ls\n", caption, msg); // must not use fwprintf, since stderr is byte-oriented
}

static ErrorReaction try_gui_display_error(const wchar_t* text, bool manual_break, bool allow_suppress, bool no_continue)
{
	// We'll run xmessage via fork/exec.
	// To avoid bad interaction between fork and pthreads, the child process
	// should only call async-signal-safe functions before exec.
	// So prepare all the child's data in advance, before forking:

	LibError err; // ignore UTF-8 errors
	std::string message = utf8_from_wstring(text, &err);

	// Replace CRLF->LF
	boost::algorithm::replace_all(message, "\r\n", "\n");

	// TODO: we ought to wrap the text if it's very long,
	// since xmessage doesn't do that and it'll get clamped
	// to the screen width

	const char* cmd = "/usr/bin/xmessage";

	char buttons[256] = "";
	const char* defaultButton = "Exit";

	if(!no_continue)
	{
		strcat_s(buttons, sizeof(buttons), "Continue:100,");
		defaultButton = "Continue";
	}

	if(allow_suppress)
		strcat_s(buttons, sizeof(buttons), "Suppress:101,");

	strcat_s(buttons, sizeof(buttons), "Break:102,Debugger:103,Exit:104");

	// Since execv wants non-const strings, we strdup them all here
	// and will clean them up later (except in the child process where
	// memory leaks don't matter)
	char* const argv[] = {
		strdup(cmd),
		strdup("-geometry"), strdup("x500"), // set height so the box will always be very visible
		strdup("-title"), strdup("0 A.D. message"), // TODO: maybe shouldn't hard-code app name
		strdup("-buttons"), strdup(buttons),
		strdup("-default"), strdup(defaultButton),
		strdup(message.c_str()),
		NULL
	};

	pid_t cpid = fork();
	if(cpid == -1)
		return ER_NOT_IMPLEMENTED;

	if(cpid == 0)
	{
		// This is the child process

		// Set ASCII charset, to avoid font warnings from xmessage
		setenv("LC_ALL", "C", 1);

		// NOTE: setenv is not async-signal-safe, so we shouldn't really use
		// it here (it might want some mutex that was held by another thread
		// in the parent process and that will never be freed within this
		// process). But setenv/getenv are not guaranteed reentrant either,
		// and this error-reporting function might get called from a non-main
		// thread, so we can't just call setenv before forking as it might
		// break the other threads. And we can't just clone environ manually
		// inside the parent thread and use execve, because other threads might
		// be calling setenv and will break our iteration over environ.
		// In the absence of a good easy solution, and given that this is only
		// an error-reporting function and shouldn't get called frequently,
		// we'll just do setenv after the fork and hope that it fails
		// extremely rarely.

		execv(cmd, argv);

		// If exec returns, it failed
		//fprintf(stderr, "Error running %s: %d\n", cmd, errno);
		exit(-1);
	}

	// This is the parent process

	// Avoid memory leaks
	for(char* const* a = argv; *a; ++a)
		free(*a);

	int status = 0;
	waitpid(cpid, &status, 0);

	// If it didn't exist successfully, fall back to the non-GUI prompt
	if(!WIFEXITED(status))
		return ER_NOT_IMPLEMENTED;

	switch(WEXITSTATUS(status))
	{
	case 103: // Debugger
		udbg_launch_debugger();
		//-fallthrough

	case 102: // Break
		if(manual_break)
			return ER_BREAK;
		debug_break();
		return ER_CONTINUE;

	case 100: // Continue
		if(!no_continue)
			return ER_CONTINUE;
		// continue isn't allowed, so this was invalid input.
		return ER_NOT_IMPLEMENTED;

	case 101: // Suppress
		if(allow_suppress)
			return ER_SUPPRESS;
		// suppress isn't allowed, so this was invalid input.
		return ER_NOT_IMPLEMENTED;

	case 104: // Exit
		abort();
		return ER_EXIT;	// placebo; never reached

	}

	// Unexpected return value - fall back to the non-GUI prompt
	return ER_NOT_IMPLEMENTED;
}

ErrorReaction sys_display_error(const wchar_t* text, size_t flags)
{
	printf("%ls\n\n", text);

	const bool manual_break   = (flags & DE_MANUAL_BREAK  ) != 0;
	const bool allow_suppress = (flags & DE_ALLOW_SUPPRESS) != 0;
	const bool no_continue    = (flags & DE_NO_CONTINUE   ) != 0;

	// Try the GUI prompt if possible
	ErrorReaction ret = try_gui_display_error(text, manual_break, allow_suppress, no_continue);
	if (ret != ER_NOT_IMPLEMENTED)
		return ret;

	// Otherwise fall back to the terminal-based input

	// Loop until valid input given:
	for(;;)
	{
		if(!no_continue)
			printf("(C)ontinue, ");
		if(allow_suppress)
			printf("(S)uppress, ");
		printf("(B)reak, Launch (D)ebugger, or (E)xit?\n");
		// TODO Should have some kind of timeout here.. in case you're unable to
		// access the controlling terminal (As might be the case if launched
		// from an xterm and in full-screen mode)
		int c = getchar();
		// note: don't use tolower because it'll choke on EOF
		switch(c)
		{
		case EOF:
		case 'd': case 'D':
			udbg_launch_debugger();
			//-fallthrough

		case 'b': case 'B':
			if(manual_break)
				return ER_BREAK;
			debug_break();
			return ER_CONTINUE;

		case 'c': case 'C':
			if(!no_continue)
				return ER_CONTINUE;
			// continue isn't allowed, so this was invalid input. loop again.
			break;
		case 's': case 'S':
			if(allow_suppress)
				return ER_SUPPRESS;
			// suppress isn't allowed, so this was invalid input. loop again.
			break;

		case 'e': case 'E':
			abort();
			return ER_EXIT;	// placebo; never reached
		}
	}
}


LibError sys_error_description_r(int err, wchar_t* buf, size_t max_chars)
{
	UNUSED2(err);
	UNUSED2(buf);
	UNUSED2(max_chars);

	// don't need to do anything: lib/errors.cpp already queries
	// libc's strerror(). if we ever end up needing translation of
	// e.g. Qt or X errors, that'd go here.
	return ERR::FAIL;
}

// stub for sys_cursor_create - we don't need to implement this (SDL/X11 only
// has monochrome cursors so we need to use the software cursor anyways)

// note: do not return ERR_NOT_IMPLEMENTED or similar because that
// would result in WARN_ERRs.
LibError sys_cursor_create(size_t w, size_t h, void* bgra_img, size_t hx, size_t hy, sys_cursor* cursor)
{
	UNUSED2(w);
	UNUSED2(h);
	UNUSED2(hx);
	UNUSED2(hy);
	UNUSED2(bgra_img);

	*cursor = 0;
	return INFO::OK;
}

// creates an empty cursor
LibError sys_cursor_create_empty(sys_cursor* cursor)
{
	/* bitmap for a fully transparent cursor */
	u8 data[] = {0};
	u8 mask[] = {0};

	// size 8x1 (cursor size must be a whole number of bytes ^^)
	// hotspot at 0,0
	// SDL will make its own copies of data and mask
	*cursor = SDL_CreateCursor(data, mask, 8, 1, 0, 0);

	return cursor ? INFO::OK : ERR::FAIL;
}

SDL_Cursor *defaultCursor=NULL;
// replaces the current system cursor with the one indicated. need only be
// called once per cursor; pass 0 to restore the default.
LibError sys_cursor_set(sys_cursor cursor)
{
	// Gaah, SDL doesn't have a good API for setting the default cursor
	// SetCursor(NULL) just /repaints/ the cursor (well, obviously! or...)
	if(!defaultCursor)
		defaultCursor = SDL_GetCursor();

	// restore default cursor.
	if(!cursor)
		SDL_SetCursor(defaultCursor);

	SDL_SetCursor((SDL_Cursor *)cursor);

	return INFO::OK;
}

// destroys the indicated cursor and frees its resources. if it is
// currently the system cursor, the default cursor is restored first.
LibError sys_cursor_free(sys_cursor cursor)
{
	// bail now to prevent potential confusion below; there's nothing to do.
	if(!cursor)
		return INFO::OK;

	// if the cursor being freed is active, restore the default cursor
	// (just for safety).
	if (SDL_GetCursor() == (SDL_Cursor *)cursor)
		WARN_ERR(sys_cursor_set(NULL));

	SDL_FreeCursor((SDL_Cursor *)cursor);

	return INFO::OK;
}

LibError sys_cursor_reset()
{
	defaultCursor = NULL;

	return INFO::OK;
}

// note: just use the sector size: Linux aio doesn't really care about
// the alignment of buffers/lengths/offsets, so we'll just pick a
// sane value and not bother scanning all drives.
size_t sys_max_sector_size()
{
	// users may call us more than once, so cache the results.
	static size_t cached_sector_size;
	if(!cached_sector_size)
		cached_sector_size = sysconf(_SC_PAGE_SIZE);
	return cached_sector_size;
}

std::wstring sys_get_user_name()
{
	// Prefer LOGNAME, fall back on getlogin

	const char* logname = getenv("LOGNAME");
	if (logname && strcmp(logname, "") != 0)
		return std::wstring(logname, logname + strlen(logname));
	// TODO: maybe we should do locale conversion?

	char buf[256];
	if (getlogin_r(buf, ARRAY_SIZE(buf)) == 0)
		return std::wstring(buf, buf + strlen(buf));

	return L"";
}

LibError sys_generate_random_bytes(u8* buf, size_t count)
{
	FILE* f = fopen("/dev/urandom", "rb");
	if (!f)
		WARN_RETURN(ERR::FAIL);
	
	while (count)
	{
		size_t numread = fread(buf, 1, count, f);
		if (numread == 0)
			WARN_RETURN(ERR::FAIL);
		buf += numread;
		count -= numread;
	}

	fclose(f);

	return INFO::OK;
}

LibError sys_open_url(const std::string& url)
{
	pid_t pid = fork();
	if (pid < 0)
	{
		debug_warn(L"Fork failed");
		return ERR::FAIL;
	}
	else if (pid == 0)
	{
		// we are the child

		execlp(URL_OPEN_COMMAND, URL_OPEN_COMMAND, url.c_str(), (const char*)NULL);

		debug_printf(L"Failed to run '" URL_OPEN_COMMAND "' command\n");

		// We can't call exit() because that'll try to free resources which were the parent's,
		// so just abort here
		abort();
	}
	else
	{
		// we are the parent

		// TODO: maybe we should wait for the child and make sure it succeeded

		return INFO::OK;
	}
}
