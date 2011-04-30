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

/**

Error handling system


Introduction
------------

This module defines error codes, translates them to/from other systems
(e.g. errno), provides several macros that simplify returning errors /
checking if a function failed, and associates codes with descriptive text.


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
which mirror those of call site checking.
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
macros to simplify this task: function calls can be wrapped in an
"enforcer" that checks whether they succeeded and can take action
(e.g. returning to caller or warning user) as appropriate.

Consider the following example:
  LibError ret = doWork();
  if(ret != INFO::OK) { warnUser(ret); return ret; }
This can be replaced by:
  CHECK_ERR(doWork());

This provides a visible sign that the code handles errors,
automatically propagates errors back to the caller, and most importantly,
allows warning the user whenever an error occurs.
Thus, no errors can be swept under the carpet by failing to
check return value or catch(...) all exceptions.


When to warn the user?
----------------------

When a function fails, there are 2 places we can raise a warning:
as soon as the error condition is known, or in the higher-level caller.
The former is the WARN_RETURN(ERR::FAIL) approach, while the latter
corresponds to the example above.

We prefer the former because it is easier to ensure that all
possible return paths have been covered: search for all "return ERR::*"
that are not followed by a "// NOWARN" comment. Also, the latter approach
raises the question of where exactly to issue the warning.
Clearly API-level routines must raise the warning, but sometimes they will
want to call each other. Multiple warnings along the call stack ensuing
from the same root cause are not nice.

Note the special case of "validator" functions that e.g. verify the
state of an object: we now discuss pros/cons of just returning errors
without warning, and having their callers take care of that.
+ they typically have many return paths (-> increased code size)
- this is balanced by validators that have many call sites.
- we want all return statements wrapped for consistency and
  easily checking if any were forgotten
- adding // NOWARN to each validator return statement would be tedious.
- there is no advantage to checking at the call site; call stack indicates
  which caller of the validator failed anyway.
Validator functions should therefore also use WARN_RETURN.


Numbering Scheme
----------------

Each module header defines its own error codes to avoid a full rebuild
whenever a new code is added.

Error codes start at -100000 (warnings are positive, but reserves a
negative value; absolute values are unique). This avoids collisions
with all known error code schemes.

Each header gets 100 possible values; the tens value may be
used to denote groups within that header.

The subsystem is denoted by the ten-thousands digit:
0 lib
1 file
2 res (resource management)
3 sysdep (system-dependent)
4 win (Windows-specific)

To summarize: +/-1SHHCC (S=subsystem, HH=header, CC=code number)

10     lib
  00CC lib_errors
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
  01CC tex
  02CC ogl_shader

13     sysdep
  00CC cpu
  01CC os_cpu

14     win
  00CC whrt

Notes:
- file is called lib_errors.h because 0ad has another errors.cpp and
  the MS linker isn't smart enough to deal with object files
  of the same name but in different paths.
**/

#ifndef INCLUDED_LIB_ERRORS
#define INCLUDED_LIB_ERRORS

#include "lib/code_annotation.h"
#include "lib/code_generation.h"

// note: this loses compiler type safety (being able to prevent
// return 1 when a LibError is the return value), but allows splitting
// up the error namespace into separate headers.
// Lint's 'strong type' checking can be used to find errors.
typedef long LibError;

// opaque - do not access its fields!
// note: must be defined here because clients instantiate them;
// fields cannot be made private due to POD requirement.
struct LibErrorAssociation
{
	LibError err;

	// must remain valid until end of program.
	const wchar_t* description;

	// POSIX errno, or -1
	int errno_equivalent;

	LibErrorAssociation* next;
};

/**
 * associate a LibError with a description and errno equivalent.
 * @return dummy integer to allow calling via static initializer.
 **/
extern int error_AddAssociation(LibErrorAssociation*);

// associate a LibError with a description and errno equivalent.
// Invoke this at file or function scope.
#define ERROR_ASSOCIATE(err, description, errno_equivalent)\
	static LibErrorAssociation UID__ = { err, description, errno_equivalent, NULL };\
	static int UID2__ = error_AddAssociation(&UID__)


/**
 * generate textual description of an error code.
 *
 * @param err LibError to be translated. if despite type checking we
 * get an invalid enum value, the string will be something like
 * "Unknown error (65536, 0x10000)".
 * @param buf destination buffer
 * @param max_chars size of buffer [characters]
 * @return buf (allows using this function in expressions)
 **/
LIB_API wchar_t* error_description_r(LibError err, wchar_t* buf, size_t max_chars);

//-----------------------------------------------------------------------------

// conversion to/from other error code definitions.
// note: other conversion routines (e.g. to/from Win32) are implemented in
// the corresponding modules to keep this header portable.

/**
 * translate errno to LibError.
 *
 * should only be called directly after a POSIX function indicates failure;
 * errno may otherwise still be set from another error cause.
 *
 * @param warn_if_failed if set, raise a warning when returning an error
 * (i.e. ERR::*, but not INFO::OK). this avoids having to wrap all
 * call sites in WARN_ERR etc.
 * @return LibError equivalent of errno, or ERR::FAIL if there's no equal.
 **/
extern LibError LibError_from_errno(bool warn_if_failed = true);

/**
 * translate a POSIX function's return/error indication to LibError.
 *
 * you should set errno to 0 before calling the POSIX function to
 * make sure we do not return any stale errors. typical usage:
 * errno = 0;
 * int ret = posix_func(..);
 * return LibError_from_posix(ret);
 *
 * @param ret return value of a POSIX function: 0 indicates success,
 * -1 is error.
 * @param warn_if_failed if set, raise a warning when returning an error
 * (i.e. ERR::*, but not INFO::OK). this avoids having to wrap all
 * call sites in WARN_ERR etc.
 * @return INFO::OK if the POSIX function succeeded, else the LibError
 * equivalent of errno, or ERR::FAIL if there's no equal.
 **/
extern LibError LibError_from_posix(int ret, bool warn_if_failed = true);

/**
 * set errno to the equivalent of a LibError.
 *
 * used in wposix - underlying functions return LibError but must be
 * translated to errno at e.g. the mmap interface level. higher-level code
 * that calls mmap will in turn convert back to LibError.
 *
 * @param err error code to set
 **/
extern void LibError_set_errno(LibError err);


//-----------------------------------------------------------------------------

// be careful here. the given expression (e.g. variable or
// function return value) may be a Handle (=i64), so it needs to be
// stored and compared as such. (very large but legitimate Handle values
// casted to int can end up negative)
// all functions using this return LibError (instead of i64) for
// efficiency and simplicity. if the input was negative, it is an
// error code and is therefore known to fit; we still mask with
// UINT_MAX to avoid VC cast-to-smaller-type warnings.

// if expression evaluates to a negative error code, warn user and
// return the number.
#define CHECK_ERR(expression)\
STMT(\
	i64 err64__ = (i64)(expression);\
	if(err64__ < 0)\
	{\
		LibError err__ = (LibError)(err64__ & ULONG_MAX);\
		DEBUG_WARN_ERR(err__);\
		return (LibError)(err__ & ULONG_MAX);\
	}\
)

// just pass on errors without any kind of annoying warning
// (useful for functions that can legitimately fail).
#define RETURN_ERR(expression)\
STMT(\
	i64 err64__ = (i64)(expression);\
	if(err64__ < 0)\
	{\
		LibError err__ = (LibError)(err64__ & ULONG_MAX);\
		return err__;\
	}\
)


// if expression evaluates to a negative error code, return 0.
#define RETURN_IF_NOT_CONTINUE(expression)\
STMT(\
	i64 err64__ = (i64)(expression);\
	if(err64__ != INFO::CB_CONTINUE)\
		return err64__;\
)

// return an error and warn about it (replaces debug_warn+return)
#define WARN_RETURN(err)\
STMT(\
	DEBUG_WARN_ERR(err);\
	return err;\
)

// if expression evaluates to a negative error code, warn user and
// throw that number.
#define THROW_ERR(expression)\
STMT(\
	i64 err64__ = (i64)(expression);\
	if(err64__ < 0)\
	{\
		LibError err__ = (LibError)(err64__ & ULONG_MAX);\
		DEBUG_WARN_ERR(err__);\
		throw err__;\
	}\
)

// if expression evaluates to a negative error code, warn user and just return
// (useful for void functions that must bail and complain)
#define WARN_ERR_RETURN(expression)\
STMT(\
	i64 err64__ = (i64)(expression);\
	if(err64__ < 0)\
	{\
		LibError err__ = (LibError)(err64__ & ULONG_MAX);\
		DEBUG_WARN_ERR(err__);\
		return;\
	}\
)

// if expression evaluates to a negative error code, warn user
// (this is similar to ENSURE but also works in release mode)
#define WARN_ERR(expression)\
STMT(\
	i64 err64__ = (i64)(expression);\
	if(err64__ < 0)\
	{\
		LibError err__ = (LibError)(err64__ & ULONG_MAX);\
		DEBUG_WARN_ERR(err__);\
	}\
)


// if expression evaluates to a negative error code, return 0.
#define RETURN0_IF_ERR(expression)\
STMT(\
	i64 err64__ = (i64)(expression);\
	if(err64__ < 0)\
		return 0;\
)

// if ok evaluates to false or FALSE, warn user and return -1.
#define WARN_RETURN_IF_FALSE(ok)\
STMT(\
	if(!(ok))\
	{\
		debug_warn(L"FYI: WARN_RETURN_IF_FALSE reports that a function failed. "\
		           L"Feel free to ignore or suppress this warning.");\
		return ERR::FAIL;\
	}\
)

// if ok evaluates to false or FALSE, return -1.
#define RETURN_IF_FALSE(ok)\
STMT(\
	if(!(ok))\
		return ERR::FAIL;\
)

// if ok evaluates to false or FALSE, warn user.
#define WARN_IF_FALSE(ok)\
STMT(\
	if(!(ok))\
		debug_warn(L"FYI: WARN_IF_FALSE reports that a function failed. "\
		           L"Feel free to ignore or suppress this warning.");\
)


//-----------------------------------------------------------------------------

namespace INFO
{
const LibError OK = 0;

// note: these values are > 100 to allow multiplexing them with
// coroutine return values, which return completion percentage.

// function is a callback and indicates that it can (but need not
// necessarily) be called again.
const LibError CB_CONTINUE    = +100000;
// notify caller that nothing was done.
const LibError SKIPPED        = +100001;
// function is incapable of doing the requested task with the given inputs.
// this implies SKIPPED, but also conveys a bit more information.
const LibError CANNOT_HANDLE  = +100002;
// function is meant to be called repeatedly, and now indicates that
// all jobs are complete.
const LibError ALL_COMPLETE   = +100003;
// (returned e.g. when inserting into container)
const LibError ALREADY_EXISTS = +100004;
}

namespace ERR
{
const LibError FAIL = -1;

// general
const LibError LOGIC     = -100010;
const LibError TIMED_OUT = -100011;
const LibError REENTERED = -100012;
const LibError CORRUPTED = -100013;
const LibError VERSION   = -100014;

// function arguments
const LibError INVALID_PARAM  = -100020;
const LibError INVALID_HANDLE = -100021;
const LibError BUF_SIZE       = -100022;

// system limitations
const LibError AGAIN           = -100030;
const LibError LIMIT           = -100031;
const LibError NO_SYS          = -100032;
const LibError NOT_IMPLEMENTED = -100033;
const LibError NOT_SUPPORTED   = -100034;
const LibError NO_MEM          = -100035;

// these are for cases where we just want a distinct value to display and
// a symbolic name + string would be overkill (e.g. the various
// test cases in a validate() call). they are shared between multiple
// functions; when something fails, the stack trace will show in which
// one it was => these errors are unambiguous.
// there are 3 tiers - 1..9 are used in most functions, 11..19 are
// used in a function that calls another validator and 21..29 are
// for for functions that call 2 other validators (this avoids
// ambiguity as to which error actually happened where)
const LibError _1  = -100101;
const LibError _2  = -100102;
const LibError _3  = -100103;
const LibError _4  = -100104;
const LibError _5  = -100105;
const LibError _6  = -100106;
const LibError _7  = -100107;
const LibError _8  = -100108;
const LibError _9  = -100109;
const LibError _11 = -100111;
const LibError _12 = -100112;
const LibError _13 = -100113;
const LibError _14 = -100114;
const LibError _15 = -100115;
const LibError _16 = -100116;
const LibError _17 = -100117;
const LibError _18 = -100118;
const LibError _19 = -100119;
const LibError _21 = -100121;
const LibError _22 = -100122;
const LibError _23 = -100123;
const LibError _24 = -100124;
const LibError _25 = -100125;
const LibError _26 = -100126;
const LibError _27 = -100127;
const LibError _28 = -100128;
const LibError _29 = -100129;

}	// namespace ERR

#endif	// #ifndef INCLUDED_LIB_ERRORS
