/**
 * =========================================================================
 * File        : lib_errors.h
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

/*

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
  if(ret != INFO_OK) { warnUser(ret); return ret; }
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
The former is the WARN_RETURN(ERR_FAIL) approach, while the latter
corresponds to the example above.

We prefer the former because it is easier to ensure that all
possible return paths have been covered: search for all "return ERR_*"
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

Notes:
- file is called lib_errors.h because 0ad has another errors.cpp and
  the MS linker isn't smart enough to deal with object files
  of the same name but in different paths.
- the first part of this file is a normal header; the second contains
  X macros and is only active if ERR is defined (i.e. someone is
  including this header for the purpose of using them).
- unfortunately Intellisense isn't smart enough to pick up the
  ERR_* definitions. This is the price of automatically associating
  descriptive text with the error code.
*/

#ifndef ERRORS_H__
#define ERRORS_H__

// limits on the errors defined above (used by error_description_r)
#define ERR_MIN 100000
#define ERR_MAX 120000

// define error codes.
enum LibError {
#define ERR(err, id, str) id = err,
#include "lib_errors.h"
	// necessary because the enum would otherwise end with a comma
	// (which is often tolerated but not standards compliant).
	// note: we cannot rely on this being the last value (in case the
	// ERR x-macros aren't arranged in order), so don't use as such.
	LIB_ERROR_DUMMY
};


// generate textual description of an error code.
// stores up to <max_chars> in the given buffer.
// if error is unknown/invalid, the string will be something like
// "Unknown error (65536, 0x10000)".
extern void error_description_r(LibError err, char* buf, size_t max_chars);


//-----------------------------------------------------------------------------

// conversion to/from other error code definitions.
// notes:
// - these functions will raise a warning (before returning any error code
//   except INFO_OK) unless warn_if_failed is explicitly set to false.
// - other conversion routines (e.g. to/from Win32) are implemented in
//   the corresponding modules to keep this header portable.

// return the LibError equivalent of errno, or ERR_FAIL if there's no equal.
// only call after a POSIX function indicates failure.
// raises a warning (avoids having to on each call site).
extern LibError LibError_from_errno(bool warn_if_failed = true);

// translate the return value of any POSIX function into LibError.
// ret is typically to -1 to indicate error and 0 on success.
// you should set errno to 0 before calling the POSIX function to
// make sure we do not return any stale errors.
extern LibError LibError_from_posix(int ret, bool warn_if_failed = true);

// set errno to the equivalent of <err>. used in wposix - underlying
// functions return LibError but must be translated to errno at
// e.g. the mmap interface level. higher-level code that calls mmap will
// in turn convert back to LibError.
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
#if OS_WIN
#define CHECK_ERR(expression)\
STMT(\
	i64 err64 = (i64)(expression);\
	if(err64 < 0)\
	{\
		LibError err = (LibError)(err64 & UINT_MAX);\
		DEBUG_WARN_ERR(err);\
		return err;\
	}\
)
#else
#define CHECK_ERR(expression)\
STMT(\
	i64 err64 = (i64)(expression);\
	if(err64 < 0)\
	{\
		LibError err = (LibError)(err64 & UINT_MAX);\
		DEBUG_WARN_ERR(err);\
		return (LibError)(err & UINT_MAX);\
	}\
)
#endif

// just pass on errors without any kind of annoying warning
// (useful for functions that can legitimately fail, e.g. vfs_exists).
#define RETURN_ERR(expression)\
STMT(\
	i64 err64 = (i64)(expression);\
	if(err64 < 0)\
	{\
		LibError err = (LibError)(err64 & UINT_MAX);\
		return err;\
	}\
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
	i64 err64 = (i64)(expression);\
	if(err64 < 0)\
	{\
		LibError err = (LibError)(err64 & UINT_MAX);\
		DEBUG_WARN_ERR(err);\
		throw err;\
	}\
)

// if expression evaluates to a negative error code, warn user and just return
// (useful for void functions that must bail and complain)
#define WARN_ERR_RETURN(expression)\
STMT(\
	i64 err64 = (i64)(expression);\
	if(err64 < 0)\
	{\
		LibError err = (LibError)(err64 & UINT_MAX);\
		DEBUG_WARN_ERR(err);\
		return;\
	}\
)

// if expression evaluates to a negative error code, warn user
// (this is similar to debug_assert but also works in release mode)
#define WARN_ERR(expression)\
STMT(\
	i64 err64 = (i64)(expression);\
	if(err64 < 0)\
	{\
		LibError err = (LibError)(err64 & UINT_MAX);\
		DEBUG_WARN_ERR(err);\
	}\
)


// if expression evaluates to a negative error code, return 0.
#define RETURN0_IF_ERR(expression)\
STMT(\
	i64 err64 = (i64)(expression);\
	if(err64 < 0)\
		return 0;\
)

// if ok evaluates to false or FALSE, warn user and return -1.
#define WARN_RETURN_IF_FALSE(ok)\
STMT(\
	if(!(ok))\
	{\
		debug_warn("FYI: WARN_RETURN_IF_FALSE reports that a function failed."\
		           "feel free to ignore or suppress this warning.");\
		return ERR_FAIL;\
	}\
)

// if ok evaluates to false or FALSE, return -1.
#define RETURN_IF_FALSE(ok)\
STMT(\
	if(!(ok))\
		return ERR_FAIL;\
)

// if ok evaluates to false or FALSE, warn user.
#define WARN_IF_FALSE(ok)\
STMT(\
	if(!(ok))\
		debug_warn("FYI: WARN_IF_FALSE reports that a function failed."\
		           "feel free to ignore or suppress this warning.");\
)


#endif	// #ifndef ERRORS_H__

//-----------------------------------------------------------------------------

#ifdef ERR

// X macros: error code, symbolic name in code, user-visible string.
// error code is usually negative; positive denotes warnings.
//   if negative, absolute value must be within [ERR_MIN, ERR_MAX).

// INFO_OK doesn't really need a string, but must be part of enum LibError
// due to compiler checks. (and calling error_description_r(0) should
// never happen, but we set the text accordingly..)
ERR(0, INFO_OK, "(but return value was 0 which indicates success)")
ERR(-1, ERR_FAIL, "Function failed (no details available)")

// note: these values are > 100 to allow multiplexing them with
// coroutine return values, which return completion percentage.
ERR(101, INFO_CB_CONTINUE,     "1 (not an error)")
// these are all basically the same thing
ERR(102, INFO_CANNOT_HANDLE,   "2 (not an error)")
ERR(103, INFO_NO_REPLACE,      "3 (not an error)")
ERR(104, INFO_SKIPPED,         "4 (not an error)")
ERR(105, INFO_ALL_COMPLETE,    "5 (not an error)")
ERR(106, INFO_ALREADY_EXISTS, "6 (not an error)")

ERR(-100000, ERR_LOGIC, "Logic error in code")
ERR(-100001, ERR_TIMED_OUT, "Timed out")
ERR(-100002, ERR_STRING_NOT_TERMINATED, "Invalid string (no 0 terminator found in buffer)")


// these are for cases where we just want a distinct value to display and
// a symbolic name + string would be overkill (e.g. the various
// test cases in a validate() call). they are shared between multiple
// functions; when something fails, the stack trace will show in which
// one it was => these errors are unambiguous.
// there are 3 tiers - 1..9 are used in most functions, 11..19 are
// used in a function that calls another validator and 21..29 are
// for for functions that call 2 other validators (this avoids
// ambiguity as to which error actually happened where)
ERR(-100101, ERR_1, "Case 1")
ERR(-100102, ERR_2, "Case 2")
ERR(-100103, ERR_3, "Case 3")
ERR(-100104, ERR_4, "Case 4")
ERR(-100105, ERR_5, "Case 5")
ERR(-100106, ERR_6, "Case 6")
ERR(-100107, ERR_7, "Case 7")
ERR(-100108, ERR_8, "Case 8")
ERR(-100109, ERR_9, "Case 9")
ERR(-100111, ERR_11, "Case 11")
ERR(-100112, ERR_12, "Case 12")
ERR(-100113, ERR_13, "Case 13")
ERR(-100114, ERR_14, "Case 14")
ERR(-100115, ERR_15, "Case 15")
ERR(-100116, ERR_16, "Case 16")
ERR(-100117, ERR_17, "Case 17")
ERR(-100118, ERR_18, "Case 18")
ERR(-100119, ERR_19, "Case 19")
ERR(-100121, ERR_21, "Case 21")
ERR(-100122, ERR_22, "Case 22")
ERR(-100123, ERR_23, "Case 23")
ERR(-100124, ERR_24, "Case 24")
ERR(-100125, ERR_25, "Case 25")
ERR(-100126, ERR_26, "Case 26")
ERR(-100127, ERR_27, "Case 27")
ERR(-100128, ERR_28, "Case 28")
ERR(-100129, ERR_29, "Case 29")

// function arguments
ERR(-100220, ERR_INVALID_PARAM, "Invalid function argument")
ERR(-100221, ERR_INVALID_HANDLE, "Invalid Handle (argument)")
ERR(-100222, ERR_BUF_SIZE, "Buffer argument too small")

// system limitations
ERR(-100240, ERR_AGAIN, "Try again later")
ERR(-100241, ERR_LIMIT, "Fixed limit exceeded")
ERR(-100242, ERR_NO_SYS, "OS doesn't provide a required API")
ERR(-100243, ERR_NOT_IMPLEMENTED, "Feature currently not implemented")
ERR(-100244, ERR_NOT_SUPPORTED, "Feature isn't and won't be supported")

// memory
ERR(-100260, ERR_NO_MEM, "Not enough memory")
ERR(-100261, ERR_ALLOC_NOT_FOUND, "Not a valid allocated address")
ERR(-100262, ERR_MEM_OVERWRITTEN, "Wrote to memory outside valid allocation")

// file + vfs
// .. path
ERR(-100300, ERR_PATH_LENGTH, "Path exceeds PATH_MAX characters")
ERR(-100301, ERR_PATH_EMPTY, "Path is an empty string")
ERR(-100302, ERR_PATH_NOT_RELATIVE, "Path is not relative")
ERR(-100303, ERR_PATH_NON_PORTABLE, "Path contains OS-specific dir separator")
ERR(-100304, ERR_PATH_NON_CANONICAL, "Path contains unsupported .. or ./")
ERR(-100305, ERR_PATH_COMPONENT_SEPARATOR, "Path component contains dir separator")
// .. tree node
ERR(-100310, ERR_TNODE_NOT_FOUND, "File/directory not found")
ERR(-100311, ERR_TNODE_WRONG_TYPE, "Using a directory as file or vice versa")
// .. open
ERR(-100320, ERR_FILE_ACCESS, "Insufficient access rights to open file")
// .. enum
ERR(-100330, ERR_DIR_END, "End of directory reached (no more files)")
// .. IO
ERR(-100340, ERR_IO, "Error during IO")
ERR(-100341, ERR_EOF, "Reading beyond end of file")
// .. mount
ERR(-100350, ERR_ALREADY_MOUNTED, "Directory (tree) already mounted")
ERR(-100351, ERR_NOT_MOUNTED, "Specified directory is not mounted")
ERR(-100352, ERR_INVALID_MOUNT_TYPE, "Invalid mount type (memory corruption?)")
ERR(-100353, ERR_ROOT_DIR_ALREADY_SET, "Attempting to set FS root dir more than once")
// .. misc
ERR(-100360, ERR_UNKNOWN_CMETHOD, "Unknown/unsupported compression method")
ERR(-100361, ERR_IS_COMPRESSED, "Invalid operation for a compressed file")
ERR(-100362, ERR_NOT_MAPPED, "File was not mapped")
ERR(-100363, ERR_NOT_IN_CACHE, "[Internal] Entry not found in cache")
ERR(-100364, ERR_TRACE_EMPTY, "No valid entries in trace")

// file format
ERR(-100400, ERR_UNKNOWN_FORMAT, "Unknown file format")
ERR(-100401, ERR_INCOMPLETE_HEADER, "File header not completely read")
ERR(-100402, ERR_CORRUPTED, "File data is corrupted")

// texture
ERR(-100500, ERR_TEX_FMT_INVALID, "Invalid/unsupported texture format")
ERR(-100501, ERR_TEX_INVALID_COLOR_TYPE, "Invalid color type")
ERR(-100502, ERR_TEX_NOT_8BIT_PRECISION, "Not 8-bit channel precision")
ERR(-100503, ERR_TEX_INVALID_LAYOUT, "Unsupported texel layout, e.g. right-to-left")
ERR(-100504, ERR_TEX_COMPRESSED, "Unsupported texture compression")
ERR(+100505, WARN_TEX_INVALID_DATA, "Warning: invalid texel data encountered")
ERR(-100506, ERR_TEX_INVALID_SIZE, "Texture size is incorrect")
ERR(+100507, INFO_TEX_CODEC_CANNOT_HANDLE, "Texture codec cannot handle the given format")

// CPU
ERR(-100600, ERR_CPU_FEATURE_MISSING, "This CPU doesn't support a required feature")
ERR(-100601, ERR_CPU_UNKNOWN_OPCODE, "Disassembly failed")
ERR(-100602, ERR_CPU_RESTRICTED_AFFINITY, "Cannot set desired CPU affinity")

// shaders
ERR(-100700, ERR_SHDR_CREATE, "Shader creation failed")
ERR(-100701, ERR_SHDR_COMPILE, "Shader compile failed")
ERR(-100702, ERR_SHDR_NO_SHADER, "Invalid shader reference")
ERR(-100703, ERR_SHDR_LINK, "Shader linking failed")
ERR(-100704, ERR_SHDR_NO_PROGRAM, "Invalid shader program reference")

// debug symbol engine
ERR(-100800, ERR_SYM_NO_STACK_FRAMES_FOUND, "No stack frames found")
ERR(-100801, ERR_SYM_UNRETRIEVABLE_STATIC, "Value unretrievable (stored in external module)")
ERR(-100802, ERR_SYM_UNRETRIEVABLE_REG, "Value unretrievable (stored in register)")
ERR(-100803, ERR_SYM_TYPE_INFO_UNAVAILABLE, "Error getting type_info")
ERR(-100804, ERR_SYM_INTERNAL_ERROR, "Exception raised while processing a symbol")
ERR(-100805, ERR_SYM_UNSUPPORTED, "Symbol type not (fully) supported")
ERR(-100806, ERR_SYM_CHILD_NOT_FOUND, "Symbol does not have the given child")
// .. this limit is to prevent infinite recursion.
ERR(-100807, ERR_SYM_NESTING_LIMIT, "Symbol nesting too deep or infinite recursion")
// .. this limit is to prevent large symbols (e.g. arrays or linked lists) from
//    hogging all stack trace buffer space.
ERR(-100808, ERR_SYM_SINGLE_SYMBOL_LIMIT, "Symbol has produced too much output")
// .. one of the dump_sym* functions decided not to output anything at
//    all (e.g. for member functions in UDTs - we don't want those).
//    therefore, skip any post-symbol formatting (e.g. ",") as well.
ERR(+100809, INFO_SYM_SUPPRESS_OUTPUT, "Symbol was suppressed")

// STL debug
ERR(-100900, ERR_STL_CNT_UNKNOWN, "Unknown STL container type_name")
// .. likely causes: not yet initialized or memory corruption.
ERR(-100901, ERR_STL_CNT_INVALID, "Container type is known but contents are invalid")

// timer
ERR(-101000, ERR_TIMER_NO_SAFE_IMPL, "No safe time source available")

#undef ERR
#endif	// #ifdef ERR
