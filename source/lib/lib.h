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
 * various utility functions.
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

#ifndef INCLUDED_LIB
#define INCLUDED_LIB

#include <cmath>	// fabsf
#include <limits>	// numeric_limits
#include <stdexcept>	// out_of_range
#include <algorithm>	// min, max

template<typename T>
T Clamp(T val, T min, T max)
{
	ASSERT(min <= max);
	return std::max(min, std::min(val, max));
}

template<typename T>
T DivideRoundUp(T dividend, T divisor)
{
	ASSERT(divisor != 0);
	return (dividend + divisor-1) / divisor;
}

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
 *   transcendental operations introduce error.
 **/
inline bool feq(double d1, double d2, double epsilon = 0.00001)
{
	return fabs(d1 - d2) < epsilon;
}

inline bool feqf(float f1, float f2, float epsilon = 0.001f)
{
	return fabsf(f1 - f2) < epsilon;
}

inline bool IsSimilarMagnitude(double d1, double d2, const double relativeErrorTolerance = 0.05)
{
	const double relativeError = fabs(d1/d2 - 1.0);
	if(relativeError > relativeErrorTolerance)
		return false;
	return true;
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

// safe downcasters: cast from any integral type to u32 or u16; 
// issues warning if larger than would fit in the target type.
//
// these are generally useful but included here (instead of e.g. lib.h) for
// several reasons:
// - including implementation in lib.h doesn't work because the definition
//   of ENSURE in turn requires lib.h's STMT.
// - separate compilation of templates via export isn't supported by
//   most compilers.

template<typename T> u8 u8_from_larger(T x)
{
	const u8 max = std::numeric_limits<u8>::max();
	if((u64)x > (u64)max)
		throw std::out_of_range("u8_from_larger");
	return (u8)(x & max);
}

template<typename T> u16 u16_from_larger(T x)
{
	const u16 max = std::numeric_limits<u16>::max();
	if((u64)x > (u64)max)
		throw std::out_of_range("u16_from_larger");
	return (u16)(x & max);
}

template<typename T> u32 u32_from_larger(T x)
{
	const u32 max = std::numeric_limits<u32>::max();
	if((u64)x > (u64)max)
		throw std::out_of_range("u32_from_larger");
	return (u32)(x & max);
}

/// convert double to u8; verifies number is in range.
extern u8 u8_from_double(double in);
/// convert double to u16; verifies number is in range.
extern u16 u16_from_double(double in);

#endif	// #ifndef INCLUDED_LIB
