#include "precompiled.h"

#include "../cpu.h"

#include <sys/sysctl.h>


double cpu_ClockFrequency()
{
	return -1.0; // don't know
}


size_t cpu_NumProcessors()
{
	// Mac OS X doesn't have sysconf(_SC_NPROCESSORS_CONF)
	int mib[]={CTL_HW, HW_NCPU};
	int ncpus;
	size_t len = sizeof(ncpus);
	if (sysctl(mib, 2, &ncpus, &len, NULL, 0) == -1)
		return -1; // don't know
	else
		return ncpus;
}


size_t cpu_PageSize()
{
	return (size_t)sysconf(_SC_PAGESIZE);
}


static int SysctlFromMemType(CpuMemoryIndicators mem_type)
{
	switch(mem_type)
	{
	case CPU_MEM_TOTAL:
		return HW_PHYSMEM;
	case CPU_MEM_AVAILABLE:
		return HW_USERMEM;
	}
	UNREACHABLE;
}

size_t cpu_MemorySize(CpuMemoryIndicators mem_type)
{
	size_t memory_size = 0;
	size_t len = sizeof(memory_size);
	// Argh, the API doesn't seem to be const-correct
	/*const*/ int mib[2] = { CTL_HW, SysctlFromMemType(mem_type) };
	sysctl(mib, 2, &memory_size, &len, 0, 0);
	return memory_size;
}

LibError cpu_CallByEachCPU(CpuCallback cb, void* param)
{
	// TODO: implement
	return ERR::NOT_IMPLEMENTED;
}
