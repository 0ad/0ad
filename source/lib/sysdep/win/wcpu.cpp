/**
 * =========================================================================
 * File        : wcpu.cpp
 * Project     : 0 A.D.
 * Description : Windows implementation of sysdep/cpu
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "../cpu.h"

#include "win.h"
#include "lib/bits.h"


static LibError ReadFrequencyFromRegistry(DWORD* freqMhz)
{
	HKEY hKey;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
		return ERR::NO_SYS;

	DWORD size = sizeof(*freqMhz);
	LONG ret = RegQueryValueEx(hKey, "~MHz", 0, 0, (LPBYTE)freqMhz, &size);

	RegCloseKey(hKey);

	if(ret != ERROR_SUCCESS)
		WARN_RETURN(ERR::FAIL);

	return INFO::OK;
}

double cpu_ClockFrequency()
{
	DWORD freqMhz;
	if(ReadFrequencyFromRegistry(&freqMhz) < 0)
		return -1.0;

	const double clockFrequency = freqMhz * 1e6;
	return clockFrequency;
}


uint cpu_NumProcessors()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);	// can't fail
	const uint numProcessors = (uint)si.dwNumberOfProcessors;
	return numProcessors;
}


size_t cpu_PageSize()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);	// can't fail
	const size_t pageSize = (size_t)si.dwPageSize;
	return pageSize;
}


size_t cpu_MemorySize(CpuMemoryIndicators mem_type)
{
	// note: we no longer bother dynamically importing GlobalMemoryStatusEx -
	// it's available on Win2k and above. this function safely handles
	// systems with > 4 GB of memory.
	MEMORYSTATUSEX mse = { sizeof(mse) };
	BOOL ok = GlobalMemoryStatusEx(&mse);
	WARN_IF_FALSE(ok);

	if(mem_type == CPU_MEM_TOTAL)
	{
		size_t memoryTotal = (size_t)mse.ullTotalPhys;

		// Richter, "Programming Applications for Windows": the reported
		// value doesn't include non-paged pool reserved during boot;
		// it's not considered available to the kernel. (the amount is
		// 528 KiB on a 512 MiB WinXP/Win2k machine). we'll round up
		// to the nearest megabyte to fix this.
		memoryTotal = round_up(memoryTotal, 1*MiB);
		return memoryTotal;
	}
	else
	{
		const size_t memoryAvailable = (size_t)mse.ullAvailPhys;
		return memoryAvailable;
	}
}


LibError cpu_CallByEachCPU(CpuCallback cb, void* param)
{
	const HANDLE hProcess = GetCurrentProcess();
	DWORD_PTR process_affinity, system_affinity;
	if(!GetProcessAffinityMask(hProcess, &process_affinity, &system_affinity))
		WARN_RETURN(ERR::FAIL);
	// our affinity != system affinity: OS is limiting the CPUs that
	// this process can run on. fail (cannot call back for each CPU).
	if(process_affinity != system_affinity)
		WARN_RETURN(ERR::CPU_RESTRICTED_AFFINITY);

	for(DWORD_PTR cpu_bit = 1; cpu_bit != 0 && cpu_bit <= process_affinity; cpu_bit *= 2)
	{
		// check if we can switch to target CPU
		if(!(process_affinity & cpu_bit))
			continue;
		// .. and do so.
		if(!SetThreadAffinityMask(GetCurrentThread(), cpu_bit))
		{
			WARN_ERR(ERR::CPU_RESTRICTED_AFFINITY);
			continue;
		}

		// reschedule to make sure we switch CPUs.
		Sleep(1);

		cb(param);
	}

	// restore to original value
	SetThreadAffinityMask(hProcess, process_affinity);

	return INFO::OK;
}
