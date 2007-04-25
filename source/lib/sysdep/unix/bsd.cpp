#include "precompiled.h"
#include "bsd.h"


static int sysctlFromMemType(CpuMemoryIndicators mem_type)
{
	switch(mem_type)
	{
	case MEM_TOTAL:
		return HW_PHYSMEM;
	case MEM_AVAILABLE:
		return HW_USERMEM;
	}
	UNREACHABLE;
}

size_t bsd_memorySize(CpuMemoryIndicators mem_type)
{
	size_t memory_size = 0;
	size_t len = sizeof(memory_size);
	const int mib[2] = { CTL_HW, sysctlFromMemType(mem_type) };
	sysctl(mib, 2, &memory_size, &len, 0, 0);
	return memory_size;
}
