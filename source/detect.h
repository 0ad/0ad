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
// currently not implemented for non-Win32 systems (returns 800x600).
extern void get_cur_resolution(int& xres, int& yres);


extern char gfx_card[];

// no-op on non-Win32 systems until OpenGL is initialized.
extern void get_gfx_card();


//
// mem
//

extern unsigned long tot_mem;
extern unsigned long avl_mem;

// updates *_mem above
extern void get_mem_status();


//
// CPU
//

extern char cpu_type[];
extern double cpu_freq;

enum
{
	TSC  = BIT(4),
	CMOV = BIT(15),
	MMX  = BIT(23),
	SSE  = BIT(25),
	SSE2 = BIT(26)
};

extern long cpu_caps;

// define instead of enum to avoid stupid sign conversion warning
#define	EXT_3DNOW_PRO BIT(30)
#define EXT_3DNOW     BIT(31)

extern long cpu_ext_caps;

// -1 if detect not yet called, or cannot be determined
extern int cpus;
extern int is_notebook;



extern void detect();


#ifdef __cplusplus
}
#endif

#endif	// #ifndef __DETECT_H__
