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

/*
 * OS-specific support functions relating to CPU and memory
 */

#ifndef INCLUDED_OS_CPU
#define INCLUDED_OS_CPU

namespace ERR
{
	const Status OS_CPU_RESTRICTED_AFFINITY = -130100;
}


//-----------------------------------------------------------------------------
// processor topology

// processor ID = [0, os_cpu_NumProcessors())
// they are a numbering of the bits of the process affinity mask where the
// least significant nonzero bit corresponds to ID 0.
// rationale: this spares users from having to deal with noncontiguous IDs,
// e.g. when administrative tools are used to restrict process affinity.


/**
 * maximum number of processors supported by the OS (determined by the
 * number of bits in an affinity mask)
 **/
static const size_t os_cpu_MaxProcessors = sizeof(uintptr_t)*CHAR_BIT;

/**
 * @return bit mask of processors that exist and are available to
 * this process.
 * its population count is by definition equal to os_cpu_NumProcessors().
 **/
LIB_API uintptr_t os_cpu_ProcessorMask();

/**
 * @return the number of processors available to this process.
 *
 * note: this function is necessary because POSIX sysconf _SC_NPROCESSORS_CONF
 * is not suppored on MacOSX, else we would use that.
 **/
LIB_API size_t os_cpu_NumProcessors();

// note: we do not provide an os_cpu_CurrentProcessor routine. that would
// require Windows 2003 or a lot of work. worse, its results would be
// worthless because they may change immediately afterwards. instead,
// the recommended approach is to pin OpenMP threads (whose ID can be
// queried) to the processor with the same number.


//-----------------------------------------------------------------------------
// CPU and memory characteristics

/**
 * @return a rough estimate of the CPU clock frequency.
 * this is usually accurate to a few MHz and is faster than measurement loops.
 **/
LIB_API double os_cpu_ClockFrequency();

/**
 * @return the size [bytes] of a MMU page (4096 on most IA-32 systems)
 **/
LIB_API size_t os_cpu_PageSize();

/**
 * @return the size [bytes] of a large MMU page (4 MiB on most IA-32 systems)
 * or zero if they are not supported.
 **/
LIB_API size_t os_cpu_LargePageSize();

/**
 * @return the size [MB] of physical memory as reported by the OS;
 * no caching/validation is performed.
 **/
LIB_API size_t os_cpu_QueryMemorySize();

/**
 * @return the size [MB] of physical memory; caches the result of
 * os_cpu_QueryMemorySize and overrides it with a more exact value
 * if SMBIOS information is available.
 **/
LIB_API size_t os_cpu_MemorySize();

/**
 * @return the current amount [MB] of available memory.
 **/
LIB_API size_t os_cpu_MemoryAvailable();


//-----------------------------------------------------------------------------
// scheduling

/**
 * restrict the current thread to a set of processors.
 *
 * @param processorMask a bit mask of acceptable processors
 * (bit index i corresponds to processor i)
 * @return the previous mask
 **/
LIB_API uintptr_t os_cpu_SetThreadAffinityMask(uintptr_t processorMask);

class os_cpu_ScopedSetThreadAffinityMask
{
public:
	os_cpu_ScopedSetThreadAffinityMask(uintptr_t processorMask)
		: m_previousProcessorMask(os_cpu_SetThreadAffinityMask(processorMask))
	{
	}

	~os_cpu_ScopedSetThreadAffinityMask()
	{
		(void)os_cpu_SetThreadAffinityMask(m_previousProcessorMask);
	}

private:
	uintptr_t m_previousProcessorMask;
};


/**
 * called by os_cpu_CallByEachCPU.
 * @param processor ID of processor running the current thread for the
 * duration of this function.
 * @param cbData user-specified data passed through os_cpu_CallByEachCPU.
 **/
typedef void (*OsCpuCallback)(size_t processor, uintptr_t cbData);

/**
 * execute the specified function once on each processor.
 * this proceeds serially (the callback is never reentered) in increasing
 * order of processor ID.
 * fails if process affinity prevents running on all processors.
 **/
LIB_API Status os_cpu_CallByEachCPU(OsCpuCallback cb, uintptr_t cbData);

#endif	// #ifndef INCLUDED_OS_CPU
