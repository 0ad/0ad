/* Copyright (C) 2010 Wildfire Games.
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

#include "precompiled.h"

#if ARCH_AMD64

#include "lib/sysdep/cpu.h"
#include "lib/sysdep/arch/amd64/amd64.h"


void cpu_ConfigureFloatingPoint()
{
	// 64-bit CPUs use SSE2 for all floating-point operations, so we
	// don't need to change the FPU control word.
}

#if MSC_VERSION

// VC 2008 and ICC 12 differ in their declaration of _Interlocked*
#if ICC_VERSION
typedef __int64* P64;
#else
typedef volatile __int64* P64;
#endif

bool cpu_CAS(volatile intptr_t* location, intptr_t expected, intptr_t newValue)
{
	const intptr_t initial = _InterlockedCompareExchange64((P64)location, newValue, expected);
	return initial == expected;
}

bool cpu_CAS64(volatile i64* location, i64 expected, i64 newValue)
{
	const i64 initial = _InterlockedCompareExchange64((P64)location, newValue, expected);
	return initial == expected;
}

intptr_t cpu_AtomicAdd(volatile intptr_t* location, intptr_t increment)
{
	return _InterlockedExchangeAdd64((P64)location, increment);
}

#elif OS_MACOSX

#include <libkern/OSAtomic.h>

intptr_t cpu_AtomicAdd(volatile intptr_t* location, intptr_t increment)
{
	cassert(sizeof(intptr_t) == sizeof(int64_t));
	return OSAtomicAdd64Barrier(increment, (volatile int64_t*)location);
}

bool cpu_CAS(volatile intptr_t* location, intptr_t expected, intptr_t newValue)
{
	cassert(sizeof(intptr_t) == sizeof(void*));
	return OSAtomicCompareAndSwapPtrBarrier((void*)expected, (void*)newValue, (void* volatile*)location);
}

bool cpu_CAS64(volatile i64* location, i64 expected, i64 newValue)
{
	return OSAtomicCompareAndSwap64Barrier(expected, newValue, location);
}

#elif GCC_VERSION

intptr_t cpu_AtomicAdd(volatile intptr_t* location, intptr_t increment)
{
	return __sync_fetch_and_add(location, increment);
}

bool cpu_CAS(volatile intptr_t* location, intptr_t expected, intptr_t newValue)
{
	return __sync_bool_compare_and_swap(location, expected, newValue);
}

bool cpu_CAS64(volatile i64* location, i64 expected, i64 newValue)
{
	return __sync_bool_compare_and_swap(location, expected, newValue);
}

#endif

#endif // ARCH_AMD64
