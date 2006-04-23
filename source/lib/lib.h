/**
 * =========================================================================
 * File        : lib.h
 * Project     : 0 A.D.
 * Description : various utility functions.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2005 Jan Wassenberg
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
#include <math.h>	// fabsf


#include "config.h"
#include "lib/types.h"
#include "sysdep/sysdep.h"
#include "sysdep/cpu.h"	// CAS


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

// must come after definition of STMT
#include "lib/lib_errors.h"

// execute the code passed as a parameter only the first time this is
// reached.
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

// execute the code passed as a parameter except the first time this is
// reached.
// may be called at any time (in particular before main), but is not
// thread-safe.
#define ONCE_NOT(ONCE_code__)\
STMT(\
	static bool ONCE_done__ = false;\
	if(!ONCE_done__)\
		ONCE_done__ = true;\
	else\
		ONCE_code__;\
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
	delete (p);	/* if p == 0, delete is a no-op */ \
	(p) = 0;\
)


#define SAFE_FREE(p)\
STMT(\
	free(p);	/* if p == 0, free is a no-op */ \
	(p) = 0;\
)



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


/*
"unreachable code" helpers

unreachable lines of code are often the source or symptom of subtle bugs.
they are flagged by compiler warnings; however, the opposite problem -
erroneously reaching certain spots (e.g. due to missing return statement)
is worse and not detected automatically.

to defend against this, the programmer can annotate their code to
indicate to humans that a particular spot should never be reached.
however, that isn't much help; better is a sentinel that raises an
error if if it is actually reached. hence, the UNREACHABLE macro.

ironically, if the code guarded by UNREACHABLE works as it should,
compilers may flag the macro's code as unreachable. this would
distract from genuine warnings, which is unacceptable.

even worse, compilers differ in their code checking: GCC only complains if
non-void functions end without returning a value (i.e. missing return
statement), while VC checks if lines are unreachable (e.g. if they are
preceded by a return on all paths).

our implementation of UNREACHABLE solves this dilemna as follows:
- on GCC: call abort(); since it has the noreturn attributes, the
  "non-void" warning disappears.
- on VC: avoid generating any code. we allow the compiler to assume the
  spot is actually unreachable, which incidentally helps optimization.
  if reached after all, a crash usually results. in that case, compile with
  CONFIG_PARANOIA, which will cause an error message to be displayed.

this approach still allows for the possiblity of automated
checking, but does not cause any compiler warnings.
*/

// 1) final build: optimize assuming this location cannot be reached.
//    may crash if that turns out to be untrue, but removes checking overhead.
#if CONFIG_FINAL
# define UNREACHABLE SYS_UNREACHABLE
// 2) normal build:
#else
//    a) normal implementation: includes "abort", which is declared with
//       noreturn attribute and therefore avoids GCC's "execution reaches
//       end of non-void function" warning.
# if !MSC_VERSION || CONFIG_PARANOIA
#  define UNREACHABLE\
	STMT(\
		debug_warn("hit supposedly unreachable code");\
		abort();\
	)
//    b) VC only: don't generate any code; squelch the warning and optimize.
# else
#  define UNREACHABLE SYS_UNREACHABLE
# endif
#endif

/*
convenient specialization of UNREACHABLE for switch statements whose
default can never be reached. example usage:
int x;
switch(x % 2)
{
	case 0: break;
	case 1: break;
	NODEFAULT;
}
*/
#define NODEFAULT default: UNREACHABLE


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

extern uint round_up_to_pow2(uint x);


// multiple must be a power of two.
extern uintptr_t round_up  (uintptr_t n, uintptr_t multiple);
extern uintptr_t round_down(uintptr_t n, uintptr_t multiple);

// these avoid a common mistake in using >> (ANSI requires shift count be
// less than the bit width of the type).
extern u32 u64_hi(u64 x);
extern u32 u64_lo(u64 x);
extern u16 u32_hi(u32 x);
extern u16 u32_lo(u32 x);


extern u64 u64_from_u32(u32 hi, u32 lo);
extern u32 u32_from_u16(u16 hi, u16 lo);




inline bool feq(float f1, float f2)
{
	// the requisite value will change with the magnitude of f1 and f2!
	// this is a sane default, but don't use this routine for very
	// large/small comparands.
	const float epsilon = 0.00001f;
	return fabsf(f1 - f2) < epsilon;
}


extern u16 fp_to_u16(double in);


// big endian!
extern void base32(const int len, const u8* in, u8* out);


// case-insensitive check if string <s> matches the pattern <w>,
// which may contain '?' or '*' wildcards. if so, return 1, otherwise 0.
// note: NULL wildcard pattern matches everything!
extern int match_wildcard(const char* s, const char* w);
extern int match_wildcardw(const wchar_t* s, const wchar_t* w);

// this is strcpy, but indicates that the programmer checked usage and
// promises it is safe.
#define SAFE_STRCPY strcpy

// return random integer in [min, max).
// avoids several common pitfalls; see discussion at
// http://www.azillionmonkeys.com/qed/random.html
extern uint rand(uint min_inclusive, uint max_exclusive);

#endif	// #ifndef LIB_H__
