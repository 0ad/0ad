#include "precompiled.h"

#include "lib.h"
#include "win_internal.h"

#include "cpu.h"


// not possible with POSIX calls.
static int on_each_cpu(void(*cb)())
{
	const HANDLE hProcess = GetCurrentProcess();

	DWORD process_affinity, system_affinity;
	if(!GetProcessAffinityMask(hProcess, &process_affinity, &system_affinity))
		return -1;

	// our affinity != system affinity: OS is limiting the CPUs that
	// this process can run on. fail (cannot call back for each CPU).
	if(process_affinity != system_affinity)
		return -1;

	for(DWORD cpu_bit = 1; cpu_bit != 0 && cpu_bit <= process_affinity; cpu_bit *= 2)
	{
		// check if we can switch to target CPU
		if(!(process_affinity & cpu_bit))
			continue;
		// .. and do so.
		if(!SetProcessAffinityMask(hProcess, process_affinity))
		{
			debug_warn("SetProcessAffinityMask failed");
			continue;
		}

		// reschedule, to make sure we switch CPUs
		Sleep(0);

		cb();
	}

	// restore to original value
	SetProcessAffinityMask(hProcess, process_affinity);

	return 0;
}


static void check_smp()
{
	on_each_cpu(cpu_check_smp);
}


static void check_speedstep()
{
	// CallNtPowerInformation
	// (manual import because it's not supported on Win95)
	NTSTATUS (WINAPI *pCNPI)(POWER_INFORMATION_LEVEL, PVOID, ULONG, PVOID, ULONG) = 0;
	HMODULE hPowrprofDll = LoadLibrary("powrprof.dll");
	*(void**)&pCNPI = GetProcAddress(hPowrprofDll, "CallNtPowerInformation");
	if(pCNPI)
	{
		// most likely not speedstep-capable if these aren't supported
		SYSTEM_POWER_CAPABILITIES spc;
		if(pCNPI(SystemPowerCapabilities, 0, 0, &spc, sizeof(spc)) == STATUS_SUCCESS)
			if(!spc.ProcessorThrottle || !spc.ThermalControl)
				cpu_speedstep = 0;

		// probably speedstep if cooling mode active.
		// the documentation of PO_TZ_* is unclear, so we can't be sure.
		SYSTEM_POWER_INFORMATION spi;
		if(pCNPI(SystemPowerInformation, 0, 0, &spi, sizeof(spi)) == STATUS_SUCCESS)
			if(spi.CoolingMode != PO_TZ_INVALID_MODE)
				cpu_speedstep = 1;

		// definitely speedstep if a CPU has thermal throttling active.
		// note that we don't care about user-defined throttles
		// (see ppi.CurrentMhz) - they don't change often.
		ULONG ppi_buf_size = cpus * sizeof(PROCESSOR_POWER_INFORMATION);
		void* ppi_buf = malloc(ppi_buf_size);
		if(pCNPI(ProcessorInformation, 0, 0, ppi_buf, ppi_buf_size) == STATUS_SUCCESS)
		{
			PROCESSOR_POWER_INFORMATION* ppi = (PROCESSOR_POWER_INFORMATION*)ppi_buf;
			for(int i = 0; i < cpus; i++)
				// thermal throttling currently active
				if(ppi[i].MaxMhz != ppi[i].MhzLimit)
				{
					cpu_speedstep = 1;
					break;
				}
		}
		free(ppi_buf);
	}
	FreeLibrary(hPowrprofDll);
		// this is most likely the only reference,
		// so don't free it (=> unload) until done with the DLL.

	// CallNtPowerInformation not available, or none of the above apply:
	// don't know yet (for certain, at least).
	if(cpu_speedstep == -1)
	{
		// check if running on a laptop
		HW_PROFILE_INFO hi;
		GetCurrentHwProfile(&hi);
		bool is_laptop = !(hi.dwDockInfo & DOCKINFO_DOCKED) ^ !(hi.dwDockInfo & DOCKINFO_UNDOCKED);
			// both flags set <==> this is a desktop machine.
			// both clear is unspecified; we assume it's not a laptop.
			// NOTE: ! is necessary (converts expression to bool)

		// we'll guess SpeedStep is active if on a laptop.
		// ia32 code will get a second crack at it.
		cpu_speedstep = (is_laptop)? 1 : 0;
	}
}


int win_get_cpu_info()
{
	// get number of CPUs (can't fail)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	cpus = si.dwNumberOfProcessors;

	// read CPU frequency from registry
	HKEY hKey;
	const char* key = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_QUERY_VALUE, &hKey) == 0)
	{
		DWORD freq_mhz;
		DWORD size = sizeof(freq_mhz);
		if(RegQueryValueEx(hKey, "~MHz", 0, 0, (LPBYTE)&freq_mhz, &size) == 0)
			cpu_freq = freq_mhz * 1e6;

		RegCloseKey(hKey);
	}

	check_speedstep();
	check_smp();

	return 0;
}
