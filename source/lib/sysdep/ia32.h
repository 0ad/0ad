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

extern u64 rdtsc(void);


#ifndef _MCW_PC
#define _MCW_PC		0x0300		// Precision Control
#endif
#ifndef _PC_24
#define	_PC_24		0x0000		// 24 bits
#endif

extern uint _control87(uint new_cw, uint mask);

extern void ia32_debug_break(void);



// CPU caps (128 bits)
// do not change the order! (breaks cpuid())
enum CpuCap
{
	// standard (ecx)
	// currently only defined by Intel
	SSE3 = 0+0,	// Streaming SIMD Extensions 3
	EST  = 0+7,	// Enhanced Speedstep Technology

	// standard (edx)
	TSC  = 32+4,	// TimeStamp Counter
	CMOV = 32+15,	// Conditional MOVe
	MMX  = 32+23,	// MultiMedia eXtensions
	SSE  = 32+25,	// Streaming SIMD Extensions
	SSE2 = 32+26,	// Streaming SIMD Extensions 2
	HT   = 32+28,	// HyperThreading

	// extended (ecx)

	// extended (edx)
	// currently only defined by AMD
	MP         = 96+19,	// MultiProcessing capable; reserved on AMD64
	_3DNOW_PRO = 96+30,
	_3DNOW     = 96+31
};

extern bool ia32_cap(CpuCap cap);


extern void ia32_get_cpu_info(void);


#endif	// #ifndef IA32_H
