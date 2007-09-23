/**
 * =========================================================================
 * File        : wcpu.cpp
 * Project     : 0 A.D.
 * Description : Windows backend for CPU related code
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "wcpu.h"

#include "lib/posix/posix_pthread.h"
#include "lib/posix/posix_time.h"
#include "win.h"
#include "wutil.h"
#include "winit.h"

WINIT_REGISTER_EARLY_INIT(wcpu_Init);	// wcpu -> whrt


//-----------------------------------------------------------------------------
// NumProcessors

static uint numProcessors = 0;

/// get number of CPUs (can't fail)
uint wcpu_NumProcessors()
{
	debug_assert(numProcessors != 0);
	return numProcessors;	
}

static void DetectNumProcessors()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);	// can't fail
	numProcessors = (uint)si.dwNumberOfProcessors;
}


//-----------------------------------------------------------------------------
// ClockFrequency

static double clockFrequency = -1.0;

double wcpu_ClockFrequency()
{
	debug_assert(clockFrequency > 0.0);
	return clockFrequency;
}

static void DetectClockFrequency()
{
	// read from registry
	HKEY hKey;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		DWORD freqMhz;
		DWORD size = sizeof(freqMhz);
		if(RegQueryValueEx(hKey, "~MHz", 0, 0, (LPBYTE)&freqMhz, &size) == STATUS_SUCCESS)
			clockFrequency = freqMhz * 1e6;
		else
			debug_assert(0);

		RegCloseKey(hKey);
	}
	else
		debug_assert(0);
}


//-----------------------------------------------------------------------------
// MemorySize

size_t wcpu_PageSize()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);	// can't fail
	return (size_t)si.dwPageSize;
}

size_t wcpu_MemorySize(CpuMemoryIndicators mem_type)
{
	// note: we no longer bother dynamically importing GlobalMemoryStatusEx -
	// it's available on Win2k and above. this function safely handles
	// systems with > 4 GB of memory.
	MEMORYSTATUSEX mse = { sizeof(mse) };
	BOOL ok = GlobalMemoryStatusEx(&mse);
	WARN_IF_FALSE(ok);

	// Richter, "Programming Applications for Windows": the reported
	// value doesn't include non-paged pool reserved during boot;
	// it's not considered available to kernel. (size is 528 KiB on
	// a 512 MiB WinXP/Win2k machine)
	// something similar may happen on other OSes, so it is fixed
	// by cpu.cpp instead of here.

	if(mem_type == CPU_MEM_TOTAL)
		return (size_t)mse.ullTotalPhys;
	else
		return (size_t)mse.ullAvailPhys;
}


//-----------------------------------------------------------------------------

// execute the specified function once on each CPU.
// this includes logical HT units and proceeds serially (function
// is never re-entered) in order of increasing OS CPU ID.
// note: implemented by switching thread affinity masks and forcing
// a reschedule, which is apparently not possible with POSIX.
//
// may fail if e.g. OS is preventing us from running on some CPUs.
// called from ia32.cpp get_cpu_count.
LibError wcpu_CallByEachCPU(CpuCallback cb, void* param)
{
	const HANDLE hProcess = GetCurrentProcess();
	DWORD process_affinity, system_affinity;
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

//-----------------------------------------------------------------------------

static LibError wcpu_Init()
{
	DetectNumProcessors();
	DetectClockFrequency();

	return INFO::OK;
}
