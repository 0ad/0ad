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

#include "precompiled.h"

#if ARCH_AMD64

#include "amd64.h"
#include "amd64_asm.h"

#include "lib/sysdep/cpu.h"
#if MSC_VERSION
#include <intrin.h>
#endif

void cpu_ConfigureFloatingPoint()
{
	// 64-bit CPU:s apparently use SSE2 for all floating-point operations, so I
	// *guess* we don't need to do anything...
}

void* cpu_memcpy(void* RESTRICT dst, const void* RESTRICT src, size_t size)
{
	return memcpy(dst, src, size);
}

#if MSC_VERSION

bool cpu_CAS(volatile uintptr_t* location, uintptr_t expected, uintptr_t newValue)
{
	const uintptr_t initial = _InterlockedCompareExchange64((volatile __int64*)location, newValue, expected);
	return initial == expected;
}

void cpu_AtomicAdd(volatile intptr_t* location, intptr_t increment)
{
	(void)_InterlockedExchangeAdd64(location, increment);
}

#elif GCC_VERSION

void cpu_AtomicAdd(volatile intptr_t* location, intptr_t increment)
{
	amd64_AtomicAdd(location, increment);
}

bool cpu_CAS(volatile uintptr_t* location, uintptr_t expected, uintptr_t newValue)
{
	return amd64_CAS(location, expected, newValue) ? true : false;
}

#endif

#endif // ARCH_AMD64
