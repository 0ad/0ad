#include "precompiled.h"
#include "amd64.h"

#include "lib/sysdep/cpu.h"
#if MSC_VERSION
#include <intrin.h>
#endif


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

#endif

// note: ACPI processor detection not yet implemented here, so we treat
// dual-core systems as multiprocessors.

size_t cpu_NumPackages()
{
	return cpu_NumProcessors();
}

size_t cpu_CoresPerPackage()
{
	return 1;
}
