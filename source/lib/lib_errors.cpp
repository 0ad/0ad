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


// generate textual description of an error code.
// stores up to <max_chars> in the given buffer.
// <err> can be one of the above error codes, POSIX ENOENT etc., or
// an OS-specific errors. if unknown, the string will be something like
// "Unknown error (65536, 0x10000)".
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


// return the LibError equivalent of errno, or ERR_FAIL if
// there's no equal.
// only call after a POSIX function indicates failure.
LibError LibError_from_errno()
{
	switch(errno)
	{
	case ENOMEM:
		return ERR_NO_MEM;

	case EINVAL:
		return ERR_INVALID_PARAM;
	case ENOSYS:
		return ERR_NOT_IMPLEMENTED;

	case ENOENT:
		return ERR_PATH_NOT_FOUND;
	case EACCES:
		return ERR_FILE_ACCESS;
	case EIO:
		return ERR_IO;
	case ENAMETOOLONG:
		return ERR_PATH_LENGTH;

	default:
		return ERR_FAIL;
	}
	UNREACHABLE;
}


// translate the return value of any POSIX function into LibError.
// ret is typically to -1 to indicate error and 0 on success.
// you should set errno to 0 before calling the POSIX function to
// make sure we do not return any stale errors.
LibError LibError_from_posix(int ret)
{
	debug_assert(ret == 0 || ret == -1);
	return (ret == 0)? ERR_OK : LibError_from_errno();
}
