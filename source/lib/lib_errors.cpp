/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * error handling system: defines error codes, associates them with
 * descriptive text, simplifies error notification.
 */

// note: this is called lib_errors.cpp because we have another
// errors.cpp; the MS linker isn't smart enough to deal with
// object files of the same name but in different paths.

#include "precompiled.h"
#include "lib/lib_errors.h"

#include <cstring>
#include <cstdlib>	// abs
#include <cstdio>
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


wchar_t* error_description_r(LibError err, wchar_t* buf, size_t max_chars)
{
	// lib error
	const LibErrorAssociation* lea = AssociationFromLibError(err);
	if(lea)
	{
		wcscpy_s(buf, max_chars, lea->description);
		return buf;
	}

	// unknown
	swprintf_s(buf, max_chars, L"Unknown error (%d, 0x%X)", (int)err, (unsigned int)err);
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
ERROR_ASSOCIATE(INFO::OK, L"(but return value was 0 which indicates success)", -1);
ERROR_ASSOCIATE(ERR::FAIL, L"Function failed (no details available)", -1);

ERROR_ASSOCIATE(INFO::CB_CONTINUE,    L"Continue (not an error)", -1);
ERROR_ASSOCIATE(INFO::SKIPPED,        L"Skipped (not an error)", -1);
ERROR_ASSOCIATE(INFO::CANNOT_HANDLE,  L"Cannot handle (not an error)", -1);
ERROR_ASSOCIATE(INFO::ALL_COMPLETE,   L"All complete (not an error)", -1);
ERROR_ASSOCIATE(INFO::ALREADY_EXISTS, L"Already exists (not an error)", -1);

ERROR_ASSOCIATE(ERR::LOGIC, L"Logic error in code", -1);
ERROR_ASSOCIATE(ERR::TIMED_OUT, L"Timed out", -1);
ERROR_ASSOCIATE(ERR::REENTERED, L"Single-call function was reentered", -1);
ERROR_ASSOCIATE(ERR::CORRUPTED, L"File/memory data is corrupted", -1);

ERROR_ASSOCIATE(ERR::INVALID_PARAM, L"Invalid function argument", EINVAL);
ERROR_ASSOCIATE(ERR::INVALID_HANDLE, L"Invalid Handle (argument)", -1);
ERROR_ASSOCIATE(ERR::BUF_SIZE, L"Buffer argument too small", -1);
ERROR_ASSOCIATE(ERR::AGAIN, L"Try again later", -1);
ERROR_ASSOCIATE(ERR::LIMIT, L"Fixed limit exceeded", -1);
ERROR_ASSOCIATE(ERR::NO_SYS, L"OS doesn't provide a required API", -1);
ERROR_ASSOCIATE(ERR::NOT_IMPLEMENTED, L"Feature currently not implemented", ENOSYS);
ERROR_ASSOCIATE(ERR::NOT_SUPPORTED, L"Feature isn't and won't be supported", -1);
ERROR_ASSOCIATE(ERR::NO_MEM, L"Not enough memory", ENOMEM);

ERROR_ASSOCIATE(ERR::_1, L"Case 1", -1);
ERROR_ASSOCIATE(ERR::_2, L"Case 2", -1);
ERROR_ASSOCIATE(ERR::_3, L"Case 3", -1);
ERROR_ASSOCIATE(ERR::_4, L"Case 4", -1);
ERROR_ASSOCIATE(ERR::_5, L"Case 5", -1);
ERROR_ASSOCIATE(ERR::_6, L"Case 6", -1);
ERROR_ASSOCIATE(ERR::_7, L"Case 7", -1);
ERROR_ASSOCIATE(ERR::_8, L"Case 8", -1);
ERROR_ASSOCIATE(ERR::_9, L"Case 9", -1);
ERROR_ASSOCIATE(ERR::_11, L"Case 11", -1);
ERROR_ASSOCIATE(ERR::_12, L"Case 12", -1);
ERROR_ASSOCIATE(ERR::_13, L"Case 13", -1);
ERROR_ASSOCIATE(ERR::_14, L"Case 14", -1);
ERROR_ASSOCIATE(ERR::_15, L"Case 15", -1);
ERROR_ASSOCIATE(ERR::_16, L"Case 16", -1);
ERROR_ASSOCIATE(ERR::_17, L"Case 17", -1);
ERROR_ASSOCIATE(ERR::_18, L"Case 18", -1);
ERROR_ASSOCIATE(ERR::_19, L"Case 19", -1);
ERROR_ASSOCIATE(ERR::_21, L"Case 21", -1);
ERROR_ASSOCIATE(ERR::_22, L"Case 22", -1);
ERROR_ASSOCIATE(ERR::_23, L"Case 23", -1);
ERROR_ASSOCIATE(ERR::_24, L"Case 24", -1);
ERROR_ASSOCIATE(ERR::_25, L"Case 25", -1);
ERROR_ASSOCIATE(ERR::_26, L"Case 26", -1);
ERROR_ASSOCIATE(ERR::_27, L"Case 27", -1);
ERROR_ASSOCIATE(ERR::_28, L"Case 28", -1);
ERROR_ASSOCIATE(ERR::_29, L"Case 29", -1);
