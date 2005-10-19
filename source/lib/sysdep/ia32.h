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

#ifndef IA32_H
#define IA32_H

#if !CPU_IA32
#error "including ia32.h without CPU_IA32=1"
#endif

#include "lib/types.h"

// some of these are implemented in asm, so make sure name mangling is
// disabled.
#ifdef __cplusplus
extern "C" {
#endif


// call before any of the following functions
extern void ia32_init();

extern double _ceil(double);

extern u64 rdtsc(void);


// these may have been defined by system headers; we redefine them to
// the real IA-32 values for use with ia32_control87.
#undef _MCW_PC
#define _MCW_PC 0x0300		// Precision Control
#undef _PC_24
#define _PC_24  0x0000		// 24 bits
#undef _MCW_EM
#define _MCW_EM 0x003f		// Exception Mask
#undef _MCW_PM
#define _MCW_PM BIT(5)
#ifndef _MCW_IM
# define _MCW_IM 0x0001		// Invalid Operation Mask
#endif
#ifndef _MCW_DM
# define _MCW_DM 0x0002		// Denormal Operand Mask
#endif
#ifndef _MCW_OM
# define _MCW_OM 0x0008		// Overflow Mask
#endif
#ifndef _MCW_UM
# define _MCW_UM 0x00010	// Underflow Mask
#endif
#ifndef _MCW_PM
# define _MCW_PM 0x0020		// Precision Mask
#endif

#define _control87 ia32_control87
extern uint ia32_control87(uint new_cw, uint mask);	// asm

extern void ia32_debug_break(void);

extern void ia32_memcpy(void* dst, const void* src, size_t nbytes);


// CPU caps (128 bits)
// do not change the order!
enum CpuCap
{
	// standard (ecx) - currently only defined by Intel
	SSE3 = 0+0,		// Streaming SIMD Extensions 3
	EST  = 0+7,		// Enhanced Speedstep Technology

	// standard (edx)
	TSC  = 32+4,	// TimeStamp Counter
	CMOV = 32+15,	// Conditional MOVe
	MMX  = 32+23,	// MultiMedia eXtensions
	SSE  = 32+25,	// Streaming SIMD Extensions
	SSE2 = 32+26,	// Streaming SIMD Extensions 2
	HT   = 32+28,	// HyperThreading

	// extended (ecx)

	// extended (edx) - currently only defined by AMD
	AMD_MP        = 96+19,	// MultiProcessing capable; reserved on AMD64
	AMD_3DNOW_PRO = 96+30,
	AMD_3DNOW     = 96+31
};

extern bool ia32_cap(CpuCap cap);


extern void ia32_get_cpu_info(void);
extern void ia32_hook_capabilities(void);


// internal use only

// order in which registers are stored in regs array
// (do not change! brand string relies on this ordering)
enum IA32Regs
{
	EAX,
	EBX,
	ECX,
	EDX
};

// try to call the specified CPUID sub-function. returns true on success or
// false on failure (i.e. CPUID or the specific function not supported).
// returns eax, ebx, ecx, edx registers in above order.
extern bool ia32_cpuid(u32 func, u32* regs);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef IA32_H
