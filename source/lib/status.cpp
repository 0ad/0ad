/* Copyright (C) 2011 Wildfire Games.
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
 * error handling system: defines status codes, translates them to/from
 * other schemes (e.g. errno), associates them with descriptive text,
 * simplifies propagating errors / checking if functions failed.
 */

#include "precompiled.h"
#include "lib/status.h"

#include <cstring>
#include <cstdio>

#include "lib/posix/posix_errno.h"


static StatusDefinitionBucket* buckets;

StatusDefinitionBucket* StatusAddDefinitions(StatusDefinitionBucket* bucket)
{
	// insert at front of list
	StatusDefinitionBucket* next = buckets;
	buckets = bucket;
	return next;
}


static const StatusDefinition* DefinitionFromStatus(Status status)
{
	for(const StatusDefinitionBucket* bucket = buckets; bucket; bucket = bucket->next)
	{
		for(size_t i = 0; i < bucket->numDefinitions; i++)
		{
			if(bucket->definitions[i].status == status)
				return &bucket->definitions[i];
		}
	}

	return 0;
}


static const StatusDefinition* DefinitionFromErrno(int errno_equivalent)
{
	for(const StatusDefinitionBucket* bucket = buckets; bucket; bucket = bucket->next)
	{
		for(size_t i = 0; i < bucket->numDefinitions; i++)
		{
			if(bucket->definitions[i].errno_equivalent == errno_equivalent)
				return &bucket->definitions[i];
		}
	}

	return 0;
}


wchar_t* StatusDescription(Status status, wchar_t* buf, size_t max_chars)
{
	const StatusDefinition* def = DefinitionFromStatus(status);
	if(def)
	{
		wcscpy_s(buf, max_chars, def->description);
		return buf;
	}

	swprintf_s(buf, max_chars, L"Unknown error (%lld, 0x%llX)", (long long)status, (unsigned long long)status);
	return buf;
}


int ErrnoFromStatus(Status status)
{
	const StatusDefinition* def = DefinitionFromStatus(status);
	if(def && def->errno_equivalent != 0)
		return def->errno_equivalent;

	// the set of errnos in wposix.h doesn't have an "unknown error".
	// we use this one as a default because it's not expected to come up often.
	return EPERM;
}


Status StatusFromErrno()
{
	if(errno == 0)
		return INFO::OK;
	const StatusDefinition* def = DefinitionFromErrno(errno);
	return def? def->status : ERR::FAIL;
}


//-----------------------------------------------------------------------------

static const StatusDefinition statusDefs[] = {

// INFO::OK doesn't really need a string because calling StatusDescription(0)
// should never happen, but we'll play it safe.
{ INFO::OK,             L"No error reported here" },
{ ERR::FAIL,            L"Function failed (no details available)" },

{ INFO::SKIPPED,        L"Skipped (not an error)" },
{ INFO::CANNOT_HANDLE,  L"Cannot handle (not an error)" },
{ INFO::ALL_COMPLETE,   L"All complete (not an error)" },

{ ERR::LOGIC,           L"Logic error in code" },
{ ERR::EXCEPTION,       L"Caught an exception" },
{ ERR::TIMED_OUT,       L"Timed out" },
{ ERR::REENTERED,       L"Single-call function was reentered" },
{ ERR::CORRUPTED,       L"File/memory data is corrupted" },
{ ERR::ABORTED,         L"Operation aborted" },

{ ERR::INVALID_ALIGNMENT, L"Invalid alignment", EINVAL },
{ ERR::INVALID_OFFSET,    L"Invalid offset",    EINVAL },
{ ERR::INVALID_HANDLE,    L"Invalid handle",    EINVAL },
{ ERR::INVALID_POINTER,   L"Invalid pointer",   EINVAL },
{ ERR::INVALID_SIZE,      L"Invalid size",      EINVAL },
{ ERR::INVALID_FLAG,      L"Invalid flag",      EINVAL },
{ ERR::INVALID_PARAM,     L"Invalid parameter", EINVAL },
{ ERR::INVALID_VERSION,   L"Invalid version",   EINVAL },

{ ERR::AGAIN,           L"Try again later", EAGAIN },
{ ERR::LIMIT,           L"Fixed limit exceeded", E2BIG },
{ ERR::NOT_SUPPORTED,   L"Function not supported", ENOSYS },
{ ERR::NO_MEM,          L"Not enough memory", ENOMEM},

{ ERR::_1, L"Case 1" },
{ ERR::_2, L"Case 2" },
{ ERR::_3, L"Case 3" },
{ ERR::_4, L"Case 4" },
{ ERR::_5, L"Case 5" },
{ ERR::_6, L"Case 6" },
{ ERR::_7, L"Case 7" },
{ ERR::_8, L"Case 8" },
{ ERR::_9, L"Case 9" },
{ ERR::_11, L"Case 11" },
{ ERR::_12, L"Case 12" },
{ ERR::_13, L"Case 13" },
{ ERR::_14, L"Case 14" },
{ ERR::_15, L"Case 15" },
{ ERR::_16, L"Case 16" },
{ ERR::_17, L"Case 17" },
{ ERR::_18, L"Case 18" },
{ ERR::_19, L"Case 19" },
{ ERR::_21, L"Case 21" },
{ ERR::_22, L"Case 22" },
{ ERR::_23, L"Case 23" },
{ ERR::_24, L"Case 24" },
{ ERR::_25, L"Case 25" },
{ ERR::_26, L"Case 26" },
{ ERR::_27, L"Case 27" },
{ ERR::_28, L"Case 28" },
{ ERR::_29, L"Case 29" }

};
STATUS_ADD_DEFINITIONS(statusDefs);
