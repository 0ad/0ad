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

/**

Error handling system


Why Error Codes?
----------------

To convey information about what failed, the alternatives are unique
integral codes and direct pointers to descriptive text. Both occupy the
same amount of space, but codes are easier to internationalize.


Method of Propagating Errors
----------------------------

When a low-level function has failed, this must be conveyed to the
higher-level application logic across several functions on the call stack.
There are two alternatives:
1) check at each call site whether a function failed;
   if so, return to the caller.
2) throw an exception.

We will discuss the advantages and disadvantages of exceptions,
which are the opposites of call site checking.
- performance: they shouldn't be used in time-critical code.
- predictability: exceptions can come up almost anywhere,
  so it is hard to say what execution path will be taken.
- interoperability: not compatible with other languages.
+ readability: cleans up code by separating application logic and
  error handling. however, this is also a disadvantage because it
  may be difficult to see at a glance if a piece of code does
  error checking at all.
+ visibility: errors are more likely to be seen than relying on
  callers to check return codes; less reliant on discipline.

Both have their place. Our recommendation is to throw error code
exceptions when checking call sites and propagating errors becomes tedious.
However, inter-module boundaries should always return error codes for
interoperability with other languages.


Simplifying Call-Site Checking
------------------------------

As mentioned above, this approach requires discipline. We provide
"enforcer" macros to simplify this task by propagating errors to
the calling function.

Consider the following example:
  Status status = doWork();
  if(status != INFO::OK)
    return status;
This can be replaced by:
  RETURN_STATUS_IF_ERR(doWork());

This provides a visible sign that the code handles errors but
reduces clutter.


When to warn the user?
----------------------

When a function fails, there are 2 places we can raise a warning:
as soon as the error condition is known, or higher on the call stack.

We prefer the former because it is easier to ensure that all
possible return paths have been covered: search for all "return ERR::*"
or "return StatusFrom*" that are not followed by a "// NOWARN" comment.
The latter approach also risks multiple warnings along the
call stack for the same error.

Note the special case of "validator" functions that e.g. verify the
state of an object: we now discuss pros/cons of just returning errors
without warning, and having their callers take care of that.
+ they typically have many return paths (-> increased code size)
- this is balanced by validators that have many call sites.
- we want all return statements wrapped for consistency and
  easily checking if any were forgotten
- adding // NOWARN to each validator return statement would be tedious.
- there is no advantage to checking at the call site; the call stack
  indicates which caller of the validator failed anyway.
Validator functions should therefore also use WARN_RETURN.


Numbering Scheme
----------------

Each module header defines its own error codes to avoid a full rebuild
whenever a new code is added.

Error codes start at -100000 (warnings are positive, but the
corresponding negative value should not be used to avoid confusion).
This scheme avoids collisions with all other known error codes.

Each header gets 100 possible values; the tens value may be
used to denote groups within that header.

The subsystem is denoted by the ten-thousands digit:
0 general
1 file
2 res (resource management)
3 sysdep (system-dependent)
4 win (Windows-specific)

To summarize: +/-1SHHCC (S=subsystem, HH=header, CC=code number)

10     general
  00CC misc
  03CC path
  04CC debug
  05CC debug_stl
  06CC secure_crt
  07CC wchar

11     file
  01CC vfs
  03CC file
  04CC archive

12     res
  00CC h_mgr
  01CC tex

13     sysdep
  00CC cpu
  01CC os_cpu

14     win
  00CC whrt
**/

#ifndef INCLUDED_STATUS
#define INCLUDED_STATUS

#include "lib/lib_api.h"

// an integral type allows defining error codes in separate headers,
// but is not as type-safe as an enum. use Lint's 'strong type' checking
// to catch errors such as Status Func() { return 1; }.
// this must be i64 because some functions may multiplex Status with
// file offsets/sizes in their return value.
typedef i64 Status;

// associates a status code with a description [and errno_equivalent].
struct StatusDefinition	// POD
{
	Status status;

	// typically a string literal; must remain valid until end of program.
	const wchar_t* description;

	// omit initializer (or initialize to 0) if there is no errno equivalent.
	int errno_equivalent;
};

// retrieving description and errno_equivalent requires going through all
// StatusDefinition instances. we avoid dynamic memory allocation (which
// is problematic because status codes may be needed before _cinit) by
// organizing them into a linked list, with nodes residing in static storage.
// since modules may introduce many status codes, they are stored in an
// array, aka "bucket", which includes a link to the next bucket.
// initialized via STATUS_ADD_DEFINITIONS; opaque.
struct StatusDefinitionBucket	// POD
{
	const StatusDefinition* definitions;
	size_t numDefinitions;
	StatusDefinitionBucket* next;
};

/**
 * (called via STATUS_ADD_DEFINITIONS)
 *
 * @param bucket is being added; its definitions and numDefinitions must
 *   already be initialized.
 * @return previous bucket in list, suitable for initializing bucket->next.
 *
 * (this function must be callable as a static initializer; initializing
 * next avoids the need for a separate dummy variable)
 **/
LIB_API StatusDefinitionBucket* StatusAddDefinitions(StatusDefinitionBucket* bucket);

/**
 * add a module's array of StatusDefinition to the list.
 * typically invoked at file scope.
 * @param definitions name (identifier) of the array
 **/
#define STATUS_ADD_DEFINITIONS(definitions) static StatusDefinitionBucket definitions##_bucket = { definitions, ARRAY_SIZE(definitions), StatusAddDefinitions(&definitions##_bucket) }


/**
 * generate textual description of a Status.
 *
 * @param buf destination buffer (allows generating strings with
 *   the code's numerical value if no definition is found)
 * @param max_chars size of buffer [characters]
 * @return buf (allows using this function in expressions)
 **/
LIB_API wchar_t* StatusDescription(Status status, wchar_t* buf, size_t max_chars);

/**
 * @return the errno equivalent of a Status.
 *
 * used in wposix - underlying functions return Status but must be
 * translated to errno at e.g. the mmap interface level. higher-level code
 * that calls mmap will in turn convert back to Status.
 **/
extern int ErrnoFromStatus(Status status);

/**
 * @return Status equivalent of errno, or ERR::FAIL if there's no equivalent.
 *
 * NB: reset errno to 0 before calling POSIX functions to avoid confusion
 * with previous errors.
 **/
extern Status StatusFromErrno();

// note: other conversion routines (e.g. to/from Win32) are implemented in
// the corresponding modules to keep this header portable.


//-----------------------------------------------------------------------------
// propagation macros

// warn and return a status. use when an error is first detected to
// begin propagating it to callers.
#define WARN_RETURN(status)\
	do\
	{\
		DEBUG_WARN_ERR(status);\
		return status;\
	}\
	while(0)

// warn if expression is negative, i.e. an error.
// (this macro is more convenient than ENSURE)
#define WARN_IF_ERR(expression)\
	do\
	{\
		const Status status_ = (expression);\
		if(status_ < 0)\
			DEBUG_WARN_ERR(status_);\
	}\
	while(0)

// return expression if it is negative, i.e. pass on errors to
// the caller. use when failures are common/expected.
#define RETURN_STATUS_IF_ERR(expression)\
	do\
	{\
		const Status status_ = (expression);\
		if(status_ < 0)\
			return status_;\
	}\
	while(0)

// warn and return expression if it is negative.
// use if a function doesn't raise warnings when it returns errors.
#define WARN_RETURN_STATUS_IF_ERR(expression)\
	do\
	{\
		const Status status_ = (expression);\
		if(status_ < 0)\
		{\
			DEBUG_WARN_ERR(status_);\
			return status_;\
		}\
	}\
	while(0)

// warn and throw a status. use when an error is first detected to
// begin propagating it to callers.
#define WARN_THROW(status)\
	do\
	{\
		DEBUG_WARN_ERR(status);\
		throw status;\
	}\
	while(0)

// throw expression if it is negative. use to propagate
// expected errors from constructors.
#define THROW_STATUS_IF_ERR(expression)\
	do\
	{\
		const Status status_ = (expression);\
		if(status_ < 0)\
			throw status_;\
	}\
	while(0)

// warn and throw expression if it is negative. use to propagate
// errors from constructors.
#define WARN_THROW_STATUS_IF_ERR(expression)\
	do\
	{\
		const Status status_ = (expression);\
		if(status_ < 0)\
		{\
			DEBUG_WARN_ERR(status_);\
			throw status_;\
		}\
	}\
	while(0)

// if expression (typically the invocation of a callback) evaluates to:
// - INFO::OK, do nothing;
// - INFO::ALL_COMPLETE, return INFO::OK;
// - anything else, return that.
#define RETURN_STATUS_FROM_CALLBACK(expression)\
	do\
	{\
		const Status status_ = (expression);\
		if(status_ == INFO::ALL_COMPLETE)\
			return INFO::OK;\
		else if(status_ != INFO::OK)\
			return status_;\
	}\
	while(0)

// return 0 if expression is negative. use in functions that return pointers.
#define RETURN_0_IF_ERR(expression)\
	do\
	{\
		const Status status_ = (expression);\
		if(status_ < 0)\
			return 0;\
	}\
	while(0)

// warn if expression is false, i.e. zero.
#define WARN_IF_FALSE(expression)\
	do\
	{\
		if(!(expression))\
			debug_warn(L"FYI: WARN_IF_FALSE reports that a function failed. Feel free to ignore or suppress this warning.");\
	}\
	while(0)

// warn and return 0 if expression is false, i.e. zero.
#define WARN_RETURN_0_IF_FALSE(expression)\
	do\
	{\
		if(!(expression))\
		{\
			debug_warn(L"FYI: WARN_RETURN_0_IF_FALSE reports that a function failed. Feel free to ignore or suppress this warning.");\
			return 0;\
		}\
	}\
	while(0)


//-----------------------------------------------------------------------------
// shared status code definitions

namespace INFO {

	const Status OK = 0;

	// note: these values are > 100 to allow multiplexing them with
	// coroutine return values, which return completion percentage.

	// notify caller that nothing was done.
	const Status SKIPPED       = +100001;

	// function is incapable of doing the requested task with the given inputs.
	// this implies SKIPPED, but also conveys a bit more information.
	const Status CANNOT_HANDLE = +100002;

	// function is meant to be called repeatedly, and now indicates that
	// all jobs are complete.
	const Status ALL_COMPLETE  = +100003;

}	// namespace INFO

namespace ERR {

	const Status FAIL = -1;	// unknown failure

	// general
	const Status LOGIC     = -100010;
	const Status EXCEPTION = -100011;
	const Status TIMED_OUT = -100012;
	const Status REENTERED = -100013;
	const Status CORRUPTED = -100014;
	const Status ABORTED   = -100015;

	// invalid values (usually function arguments)
	const Status INVALID_ALIGNMENT = -100020;
	const Status INVALID_OFFSET    = -100021;
	const Status INVALID_HANDLE    = -100022;
	const Status INVALID_POINTER   = -100023;
	const Status INVALID_SIZE      = -100024;
	const Status INVALID_FLAG      = -100025;
	const Status INVALID_PARAM     = -100026;
	const Status INVALID_VERSION   = -100027;

	// system limitations
	const Status AGAIN           = -100030;
	const Status LIMIT           = -100031;
	const Status NOT_SUPPORTED   = -100032;
	const Status NO_MEM          = -100033;

	// these are for cases where we just want a distinct value to display and
	// a symbolic name + string would be overkill (e.g. the various
	// test cases in a validate() call). they are shared between multiple
	// functions; when something fails, the stack trace will show in which
	// one it was => these errors are unambiguous.
	// there are 3 tiers - 1..9 are used in most functions, 11..19 are
	// used in a function that calls another validator and 21..29 are
	// for for functions that call 2 other validators (this avoids
	// ambiguity as to which error actually happened where)
	const Status _1  = -100101;
	const Status _2  = -100102;
	const Status _3  = -100103;
	const Status _4  = -100104;
	const Status _5  = -100105;
	const Status _6  = -100106;
	const Status _7  = -100107;
	const Status _8  = -100108;
	const Status _9  = -100109;
	const Status _11 = -100111;
	const Status _12 = -100112;
	const Status _13 = -100113;
	const Status _14 = -100114;
	const Status _15 = -100115;
	const Status _16 = -100116;
	const Status _17 = -100117;
	const Status _18 = -100118;
	const Status _19 = -100119;
	const Status _21 = -100121;
	const Status _22 = -100122;
	const Status _23 = -100123;
	const Status _24 = -100124;
	const Status _25 = -100125;
	const Status _26 = -100126;
	const Status _27 = -100127;
	const Status _28 = -100128;
	const Status _29 = -100129;

}	// namespace ERR

#endif	// #ifndef INCLUDED_STATUS
