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

// things missing in POSIX and SDL :P

#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include "win.h"
#endif

#include "detect.h"
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


unsigned long tot_mem = 0;
unsigned long avl_mem = 0;

void get_mem_status()
{
// Win32
#ifdef _WIN32

	MEMORYSTATUS ms;
	GlobalMemoryStatus(&ms);
	tot_mem = round_up(ms.dwTotalPhys, 1*MB);
		// fixes results for my machine - off by 528 KB. why?!
	avl_mem = ms.dwAvailPhys;

// Sys V derived (GNU/Linux, Solaris)
#elif defined(_SC_PAGESIZE) && defined(_SC_AVPHYS_PAGES)

	long page_size = sysconf(_SC_PAGESIZE);
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
}


char gfx_card[512] = "";	// = D3D8 MAX_DEVICE_IDENTIFIER_STRING

#ifdef D3D8
#include <d3d8.h>
#ifdef _MSC_VER
#pragma comment(lib, "d3d8.lib")
#endif
#endif


// no-op on non-Win32 systems until OpenGL is initialized.
void get_gfx_card()
{
	// already successfully detected
	if(gfx_card[0] != 0)
		return;

#ifdef D3D8

	IDirect3D8* d3d = Direct3DCreate8(D3D_SDK_VERSION);
	D3DADAPTER_IDENTIFIER8 id;
	d3d->GetAdapterIdentifier(D3DADAPTER_DEFAULT, D3DENUM_NO_WHQL_LEVEL, &id);
	d3d->Release();

	strcpy(gfx_card, id.Description);

#else

	char* v = (char*)glGetString(GL_VENDOR);
	if(!v)		// OpenGL probably not initialized yet
		return;
	strncpy(gfx_card, v, sizeof(gfx_card));
	if(!strcmp(gfx_card, "ATI Technologies Inc."))
		gfx_card[3] = 0;
	if(!strcmp(gfx_card, "NVIDIA Corporation"))
		gfx_card[6] = 0;
	strcat(gfx_card, (char*)glGetString(GL_RENDERER));

#endif
}


//
// CPU
//

char cpu_type[49] = "unknown CPU";	// processor brand string is 48 chars
double cpu_freq = 0.f;

long cpu_caps = 0;
long cpu_ext_caps = 0;

// -1 if detect not yet called, or cannot be determined
int cpus = -1;
int is_notebook = -1;


#ifdef _M_IX86

int has_tsc = -1;

inline u64 rdtsc()
{
	u64 c;
__asm
{
	cpuid
	rdtsc
	mov		dword ptr [c], eax
	mov		dword ptr [c+4], edx
}
	// 64 bit values are returned in edx:eax, but we do it the safe way
	return c;
}

#endif


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

	// fixup Intel's as well
	int a, b;	// not needed, but sscanf returns # fields actually stored
	if(sscanf(cpu_type, "Intel ® Pentium 4 CPU %d.%d GHz", &a, &b) == 2)
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
