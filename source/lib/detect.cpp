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

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "lib.h"
#include "detect.h"
#include "sysdep/ia32.h"
#include "timer.h"
#include "ogl.h"
#include "sdl.h"

// HACK
extern int win_get_gfx_card();
extern int win_get_gfx_drv();
extern int win_get_cpu_info();

/*
// useful for choosing a video mode. not called by detect().
void get_cur_resolution(int& xres, int& yres)
{
	// guess
	xres = 1024; yres = 768;
}
*/

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

char gfx_card[64] = "unknown";
char gfx_drv[128] = "unknown";


// attempt to detect graphics card without OpenGL (in case ogl init fails,
// or we want more detailed info). gfx_card[] is unchanged on failure.
inline void get_gfx_info()
{
#ifdef _WIN32
	win_get_gfx_card();
	win_get_gfx_drv();
#endif
}


//
// CPU
//

char cpu_type[64] = "unknown";	// processor brand string is 48 chars
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
