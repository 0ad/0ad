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
#include "posix.h"
#include "detect.h"
#include "timer.h"
#include "sdl.h"

#if CPU_IA32
#include "sysdep/ia32.h"
#endif

#if OS_WIN
extern int win_get_gfx_info();
extern int win_get_cpu_info();
extern int win_get_snd_info();
#elif OS_UNIX
extern int unix_get_cpu_info();
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
#if defined(_SC_AVPHYS_PAGES)

	tot_mem = sysconf(_SC_PHYS_PAGES  ) * page_size;
	avl_mem = sysconf(_SC_AVPHYS_PAGES) * page_size;

// BSD / Mac OS X
#elif defined(HW_PHYSMEM)

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

char gfx_card[GFX_CARD_LEN] = "";
char gfx_drv_ver[GFX_DRV_VER_LEN] = "";

int gfx_mem = -1;	// [MiB]; approximate


// attempt to detect graphics card without OpenGL (in case ogl init fails,
// or we want more detailed info). gfx_card[] is unchanged on failure.
void get_gfx_info()
{
	// TODO: add sizeof(FB)?
	gfx_mem = (SDL_GetVideoInfo()->video_mem) / 1048576;	// [MiB]

	// try platform-specific version: they return more
	// detailed information, and don't require OpenGL to be ready.
#if OS_WIN
	if(win_get_gfx_info() < 0)
#endif
	{
		// the OpenGL version should always work, unless OpenGL isn't ready for use,
		// or we were called between glBegin and glEnd.
		ogl_get_gfx_info();
	}

	// remove crap from vendor names. (don't dare touch the model name -
	// it's too risky, there are too many different strings)
#define SHORTEN(what, chars_to_keep)\
	if(!strncmp(gfx_card, what, ARRAY_SIZE(what)-1))\
		memmove(gfx_card+chars_to_keep, gfx_card+ARRAY_SIZE(what)-1, strlen(gfx_card)-(ARRAY_SIZE(what)-1)+1);
	SHORTEN("ATI Technologies Inc.", 3);
	SHORTEN("NVIDIA Corporation", 6);
	SHORTEN("S3 Graphics", 2);					// EnumDisplayDevices
	SHORTEN("S3 Graphics, Incorporated", 2);	// GL_VENDOR
#undef SHORTEN
}


//
// CPU
//

char cpu_type[CPU_TYPE_LEN] = "";	// processor brand string is <= 48 chars
double cpu_freq = 0.f;

// -1 if detect not yet called, or cannot be determined
int cpus = -1;
int cpu_ht_units = -1;
int cpu_cores = -1;
int cpu_speedstep = -1;

static void get_cpu_info()
{
#if OS_WIN
	win_get_cpu_info();
#elif OS_UNIX
	unix_get_cpu_info();
#endif

#if CPU_IA32
	ia32_get_cpu_info();
#endif
}


void cpu_init()
{
#if CPU_IA32
	// must come before any uses of ia32.asm, e.g. by get_cpu_info
	ia32_init();

	// no longer set 24 bit (float) precision by default: for
	// very long game uptimes (> 1 day; e.g. dedicated server),
	// we need full precision when calculating the time.
	// if there's a spot where we want to speed up divides|sqrts,
	// we can temporarily change precision there.
	//_control87(_PC_24, _MCW_PC);

	// to help catch bugs, enable as many floating-point exceptions as
	// possible. that means only zero-divide, because the JS engine is
	// triggering the rest.
	_control87(_MCW_PM|_MCW_IM|_MCW_UM|_MCW_OM|_MCW_DM, _MCW_EM);

	// If possible, hook up capability-sensitive assembler routines
	ia32_hook_capabilities();
#endif

	// detects CPU clock frequency and capabilities, which are prerequisites
	// for using the TSC as a timer (desirable due to its high resolution).
	// do this before lengthy init so we can time those accurately.
	get_cpu_info();
}


//
// sound
//

char snd_card[SND_CARD_LEN];
char snd_drv_ver[SND_DRV_VER_LEN];

void get_snd_info()
{
#if OS_WIN
	win_get_snd_info();
#else
	// At least reset the values for unhandled platforms. Should perhaps do
	// something like storing the OpenAL version or similar.
	debug_assert(SND_CARD_LEN >= 8 && SND_DRV_VER_LEN >= 8);	// protect strcpy
	strcpy(snd_card, "Unknown");	// safe
	strcpy(snd_drv_ver, "Unknown");	// safe
#endif
}
