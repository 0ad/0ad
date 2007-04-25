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

#include "ia32_asm.h"
#include "ia32_memcpy.h"


// call before any of the following functions
extern void ia32_init();


// fpclassify return values
#define IA32_FP_NAN       0x0100
#define IA32_FP_NORMAL    0x0400
#define IA32_FP_INFINITE  (IA32_FP_NAN | IA32_FP_NORMAL)
#define IA32_FP_ZERO      0x4000
#define IA32_FP_SUBNORMAL (IA32_FP_NORMAL | IA32_FP_ZERO)

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



extern void ia32_mfence();
extern void ia32_serialize();


extern u64 ia32_rdtsc_safe(void);
#if CONFIG_RETURN64_EDX_EAX
# define ia32_rdtsc ia32_asm_rdtsc_edx_eax
#else
# define ia32_rdtsc ia32_rdtsc_safe
#endif

extern void ia32_debug_break(void);


// order in which registers are stored in CPUID regs array
// (do not change! brand string relies on this ordering)
enum IA32Regs
{
	EAX,
	EBX,
	ECX,
	EDX
};

// CPU capability flags (128 bits)
// do not change the order!
enum IA32Cap
{
	// standard (ecx) - currently only defined by Intel
	IA32_CAP_SSE3 = 0+0,	// Streaming SIMD Extensions 3
	IA32_CAP_EST  = 0+7,	// Enhanced Speedstep Technology

	// standard (edx)
	IA32_CAP_FPU  = 32+0,	// Floating Point Unit
	IA32_CAP_TSC  = 32+4,	// TimeStamp Counter
	IA32_CAP_CMOV = 32+15,	// Conditional MOVe
	IA32_CAP_MMX  = 32+23,	// MultiMedia eXtensions
	IA32_CAP_SSE  = 32+25,	// Streaming SIMD Extensions
	IA32_CAP_SSE2 = 32+26,	// Streaming SIMD Extensions 2
	IA32_CAP_HT   = 32+28,	// HyperThreading

	// extended (ecx)

	// extended (edx) - currently only defined by AMD
	IA32_CAP_AMD_MP        = 96+19,	// MultiProcessing capable; reserved on AMD64
	IA32_CAP_AMD_MMX_EXT   = 96+22,
	IA32_CAP_AMD_3DNOW_PRO = 96+30,
	IA32_CAP_AMD_3DNOW     = 96+31
};

// indicate if the CPU supports the indicated cap.
extern bool ia32_cap(IA32Cap cap);

// checks if there is an IA-32 CALL instruction right before ret_addr.
// returns INFO::OK if so and ERR::FAIL if not.
// also attempts to determine the call target. if that is possible
// (directly addressed relative or indirect jumps), it is stored in
// target, which is otherwise 0.
//
// this is useful for walking the stack manually.
extern LibError ia32_get_call_target(void* ret_addr, void** target);


extern const char* ia32_identifierString();
extern int ia32_isThrottlingPossible();
extern double ia32_clockFrequency();

extern uint ia32_logicalPerPackage();
extern uint ia32_coresPerPackage();
extern uint ia32_numPackages();

#endif	// #ifndef IA32_H
