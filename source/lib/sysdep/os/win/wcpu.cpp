/* Copyright (C) 2014 Wildfire Games.
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
 * Windows implementation of sysdep/cpu
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/wcpu.h"
#include "lib/sysdep/os_cpu.h"

#include "lib/bits.h"
#include "lib/alignment.h"
#include "lib/module_init.h"
#include "lib/sysdep/os/win/wutil.h"
#include "lib/sysdep/arch/x86_x64/x86_x64.h"


uintptr_t os_cpu_ProcessorMask()
{
	static uintptr_t processorMask;

	if(!processorMask)
	{
		const HANDLE hProcess = GetCurrentProcess();
		DWORD_PTR processAffinity, systemAffinity;
		const BOOL ok = GetProcessAffinityMask(hProcess, &processAffinity, &systemAffinity);
		ENSURE(ok);
		ENSURE(processAffinity != 0);
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
		ENSURE(numProcessors <= (size_t)si.dwNumberOfProcessors);
		ENSURE(numProcessors >= 1);
	}

	return numProcessors;
}


//-----------------------------------------------------------------------------

Status wcpu_ReadFrequencyFromRegistry(u32& freqMhz)
{
	HKEY hKey;
	if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
		return ERR::NOT_SUPPORTED;

	DWORD size = sizeof(freqMhz);
	LONG ret = RegQueryValueExW(hKey, L"~MHz", 0, 0, (LPBYTE)&freqMhz, &size);

	RegCloseKey(hKey);

	if(ret != ERROR_SUCCESS)
		WARN_RETURN(ERR::FAIL);

	return INFO::OK;
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
		WUTIL_FUNC(pGetLargePageMinimum, SIZE_T, (void));
		WUTIL_IMPORT_KERNEL32(GetLargePageMinimum, pGetLargePageMinimum);
		if(pGetLargePageMinimum)
		{
			largePageSize = pGetLargePageMinimum();
			// Note: checks disabled due to failing on Vista SP2 with old Xeon CPU
			//	see http://trac.wildfiregames.com/ticket/2346
			//ENSURE(largePageSize != 0);	// IA-32 and AMD64 definitely support large pages
			//ENSURE(largePageSize > os_cpu_PageSize());
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

size_t os_cpu_QueryMemorySize()
{
	MEMORYSTATUSEX mse;
	GetMemoryStatus(mse);
	DWORDLONG memorySize = mse.ullTotalPhys;

	// Richter, "Programming Applications for Windows": the reported
	// value doesn't include non-paged pool reserved during boot;
	// it's not considered available to the kernel. (the amount is
	// 528 KiB on a 512 MiB WinXP/Win2k machine). we'll round up
	// to the nearest megabyte to fix this.
	memorySize = round_up(memorySize, DWORDLONG(1*MiB));		// (Align<> cannot compute DWORDLONG)

	return size_t(memorySize / MiB);
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
			++processor;	// index among the affinity's set bits

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


//-----------------------------------------------------------------------------

static void VerifyRunningOnCorrectProcessors(DWORD_PTR affinity)
{
	DWORD currentProcessor;

	// note: NtGetCurrentProcessorNumber and RtlGetCurrentProcessorNumber aren't
	// implemented on WinXP SP2.
	WUTIL_FUNC(pGetCurrentProcessorNumber, DWORD, (void));
	WUTIL_IMPORT_KERNEL32(GetCurrentProcessorNumber, pGetCurrentProcessorNumber);
	if(pGetCurrentProcessorNumber)
		currentProcessor = pGetCurrentProcessorNumber();
	else
	{
		// note: searching for the current APIC ID or IDT address in a
		// table won't work because initializing the table also requires
		// this function. LSL only works on Vista (which already
		// has GetCurrentProcessorNumber).
		return;
	}

	ENSURE(IsBitSet(affinity, currentProcessor));
}


uintptr_t os_cpu_SetThreadAffinityMask(uintptr_t processorMask)
{
	const size_t numProcessors = os_cpu_NumProcessors();
	// (avoid undefined result when right shift count >= number of bits)
	ENSURE(numProcessors == sizeof(processorMask)*CHAR_BIT || (processorMask >> numProcessors) == 0);

	DWORD_PTR processAffinity, systemAffinity;
	const BOOL ok = GetProcessAffinityMask(GetCurrentProcess(), &processAffinity, &systemAffinity);
	WARN_IF_FALSE(ok);

	const DWORD_PTR affinity = wcpu_AffinityFromProcessorMask(processAffinity, processorMask);
	const DWORD_PTR previousAffinity = SetThreadAffinityMask(GetCurrentThread(), affinity);
	ENSURE(previousAffinity != 0);	// ensure function didn't fail
	// (MSDN says SetThreadAffinityMask takes care of rescheduling)
	VerifyRunningOnCorrectProcessors(affinity);

	const uintptr_t previousProcessorMask = wcpu_ProcessorMaskFromAffinity(processAffinity, previousAffinity);
	return previousProcessorMask;
}


Status os_cpu_CallByEachCPU(OsCpuCallback cb, uintptr_t cbData)
{
	// abort if we can't run on all system processors
	DWORD_PTR processAffinity, systemAffinity;
	{
		const BOOL ok = GetProcessAffinityMask(GetCurrentProcess(), &processAffinity, &systemAffinity);
		WARN_IF_FALSE(ok);
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
