// Windows-specific system detect
//
// Copyright (c) 2004 Jan Wassenberg
//
// This file is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This file is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include "detect.h"
#include "lib.h"

#include "win_internal.h"

#ifdef _MSC_VER
#pragma comment(lib, "version.lib")
#pragma comment(lib, "advapi32.lib")
// powrprof is loaded manually - we only need 1 function.
#endif


// EnumDisplayDevices is not available on Win95 or NT.
// try to import it manually here; return -1 if not available.
// note: FreeLibrary at exit avoids BoundsChecker resource leak warnings.
static BOOL (WINAPI *pEnumDisplayDevicesA)(void*, DWORD, void*, DWORD);
static int import_EnumDisplayDevices()
{
	if(!pEnumDisplayDevicesA)
	{
		static HMODULE hUser32Dll = LoadLibrary("user32.dll");
		*(void**)&pEnumDisplayDevicesA = GetProcAddress(hUser32Dll, "EnumDisplayDevicesA");
		ONCE(atexit2(FreeLibrary, (uintptr_t)hUser32Dll, CC_STDCALL_1));
	}

	return pEnumDisplayDevicesA? 0 : -1;
}


// useful for choosing a video mode. not called by detect().
// if we fail, outputs are unchanged (assumed initialized to defaults)
int get_cur_resolution(int& xres, int& yres)
{
	DEVMODEA dm;
	memset(&dm, 0, sizeof(dm));
	dm.dmSize = sizeof(dm);
	// dm.dmDriverExtra already set to 0 by memset

	if(!EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &dm))
		return -1;

	xres = dm.dmPelsWidth;
	yres = dm.dmPelsHeight;
	return 0;
}


// useful for determining aspect ratio. not called by detect().
// if we fail, outputs are unchanged (assumed initialized to defaults)
int get_monitor_size(int& width_mm, int& height_mm)
{
	HDC dc = GetDC(0);	// dc for entire screen

	 width_mm = GetDeviceCaps(dc, HORZSIZE);
	height_mm = GetDeviceCaps(dc, VERTSIZE);

	ReleaseDC(0, dc);
	return 0;
}


int win_get_gfx_card()
{
	// make sure EnumDisplayDevices is available (as pEnumDisplayDevicesA)
	if(import_EnumDisplayDevices() < 0)
		return -1;

	DISPLAY_DEVICEA dev = { sizeof(dev) };
	if(!pEnumDisplayDevicesA(0, 0, &dev, 0))
		return -1;

	strncpy(gfx_card, (const char*)dev.DeviceString, sizeof(gfx_card)-1);
	return 0;
}


// split out so we can return on failure, instead of goto
// method: http://www.opengl.org/discussion_boards/ubb/Forum3/HTML/009679.html
int win_get_gfx_drv()
{
	// get driver DLL name
	DEVMODEA dm;
		// note: dmDriverVersion is not what we're looking for
	memset(&dm, 0, sizeof(dm));
	dm.dmSize = sizeof(dm);
		// note: dmSize isn't the first member!
	if(!EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &dm))
		return -1;
	char drv_name[CCHDEVICENAME+5];	// we add ".dll"
	strcpy(drv_name, (const char*)dm.dmDeviceName);
	strcat(drv_name, ".dll");

	// don't want to return 0 on success - we'd need to duplicate free(buf).
	// instead, set this variable and return that.
	int ret = -1;

	// read the DLL's version info
	DWORD unused;
	DWORD ver_size = GetFileVersionInfoSize(drv_name, &unused);
	if(!ver_size)
		return -1;
	void* buf = malloc(ver_size);
	if(!buf)
		return -1;
	if(GetFileVersionInfo(drv_name, 0, ver_size, buf))
	{
		u16* lang;	// -> 16 bit language ID, 16 bit codepage
		uint lang_len;
		BOOL ok = VerQueryValue(buf, "\\VarFileInfo\\Translation", (void**)&lang, &lang_len);
		if(ok && lang && lang_len == 4)
		{
			char subblock[64];
			sprintf(subblock, "\\StringFileInfo\\%04X%04X\\ProductName", lang[0], lang[1]);
			const char* ver;
			uint ver_len;
			if(VerQueryValue(buf, subblock, (void**)&ver, &ver_len))
			{
				strncpy(gfx_drv, ver, sizeof(gfx_drv)-1);
				ret = 0;	// success
			}
		}
	}
	free(buf);

	return ret;
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

	// determine whether system is a laptop.
	// (if SpeedStep detect below fails, guess SpeedStep <==> laptop)
	HW_PROFILE_INFO hi;
	GetCurrentHwProfile(&hi);
	bool is_laptop = !(hi.dwDockInfo & DOCKINFO_DOCKED) ^
	                 !(hi.dwDockInfo & DOCKINFO_UNDOCKED);
		// both flags set <==> this is a desktop machine.
		// both clear is unspecified; we assume it's not a laptop.
		// NOTE: ! is necessary (converts expression to bool)

	//
	// check for speedstep
	//

	// CallNtPowerInformation
	// (manual import because it's not supported on Win95)
	NTSTATUS (WINAPI *pCNPI)(POWER_INFORMATION_LEVEL, PVOID, ULONG, PVOID, ULONG) = 0;
	HMODULE hPowrProfDll = LoadLibrary("powrprof.dll");
	*(void**)&pCNPI = GetProcAddress(hPowrProfDll, "CallNtPowerInformation");
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

		// none of the above => don't know yet (for certain, at least).
		if(cpu_speedstep == -1)
			// guess speedstep active if on a laptop.
			// ia32 code gets a second crack at it.
			cpu_speedstep = (is_laptop)? 1 : 0;
	}
	FreeLibrary(hPowrProfDll);

	return 0;
}
