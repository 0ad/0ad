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

#include <stdio.h>
#include <stdlib.h>

#include "detect.h"

#include "win_internal.h"

#ifdef _MSC_VER
#pragma comment(lib, "version.lib")
#pragma comment(lib, "advapi32.lib")
// powrprof is loaded manually - we only need 1 function.
#endif


// useful for choosing a video mode. not called by detect().
// if we fail, don't change the outputs (assumed initialized to defaults)
void get_cur_resolution(int& xres, int& yres)
{
	DEVMODEA dm;
	memset(&dm, 0, sizeof(dm));
	dm.dmSize = sizeof(dm);
	// dm.dmDriverExtra already set to 0 by memset

	if(EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &dm))
	{
		xres = dm.dmPelsWidth;
		yres = dm.dmPelsHeight;
	}
}


int win_get_gfx_card()
{
	// EnumDisplayDevices is not available on Win95 or NT
	HMODULE h = LoadLibrary("user32.dll");
	int (WINAPI *pEnumDisplayDevicesA)(void*, DWORD, void*, DWORD);
	*(void**)&pEnumDisplayDevicesA = GetProcAddress(h, "EnumDisplayDevicesA");
	if(pEnumDisplayDevicesA)
	{
		DISPLAY_DEVICEA dev;
		dev.cb = sizeof(dev);
		if(pEnumDisplayDevicesA(0, 0, &dev, 0))
		{
			strcpy(gfx_card, (const char*)dev.DeviceString);
			return 0;
		}
	}

	return -1;
}


// split out so we can return on failure, instead of goto
// method: http://www.opengl.org/discussion_boards/ubb/Forum3/HTML/009679.html
int win_get_gfx_drv()
{
	// get driver DLL name
	static DEVMODE dm;	// note: dmDriverVersion is something else
	dm.dmSize = sizeof(dm);
	if(!EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dm))
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
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	cpus = si.dwNumberOfProcessors;

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
