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

/**

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

**/

#ifndef LIB_H__
#define LIB_H__

#include <stddef.h>
#include <math.h>	// fabsf


#include "config.h"
#include "lib/types.h"
#include "sysdep/sysdep.h"
#include "sysdep/cpu.h"	// CAS
//#include "sysdep/sysdep.h"	// moved down; see below.


#if defined(__cplusplus)
# define EXTERN_C extern "C"
#else
# define EXTERN_C extern
#endif


const size_t KiB = 1ul << 10;
const size_t MiB = 1ul << 20;
const size_t GiB = 1ul << 30;


//-----------------------------------------------------------------------------
// code-generating macros
//-----------------------------------------------------------------------------

/**
 * package code into a single statement.
 *
 * @param STMT_code__ code to be bundled. (must be interpretable as
 * a macro argument, i.e. sequence of tokens).
 * the argument name is chosen to avoid conflicts.
 *
 * notes:
 * - for(;;) { break; } and {} don't work because invocations of macros
 *   implemented with STMT often end with ";", thus breaking if() expressions.
 * - we'd really like to eliminate "conditional expression is constant"
 *   warnings. replacing 0 literals with extern volatile variables fools
 *   VC7 but isn't guaranteed to be free of overhead. we will just
 *   squelch the warning (unfortunately non-portable).
 **/
#define STMT(STMT_code__) do { STMT_code__; } while(false)

// must come after definition of STMT
#include "lib/lib_errors.h"

/**
 * execute the code passed as a parameter only the first time this is
 * reached.
 * may be called at any time (in particular before main), but is not
 * thread-safe. if that's important, use pthread_once() instead.
 **/
#define ONCE(ONCE_code__)\
STMT(\
	static bool ONCE_done__ = false;\
	if(!ONCE_done__)\
	{\
		ONCE_done__ = true;\
		ONCE_code__;\
	}\
)

/**
 * execute the code passed as a parameter except the first time this is
 * reached.
 * may be called at any time (in particular before main), but is not
 * thread-safe.
 **/
#define ONCE_NOT(ONCE_code__)\
STMT(\
	static bool ONCE_done__ = false;\
	if(!ONCE_done__)\
		ONCE_done__ = true;\
	else\
		ONCE_code__;\
)


/**
 * C++ new wrapper: allocates an instance of the given type and stores a
 * pointer to it. sets pointer to 0 on allocation failure.
 *
 * this simplifies application code when on VC6, which may or
 * may not throw/return 0 on failure.
 **/
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

/**
 * delete memory ensuing from new and set the pointer to zero
 * (thus making double-frees safe / a no-op)
 **/
#define SAFE_DELETE(p)\
STMT(\
	delete (p);	/* if p == 0, delete is a no-op */ \
	(p) = 0;\
)

/**
 * delete memory ensuing from new[] and set the pointer to zero
 * (thus making double-frees safe / a no-op)
 **/
#define SAFE_ARRAY_DELETE(p)\
STMT(\
	delete[] (p);	/* if p == 0, delete is a no-op */ \
	(p) = 0;\
)

/**
 * free memory ensuing from malloc and set the pointer to zero
 * (thus making double-frees safe / a no-op)
 **/
#define SAFE_FREE(p)\
STMT(\
	free(p);	/* if p == 0, free is a no-op */ \
	(p) = 0;\
)


//-----------------------------------------------------------------------------
// source code annotation
//-----------------------------------------------------------------------------

/**
 * mark a function local variable or parameter as unused and avoid
 * the corresponding compiler warning.
 * use inside the function body, e.g. void f(int x) { UNUSED2(x); }
 **/
#define UNUSED2(param) (void)param;

/**
 * mark a function parameter as unused and avoid
 * the corresponding compiler warning.
 * wrap around the parameter name, e.g. void f(int UNUSED(x))
 **/
#define UNUSED(param)

/**
 * mark the copy constructor as inaccessible. this squelches
 * "cannot be generated" warnings for classes with const members.
 *
 * intended to be used at end of class definition.
 * must be followed by semicolon.
 **/
#define NO_COPY_CTOR(class_name)\
	private:\
	class_name& operator=(const class_name&)


/**
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
**/
#define UNREACHABLE	// actually defined below.. this is for
# undef UNREACHABLE	// CppDoc's benefit only.

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

/**
convenient specialization of UNREACHABLE for switch statements whose
default can never be reached. example usage:
int x;
switch(x % 2)
{
	case 0: break;
	case 1: break;
	NODEFAULT;
}
**/
#define NODEFAULT default: UNREACHABLE

//-----------------------------------------------------------------------------
// cassert

// generate a symbol containing the line number of the macro invocation.
// used to give a unique name (per file) to types made by cassert.
// we can't prepend __FILE__ to make it globally unique - the filename
// may be enclosed in quotes. need the 2 macro expansions to make sure
// __LINE__ is expanded correctly.
#define MAKE_UID2__(l) LINE_ ## l
#define MAKE_UID1__(l) MAKE_UID2__(l)
#define UID__ MAKE_UID1__(__LINE__)

/**
 * compile-time debug_assert. causes a compile error if the expression
 * evaluates to zero/false.
 *
 * no runtime overhead; may be used anywhere, including file scope.
 * especially useful for testing sizeof types.
 *
 * this version has a more descriptive error message, but may cause a
 * struct redefinition warning if used from the same line in different files.
 *
 * note: alternative method in C++: specialize a struct only for true;
 * using it will raise 'incomplete type' errors if instantiated with false.
 *
 * @param expression that is expected to evaluate to non-zero at compile-time.
 **/
#define cassert(expr) struct UID__ { int CASSERT_FAILURE: (expr); }

/**
 * compile-time debug_assert. causes a compile error if the expression
 * evaluates to zero/false.
 *
 * no runtime overhead; may be used anywhere, including file scope.
 * especially useful for testing sizeof types.
 *
 * this version has a less helpful error message, but redefinition doesn't
 * trigger warnings.
 *
 * @param expression that is expected to evaluate to non-zero at compile-time.
 **/
#define cassert2(expr) extern char CASSERT_FAILURE[1][(expr)]


//-----------------------------------------------------------------------------
// bit bashing
//-----------------------------------------------------------------------------

/**
 * value of bit number <n>.
 *
 * @param n bit index (0..CHAR_BIT*sizeof(int)-1)
 **/
#define BIT(n) (1ul << (n))


// these are declared in the header and inlined to aid compiler optimizations
// (they can easily end up being time-critical).
// note: GCC can't inline extern functions, while VC's "Whole Program
// Optimization" can.

/**
 * a mask that includes the lowest N bits
 *
 * @param num_bits number of bits in mask
 **/
inline uint bit_mask(uint num_bits)
{
	return (1u << num_bits)-1;
}

/**
 * extract the value of bits hi_idx:lo_idx within num
 *
 * example: bits(0x69, 2, 5) == 0x0A
 *
 * @param num number whose bits are to be extracted
 * @param lo_idx bit index of lowest  bit to include
 * @param hi_idx bit index of highest bit to include
 * @return value of extracted bits.
 **/
inline uint bits(uint num, uint lo_idx, uint hi_idx)
{
	const uint count = (hi_idx - lo_idx)+1;	// # bits to return
	uint result = num >> lo_idx;
	result &= bit_mask(count);
	return result;
}

/// is the given number a power of two?
extern bool is_pow2(uint n);

/**
 * @return -1 if not an integral power of 2,
 * otherwise the base2 logarithm.
 **/
extern int ilog2(uint n);

/**
 * @return log base 2, rounded up.
 **/
extern uint log2(uint x);

/**
 * another implementation; uses the FPU normalization hardware.
 *
 * @return log base 2, rounded up.
 **/
extern int ilog2(const float x);

/**
 * round up to nearest power of two; no change if already POT.
 **/
extern uint round_up_to_pow2(uint x);


//-----------------------------------------------------------------------------
// misc arithmetic

/// canonical minimum macro
#ifndef MIN
#define MIN(a, b) (((a) < (b))? (a) : (b))
#endif

/// canonical maximum macro
#ifndef MAX
#define MAX(a, b) (((a) > (b))? (a) : (b))
#endif

/// number of array elements
#define ARRAY_SIZE(name) (sizeof(name) / sizeof(name[0]))

/**
 * round number up/down to the next given multiple.
 *
 * @param multiple: must be a power of two.
 **/
extern uintptr_t round_up  (uintptr_t n, uintptr_t multiple);
extern uintptr_t round_down(uintptr_t n, uintptr_t multiple);

/// 16-bit saturating (does not overflow) addition.
extern u16 addusw(u16 x, u16 y);
/// 16-bit saturating (does not underflow) subtraction.
extern u16 subusw(u16 x, u16 y);

/**
 * are the given floats nearly "equal"?
 *
 * @return whether the numbers are within "epsilon" of each other.
 *
 * notes:
 * - the epsilon magic number varies with the magnitude of the inputs.
 *   we use a sane default, but don't use this routine for very
 *   large/small comparands.
 * - floating-point numbers don't magically lose precision. addition,
 *   subtraction and multiplication results are precise up to the mantissa's
 *   least-significant bit. only division, sqrt, sin/cos and other
 *   trancendental operations introduce error.
 **/
inline bool feq(float f1, float f2)
{
	const float epsilon = 0.00001f;
	return fabsf(f1 - f2) < epsilon;
}


/**
* return random integer in [min, max).
* avoids several common pitfalls; see discussion at
* http://www.azillionmonkeys.com/qed/random.html
**/
extern uint rand(uint min_inclusive, uint max_exclusive);


//-----------------------------------------------------------------------------
// type conversion


// note: these avoid a common mistake in using >> (ANSI requires
// shift count be less than the bit width of the type).

extern u32 u64_hi(u64 x);	/// return upper 32-bits
extern u32 u64_lo(u64 x);	/// return lower 32-bits
extern u16 u32_hi(u32 x);	/// return upper 16-bits
extern u16 u32_lo(u32 x);	/// return lower 16-bits

extern u64 u64_from_u32(u32 hi, u32 lo);	/// assemble u64 from u32
extern u32 u32_from_u16(u16 hi, u16 lo);	/// assemble u32 from u16

/**
 * zero-extend <size> (truncated to 8) bytes of little-endian data to u64,
 * starting at address <p> (need not be aligned).
 **/
extern u64 movzx_64le(const u8* p, size_t size);

/**
 * sign-extend <size> (truncated to 8) bytes of little-endian data to i64,
 * starting at address <p> (need not be aligned).
 **/
extern i64 movsx_64le(const u8* p, size_t size);

/// convert double to u8; verifies number is in range.
extern u8  fp_to_u8 (double in);
/// convert double to u16; verifies number is in range.
extern u16 fp_to_u16(double in);


//-----------------------------------------------------------------------------
// string processing

/**
 * this is strcpy, but indicates that the programmer checked usage and
 * promises it is safe.
 **/
#define SAFE_STRCPY strcpy


/**
 * generate the base32 textual representation of a buffer.
 *
 * @param len size [bytes] of input
 * @param big-endian input data (assumed to be integral number of bytes)
 * @param output string; zero-terminated. must be big enough
 * (i.e. at least ceil(len*CHAR_BIT/5) + 1 chars)
 **/
extern void base32(const size_t len, const u8* in, u8* out);


/**
 * partial regex implementation: see if string matches pattern.
 *
 * @param s input string
 * @param w pseudo-regex to match against. case-insensitive;
 * may contain '?' and/or '*' wildcards. if NULL, matches everything.
 *
 * @return 1 if they match, otherwise 0.
 *
 * algorithmfrom http://www.codeproject.com/string/wildcmp.asp.
 **/
extern int match_wildcard(const char* s, const char* w);
/// unicode version of match_wildcard.
extern int match_wildcardw(const wchar_t* s, const wchar_t* w);


/**
 * calculate FNV1-A hash.
 *
 * @param buf input buffer.
 * @param len if 0 (default), treat buf as a C-string; otherwise,
 * indicates how many bytes of buffer to hash.
 * @return hash result. note: results are distinct for buffers containing
 * differing amounts of zero bytes because the hash value is seeded.
 *
 * rationale: this algorithm was chosen because it delivers 'good' results
 * for string data and is relatively simple. other good alternatives exist;
 * see Ozan Yigit's hash roundup.
 **/
extern u32 fnv_hash(const void* buf, size_t len = 0);
/// 64-bit version of fnv_hash.
extern u64 fnv_hash64(const void* buf, size_t len = 0);

/**
 * special version of fnv_hash for strings: first converts to lowercase
 * (useful for comparing mixed-case filenames)
 **/
extern u32 fnv_lc_hash(const char* str, size_t len = 0);

#endif	// #ifndef LIB_H__
