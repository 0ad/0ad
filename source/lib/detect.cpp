// system detect
//
// Copyright (c) 2003 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

// things missing in POSIX, SDL, and OpenGL :P

#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
#include "win.h"
#ifdef _MSC_VER
#pragma comment(lib, "version.lib")
#pragma comment(lib, "advapi32.lib")
#endif
#endif

#include "detect.h"
#include "ia32.h"
#include "time.h"
#include "ogl.h"
#include "wsdl.h"


// useful for choosing a video mode. not called by detect().
// currently not implemented for non-Win32 systems (returns 800x600).
void get_cur_resolution(int& xres, int& yres)
{
	// guess
	xres = 800; yres = 600;

#ifdef _WIN32
	static DEVMODE dm;
	dm.dmSize = sizeof(dm);
	EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dm);
	xres = dm.dmPelsWidth;
	yres = dm.dmPelsHeight;
#endif
}


size_t tot_mem = 0;
size_t avl_mem = 0;

size_t page_size = 0;

void get_mem_status()
{
// Win32
#ifdef _WIN32

	MEMORYSTATUS ms;
	GlobalMemoryStatus(&ms);
	tot_mem = round_up(ms.dwTotalPhys, 1*MB);
		// fixes results for my machine - off by 528 KB. why?!
	avl_mem = ms.dwAvailPhys;

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	page_size = si.dwPageSize;

#else	// !_WIN32

#ifdef _SC_PAGESIZE
	page_size = (size_t)sysconf(_SC_PAGESIZE);
#endif

// Sys V derived (GNU/Linux, Solaris)
#ifdef _SC_AVPHYS_PAGES

	tot_mem = sysconf(_SC_PHYS_PAGES  ) * page_size;
	avl_mem = sysconf(_SC_AVPHYS_PAGES) * page_size;

// BSD / Mac OS X
#elif HAVE_SYSCTL && defined(HW_PHYSMEM)

	size_t len = sizeof(tot_mem);
	int mib[2] = { CTL_HW, HW_PHYSMEM };
	sysctl(mib, 2, &tot_mem, &len, 0, 0);
	mib[1] = HW_USERMEM;
	sysctl(mib, 2, &avl_mem, &len, 0, 0);

#endif

#endif	// _WIN32
}


char gfx_card[64] = "unknown";
char gfx_drv[128] = "unknown";


// attempt to detect graphics card without OpenGL (in case ogl init fails,
// or we want more detailed info). gfx_card[] is unchanged on failure.
void get_gfx_card()
{
#ifdef _WIN32

	// gfx card
	// EnumDisplayDevices is not available on Win95 or NT
	HMODULE h = LoadLibrary("user32.dll");
	int (__stdcall *_EnumDisplayDevices)(void*, u32, void*, u32);
	*(void**)&_EnumDisplayDevices = GetProcAddress(h, "EnumDisplayDevicesA");
	if(_EnumDisplayDevices)
	{
		DISPLAY_DEVICE dev;
		dev.cb = sizeof(dev);
		if(_EnumDisplayDevices(0, 0, &dev, 0))
			strcpy(gfx_card, (const char*)dev.DeviceString);
	}

	// driver
	{
	// .. get driver DLL name
	static DEVMODE dm;	// note: dmDriverVersion is something else
	dm.dmSize = sizeof(dm);
	if(!EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dm))
		goto ver_fail;
	char drv_name[CCHDEVICENAME+4];
	strcpy(drv_name, (const char*)dm.dmDeviceName);
	strcat(drv_name, ".dll");
	// .. read the DLL's version info
	DWORD unused;
	DWORD ver_size = GetFileVersionInfoSize(drv_name, &unused);
	if(!ver_size)
		goto ver_fail;
	void* buf = malloc(ver_size);
	if(!buf)
		goto ver_fail;
	if(GetFileVersionInfo(drv_name, 0, ver_size, buf))
	{
		u16* lang;	// -> 16 bit language ID, 16 bit codepage
		uint lang_len;
		int ret = VerQueryValue(buf, "\\VarFileInfo\\Translation", (void**)&lang, &lang_len);
		if(ret && lang && lang_len == 4)
		{
			char subblock[37];
			sprintf(subblock, "\\StringFileInfo\\%04X%04X\\ProductName", lang[0], lang[1]);
			const char* ver;
			uint len;
			if(VerQueryValue(buf, subblock, (void**)&ver, &len))
				strncpy(gfx_drv, ver, sizeof(gfx_drv));
		}
	}
	free(buf);
	}
ver_fail:;

#endif

// driver version: http://www.opengl.org/discussion_boards/ubb/Forum3/HTML/009679.html
}


//
// CPU
//

char cpu_type[49] = "unknown";	// processor brand string is 48 chars
double cpu_freq = 0.f;

long cpu_caps = 0;
long cpu_ext_caps = 0;

// -1 if detect not yet called, or cannot be determined
int cpus = -1;
int is_notebook = -1;
int has_tsc = -1;


static void get_cpu_info()
{
#ifdef _M_IX86
	static char cpu_vendor[13];
	int family, model;

	__asm
	{
	; make sure CPUID is supported (size opt.)
		pushfd
		or			byte ptr [esp+2], 32
		popfd
		pushfd
		pop			eax
		shr			eax, 22							; bit 21 toggled?
		jnc			no_cpuid

	; get vendor string
		xor			eax, eax
		cpuid
		mov			dword ptr [cpu_vendor], ebx
		mov			dword ptr [cpu_vendor+4], edx
		mov			dword ptr [cpu_vendor+8], ecx
		; (already 0 terminated)

	; get CPU signature and std feature bits
		mov			eax, 1
		cpuid
		mov			[cpu_caps], edx
		mov			edx, eax
		shr			edx, 4
		and			edx, 0x0f
		mov			[model], edx
		shr			eax, 8
		and			eax, 0x0f
		mov			[family], eax

	; make sure CPUID ext functions are supported
		mov			eax, 0x80000000
		cpuid
		cmp			eax, 0x80000000
		jbe			no_brand_str

	; get CPU brand string (>= Athlon XP, P4)
		mov			edi, offset cpu_type
		mov			esi, -2							; loop counter: -2 to 0
	$1:	lea			eax, [0x80000004+esi]
		cpuid
		stosd
		xchg		eax, ebx
		stosd
		xchg		eax, ecx
		stosd
		xchg		eax, edx
		stosd
		inc			esi
		jle			$1
		; already 0 terminated

	; get extended feature flags
		mov			eax, 0x80000001
		cpuid
		mov			[cpu_ext_caps], edx
	}

	// strip (tm) from Athlon string
	if(!strncmp(cpu_type, "AMD Athlon(tm)", 14))
		memmove(cpu_type+10, cpu_type+14, 34);

	// remove 2x (R) and CPU freq from P4 string
	float a;	// not used; sscanf returns # fields actually stored
	if(sscanf(cpu_type, " Intel(R) Pentium(R) 4 CPU %fGHz", &a) == 1)
		strcpy(cpu_type, "Intel Pentium 4");

	goto have_brand_str;

no_brand_str:

	// AMD
	if(!strcmp(cpu_vendor, "AuthenticAMD"))
	{
		if(family == 6)
			strcpy(cpu_type, (model == 3)? "AMD Duron" : "AMD Athlon");
	}
	// Intel
	else if(!strcmp(cpu_vendor, "GenuineIntel"))
	{
		if(family == 6 && model >= 7)
			strcpy(cpu_type, "Intel Pentium III / Celeron");
	}

have_brand_str:

	// calc CPU freq (count clocks in 50 ms)
	if(cpu_caps & TSC)
	{
		u64 clocks1 = rdtsc();

		// .. wait at at least 50 ms
		double t1 = get_time();
		double t2;
		do
			t2 = get_time();
		while(t2 < t1 + 50e-3);

		u64 clocks2 = rdtsc();

		// .. freq = (clocks / 50 [ms]) / 50 [ms] * 1000
		//    cpuid/rdtsc overhead is negligible
		cpu_freq = (__int64)(clocks2-clocks1) / (t2-t1);
			// VC6 can't convert u64 -> double, and we don't need full range
	}
	// don't bother with a WAG timing loop

no_cpuid:

#endif		// #ifdef _M_IX86

#ifdef _WIN32
	HW_PROFILE_INFO hi;
	GetCurrentHwProfile(&hi);
	is_notebook = !(hi.dwDockInfo & DOCKINFO_DOCKED) ^
	              !(hi.dwDockInfo & DOCKINFO_UNDOCKED);
		// both flags set <==> this is a desktop machine.
		// both clear is unspecified; we assume it's not a notebook.

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	cpus = si.dwNumberOfProcessors;
#endif
}


void detect()
{
	get_mem_status();
	get_gfx_card();
	get_cpu_info();
}
