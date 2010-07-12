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
 * CPU and memory detection.
 */

#ifndef INCLUDED_CPU
#define INCLUDED_CPU

#include "lib/sysdep/compiler.h"


namespace ERR
{
	const LibError CPU_FEATURE_MISSING     = -130000;
	const LibError CPU_UNKNOWN_OPCODE      = -130001;
	const LibError CPU_UNKNOWN_VENDOR      = -130002;
	
}


//-----------------------------------------------------------------------------
// CPU detection

/**
 * @return string identifying the CPU (usually a cleaned-up version of the
 * brand string)
 **/
LIB_API const char* cpu_IdentifierString();


//-----------------------------------------------------------------------------
// lock-free support routines

/**
 * prevent the CPU from reordering previous loads or stores past the barrier,
 * thus ensuring they retire before any subsequent memory operations.
 * this also prevents compiler reordering.
 **/
#if MSC_VERSION
# include <intrin.h>
# if !ICC_VERSION
#  pragma intrinsic(_ReadWriteBarrier)
# endif
# define cpu_MemoryBarrier() _ReadWriteBarrier()
#elif GCC_VERSION
# define cpu_MemoryBarrier() asm volatile("" : : : "memory")
#else
# define cpu_MemoryBarrier()
#endif

/**
 * atomic "compare and swap".
 *
 * @param location address of the word to compare and possibly overwrite
 * @param expected its expected value
 * @param newValue the value with which to replace it
 * @return false if the target word doesn't match the expected value,
 * otherwise true (also overwriting the contents of location)
 **/
LIB_API bool cpu_CAS(volatile intptr_t* location, intptr_t expected, intptr_t newValue);

/**
 * specialization of cpu_CAS for pointer types. this avoids error-prone
 * casting in user code.
 **/
template<typename T>
bool cpu_CAS(volatile T* location, T expected, T new_value)
{
	return cpu_CAS((volatile intptr_t*)location, (intptr_t)expected, (intptr_t)new_value);
}

/**
 * add a signed value to a variable without the possibility of interference
 * from other threads/CPUs.
 **/
LIB_API void cpu_AtomicAdd(volatile intptr_t* location, intptr_t increment);

/**
 * enforce strict instruction ordering in the CPU pipeline.
 **/
LIB_API void cpu_Serialize();

/**
 * pause in spin-wait loops, as a performance optimisation.
 **/
inline void cpu_Pause()
{
#if MSC_VER && (ARCH_IA32 || ARCH_AMD64)
	_mm_pause();
#elif GCC_VER && (ARCH_IA32 || ARCH_AMD64)
	__asm__ __volatile__( "rep; nop" : : : "memory" );
#endif
}

//-----------------------------------------------------------------------------
// misc

/**
 * drop-in replacement for POSIX memcpy().
 **/
LIB_API void* cpu_memcpy(void* RESTRICT dst, const void* RESTRICT src, size_t size);


/**
 * set the FPU control word to "desirable" values (see implementation)
 **/
LIB_API void cpu_ConfigureFloatingPoint();

// convert float to int much faster than _ftol2, which would normally be
// used by (int) casts.
// VC8 and GCC with -ffast-math now manage to generate SSE instructions,
// so our implementation is no longer needed.
#define cpu_i32FromFloat(f)  ((i32)(f))
#define cpu_i32FromDouble(d) ((i32)(d))
#define cpu_i64FromDouble(d) ((i64)(d))

#endif	// #ifndef INCLUDED_CPU
