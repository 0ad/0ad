#include "precompiled.h"

#include "../cpu.h"

#include <sys/sysctl.h>


double os_cpu_ClockFrequency()
{
	return -1; // don't know
}


size_t os_cpu_NumProcessors()
{
	static size_t numProcessors;

	if(numProcessors == 0)
	{
		// Mac OS X doesn't have sysconf(_SC_NPROCESSORS_CONF)
		int mib[]={CTL_HW, HW_NCPU};
		int ncpus;
		size_t len = sizeof(ncpus);
		int ret = sysctl(mib, 2, &ncpus, &len, NULL, 0);
		debug_assert(ret != -1);
		numProcessors = (size_t)ncpus;
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
		size_t len = sizeof(memorySize);
		// Argh, the API doesn't seem to be const-correct
		/*const*/ int mib[2] = { CTL_HW, HW_PHYSMEM };
		sysctl(mib, 2, &memorySize, &len, 0, 0);
	}

	return memorySize;
}


size_t os_cpu_MemoryAvailable()
{
	size_t memoryAvailable = 0;
	size_t len = sizeof(memoryAvailable);
	// Argh, the API doesn't seem to be const-correct
	/*const*/ int mib[2] = { CTL_HW, HW_USERMEM };
	sysctl(mib, 2, &memoryAvailable, &len, 0, 0);
	return memoryAvailable;
}


uintptr_t os_cpu_SetThreadAffinityMask(uintptr_t processorMask)
{
	// not yet implemented. when doing so, see http://developer.apple.com/releasenotes/Performance/RN-AffinityAPI/

	return os_cpu_ProcessorMask();
}


LibError cpu_CallByEachCPU(OsCpuCallback cb, uintptr_t cbData)
{
	for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
	{
		os_cpu_SetThreadAffinity(processor);
		cb(processor, cbData);
	}

	return INFO::OK;
}
