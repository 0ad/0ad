/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * =========================================================================
 * File        : lib_errors.cpp
 * Project     : 0 A.D.
 * Description : error handling system: defines error codes, associates
 *             : them with descriptive text, simplifies error notification.
 * =========================================================================
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

// linked list (most recent first)
// note: no memory is allocated, all nodes are static instances.
//
// rationale: don't use a std::map. we don't care about lookup speed,
// dynamic allocation would be ugly, and returning a local static object
// from a function doesn't work, either (the compiler generates calls to
// atexit, which leads to disaster since we're sometimes called by
// winit functions before the CRT has initialized)
static LibErrorAssociation* associations;

int error_AddAssociation(LibErrorAssociation* lea)
{
	// insert at front of list
	lea->next = associations;
	associations = lea;

	return 0;	// stored in dummy variable
}

static const LibErrorAssociation* AssociationFromLibError(LibError err)
{
	for(const LibErrorAssociation* lea = associations; lea; lea = lea->next)
	{
		if(lea->err == err)
			return lea;
	}

	return 0;
}

static const LibErrorAssociation* AssociationFromErrno(errno_t errno_equivalent)
{
	for(const LibErrorAssociation* lea = associations; lea; lea = lea->next)
	{
		if(lea->errno_equivalent == errno_equivalent)
			return lea;
	}

	return 0;
}


char* error_description_r(LibError err, char* buf, size_t max_chars)
{
	// lib error
	const LibErrorAssociation* lea = AssociationFromLibError(err);
	if(lea)
	{
		strcpy_s(buf, max_chars, lea->description);
		return buf;
	}

	// unknown
	snprintf(buf, max_chars, "Unknown error (%d, 0x%X)", (int)err, (unsigned int)err);
	return buf;
}


LibError LibError_from_errno(bool warn_if_failed)
{
	LibError ret = ERR::FAIL;

	const LibErrorAssociation* lea = AssociationFromErrno(errno);
	if(lea)
		ret = lea->err;
	
	if(warn_if_failed)
		DEBUG_WARN_ERR(ret);
	return ret;
}


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
	const LibErrorAssociation* lea = AssociationFromLibError(err);
	if(lea && lea->errno_equivalent != -1)
		return lea->errno_equivalent;

	// somewhat of a quandary: the set of errnos in wposix.h doesn't
	// have an "unknown error". we pick EPERM because we don't expect
	// that to come up often otherwise.
	return EPERM;
}

void LibError_set_errno(LibError err)
{
	errno = return_errno_from_LibError(err);
}


//-----------------------------------------------------------------------------

// INFO::OK doesn't really need a string because calling error_description_r(0) should never happen, but go the safe route.
ERROR_ASSOCIATE(INFO::OK, "(but return value was 0 which indicates success)", -1);
ERROR_ASSOCIATE(ERR::FAIL, "Function failed (no details available)", -1);

ERROR_ASSOCIATE(INFO::CB_CONTINUE,    "Continue (not an error)", -1);
ERROR_ASSOCIATE(INFO::SKIPPED,        "Skipped (not an error)", -1);
ERROR_ASSOCIATE(INFO::CANNOT_HANDLE,  "Cannot handle (not an error)", -1);
ERROR_ASSOCIATE(INFO::ALL_COMPLETE,   "All complete (not an error)", -1);
ERROR_ASSOCIATE(INFO::ALREADY_EXISTS, "Already exists (not an error)", -1);

ERROR_ASSOCIATE(ERR::LOGIC, "Logic error in code", -1);
ERROR_ASSOCIATE(ERR::TIMED_OUT, "Timed out", -1);
ERROR_ASSOCIATE(ERR::REENTERED, "Single-call function was reentered", -1);
ERROR_ASSOCIATE(ERR::CORRUPTED, "File/memory data is corrupted", -1);
ERROR_ASSOCIATE(ERR::ASSERTION_FAILED, "Assertion failed", -1);

ERROR_ASSOCIATE(ERR::INVALID_PARAM, "Invalid function argument", EINVAL);
ERROR_ASSOCIATE(ERR::INVALID_HANDLE, "Invalid Handle (argument)", -1);
ERROR_ASSOCIATE(ERR::BUF_SIZE, "Buffer argument too small", -1);
ERROR_ASSOCIATE(ERR::AGAIN, "Try again later", -1);
ERROR_ASSOCIATE(ERR::LIMIT, "Fixed limit exceeded", -1);
ERROR_ASSOCIATE(ERR::NO_SYS, "OS doesn't provide a required API", -1);
ERROR_ASSOCIATE(ERR::NOT_IMPLEMENTED, "Feature currently not implemented", ENOSYS);
ERROR_ASSOCIATE(ERR::NOT_SUPPORTED, "Feature isn't and won't be supported", -1);
ERROR_ASSOCIATE(ERR::NO_MEM, "Not enough memory", ENOMEM);

ERROR_ASSOCIATE(ERR::_1, "Case 1", -1);
ERROR_ASSOCIATE(ERR::_2, "Case 2", -1);
ERROR_ASSOCIATE(ERR::_3, "Case 3", -1);
ERROR_ASSOCIATE(ERR::_4, "Case 4", -1);
ERROR_ASSOCIATE(ERR::_5, "Case 5", -1);
ERROR_ASSOCIATE(ERR::_6, "Case 6", -1);
ERROR_ASSOCIATE(ERR::_7, "Case 7", -1);
ERROR_ASSOCIATE(ERR::_8, "Case 8", -1);
ERROR_ASSOCIATE(ERR::_9, "Case 9", -1);
ERROR_ASSOCIATE(ERR::_11, "Case 11", -1);
ERROR_ASSOCIATE(ERR::_12, "Case 12", -1);
ERROR_ASSOCIATE(ERR::_13, "Case 13", -1);
ERROR_ASSOCIATE(ERR::_14, "Case 14", -1);
ERROR_ASSOCIATE(ERR::_15, "Case 15", -1);
ERROR_ASSOCIATE(ERR::_16, "Case 16", -1);
ERROR_ASSOCIATE(ERR::_17, "Case 17", -1);
ERROR_ASSOCIATE(ERR::_18, "Case 18", -1);
ERROR_ASSOCIATE(ERR::_19, "Case 19", -1);
ERROR_ASSOCIATE(ERR::_21, "Case 21", -1);
ERROR_ASSOCIATE(ERR::_22, "Case 22", -1);
ERROR_ASSOCIATE(ERR::_23, "Case 23", -1);
ERROR_ASSOCIATE(ERR::_24, "Case 24", -1);
ERROR_ASSOCIATE(ERR::_25, "Case 25", -1);
ERROR_ASSOCIATE(ERR::_26, "Case 26", -1);
ERROR_ASSOCIATE(ERR::_27, "Case 27", -1);
ERROR_ASSOCIATE(ERR::_28, "Case 28", -1);
ERROR_ASSOCIATE(ERR::_29, "Case 29", -1);
