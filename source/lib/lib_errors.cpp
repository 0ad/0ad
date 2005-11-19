// note: this is called lib_errors.cpp because we have another
// errors.cpp; the MS linker isn't smart enough to deal with
// object files of the same name but in different paths.

#include "precompiled.h"

#include "lib_errors.h"
#include "sysdep/sysdep.h"

#include <string.h>
#include <stdlib.h>	// abs


static const char* lib_error_description(int err)
{
	// not in our range
	if(!(ERR_MIN <= abs(err) && abs(err) < ERR_MAX))
		return 0;

	switch(err)
	{
#define ERR(err, id, str) case id: return str;
#include "lib_errors.h"
	default: return "Unknown lib error";
	}
	UNREACHABLE;
}


void error_description_r(int err, char* buf, size_t max_chars)
{
	// lib error
	const char* str = lib_error_description(err);
	if(str)
	{
		// <err> was one of our error codes (chosen so as not to conflict
		// with any others), so we're done.
		strcpy_s(buf, max_chars, str);
		return;
	}

	// Win32 GetLastError and errno both define values in [0,100).
	// what we'll do is always try the OS-specific translation,
	// add libc's interpretation if <err> is a valid errno, and
	// output "Unknown" if none of the above succeeds.
	const bool should_display_libc_err = (0 <= err && err < sys_nerr);
	bool have_output = false;

	// OS-specific error
	if(sys_error_description_r(err, buf, max_chars) == 0)	// success
	{
		have_output = true;

		// add a separator text before libc description
		if(should_display_libc_err)
			strcat_s(buf, max_chars, "; libc err=");
	}

	// libc error
	if(should_display_libc_err)
	{
		strcat_s(buf, max_chars, strerror(err));
		// note: we are sure to get meaningful output (not just "unknown")
		// because err < sys_nerr.
		have_output = true;
	}

	// fallback
	if(!have_output)
		snprintf(buf, max_chars, "Unknown error (%d, 0x%X)", err, err);
}


const char* error_description(int err)
{
	static char buf[200];
	error_description_r(err, buf, ARRAY_SIZE(buf));
	return buf;
}