/* Copyright (c) 2011 Wildfire Games
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


// linked list (most recent first)
// note: no memory is allocated, all nodes are static instances.
//
// rationale: don't use a std::map. we don't care about lookup speed,
// dynamic allocation would be ugly, and returning a local static object
// from a function doesn't work, either (the compiler generates calls to
// atexit, which leads to disaster since we're sometimes called before
// the CRT has initialized)
static StatusDefinition* definitions;

int StatusAddDefinition(StatusDefinition* def)
{
	// insert at front of list
	def->next = definitions;
	definitions = def;

	return 0;	// stored in dummy variable
}


static const StatusDefinition* DefinitionFromStatus(Status status)
{
	for(const StatusDefinition* def = definitions; def; def = def->next)
	{
		if(def->status == status)
			return def;
	}

	return 0;
}


static const StatusDefinition* DefinitionFromErrno(int errno_equivalent)
{
	for(const StatusDefinition* def = definitions; def; def = def->next)
	{
		if(def->errno_equivalent == errno_equivalent)
			return def;
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

	swprintf_s(buf, max_chars, L"Unknown error (%d, 0x%X)", (int)status, (unsigned int)status);
	return buf;
}


int ErrnoFromStatus(Status status)
{
	const StatusDefinition* def = DefinitionFromStatus(status);
	if(def && def->errno_equivalent != -1)
		return def->errno_equivalent;

	// the set of errnos in wposix.h doesn't have an "unknown error".
	// we use this one as a default because it's not expected to come up often.
	return EPERM;
}


Status StatusFromErrno()
{
	const StatusDefinition* def = DefinitionFromErrno(errno);
	return def? def->status : ERR::FAIL;
}


//-----------------------------------------------------------------------------

// INFO::OK doesn't really need a string because calling StatusDescription(0)
// should never happen, but we'll play it safe.
STATUS_DEFINE(INFO, OK, L"(but return value was 0 which indicates success)", -1);
STATUS_DEFINE(ERR, FAIL, L"Function failed (no details available)", -1);

STATUS_DEFINE(INFO, CB_CONTINUE,    L"Continue (not an error)", -1);
STATUS_DEFINE(INFO, SKIPPED,        L"Skipped (not an error)", -1);
STATUS_DEFINE(INFO, CANNOT_HANDLE,  L"Cannot handle (not an error)", -1);
STATUS_DEFINE(INFO, ALL_COMPLETE,   L"All complete (not an error)", -1);
STATUS_DEFINE(INFO, ALREADY_EXISTS, L"Already exists (not an error)", -1);

STATUS_DEFINE(ERR, LOGIC, L"Logic error in code", -1);
STATUS_DEFINE(ERR, TIMED_OUT, L"Timed out", -1);
STATUS_DEFINE(ERR, REENTERED, L"Single-call function was reentered", -1);
STATUS_DEFINE(ERR, CORRUPTED, L"File/memory data is corrupted", -1);
STATUS_DEFINE(ERR, VERSION,   L"Version mismatch", -1);
STATUS_DEFINE(ERR, ABORTED,   L"Operation aborted", -1);

STATUS_DEFINE(ERR, INVALID_PARAM, L"Invalid function argument", EINVAL);
STATUS_DEFINE(ERR, INVALID_HANDLE, L"Invalid Handle (argument)", -1);
STATUS_DEFINE(ERR, BUF_SIZE, L"Buffer argument too small", -1);
STATUS_DEFINE(ERR, AGAIN, L"Try again later", -1);
STATUS_DEFINE(ERR, LIMIT, L"Fixed limit exceeded", -1);
STATUS_DEFINE(ERR, NO_SYS, L"OS doesn't provide a required API", -1);
STATUS_DEFINE(ERR, NOT_IMPLEMENTED, L"Feature currently not implemented", ENOSYS);
STATUS_DEFINE(ERR, NOT_SUPPORTED, L"Feature isn't and won't be supported", -1);
STATUS_DEFINE(ERR, NO_MEM, L"Not enough memory", ENOMEM);

STATUS_DEFINE(ERR, _1, L"Case 1", -1);
STATUS_DEFINE(ERR, _2, L"Case 2", -1);
STATUS_DEFINE(ERR, _3, L"Case 3", -1);
STATUS_DEFINE(ERR, _4, L"Case 4", -1);
STATUS_DEFINE(ERR, _5, L"Case 5", -1);
STATUS_DEFINE(ERR, _6, L"Case 6", -1);
STATUS_DEFINE(ERR, _7, L"Case 7", -1);
STATUS_DEFINE(ERR, _8, L"Case 8", -1);
STATUS_DEFINE(ERR, _9, L"Case 9", -1);
STATUS_DEFINE(ERR, _11, L"Case 11", -1);
STATUS_DEFINE(ERR, _12, L"Case 12", -1);
STATUS_DEFINE(ERR, _13, L"Case 13", -1);
STATUS_DEFINE(ERR, _14, L"Case 14", -1);
STATUS_DEFINE(ERR, _15, L"Case 15", -1);
STATUS_DEFINE(ERR, _16, L"Case 16", -1);
STATUS_DEFINE(ERR, _17, L"Case 17", -1);
STATUS_DEFINE(ERR, _18, L"Case 18", -1);
STATUS_DEFINE(ERR, _19, L"Case 19", -1);
STATUS_DEFINE(ERR, _21, L"Case 21", -1);
STATUS_DEFINE(ERR, _22, L"Case 22", -1);
STATUS_DEFINE(ERR, _23, L"Case 23", -1);
STATUS_DEFINE(ERR, _24, L"Case 24", -1);
STATUS_DEFINE(ERR, _25, L"Case 25", -1);
STATUS_DEFINE(ERR, _26, L"Case 26", -1);
STATUS_DEFINE(ERR, _27, L"Case 27", -1);
STATUS_DEFINE(ERR, _28, L"Case 28", -1);
STATUS_DEFINE(ERR, _29, L"Case 29", -1);
