// system detect
//
// Copyright (c) 2003 Jan Wassenberg
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

// things missing in POSIX, SDL, and OpenGL :P
// large platform-specific routines (e.g. CPU or gfx card info)
// are split out of here.

#include "precompiled.h"

#include "lib.h"
#include "detect.h"
#include "timer.h"

#ifdef _M_IX86
extern void ia32_get_cpu_info();
#endif

#ifdef _WIN32
extern int win_get_gfx_info();
extern int win_get_cpu_info();
#endif

extern "C" int ogl_get_gfx_info();

//
// memory
//

size_t tot_mem = 0;
size_t avl_mem = 0;

size_t page_size = 0;

void get_mem_status()
{
#ifdef _SC_PAGESIZE
	if(!page_size)
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
}

//
// graphics card
//

char gfx_card[64] = "";
char gfx_drv_ver[64] = "";


// attempt to detect graphics card without OpenGL (in case ogl init fails,
// or we want more detailed info). gfx_card[] is unchanged on failure.
void get_gfx_info()
{
	int ret = -1;

	// try platform-specific versions first: they return more
	// detailed information, and don't require OpenGL to be ready.

#ifdef _WIN32
	ret = win_get_gfx_info();
#endif

	// success; done.
	if(ret == 0)
		return;

	// the OpenGL version should always work, unless OpenGL isn't ready for use,
	// or we were called between glBegin and glEnd.
	ogl_get_gfx_info();
}


//
// CPU
//

char cpu_type[64] = "";	// processor brand string is 48 chars
double cpu_freq = 0.f;

// -1 if detect not yet called, or cannot be determined
int cpus = -1;
int cpu_speedstep = -1;


static inline void get_cpu_info()
{
#ifdef _WIN32
	win_get_cpu_info();
#endif

#ifdef _M_IX86
	ia32_get_cpu_info();
#endif
}




void detect()
{
	get_mem_status();
	get_gfx_info();
	get_cpu_info();
}
