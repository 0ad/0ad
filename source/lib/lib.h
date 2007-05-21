/**
 * =========================================================================
 * File        : lib.h
 * Project     : 0 A.D.
 * Description : various utility functions.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

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

#ifndef INCLUDED_LIB
#define INCLUDED_LIB

#include <stddef.h>
#include <math.h>	// fabsf

#include "config.h"


const size_t KiB = 1ul << 10;
const size_t MiB = 1ul << 20;
const size_t GiB = 1ul << 30;


/// number of array elements
#define ARRAY_SIZE(name) (sizeof(name) / sizeof(name[0]))


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
 * execute the code passed as a parameter before main is entered.
 *
 * WARNING: if the source file containing this is not directly referenced
 * from anywhere, linkers may discard that object file (unless linking
 * statically). see http://www.cbloom.com/3d/techdocs/more_coding.txt
 **/
#define AT_STARTUP(code__)\
	namespace { struct UID__\
	{\
		UID__() { code__; }\
	} UID2__; }


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
inline bool feq(double d1, double d2, double epsilon = 0.00001)
{
	return fabs(d1 - d2) < epsilon;
}

inline bool feqf(float f1, float f2, float epsilon = 0.001f)
{
	return fabsf(f1 - f2) < epsilon;
}


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

/// convert double to u8; verifies number is in range.
extern u8 u8_from_double(double in);
/// convert double to u16; verifies number is in range.
extern u16 u16_from_double(double in);

#endif	// #ifndef INCLUDED_LIB
