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

#ifndef __DETECT_H__
#define __DETECT_H__

#include "misc.h"

#ifdef __cplusplus
extern "C" {
#endif


// useful for choosing a video mode. not called by detect().
// if we fail, don't change the outputs (assumed initialized to defaults)
extern void get_cur_resolution(int& xres, int& yres);


extern char gfx_card[64];	// default: "unknown"
extern char gfx_drv[128];	// default: "unknown"

// attempt to detect graphics card without OpenGL (in case ogl init fails,
// or we want more detailed info). gfx_card[] is unchanged on failure.
extern void get_gfx_info();


//
// mem
//

extern size_t tot_mem;
extern size_t avl_mem;

// updates *_mem above
extern void get_mem_status();


//
// CPU
//

extern char cpu_type[];
extern double cpu_freq;

// -1 if detect not yet called, or cannot be determined
extern int cpus;
extern int cpu_speedstep;

extern void detect();


#ifdef __cplusplus
}
#endif

#endif	// #ifndef __DETECT_H__
