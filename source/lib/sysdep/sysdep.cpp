#include "precompiled.h"

#include "sysdep.h"

// portable debug output routines. Win32 offers better versions, which
// override these.

#ifndef _WIN32

#include <stdio.h>
#include <wchar.h>
#include <stdarg.h>

// portable output routines (win.cpp overrides these)

void display_msg(const char* caption, const char* msg)
{
	fprintf(stderr, "%s: %s\n", caption, msg);
}

void wdisplay_msg(const wchar_t* caption, const wchar_t* msg)
{
	fwprintf(stderr, L"%ls: %ls\n", caption, msg);
}



void debug_out(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	fflush(stdout);
}


void check_heap()
{
}

#endif	// #ifndef _WIN32


void debug_break()
{
#ifndef NDEBUG
# if defined(_WIN32)
	win_debug_break();
# elif defined(_M_IX86)
	ia32_debug_break();
# elif defined(OS_UNIX)
	unix_debug_break();
# else
#  error "port"
# endif
#endif
}



#ifdef _MSC_VER

double round(double x)
{
	return (long)(x + 0.5);
}

#endif


#ifndef HAVE_C99

float fminf(float a, float b)
{
	return (a < b)? a : b;
}

float fmaxf(float a, float b)
{
	return (a > b)? a : b;
}

#endif
