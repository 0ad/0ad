#include "precompiled.h"

#include <cstdio>
#include <cstdarg>

// See declaration in sysdep.h for explanation of need

int sys_vsnprintf(char* buffer, size_t count, const char* format, va_list argptr)
{
	int ret = vsnprintf(buffer, count, format, argptr);

	/*
	"The glibc implementation of the functions snprintf() and vsnprintf() conforms
	to the C99 standard ... since glibc version 2.1. Until glibc 2.0.6 they would
	return -1 when the output was truncated."
	- man printf

	MSVC's _vsnprintf still returns -1, so we want this one to do the same (for
	compatibility), if the output (including the terminating null) is truncated.
	*/

	if (ret >= (int)count)
		return -1;

	return ret;
}
