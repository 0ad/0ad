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

/**
 * =========================================================================
 * File        : os_cpu.h
 * Project     : 0 A.D.
 * Description : OS-specific support functions relating to CPU and memory
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_OS_CPU
#define INCLUDED_OS_CPU

namespace ERR
{
	const LibError OS_CPU_RESTRICTED_AFFINITY = -130100;
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
 * @return the size [MB] of physical memory.
 **/
LIB_API size_t os_cpu_MemorySize();

/**
 * @return the size [MB] of currently available memory.
 **/
LIB_API size_t os_cpu_MemoryAvailable();


//-----------------------------------------------------------------------------
// scheduling

/**
 * restrict the current thread to a set of processors.
 * it will not be rescheduled until affinity is again changed.
 *
 * @param processorMask a bit mask of acceptable processors
 * (bit index i corresponds to processor i)
 * @return the previous mask
 **/
LIB_API uintptr_t os_cpu_SetThreadAffinityMask(uintptr_t processorMask);

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
LIB_API LibError os_cpu_CallByEachCPU(OsCpuCallback cb, uintptr_t cbData);

#endif	// #ifndef INCLUDED_OS_CPU
