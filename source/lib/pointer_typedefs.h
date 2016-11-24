/* Copyright (c) 2015 Wildfire Games
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

#ifndef INCLUDED_POINTER_TYPEDEFS
#define INCLUDED_POINTER_TYPEDEFS

#include "lib/sysdep/compiler.h"	// HAVE_SSE

#if HAVE_SSE
# include <xmmintrin.h>  // __m64, __m128
#endif
#if HAVE_SSE2
# include <emmintrin.h>  // __m128i, __m128d
#endif

// convenience typedefs for shortening parameter lists.
// naming convention: [const] [restrict] pointer to [const] type
// supported types: void, signed/unsigned 8/16/32/64 integers, float, double, XMM

// which general type of pointer should be used in various situations?
// it would be convenient (especially for allocators) to allow easy
// pointer arithmetic. char* is one such option. However, that type of
// pointer (and also u8*) has a special dispensation for aliasing with
// _anything_ [C99 6.5(7) - we refer to that standard because __restrict
// compiler extensions are most likely to conform to its semantics],
// which might prevent optimizations.
//
// storing the address in uintptr_t (signed integers must never overflow)
// does allow easy arithmetic. unfortunately, the ASSUME_ALIGNED macro
// requires a pointer type. it is not clear whether casting to a pointer
// provides the same benefit.
//
// GCC and MSVC also provide a function attribute indicating a
// non-aliased pointer is being returned. however, this propagation
// does not seem to be reliable, especially because compilers may
// lose track of pointers [1]. it is therefore a waste of effort to
// annotate ALL pointers with cumbersome declarations, or pollute
// public interfaces with the following (little-known) typedefs.
//
// performance-critical code should instead introduce restricted
// pointers immediately before they are used to access memory.
// when combined with ASSUME_ALIGNED annotations, this would seem to
// provide all of the benefits of pointer analysis. it is therefore
// safe to use uintptr_t wherever convenient. however, callers usually
// expect a void* parameter or return value (e.g. in STL allocators).
// that seems a reasonable convention without any apparent downsides.
//
// in summary:
// - void* for public allocator interfaces;
// - uintptr_t for variables/arguments (even some public ones)
//   which are frequently the subject of address manipulations;
// - restrict-qualified pointers via the following typedefs
//   [plus ASSUME_ALIGNED, if applicable] shortly before
//   accessing the underlying storage.
//
// 1: http://drdobbs.com/cpp/184403676?pgno=3

// NB: `restrict' is not going into C++0x, so use __restrict to
// maintain compatibility with VC2010.

typedef void* pVoid;
typedef void* const cpVoid;
typedef void* __restrict rpVoid;
typedef void* const __restrict crpVoid;
typedef const void* pcVoid;
typedef const void* const cpcVoid;
typedef const void* __restrict rpcVoid;
typedef const void* const __restrict crpcVoid;

typedef int8_t* pI8;
typedef int8_t* const cpI8;
typedef int8_t* __restrict rpI8;
typedef int8_t* const __restrict crpI8;
typedef const int8_t* pcI8;
typedef const int8_t* const cpcI8;
typedef const int8_t* __restrict rpcI8;
typedef const int8_t* const __restrict crpcI8;

typedef int16_t* pI16;
typedef int16_t* const cpI16;
typedef int16_t* __restrict rpI16;
typedef int16_t* const __restrict crpI16;
typedef const int16_t* pcI16;
typedef const int16_t* const cpcI16;
typedef const int16_t* __restrict rpcI16;
typedef const int16_t* const __restrict crpcI16;

typedef int32_t* pI32;
typedef int32_t* const cpI32;
typedef int32_t* __restrict rpI32;
typedef int32_t* const __restrict crpI32;
typedef const int32_t* pcI32;
typedef const int32_t* const cpcI32;
typedef const int32_t* __restrict rpcI32;
typedef const int32_t* const __restrict crpcI32;

typedef int64_t* pI64;
typedef int64_t* const cpI64;
typedef int64_t* __restrict rpI64;
typedef int64_t* const __restrict crpI64;
typedef const int64_t* pcI64;
typedef const int64_t* const cpcI64;
typedef const int64_t* __restrict rpcI64;
typedef const int64_t* const __restrict crpcI64;

typedef uint8_t* pU8;
typedef uint8_t* const cpU8;
typedef uint8_t* __restrict rpU8;
typedef uint8_t* const __restrict crpU8;
typedef const uint8_t* pcU8;
typedef const uint8_t* const cpcU8;
typedef const uint8_t* __restrict rpcU8;
typedef const uint8_t* const __restrict crpcU8;

typedef uint16_t* pU16;
typedef uint16_t* const cpU16;
typedef uint16_t* __restrict rpU16;
typedef uint16_t* const __restrict crpU16;
typedef const uint16_t* pcU16;
typedef const uint16_t* const cpcU16;
typedef const uint16_t* __restrict rpcU16;
typedef const uint16_t* const __restrict crpcU16;

typedef uint32_t* pU32;
typedef uint32_t* const cpU32;
typedef uint32_t* __restrict rpU32;
typedef uint32_t* const __restrict crpU32;
typedef const uint32_t* pcU32;
typedef const uint32_t* const cpcU32;
typedef const uint32_t* __restrict rpcU32;
typedef const uint32_t* const __restrict crpcU32;

typedef uint64_t* pU64;
typedef uint64_t* const cpU64;
typedef uint64_t* __restrict rpU64;
typedef uint64_t* const __restrict crpU64;
typedef const uint64_t* pcU64;
typedef const uint64_t* const cpcU64;
typedef const uint64_t* __restrict rpcU64;
typedef const uint64_t* const __restrict crpcU64;

typedef float* pFloat;
typedef float* const cpFloat;
typedef float* __restrict rpFloat;
typedef float* const __restrict crpFloat;
typedef const float* pcFloat;
typedef const float* const cpcFloat;
typedef const float* __restrict rpcFloat;
typedef const float* const __restrict crpcFloat;

typedef double* pDouble;
typedef double* const cpDouble;
typedef double* __restrict rpDouble;
typedef double* const __restrict crpDouble;
typedef const double* pcDouble;
typedef const double* const cpcDouble;
typedef const double* __restrict rpcDouble;
typedef const double* const __restrict crpcDouble;

#if HAVE_SSE
typedef __m64* pM64;
typedef __m64* const cpM64;
typedef __m64* __restrict rpM64;
typedef __m64* const __restrict crpM64;
typedef const __m64* pcM64;
typedef const __m64* const cpcM64;
typedef const __m64* __restrict rpcM64;
typedef const __m64* const __restrict crpcM64;

typedef __m128* pM128;
typedef __m128* const cpM128;
typedef __m128* __restrict rpM128;
typedef __m128* const __restrict crpM128;
typedef const __m128* pcM128;
typedef const __m128* const cpcM128;
typedef const __m128* __restrict rpcM128;
typedef const __m128* const __restrict crpcM128;
#endif // #if HAVE_SSE

#if HAVE_SSE2
typedef __m128i* pM128I;
typedef __m128i* const cpM128I;
typedef __m128i* __restrict rpM128I;
typedef __m128i* const __restrict crpM128I;
typedef const __m128i* pcM128I;
typedef const __m128i* const cpcM128I;
typedef const __m128i* __restrict rpcM128I;
typedef const __m128i* const __restrict crpcM128I;

typedef __m128d* pM128D;
typedef __m128d* const cpM128D;
typedef __m128d* __restrict rpM128D;
typedef __m128d* const __restrict crpM128D;
typedef const __m128d* pcM128D;
typedef const __m128d* const cpcM128D;
typedef const __m128d* __restrict rpcM128D;
typedef const __m128d* const __restrict crpcM128D;
#endif // #if HAVE_SSE2

#endif	// #ifndef INCLUDED_POINTER_TYPEDEFS
