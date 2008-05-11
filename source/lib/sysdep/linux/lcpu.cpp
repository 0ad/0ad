#include "precompiled.h"

#include "../cpu.h"

#if OS_LINUX
#include "valgrind.h"
#endif


double cpu_ClockFrequency()
{
	return -1; // don't know
}


size_t cpu_NumProcessors()
{
	long res = sysconf(_SC_NPROCESSORS_CONF);
	if (res == -1)
		return 0;
	else
		return (size_t)res;
}


size_t cpu_PageSize()
{
	return (size_t)sysconf(_SC_PAGESIZE);
}


static int SysconfFromMemType(CpuMemoryIndicators mem_type)
{
	switch(mem_type)
	{
	case CPU_MEM_TOTAL:
		return _SC_PHYS_PAGES;
	case CPU_MEM_AVAILABLE:
		return _SC_AVPHYS_PAGES;
	}
	UNREACHABLE;
}

size_t cpu_MemorySize(CpuMemoryIndicators mem_type)
{
	const int sc_name = SysconfFromMemType(mem_type);
	const size_t pageSize = sysconf(_SC_PAGESIZE);
	const size_t memory_size = sysconf(sc_name) * pageSize;
	return memory_size;
}


LibError cpu_CallByEachCPU(CpuCallback cb, void* param)
{
	long ncpus = sysconf(_SC_NPROCESSORS_CONF);

	// Valgrind reports the number of real CPUs, but only emulates a single CPU.
	// That causes problems when we expect all those CPUs to be distinct, so
	// just pretend there's only one CPU
	if (RUNNING_ON_VALGRIND)
		ncpus = 1;

	cpu_set_t set;
	for (long i = 0; i < ncpus && i < CPU_SETSIZE; ++i)
	{
		CPU_ZERO(&set);
		CPU_SET(i, &set);

		int ret = sched_setaffinity(0, sizeof(set), &set);
		if (ret)
			WARN_RETURN(ERR::FAIL);
		// (The process gets migrated immediately by the setaffinity call)

		cb(param);
	}
	return INFO::OK;
}
