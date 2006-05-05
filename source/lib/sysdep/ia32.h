/**
 * =========================================================================
 * File        : ia32.h
 * Project     : 0 A.D.
 * Description : C++ and inline asm implementations for IA-32.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

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


//
// fast implementations of some sysdep.h functions; see documentation there
//

extern float ia32_rintf(float f);
extern double ia32_rint(double f);

extern float ia32_fminf(float f1, float f2);
extern float ia32_fmaxf(float f1, float f2);

extern i32 ia32_i32_from_float(float f);
extern i32 ia32_i32_from_double(double d);
extern i64 ia32_i64_from_double(double d);

// fpclassify return values
#define IA32_FP_NAN       0x0100
#define IA32_FP_NORMAL    0x0400
#define IA32_FP_INFINITE  (IA32_FP_NAN | IA32_FP_NORMAL)
#define IA32_FP_ZERO      0x4000
#define IA32_FP_SUBNORMAL (IA32_FP_NORMAL | IA32_FP_ZERO)

extern uint ia32_fpclassify(double d);
extern uint ia32_fpclassifyf(float f);

extern void* ia32_memcpy(void* dst, const void* src, size_t nbytes);	// asm


// FPU control word
// .. Precision Control:
#define IA32_MCW_PC 0x0300
#define IA32_PC_24  0x0000
// .. Rounding Control:
#define IA32_MCW_RC  0x0C00
#define IA32_RC_NEAR 0x0000
#define IA32_RC_DOWN 0x0400
#define IA32_RC_UP   0x0800
#define IA32_RC_CHOP 0x0C00
// .. Exception Mask:
#define IA32_MCW_EM 0x003f
#define IA32_EM_INVALID    BIT(0)
#define IA32_EM_DENORMAL   BIT(1)
#define IA32_EM_ZERODIVIDE BIT(2)
#define IA32_EM_OVERFLOW   BIT(3)
#define IA32_EM_UNDERFLOW  BIT(4)
#define IA32_EM_INEXACT    BIT(5)

extern uint ia32_control87(uint new_val, uint mask);	// asm


extern u64 rdtsc(void);

extern void ia32_debug_break(void);


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
	AMD_MMX_EXT   = 96+22,
	AMD_3DNOW_PRO = 96+30,
	AMD_3DNOW     = 96+31
};

extern bool ia32_cap(CpuCap cap);


extern void ia32_get_cpu_info(void);


//-----------------------------------------------------------------------------
// internal use only

// write the current execution state (e.g. all register values) into
// (Win32::CONTEXT*)pcontext (defined as void* to avoid dependency).
extern void ia32_get_current_context(void* pcontext);

extern void ia32_asm_init();

// checks if there is an IA-32 CALL instruction right before ret_addr.
// returns ERR_OK if so and ERR_FAIL if not.
// also attempts to determine the call target. if that is possible
// (directly addressed relative or indirect jumps), it is stored in
// target, which is otherwise 0.
//
// this is useful for walking the stack manually.
extern LibError ia32_get_call_target(void* ret_addr, void** target);

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
