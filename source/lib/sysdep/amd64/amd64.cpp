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
