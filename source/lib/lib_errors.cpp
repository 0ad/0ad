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

#include <string.h>
#include <stdlib.h>	// abs
#include <map>

#include "lib/posix/posix_errno.h"
#include "lib/sysdep/sysdep.h"


struct LibErrorAssociation
{
	const char* description_;
	int errno_equivalent_;
	LibErrorAssociation()
		: description_(0), errno_equivalent_(-1) {}

	void setDescription(const char* description)
	{
		debug_assert(description_ == 0);	// not already set
		description_ = description;
	}

	void setEquivalent(int errno_equivalent)
	{
		debug_assert(errno_equivalent_ == -1);	// not already set
		errno_equivalent_ = errno_equivalent;
	}
};
typedef std::map<LibError, LibErrorAssociation> LibErrorAssociations;

// wrapper required because error_set* is called from AT_STARTUP
// (potentially before our static map is constructed)
static LibErrorAssociations& associations()
{
	static LibErrorAssociations associations_;
	return associations_;
}

void error_setDescription(LibError err, const char* description)
{
	associations()[err].setDescription(description);
}

void error_setEquivalent(LibError err, int errno_equivalent)
{
	associations()[err].setEquivalent(errno_equivalent);
}


// generate textual description of an error code.
// stores up to <max_chars> in the given buffer.
// if error is unknown/invalid, the string will be something like
// "Unknown error (65536, 0x10000)".
char* error_description_r(LibError err, char* buf, size_t max_chars)
{
	// lib error
	LibErrorAssociations::iterator it = associations().find(err);
	if(it != associations().end())
	{
		const LibErrorAssociation& a = it->second;
		if(a.description_)
		{
			strcpy_s(buf, max_chars, a.description_);
			return buf;
		}
	}

	// unknown
	snprintf(buf, max_chars, "Unknown error (%d, 0x%X)", err, err);
	return buf;
}


// return the LibError equivalent of errno, or ERR::FAIL if there's no equal.
// only call after a POSIX function indicates failure.
// raises a warning (avoids having to on each call site).
LibError LibError_from_errno(bool warn_if_failed)
{
	LibError ret = ERR::FAIL;

	LibErrorAssociations::iterator it;
	for(it = associations().begin(); it != associations().end(); ++it)
	{
		const LibErrorAssociation& a = it->second;
		if(a.errno_equivalent_ == errno)
		{
			ret = it->first;
			break;
		}
	}
	
	if(warn_if_failed)
		DEBUG_WARN_ERR(ret);
	return ret;
}

// translate the return value of any POSIX function into LibError.
// ret is typically to -1 to indicate error and 0 on success.
// you should set errno to 0 before calling the POSIX function to
// make sure we do not return any stale errors.
LibError LibError_from_posix(int ret, bool warn_if_failed)
{
	debug_assert(ret == 0 || ret == -1);
	if(ret == 0)
		return INFO::OK;
	return LibError_from_errno(warn_if_failed);
}


// return the errno.h equivalent of <err>.
// does not assign to errno (this simplifies code by allowing direct return)
static int return_errno_from_LibError(LibError err)
{
	LibErrorAssociations::iterator it = associations().find(err);
	if(it != associations().end())
	{
		const LibErrorAssociation& a = it->second;
		if(a.errno_equivalent_ != -1)
			return a.errno_equivalent_;
	}

	// somewhat of a quandary: the set of errnos in wposix.h doesn't
	// have an "unknown error". we pick EPERM because we don't expect
	// that to come up often otherwise.
	return EPERM;
}


// set errno to the equivalent of <err>. used in wposix - underlying
// functions return LibError but must be translated to errno at
// e.g. the mmap interface level. higher-level code that calls mmap will
// in turn convert back to LibError.
void LibError_set_errno(LibError err)
{
	errno = return_errno_from_LibError(err);
}


//-----------------------------------------------------------------------------

AT_STARTUP(\
	/* INFO::OK doesn't really need a string because calling error_description_r(0) should never happen, but go the safe route. */\
	error_setDescription(INFO::OK, "(but return value was 0 which indicates success)");\
	error_setDescription(ERR::FAIL, "Function failed (no details available)");\
	\
	error_setDescription(INFO::CB_CONTINUE,    "Continue (not an error)");\
	error_setDescription(INFO::SKIPPED,        "Skipped (not an error)");\
	error_setDescription(INFO::CANNOT_HANDLE,  "Cannot handle (not an error)");\
	error_setDescription(INFO::ALL_COMPLETE,   "All complete (not an error)");\
	error_setDescription(INFO::ALREADY_EXISTS, "Already exists (not an error)");\
	\
	error_setDescription(ERR::LOGIC, "Logic error in code");\
	error_setDescription(ERR::TIMED_OUT, "Timed out");\
	error_setDescription(ERR::REENTERED, "Single-call function was reentered");\
	error_setDescription(ERR::CORRUPTED, "File/memory data is corrupted");\
	\
	error_setDescription(ERR::INVALID_PARAM, "Invalid function argument");\
	error_setDescription(ERR::INVALID_HANDLE, "Invalid Handle (argument)");\
	error_setDescription(ERR::BUF_SIZE, "Buffer argument too small");\
	error_setDescription(ERR::AGAIN, "Try again later");\
	error_setDescription(ERR::LIMIT, "Fixed limit exceeded");\
	error_setDescription(ERR::NO_SYS, "OS doesn't provide a required API");\
	error_setDescription(ERR::NOT_IMPLEMENTED, "Feature currently not implemented");\
	error_setDescription(ERR::NOT_SUPPORTED, "Feature isn't and won't be supported");\
	error_setDescription(ERR::NO_MEM, "Not enough memory");\
	\
	error_setDescription(ERR::_1, "Case 1");\
	error_setDescription(ERR::_2, "Case 2");\
	error_setDescription(ERR::_3, "Case 3");\
	error_setDescription(ERR::_4, "Case 4");\
	error_setDescription(ERR::_5, "Case 5");\
	error_setDescription(ERR::_6, "Case 6");\
	error_setDescription(ERR::_7, "Case 7");\
	error_setDescription(ERR::_8, "Case 8");\
	error_setDescription(ERR::_9, "Case 9");\
	error_setDescription(ERR::_11, "Case 11");\
	error_setDescription(ERR::_12, "Case 12");\
	error_setDescription(ERR::_13, "Case 13");\
	error_setDescription(ERR::_14, "Case 14");\
	error_setDescription(ERR::_15, "Case 15");\
	error_setDescription(ERR::_16, "Case 16");\
	error_setDescription(ERR::_17, "Case 17");\
	error_setDescription(ERR::_18, "Case 18");\
	error_setDescription(ERR::_19, "Case 19");\
	error_setDescription(ERR::_21, "Case 21");\
	error_setDescription(ERR::_22, "Case 22");\
	error_setDescription(ERR::_23, "Case 23");\
	error_setDescription(ERR::_24, "Case 24");\
	error_setDescription(ERR::_25, "Case 25");\
	error_setDescription(ERR::_26, "Case 26");\
	error_setDescription(ERR::_27, "Case 27");\
	error_setDescription(ERR::_28, "Case 28");\
	error_setDescription(ERR::_29, "Case 29");\
)


AT_STARTUP(\
	error_setEquivalent(ERR::NO_MEM, ENOMEM);\
	error_setEquivalent(ERR::INVALID_PARAM, EINVAL);\
	error_setEquivalent(ERR::NOT_IMPLEMENTED, ENOSYS);\
)
