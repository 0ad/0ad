/* Copyright (c) 2012 Wildfire Games
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

#include "lib/sysdep/os_cpu.h"
#include "lib/alignment.h"
#include "lib/bits.h"
#include "lib/module_init.h"

#if OS_LINUX
#include "valgrind.h"
#endif


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
			ENSURE(res != -1);
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


size_t os_cpu_QueryMemorySize()
{
	const uint64_t memorySize = (uint64_t)sysconf(_SC_PHYS_PAGES) * os_cpu_PageSize();
	return size_t(memorySize / MiB);
}


size_t os_cpu_MemoryAvailable()
{
	const uint64_t memoryAvailableBytes = (uint64_t)sysconf(_SC_AVPHYS_PAGES) * os_cpu_PageSize();
	const size_t memoryAvailable = size_t(memoryAvailableBytes / MiB);
	return memoryAvailable;
}

#if OS_ANDROID
// the current Android NDK (r7-crystax-4) doesn't support sched_setaffinity,
// so provide a stub implementation instead

uintptr_t os_cpu_SetThreadAffinityMask(uintptr_t UNUSED(processorMask))
{
	// not yet implemented
	return os_cpu_ProcessorMask();
}

#else

// glibc __CPU_SETSIZE=1024 is smaller than required on some Linux (4096),
// but the CONFIG_NR_CPUS in a header may not reflect the actual kernel,
// so we have to detect the limit at runtime.
// (see http://trac.wildfiregames.com/ticket/547 for additional information)
static size_t maxCpus;

static bool IsMaxCpusSufficient()
{
	const size_t setSize = CPU_ALLOC_SIZE(maxCpus);
	cpu_set_t* set = CPU_ALLOC(maxCpus);
	ENSURE(set);
	const int ret = sched_getaffinity(0, setSize, set);
	CPU_FREE(set);
	if(ret == 0)
		return true;
	ENSURE(errno == EINVAL);
	return false;
}


static Status DetectMaxCpus()
{
	// the most I have ever heard of is CONFIG_NR_CPUS=4096,
	// and even that limit should be enough for years and years.
	for(maxCpus = 64; maxCpus <= 65536; maxCpus *= 2)
	{
		if(IsMaxCpusSufficient())
			return INFO::OK;
	}
	return ERR::FAIL;
}


uintptr_t os_cpu_SetThreadAffinityMask(uintptr_t processorMask)
{
	static ModuleInitState maxCpusInitState;
	(void)ModuleInit(&maxCpusInitState, DetectMaxCpus);
	const size_t setSize = CPU_ALLOC_SIZE(maxCpus);
	cpu_set_t* set = CPU_ALLOC(maxCpus);
	ENSURE(set);

	uintptr_t previousProcessorMask = 0;
	{
		int ret = sched_getaffinity(0, setSize, set);
		ENSURE(ret == 0);

		for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
		{
			if(CPU_ISSET_S(processor, setSize, set))
				previousProcessorMask |= uintptr_t(1) << processor;
		}
	}
	
	CPU_ZERO_S(setSize, set);
	for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
	{
		if(IsBitSet(processorMask, processor))
			CPU_SET_S(processor, setSize, set);
	}

	int ret = sched_setaffinity(0, setSize, set);
	ENSURE(ret == 0);
	// (The process gets migrated immediately by the setaffinity call)

	CPU_FREE(set);
	return previousProcessorMask;
}

#endif

Status os_cpu_CallByEachCPU(OsCpuCallback cb, uintptr_t cbData)
{
	const uintptr_t previousAffinity = os_cpu_SetThreadAffinityMask(os_cpu_ProcessorMask());

	for(size_t processor = 0; processor < os_cpu_NumProcessors(); processor++)
	{
		const uintptr_t processorMask = uintptr_t(1) << processor;
		os_cpu_SetThreadAffinityMask(processorMask);
		cb(processor, cbData);
	}

	(void)os_cpu_SetThreadAffinityMask(previousAffinity);

	return INFO::OK;
}
