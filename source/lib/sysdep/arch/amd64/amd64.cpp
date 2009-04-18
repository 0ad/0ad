/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
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
