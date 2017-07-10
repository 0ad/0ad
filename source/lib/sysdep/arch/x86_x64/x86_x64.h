/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * CPU-specific routines common to 32 and 64-bit x86
 */

#ifndef INCLUDED_X86_X64
#define INCLUDED_X86_X64

#include "lib/lib_api.h"

#if !ARCH_X86_X64
#error "including x86_x64.h without ARCH_X86_X64=1"
#endif

#if MSC_VERSION
#include <intrin.h>	// __rdtsc
#endif

namespace x86_x64 {

/**
 * registers used/returned by cpuid
 **/
#pragma pack(push, 1)	// (allows casting to int*)
struct CpuidRegs
{
	u32 eax;
	u32 ebx;
	u32 ecx;
	u32 edx;
};
#pragma pack(pop)

/**
 * invoke CPUID instruction.
 * @param regs input/output registers.
 *   regs->eax must be set to the desired function.
 *   some functions (e.g. 4) require regs->ecx to be set as well.
 *   rationale: this interface (input/output structure vs. function parameters)
 *     avoids unnecessary copying/initialization if some inputs aren't needed
 *     and allows graceful expansion to functions that require further inputs.
 * @return true on success or false if the sub-function isn't supported.
 **/
LIB_API bool cpuid(CpuidRegs* regs);

/**
 * CPU vendor.
 * (this is exposed because some CPUID functions are vendor-specific.)
 * (an enum is easier to compare than the original string values.)
 **/
enum Vendors
{
	VENDOR_UNKNOWN,
	VENDOR_INTEL,
	VENDOR_AMD
};

LIB_API Vendors Vendor();


enum Models
{
	MODEL_NEHALEM_EP     = 0x1A, // Bloomfield (X35xx), Gainestown (X55xx)
	MODEL_NEHALEM_EP_2   = 0x1E, // Clarksfield, Lynnfield (X34xx), Jasper Forest (C35xx, C55xx)
	MODEL_I7_I5          = 0x1F, // similar to 1E; mentioned in 253665-041US, no codename known
	MODEL_CLARKDALE      = 0x25, // Arrandale, Clarkdale (L34xx)
	MODEL_WESTMERE_EP    = 0x2C, // Gulftown (X36xx, X56xx)
	MODEL_NEHALEM_EX     = 0x2E, // Beckton (X75xx)
	MODEL_WESTMERE_EX    = 0x2F, // Gulftown uarch, Beckton package (E7-48xx)
	MODEL_SANDY_BRIDGE   = 0x2A, // (E3-12xx, E5-26xx)
	MODEL_SANDY_BRIDGE_2 = 0x2D, // (E5-26xx, E5-46xx)
};

LIB_API size_t Model();

LIB_API size_t Family();


/**
 * @return the colloquial processor generation
 * (5 = Pentium, 6 = Pentium Pro/II/III / K6, 7 = Pentium4 / Athlon, 8 = Core / Opteron)
 **/
LIB_API size_t Generation();


/**
 * bit indices of CPU capability flags (128 bits).
 * values are defined by IA-32 CPUID feature flags - do not change!
 **/
enum Caps
{
	// standard (ecx) - currently only defined by Intel
	CAP_SSE3            = 0+0,	// Streaming SIMD Extensions 3
	CAP_EST             = 0+7,	// Enhanced Speedstep Technology
	CAP_SSSE3           = 0+9,	// Supplemental Streaming SIMD Extensions 3
	CAP_SSE41           = 0+19,	// Streaming SIMD Extensions 4.1
	CAP_SSE42           = 0+20,	// Streaming SIMD Extensions 4.2

	// standard (edx)
	CAP_FPU             = 32+0,  // Floating Point Unit
	CAP_TSC             = 32+4,  // TimeStamp Counter
	CAP_MSR             = 32+5,	 // Model Specific Registers
	CAP_CMOV            = 32+15, // Conditional MOVe
	CAP_TM_SCC          = 32+22, // Thermal Monitoring and Software Controlled Clock
	CAP_MMX             = 32+23, // MultiMedia eXtensions
	CAP_SSE             = 32+25, // Streaming SIMD Extensions
	CAP_SSE2            = 32+26, // Streaming SIMD Extensions 2
	CAP_HT              = 32+28, // HyperThreading

	// extended (ecx)
	CAP_AMD_CMP_LEGACY  = 64+1,  // N-core and CAP_HT is falsely set

	// extended (edx)
	CAP_AMD_MP          = 96+19,  // MultiProcessing capable; reserved on AMD64
	CAP_AMD_MMX_EXT     = 96+22,
	CAP_AMD_3DNOW_PRO   = 96+30,
	CAP_AMD_3DNOW       = 96+31
};

/**
 * @return whether the CPU supports the indicated Cap / feature flag.
 **/
LIB_API bool Cap(Caps cap);

LIB_API void GetCapBits(u32* d0, u32* d1, u32* d2, u32* d3);


//-----------------------------------------------------------------------------
// stateless

/**
 * @return the current value of the TimeStampCounter (a counter of
 * CPU cycles since power-on, which is useful for high-resolution timing
 * but potentially differs between multiple CPUs)
 *
 * notes:
 * - a macro avoids call overhead, which is important for TIMER_ACCRUE.
 * - x64 RDTSC writes to edx:eax and clears the upper halves of rdx and rax.
 **/
#if MSC_VERSION
static inline u64 rdtsc() { return __rdtsc(); }
#else
LIB_API u64 rdtsc();
#endif

/**
 * trigger a breakpoint inside this function when it is called.
 **/
LIB_API void DebugBreak();

/**
 * measure the CPU clock frequency via rdtsc and timer_Time.
 * (it follows that this must not be called from WHRT init.)
 * this takes several milliseconds (i.e. much longer than
 * os_cpu_ClockFrequency) but delivers accurate measurements.
 **/
LIB_API double ClockFrequency();

}	// namespace x86_x64

#endif	// #ifndef INCLUDED_X86_X64
