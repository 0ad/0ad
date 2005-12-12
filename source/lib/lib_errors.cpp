// note: this is called lib_errors.cpp because we have another
// errors.cpp; the MS linker isn't smart enough to deal with
// object files of the same name but in different paths.

#include "precompiled.h"

#include "lib_errors.h"
#include "sysdep/sysdep.h"

#include <string.h>
#include <stdlib.h>	// abs


static const char* LibError_description(LibError err)
{
	// not in our range
	const int ierr = abs((int)err);
	if(!(ERR_MIN <= ierr && ierr < ERR_MAX))
		return 0;

	switch(err)
	{
#define ERR(err, id, str) case id: return str;
#include "lib_errors.h"
	default: return "Unknown error";
	}
	UNREACHABLE;
}


// generate textual description of an error code.
// stores up to <max_chars> in the given buffer.
// if error is unknown/invalid, the string will be something like
// "Unknown error (65536, 0x10000)".
void error_description_r(LibError err, char* buf, size_t max_chars)
{
	// lib error
	const char* str = LibError_description(err);
	if(str)
	{
		// <err> was one of our error codes (chosen so as not to conflict
		// with any others), so we're done.
		strcpy_s(buf, max_chars, str);
		return;
	}

	// fallback
	snprintf(buf, max_chars, "Unknown error (%d, 0x%X)", err, err);
}


// return the LibError equivalent of errno, or ERR_FAIL if
// there's no equal.
// only call after a POSIX function indicates failure.
LibError LibError_from_errno()
{
	switch(errno)
	{
	case ENOMEM: return ERR_NO_MEM;

	case EINVAL: return ERR_INVALID_PARAM;
	case ENOSYS: return ERR_NOT_IMPLEMENTED;

	case ENOENT: return ERR_PATH_NOT_FOUND;
	case EACCES: return ERR_FILE_ACCESS;
	case EIO: return ERR_IO;
	case ENAMETOOLONG: return ERR_PATH_LENGTH;

	default: return ERR_FAIL;
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


// return the errno.h equivalent of <err>.
// does not assign to errno (this simplifies code by allowing direct return)
static int return_errno_from_LibError(LibError err)
{
	switch(err)
	{
	case ERR_NO_MEM: return ENOMEM;

	case ERR_INVALID_PARAM: return EINVAL;
	case ERR_NOT_IMPLEMENTED: return ENOSYS;

	case ERR_PATH_NOT_FOUND: return ENOENT;
	case ERR_FILE_ACCESS: return EACCES;
	case ERR_IO: return EIO;
	case ERR_PATH_LENGTH: return ENAMETOOLONG;

	// somewhat of a quandary: the set of errnos in wposix.h doesn't
	// have an "unknown error". we pick EPERM because we don't expect
	// that to come up often otherwise.
	default: return EPERM;
	}
}

// set errno to the equivalent of <err>. used in wposix - underlying
// functions return LibError but must be translated to errno at
// e.g. the mmap interface level. higher-level code that calls mmap will
// in turn convert back to LibError.
void LibError_set_errno(LibError err)
{
	errno = return_errno_from_LibError(err);
}
