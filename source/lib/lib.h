// Copyright (c) 2003-2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

/*

[KEEP IN SYNC WITH WIKI]

low-level aka "lib"
-------------------

this codebase was grown from modules shared between several projects,
i.e. my personal library; hence the name "lib". it has been expanded to
fit the needs of 0ad - in particular, resource loading.

owing to the dual-use situation, the 0ad coding conventions are not met;
also, major changes are ill-advised because they may break other projects.


design goals
------------

- fast and low-overhead, including startup time
- portable: must run on Win32, Mac OS X and Linux
- reusable across projects, i.e. no dependency on a
  central 'manager' that ties modules together.


scope
-----

- POSIX definitions
- resource management
- debugging tools (including memory tracker)
- low-level helper functions, e.g. ADTs, endian conversion and timing
- platform-dependent system/feature detection

*/

#ifndef LIB_H__
#define LIB_H__

#include <stddef.h>


#include "config.h"
#include "lib/types.h"

#include "sysdep/sysdep.h"


#if defined(__cplusplus)
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

// package code into a single statement.
// notes:
// - for(;;) { break; } and {} don't work because invocations of macros
//   implemented with STMT often end with ";", thus breaking if() expressions.
// - we'd really like to eliminate "conditional expression is constant"
//   warnings. replacing 0 literals with extern volatile variables fools
//   VC7 but isn't guaranteed to be free of overhead. we will just
//   squelch the warning (unfortunately non-portable).
#define STMT(STMT_code__) do { STMT_code__; } while(false)

// may be called at any time (in particular before main), but is not
// thread-safe. if that's important, use pthread_once() instead.
#define ONCE(ONCE_code__)\
STMT(\
	static bool ONCE_done__ = false;\
	if(!ONCE_done__)\
	{\
		ONCE_done__ = true;\
		ONCE_code__;\
	}\
)


// be careful here. the given expression (e.g. variable or
// function return value) may be a Handle (=i64), so it needs to be
// stored and compared as such. (very large but legitimate Handle values
// casted to int can end up negative)
// all functions using this return int (instead of i64) for efficiency and
// simplicity. if the input was negative, it is an error code and is
// therefore known to fit; we still mask with UINT_MAX to avoid
// VC cast-to-smaller-type warnings.

// if expression evaluates to a negative i64, warn user and return the number.
#if OS_WIN
#define CHECK_ERR(expression)\
STMT(\
	i64 err__ = (i64)(expression);\
	if(err__ < 0)\
	{\
		debug_warn("FYI: CHECK_ERR reports that a function failed."\
		           "feel free to ignore or suppress this warning.");\
		return (int)(err__ & UINT_MAX);\
	}\
)
#else
#define CHECK_ERR(expression)\
STMT(\
	i64 err__ = (i64)(expression);\
	if(err__ < 0)\
	{\
		debug_printf("%s:%d: FYI: CHECK_ERR reports that a function failed."\
		            "feel free to ignore or suppress this warning.\n", __FILE__, __LINE__);\
		return (int)(err__ & UINT_MAX);\
	}\
)
#endif

// just pass on errors without any kind of annoying warning
// (useful for functions that can legitimately fail, e.g. vfs_exists).
#define RETURN_ERR(expression)\
STMT(\
	i64 err__ = (i64)(expression);\
	if(err__ < 0)\
		return (int)(err__ & UINT_MAX);\
)

// if expression evaluates to a negative i64, warn user and throw the number.
#define THROW_ERR(expression)\
STMT(\
	i64 err__ = (i64)(expression);\
	if(err__ < 0)\
	{\
		debug_warn("FYI: THROW_ERR reports that a function failed."\
		           "feel free to ignore or suppress this warning.");\
		throw (int)(err__ & UINT_MAX);\
	}\
)

// if expression evaluates to a negative i64, warn user and just return
// (useful for void functions that must bail and complain)
#define WARN_ERR_RETURN(expression)\
STMT(\
	i64 err__ = (i64)(expression);\
	if(err__ < 0)\
	{\
		debug_warn("FYI: WARN_ERR_RETURN reports that a function failed."\
		           "feel free to ignore or suppress this warning.");\
		return;\
	}\
)

// if expression evaluates to a negative i64, warn user
// (this is similar to debug_assert but also works in release mode)
#define WARN_ERR(expression)\
STMT(\
	i64 err__ = (i64)(expression);\
	if(err__ < 0)\
		debug_warn("FYI: WARN_ERR reports that a function failed."\
		           "feel free to ignore or suppress this warning.");\
)



// if ok evaluates to false or FALSE, warn user and return -1.
#define WARN_RETURN_IF_FALSE(ok)\
STMT(\
	if(!(ok))\
	{\
		debug_warn("FYI: WARN_RETURN_IF_FALSE reports that a function failed."\
		           "feel free to ignore or suppress this warning.");\
		return -1;\
	}\
)

// if ok evaluates to false or FALSE, return -1.
#define RETURN_IF_FALSE(ok)\
STMT(\
	if(!(ok))\
		return -1;\
)

// if ok evaluates to false or FALSE, warn user.
#define WARN_IF_FALSE(ok)\
STMT(\
	if(!(ok))\
		debug_warn("FYI: WARN_IF_FALSE reports that a function failed."\
		           "feel free to ignore or suppress this warning.");\
)



// useful because VC6 may return 0 on failure, instead of throwing.
// this wraps the exception handling, and creates a NULL pointer on failure.
#define SAFE_NEW(type, ptr)\
	type* ptr;\
	try\
	{\
		ptr = new type();\
	}\
	catch(std::bad_alloc)\
	{\
		ptr = 0;\
	}

#define SAFE_DELETE(p)\
STMT(\
	if((p))\
	{\
		delete (p);\
		(p) = 0;\
	}\
)


enum LibError
{
	// function arguments
	ERR_INVALID_PARAM   = -1000,
	ERR_INVALID_HANDLE  = -1001,
	ERR_BUF_SIZE        = -1002,

	// system limitations
	// .. out of memory
	ERR_NO_MEM           = -1020,
	// .. try again later
	ERR_AGAIN            = -1021,
	// .. fixed limit exceeded
	ERR_LIMIT            = -1022,
	// .. the OS doesn't provide an API we need
	ERR_NO_SYS           = -1023,
	// .. feature not currently implemented (will probably change)
	ERR_NOT_IMPLEMENTED  = -1024,
	// .. feature won't be supported
	ERR_NOT_SUPPORTED    = -1025,

	ERR_TIMED_OUT         = -1060,

	// file + vfs
	ERR_FILE_NOT_FOUND  = -1200,
	ERR_PATH_NOT_FOUND  = -1201,
	ERR_PATH_LENGTH     = -1202,
	ERR_PATH_INVALID    = -1203,
	ERR_DIR_END         = -1210,
	ERR_NOT_FILE        = -1220,
	ERR_FILE_ACCESS     = -1230,
	ERR_IO              = -1231,
	ERR_EOF             = -1232,

	// file format
	ERR_UNKNOWN_FORMAT    = -1400,
	ERR_INCOMPLETE_HEADER = -1401,
	// .. data (e.g. in file) is obviously incorrect
	ERR_CORRUPTED         = -1402,

	// texture
	ERR_TEX_FMT_INVALID         = -1500,
	ERR_TEX_INVALID_COLOR_TYPE  = -1501,
	ERR_TEX_NOT_8BIT_PRECISION  = -1502,
	ERR_TEX_INVALID_LAYOUT      = -1503,
	ERR_TEX_COMPRESSED          = -1504,
	WARN_TEX_INVALID_DATA       = +1505,
	ERR_TEX_INVALID_SIZE        = -1506,

	ERR_CPU_FEATURE_MISSING     = -1600,


	ERR_LAST
};




#ifndef MIN
#define MIN(a, b) (((a) < (b))? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b))? (a) : (b))
#endif


// 2 ways of avoiding "unreferenced formal parameter" warnings:
// .. inside the function body, e.g. void f(int x) { UNUSED2(x); }
#define UNUSED2(param) (void)param;
// .. wrapped around the parameter name, e.g. void f(int UNUSED(x))
#define UNUSED(param)

// indicates a piece of code cannot be reached (e.g. because all
// control paths before it end up returning). this is mostly for
// human benefit, but it may also help optimization.
#if MSC_VERSION
# define UNREACHABLE __assume(0)
#else
# define UNREACHABLE
#endif

#define ARRAY_SIZE(name) (sizeof(name) / sizeof(name[0]))


//
// compile-time debug_assert, especially useful for testing sizeof().
// no runtime overhead; may be used anywhere, including file scope.
//

// generate a symbol containing the line number of the macro invocation.
// used to give a unique name (per file) to types made by cassert.
// we can't prepend __FILE__ to make it globally unique - the filename
// may be enclosed in quotes. need the 2 macro expansions to make sure
// __LINE__ is expanded correctly.
#define MAKE_UID2__(l) LINE_ ## l
#define MAKE_UID1__(l) MAKE_UID2__(l)
#define UID__ MAKE_UID1__(__LINE__)

// more descriptive error message, but may cause a struct redefinition
// warning if used from the same line in different files.
#define cassert(expr) struct UID__ { int CASSERT_FAILURE: (expr); }

// less helpful error message, but redefinition doesn't trigger warnings.
#define cassert2(expr) extern char CASSERT_FAILURE[1][(expr)]

// note: alternative method in C++: specialize a struct only for true;
// using it will raise 'incomplete type' errors if instantiated with false.








const size_t KiB = 1ul << 10;
const size_t MiB = 1ul << 20;
const size_t GiB = 1ul << 30;


#if OS_WIN
#define DIR_SEP '\\'
#else
#define DIR_SEP '/'
#endif


#define BIT(n) (1ul << (n))


// these are declared in the header and inlined to aid compiler optimizations
// (they can easily end up being time-critical).

inline uint bit_mask(uint num_bits)
{
	return (1u << num_bits)-1;
}

inline uint bits(uint num, uint lo_idx, uint hi_idx)
{
	const uint count = (hi_idx - lo_idx)+1;	// # bits to return
	uint result = num >> lo_idx;
	result &= bit_mask(count);
	return result;
}


// FNV1-A hash - good for strings.
// if len = 0 (default), treat buf as a C-string;
// otherwise, hash <len> bytes of buf.
extern u32 fnv_hash(const void* buf, size_t len = 0);
extern u64 fnv_hash64(const void* buf, size_t len = 0);

// special version for strings: first converts to lowercase
// (useful for comparing mixed-case filenames)
extern u32 fnv_lc_hash(const char* str, size_t len = 0);

// hash (currently FNV) of a filename
typedef u32 FnHash;


extern u16 addusw(u16 x, u16 y);
extern u16 subusw(u16 x, u16 y);

// zero-extend <size> (truncated to 8) bytes of little-endian data to u64,
// starting at address <p> (need not be aligned).
extern u64 movzx_64le(const u8* p, size_t size);

// sign-extend <size> (truncated to 8) bytes of little-endian data to i64,
// starting at address <p> (need not be aligned).
extern i64 movsx_64le(const u8* p, size_t size);


extern bool is_pow2(uint n);

// return -1 if not an integral power of 2,
// otherwise the base2 logarithm
extern int ilog2(uint n);

// return log base 2, rounded up.
extern uint log2(uint x);


// multiple must be a power of two.
extern uintptr_t round_up(uintptr_t val, uintptr_t multiple);

// these avoid a common mistake in using >> (ANSI requires shift count be
// less than the bit width of the type).
extern u32 u64_hi(u64 x);
extern u32 u64_lo(u64 x);

extern u64 u64_from_u32(u32 hi, u32 lo);


extern u16 fp_to_u16(double in);

// return random integer in [0, limit).
// does not use poorly distributed lower bits of rand().
extern int rand_up_to(int limit);

// big endian!
extern void base32(const int len, const u8* in, u8* out);


// case-insensitive check if string <s> matches the pattern <w>,
// which may contain '?' or '*' wildcards. if so, return 1, otherwise 0.
// note: NULL wildcard pattern matches everything!
extern int match_wildcard(const char* s, const char* w);
extern int match_wildcardw(const wchar_t* s, const wchar_t* w);

#endif	// #ifndef LIB_H__
