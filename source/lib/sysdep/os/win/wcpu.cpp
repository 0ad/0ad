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
 * File        : wcpu.cpp
 * Project     : 0 A.D.
 * Description : Windows implementation of sysdep/cpu
 * =========================================================================
 */

#include "precompiled.h"
#include "lib/sysdep/os_cpu.h"

#include "win.h"
#include "lib/bits.h"
#include "lib/module_init.h"

#ifdef _OPENMP
# include <omp.h>
#endif


uintptr_t os_cpu_ProcessorMask()
{
	static uintptr_t processorMask;

	if(!processorMask)
	{
		const HANDLE hProcess = GetCurrentProcess();
		DWORD_PTR processAffinity, systemAffinity;
		const BOOL ok = GetProcessAffinityMask(hProcess, &processAffinity, &systemAffinity);
		debug_assert(ok);
		processorMask = processAffinity;
	}

	return processorMask;
}


size_t os_cpu_NumProcessors()
{
	static size_t numProcessors;

	if(!numProcessors)
	{
		numProcessors = PopulationCount(os_cpu_ProcessorMask());

		// sanity check
		SYSTEM_INFO si;
		GetSystemInfo(&si);	// guaranteed to succeed
		debug_assert(numProcessors <= (size_t)si.dwNumberOfProcessors);
	}

	return numProcessors;
}


//-----------------------------------------------------------------------------

static LibError ReadFrequencyFromRegistry(DWORD& freqMhz)
{
	HKEY hKey;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
		return ERR::NO_SYS;

	DWORD size = sizeof(&freqMhz);
	LONG ret = RegQueryValueEx(hKey, "~MHz", 0, 0, (LPBYTE)&freqMhz, &size);

	RegCloseKey(hKey);

	if(ret != ERROR_SUCCESS)
		WARN_RETURN(ERR::FAIL);

	return INFO::OK;
}

double os_cpu_ClockFrequency()
{
	static double clockFrequency;

	if(clockFrequency == 0.0)
	{
		DWORD freqMhz;
		if(ReadFrequencyFromRegistry(freqMhz) == INFO::OK)
			clockFrequency = freqMhz * 1e6;
		else
			clockFrequency = -1.0;
	}

	return clockFrequency;
}


size_t os_cpu_PageSize()
{
	static size_t systemPageSize;

	if(!systemPageSize)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);	// guaranteed to succeed
		systemPageSize = (size_t)si.dwPageSize;
	}

	return systemPageSize;
}


size_t os_cpu_LargePageSize()
{
	static size_t largePageSize = ~(size_t)0;	// "0" has special significance

	if(largePageSize == ~(size_t)0)
	{
		typedef SIZE_T (WINAPI *PGetLargePageMinimum)(void);
		const HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
		const PGetLargePageMinimum pGetLargePageMinimum = (PGetLargePageMinimum)GetProcAddress(hKernel32, "GetLargePageMinimum");
		if(pGetLargePageMinimum)
		{
			largePageSize = pGetLargePageMinimum();
			debug_assert(largePageSize != 0);	// IA-32 and AMD64 definitely support large pages
			debug_assert(largePageSize > os_cpu_PageSize());
		}
		// no OS support for large pages
		else
			largePageSize = 0;
	}

	return largePageSize;
}


static void GetMemoryStatus(MEMORYSTATUSEX& mse)
{
	// note: we no longer bother dynamically importing GlobalMemoryStatusEx -
	// it's available on Win2k and above. this function safely handles
	// systems with > 4 GB of memory.
	mse.dwLength = sizeof(mse);
	const BOOL ok = GlobalMemoryStatusEx(&mse);
	WARN_IF_FALSE(ok);
}

size_t os_cpu_MemorySize()
{
	static size_t memorySizeMiB;

	if(memorySizeMiB == 0)
	{
		MEMORYSTATUSEX mse;
		GetMemoryStatus(mse);
		DWORDLONG memorySize = mse.ullTotalPhys;

		// Richter, "Programming Applications for Windows": the reported
		// value doesn't include non-paged pool reserved during boot;
		// it's not considered available to the kernel. (the amount is
		// 528 KiB on a 512 MiB WinXP/Win2k machine). we'll round up
		// to the nearest megabyte to fix this.
		memorySize = round_up(memorySize, DWORDLONG(1*MiB));

		memorySizeMiB = size_t(memorySize / MiB);
	}

	return memorySizeMiB;
}

size_t os_cpu_MemoryAvailable()
{
	MEMORYSTATUSEX mse;
	GetMemoryStatus(mse);
	const size_t memoryAvailableMiB = size_t(mse.ullAvailPhys / MiB);
	return memoryAvailableMiB;
}


//-----------------------------------------------------------------------------

DWORD_PTR wcpu_AffinityFromProcessorMask(DWORD_PTR processAffinity, uintptr_t processorMask)
{
	DWORD_PTR affinity = 0;

	size_t processor = (size_t)-1;
	for(DWORD processorNumber = 0; processorNumber < (DWORD)os_cpu_MaxProcessors; processorNumber++)
	{
		if(IsBitSet(processAffinity, processorNumber))
		{
			++processor;	// now corresponds to processorNumber

			if(IsBitSet(processorMask, processor))
				affinity |= DWORD_PTR(1) << processorNumber;
		}
	}

	return affinity;
}

uintptr_t wcpu_ProcessorMaskFromAffinity(DWORD_PTR processAffinity, DWORD_PTR affinity)
{
	uintptr_t processorMask = 0;

	size_t processor = (size_t)-1;
	for(DWORD processorNumber = 0; processorNumber < (DWORD)os_cpu_MaxProcessors; processorNumber++)
	{
		if(IsBitSet(processAffinity, processorNumber))
		{
			++processor;	// now corresponds to processorNumber

			if(IsBitSet(affinity, processorNumber))
				processorMask |= uintptr_t(1) << processor;
		}
	}

	return processorMask;
}


static const DWORD invalidProcessorNumber = (DWORD)-1;

static DWORD CurrentProcessorNumber()
{
	typedef DWORD (WINAPI *PGetCurrentProcessorNumber)(void);
	static PGetCurrentProcessorNumber pGetCurrentProcessorNumber;

	static bool initialized;
	if(!initialized)
	{
		initialized = true;
		const HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
		// note: NtGetCurrentProcessorNumber and RtlGetCurrentProcessorNumber aren't
		// implemented on WinXP SP2, so we can't use those either.
		pGetCurrentProcessorNumber = (PGetCurrentProcessorNumber)GetProcAddress(hKernel32, "GetCurrentProcessorNumber");
	}

	if(pGetCurrentProcessorNumber)
		return pGetCurrentProcessorNumber();
	else
	{
		// note: we won't bother mapping APIC IDs to processor numbers or
		// using LSL to re-implement GetCurrentProcessorNumber because
		// this routine is just a debug aid.
		return invalidProcessorNumber;
	}
}


uintptr_t os_cpu_SetThreadAffinityMask(uintptr_t processorMask)
{
	debug_assert((processorMask >> os_cpu_NumProcessors()) == 0);

	DWORD_PTR processAffinity, systemAffinity;
	const BOOL ok = GetProcessAffinityMask(GetCurrentProcess(), &processAffinity, &systemAffinity);
	debug_assert(ok);

	const DWORD_PTR affinity = wcpu_AffinityFromProcessorMask(processAffinity, processorMask);
	const DWORD_PTR previousAffinity = SetThreadAffinityMask(GetCurrentThread(), affinity);
	debug_assert(previousAffinity != 0);	// ensure function didn't fail

	// hopefully reschedule our thread
	Sleep(0);

	// verify we're running on the correct processor
	const DWORD currentProcessorNumber = CurrentProcessorNumber();
	if(currentProcessorNumber != invalidProcessorNumber)
		debug_assert(IsBitSet(affinity, currentProcessorNumber));

	const uintptr_t previousProcessorMask = wcpu_ProcessorMaskFromAffinity(processAffinity, previousAffinity);
	return previousProcessorMask;
}


LibError os_cpu_CallByEachCPU(OsCpuCallback cb, uintptr_t cbData)
{
	// abort if we can't run on all system processors
	DWORD_PTR processAffinity, systemAffinity;
	{
		const BOOL ok = GetProcessAffinityMask(GetCurrentProcess(), &processAffinity, &systemAffinity);
		debug_assert(ok);
		if(processAffinity != systemAffinity)
			return ERR::OS_CPU_RESTRICTED_AFFINITY;	// NOWARN
	}

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
