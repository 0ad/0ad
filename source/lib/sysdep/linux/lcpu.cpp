#include "precompiled.h"

#include "lib/sysdep/os_cpu.h"
#include "lib/bits.h"

#if OS_LINUX
#include "valgrind.h"
#endif


double os_cpu_ClockFrequency()
{
	return -1; // don't know
}


size_t os_cpu_NumProcessors()
{
	static size_t numProcessors;

	if(numProcessors == 0)
	{
		// Valgrind reports the number of real CPUs, but only emulates a single CPU.
		// That causes problems when we expect all those CPUs to be distinct, so
		// just pretend there's only one CPU
		if (RUNNING_ON_VALGRIND)
			numProcessors = 1;
		else
		{
			long res = sysconf(_SC_NPROCESSORS_CONF);
			debug_assert(res != -1);
			numProcessors = (size_t)res;
		}
	}

	return numProcessors;
}


uintptr_t os_cpu_ProcessorMask()
{
	static uintptr_t processorMask;

	if(!processorMask)
		processorMask = bit_mask<uintptr_t>(os_cpu_NumProcessors());

	return processorMask;
}


size_t os_cpu_PageSize()
{
	static size_t pageSize;

	if(!pageSize)
		pageSize = (size_t)sysconf(_SC_PAGESIZE);

	return pageSize;
}


size_t os_cpu_LargePageSize()
{
	// assume they're unsupported.
	return 0;
}


size_t os_cpu_MemorySize()
{
	static size_t memorySize;

	if(!memorySize)
	{
		const uint64_t memorySizeBytes = (uint64_t)sysconf(_SC_PHYS_PAGES) * os_cpu_PageSize();
		memorySize = size_t(memorySizeBytes / MiB);
	}

	return memorySize;
}


size_t os_cpu_MemoryAvailable()
{
	const uint64_t memoryAvailableBytes = (uint64_t)sysconf(_SC_AVPHYS_PAGES) * os_cpu_PageSize();
	const size_t memoryAvailable = size_t(memoryAvailableBytes / MiB);
	return memoryAvailable;
}


uintptr_t os_cpu_SetThreadAffinityMask(uintptr_t processorMask)
{
	int ret;
	cpu_set_t set;

	uintptr_t previousProcessorMask = 0;
	{
		ret = sched_getaffinity(0, sizeof(set), &set);
		debug_assert(ret == 0);

		for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
		{
			if(CPU_ISSET(processor, &set))
				previousProcessorMask |= uintptr_t(1) << processor;
		}
	}
	
	CPU_ZERO(&set);
	for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
	{
		if(IsBitSet(processorMask, processor))
			CPU_SET(processor, &set);
	}

	ret = sched_setaffinity(0, sizeof(set), &set);
	debug_assert(ret == 0);
	// (The process gets migrated immediately by the setaffinity call)

	return previousProcessorMask;
}


LibError cpu_CallByEachCPU(OsCpuCallback cb, uintptr_t cbData)
{
	for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
	{
		const uintptr_t processorMask = uintptr_t(1) << processor;
		os_cpu_SetThreadAffinityMask(processorMask);
		cb(processor, cbData);
	}

	return INFO::OK;
}
