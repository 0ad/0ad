#include "precompiled.h"

#include "sysdep.h"
#include <memory.h>

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


wchar_t MicroBuffer[MICROLOG_SIZE];
int MicroBuffer_off = 0;

void debug_microlog(const wchar_t *fmt, ...)
{
	va_list argp;
	wchar_t buffer[512];

	va_start(argp, fmt);
	vswprintf(buffer, sizeof(buffer), fmt, argp);
	va_end(argp);

	wcscat(buffer, L"\r\n");

	int len = (int)wcslen(buffer);

	// Lose half of the old data if the log's too big
	if (MicroBuffer_off + len >= MICROLOG_SIZE)
	{
		memcpy(&MicroBuffer, (void*)&MicroBuffer[MICROLOG_SIZE/2], sizeof(wchar_t)*MICROLOG_SIZE/2);
		memset(&MicroBuffer[MICROLOG_SIZE/2], 0, sizeof(wchar_t)*MICROLOG_SIZE/2);
		MicroBuffer_off -= MICROLOG_SIZE/2;
	}
	memcpy(&MicroBuffer[MicroBuffer_off], buffer, sizeof(wchar_t)*len);
	MicroBuffer_off += len;
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
