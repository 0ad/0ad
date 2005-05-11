#include "precompiled.h"

#include "lib.h"
#include "sysdep.h"
#include <memory.h>
#include <stdarg.h>

// portable debug output routines. Win32 offers better versions, which
// override these.

#ifndef _WIN32

#include <stdio.h>
#include <wchar.h>

// misc. portable versions (win.cpp overrides these)

void display_msg(const char* caption, const char* msg)
{
	fprintf(stderr, "%s: %s\n", caption, msg);
}

void wdisplay_msg(const wchar_t* caption, const wchar_t* msg)
{
	fwprintf(stderr, L"%ls: %ls\n", caption, msg);
}


int get_executable_name(char* n_path, size_t buf_size)
{
	UNUSED(n_path);
	UNUSED(buf_size);
	return -ENOSYS;
}

#endif	// #ifndef _WIN32




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
