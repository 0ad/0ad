// IA-32 (x86) specific code
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

#ifndef _M_IX86
#error "including ia32.h without _M_IX86 defined"
#endif

#ifndef IA32_H
#define IA32_H

#include "lib.h"


extern double _ceil(double);

extern u64 rdtsc();


#ifndef _MCW_PC
#define _MCW_PC		0x0300		// Precision Control
#endif
#ifndef _PC_24
#define	_PC_24		0x0000		// 24 bits
#endif

extern uint _control87(uint new_cw, uint mask);


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
#define EXT_MP_CAPABLE BIT(19)
#define	EXT_3DNOW_PRO  BIT(30)
#define EXT_3DNOW      BIT(31)

extern long cpu_ext_caps;

extern int tsc_is_safe;


extern void ia32_get_cpu_info();


#endif	// #ifndef IA32_H
