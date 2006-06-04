/**
 * =========================================================================
 * File        : lib_errors.cpp
 * Project     : 0 A.D.
 * Description : error handling system: defines error codes, associates
 *             : them with descriptive text, simplifies error notification.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

// note: this is called lib_errors.cpp because we have another
// errors.cpp; the MS linker isn't smart enough to deal with
// object files of the same name but in different paths.

#include "precompiled.h"

#include "lib_errors.h"
#include "lib/sysdep/sysdep.h"

#include <string.h>
#include <stdlib.h>	// abs
#include <errno.h>

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


// return the LibError equivalent of errno, or ERR_FAIL if there's no equal.
// only call after a POSIX function indicates failure.
// raises a warning (avoids having to on each call site).
LibError LibError_from_errno(bool warn_if_failed)
{
	LibError err;
	switch(errno)
	{
	case ENOMEM: err = ERR_NO_MEM; break;

	case EINVAL: err =  ERR_INVALID_PARAM; break;
	case ENOSYS: err =  ERR_NOT_IMPLEMENTED; break;

	case ENOENT: err =  ERR_TNODE_NOT_FOUND; break;
	case EACCES: err =  ERR_FILE_ACCESS; break;
	case EIO:    err =  ERR_IO; break;
	case ENAMETOOLONG: err =  ERR_PATH_LENGTH; break;

	default:     err =  ERR_FAIL; break;
	}

	if(warn_if_failed)
		DEBUG_WARN_ERR(err);
	return err;
}

// translate the return value of any POSIX function into LibError.
// ret is typically to -1 to indicate error and 0 on success.
// you should set errno to 0 before calling the POSIX function to
// make sure we do not return any stale errors.
LibError LibError_from_posix(int ret, bool warn_if_failed)
{
	debug_assert(ret == 0 || ret == -1);
	if(ret == 0)
		return INFO_OK;
	return LibError_from_errno(warn_if_failed);
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

	case ERR_TNODE_NOT_FOUND: return ENOENT;
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
