#include <stdio.h>
#include <stdarg.h>

#include "sysdep.h"

// portable debug output routines. Win32 offers better versions, which
// override these.

#ifndef _WIN32

// portable output routines (win.cpp overrides these)
void display_msg(const wchar_t* caption, const wchar_t* msg)
{
	fwprintf(stderr, L"%ws: %ws\n", caption, msg);
}


void debug_out(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}


void check_heap()
{
}

#endif	// #ifndef _WIN32