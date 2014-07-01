/* Copyright (c) 2014 Wildfire Games
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

#include "lib/utf8.h"
#include "lib/sysdep/sysdep.h"
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

bool sys_IsDebuggerPresent()
{
	return false;
}

std::wstring sys_WideFromArgv(const char* argv_i)
{
	// argv is usually UTF-8 on Linux, unsure about OS X..
	return wstring_from_utf8(argv_i);
}

// these are basic POSIX-compatible backends for the sysdep.h functions.
// Win32 has better versions which override these.

void sys_display_msg(const wchar_t* caption, const wchar_t* msg)
{
	fprintf(stderr, "%ls: %ls\n", caption, msg); // must not use fwprintf, since stderr is byte-oriented
}

#if OS_MACOSX || OS_ANDROID
static ErrorReactionInternal try_gui_display_error(const wchar_t* text, bool manual_break, bool allow_suppress, bool no_continue)
{
	// TODO: implement this, in a way that doesn't rely on X11
	// and doesn't occasionally cause crazy errors like
	// "The process has forked and you cannot use this
	// CoreFoundation functionality safely. You MUST exec()."

	return ERI_NOT_IMPLEMENTED;
}
#else
static ErrorReactionInternal try_gui_display_error(const wchar_t* text, bool manual_break, bool allow_suppress, bool no_continue)
{
	// We'll run xmessage via fork/exec.
	// To avoid bad interaction between fork and pthreads, the child process
	// should only call async-signal-safe functions before exec.
	// So prepare all the child's data in advance, before forking:

	Status err; // ignore UTF-8 errors
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
	{
		for(char* const* a = argv; *a; ++a)
			free(*a);
		return ERI_NOT_IMPLEMENTED;
	}

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
		return ERI_NOT_IMPLEMENTED;

	switch(WEXITSTATUS(status))
	{
	case 103: // Debugger
		udbg_launch_debugger();
		//-fallthrough

	case 102: // Break
		if(manual_break)
			return ERI_BREAK;
		debug_break();
		return ERI_CONTINUE;

	case 100: // Continue
		if(!no_continue)
			return ERI_CONTINUE;
		// continue isn't allowed, so this was invalid input.
		return ERI_NOT_IMPLEMENTED;

	case 101: // Suppress
		if(allow_suppress)
			return ERI_SUPPRESS;
		// suppress isn't allowed, so this was invalid input.
		return ERI_NOT_IMPLEMENTED;

	case 104: // Exit
		abort();
		return ERI_EXIT;	// placebo; never reached

	}

	// Unexpected return value - fall back to the non-GUI prompt
	return ERI_NOT_IMPLEMENTED;
}
#endif

ErrorReactionInternal sys_display_error(const wchar_t* text, size_t flags)
{
	debug_printf(L"%ls\n\n", text);

	const bool manual_break   = (flags & DE_MANUAL_BREAK  ) != 0;
	const bool allow_suppress = (flags & DE_ALLOW_SUPPRESS) != 0;
	const bool no_continue    = (flags & DE_NO_CONTINUE   ) != 0;

	// Try the GUI prompt if possible
	ErrorReactionInternal ret = try_gui_display_error(text, manual_break, allow_suppress, no_continue);
	if (ret != ERI_NOT_IMPLEMENTED)
		return ret;

#if OS_ANDROID
	// Android has no easy way to get user input here,
	// so continue or exit automatically
	if(no_continue)
		abort();
	else
		return ERI_CONTINUE;
#else
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
				return ERI_BREAK;
			debug_break();
			return ERI_CONTINUE;

		case 'c': case 'C':
			if(!no_continue)
				return ERI_CONTINUE;
			// continue isn't allowed, so this was invalid input. loop again.
			break;
		case 's': case 'S':
			if(allow_suppress)
				return ERI_SUPPRESS;
			// suppress isn't allowed, so this was invalid input. loop again.
			break;

		case 'e': case 'E':
			abort();
			return ERI_EXIT;	// placebo; never reached
		}
	}
#endif
}


Status sys_StatusDescription(int err, wchar_t* buf, size_t max_chars)
{
	UNUSED2(err);
	UNUSED2(buf);
	UNUSED2(max_chars);

	// don't need to do anything: lib/errors.cpp already queries
	// libc's strerror(). if we ever end up needing translation of
	// e.g. Qt or X errors, that'd go here.
	return ERR::FAIL;
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

#if OS_ANDROID
#warning TODO: sys_get_user_name: do something more appropriate and more thread-safe
	char* buf = getlogin();
	if (buf)
		return std::wstring(buf, buf + strlen(buf));
#else
	char buf[256];
	if (getlogin_r(buf, ARRAY_SIZE(buf)) == 0)
		return std::wstring(buf, buf + strlen(buf));
#endif

	return L"";
}

Status sys_generate_random_bytes(u8* buf, size_t count)
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

Status sys_get_proxy_config(const std::wstring& UNUSED(url), std::wstring& UNUSED(proxy))
{
	return INFO::SKIPPED;
}

Status sys_open_url(const std::string& url)
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

FILE* sys_OpenFile(const OsPath& pathname, const char* mode)
{
	return fopen(OsString(pathname).c_str(), mode);
}
