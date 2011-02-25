/* Copyright (c) 2010 Wildfire Games
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

#if !ARCH_X86_X64
#error "including x86_x64.h without ARCH_X86_X64=1"
#endif

#if MSC_VERSION
#include <intrin.h>	// __rdtsc
#endif

/**
 * registers used/returned by x86_x64_cpuid
 **/
#pragma pack(push, 1)	// (allows casting to int*)
struct x86_x64_CpuidRegs
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
LIB_API bool x86_x64_cpuid(x86_x64_CpuidRegs* regs);

/**
 * CPU vendor.
 * (this is exposed because some CPUID functions are vendor-specific.)
 * (an enum is easier to compare than the original string values.)
 **/
enum x86_x64_Vendors
{
	X86_X64_VENDOR_UNKNOWN,
	X86_X64_VENDOR_INTEL,
	X86_X64_VENDOR_AMD
};

LIB_API x86_x64_Vendors x86_x64_Vendor();


LIB_API size_t x86_x64_Model();

LIB_API size_t x86_x64_Family();


/**
 * @return the colloquial processor generation
 * (5 = Pentium, 6 = Pentium Pro/II/III / K6, 7 = Pentium4 / Athlon, 8 = Core / Opteron)
 **/
LIB_API size_t x86_x64_Generation();


/**
 * bit indices of CPU capability flags (128 bits).
 * values are defined by IA-32 CPUID feature flags - do not change!
 **/
enum x86_x64_Cap
{
	// standard (ecx) - currently only defined by Intel
	X86_X64_CAP_SSE3            = 0+0,	// Streaming SIMD Extensions 3
	X86_X64_CAP_EST             = 0+7,	// Enhanced Speedstep Technology
	X86_X64_CAP_SSSE3           = 0+9,	// Supplemental Streaming SIMD Extensions 3
	X86_X64_CAP_SSE41           = 0+19,	// Streaming SIMD Extensions 4.1
	X86_X64_CAP_SSE42           = 0+20,	// Streaming SIMD Extensions 4.2

	// standard (edx)
	X86_X64_CAP_FPU             = 32+0,  // Floating Point Unit
	X86_X64_CAP_TSC             = 32+4,  // TimeStamp Counter
	X86_X64_CAP_MSR             = 32+5,	 // Model Specific Registers
	X86_X64_CAP_CMOV            = 32+15, // Conditional MOVe
	X86_X64_CAP_TM_SCC          = 32+22, // Thermal Monitoring and Software Controlled Clock
	X86_X64_CAP_MMX             = 32+23, // MultiMedia eXtensions
	X86_X64_CAP_SSE             = 32+25, // Streaming SIMD Extensions
	X86_X64_CAP_SSE2            = 32+26, // Streaming SIMD Extensions 2
	X86_X64_CAP_HT              = 32+28, // HyperThreading

	// extended (ecx)
	X86_X64_CAP_AMD_CMP_LEGACY  = 64+1,  // N-core and X86_X64_CAP_HT is falsely set

	// extended (edx)
	X86_X64_CAP_AMD_MP          = 96+19,  // MultiProcessing capable; reserved on AMD64
	X86_X64_CAP_AMD_MMX_EXT     = 96+22,
	X86_X64_CAP_AMD_3DNOW_PRO   = 96+30,
	X86_X64_CAP_AMD_3DNOW       = 96+31
};

/**
 * @return whether the CPU supports the indicated x86_x64_Cap / feature flag.
 **/
LIB_API bool x86_x64_cap(x86_x64_Cap cap);

LIB_API void x86_x64_caps(u32* d0, u32* d1, u32* d2, u32* d3);

//-----------------------------------------------------------------------------
// cache and TLB

enum x86_x64_CacheType
{
	// (values match the CPUID.4 definition)
	X86_X64_CACHE_TYPE_NULL,
	X86_X64_CACHE_TYPE_DATA,
	X86_X64_CACHE_TYPE_INSTRUCTION,
	X86_X64_CACHE_TYPE_UNIFIED
	// note: further values are "reserved"
};

const u8 x86_x64_fullyAssociative = 0xFF;

struct x86_x64_Cache
{
	/**
	 * (used to determine if this cache is unified or disabled)
	 **/
	x86_x64_CacheType type;
	size_t level;
	size_t associativity;
	size_t lineSize;	/// [bytes]
	size_t sharedBy;
	size_t totalSize;	/// [bytes]
};

/**
 * describes all levels of a cache.
 * instruction and data caches are returned separately by the corresponding
 * accessor function; unified cache levels are reported by both.
 **/
struct x86_x64_Caches
{
	size_t numLevels;
	x86_x64_Cache* levels;
};

/**
 * @return pointer to a static x86_x64_Caches describing the instruction caches.
 **/
LIB_API const x86_x64_Caches* x86_x64_ICaches();

/**
 * @return pointer to a static x86_x64_Caches describing the data caches.
 **/
LIB_API const x86_x64_Caches* x86_x64_DCaches();

LIB_API size_t x86_x64_L1CacheLineSize();
LIB_API size_t x86_x64_L2CacheLineSize();

/**
 * Translation Lookaside Buffer.
 **/
struct x86_x64_TLB
{
	x86_x64_CacheType type;
	size_t level;
	size_t associativity;
	size_t pageSize;
	size_t entries;
};

/**
 * describes all levels of a TLB.
 **/
struct x86_x64_TLBs
{
	size_t numLevels;
	x86_x64_TLB* levels;
};

/**
 * @return pointer to a static x86_x64_TLB describing the instruction TLBs.
 **/
LIB_API const x86_x64_TLBs* x86_x64_ITLBs();

/**
 * @return pointer to a static x86_x64_TLB describing the data TLB.
 **/
LIB_API const x86_x64_TLBs* x86_x64_DTLBs();

/**
 * @return coverage, i.e. total size [MiB] of the given TLBs
 **/
LIB_API size_t x86_x64_TLBCoverage(const x86_x64_TLBs* tlb);


//-----------------------------------------------------------------------------
// stateless

/**
 * @return APIC ID of the currently executing processor or zero if the
 * platform does not have an xAPIC (i.e. 7th generation x86 or below).
 *
 * rationale: the alternative of accessing the APIC mmio registers is not
 * feasible - mahaf_MapPhysicalMemory only works reliably on WinXP. we also
 * don't want to intefere with the OS's constant use of the APIC registers.
 **/
LIB_API u8 x86_x64_ApicId();

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
#define x86_x64_rdtsc __rdtsc
#else
LIB_API u64 x86_x64_rdtsc();
#endif

/**
 * trigger a breakpoint inside this function when it is called.
 **/
LIB_API void x86_x64_DebugBreak();

/**
 * measure the CPU clock frequency via x86_x64_rdtsc and timer_Time.
 * (it follows that this must not be called from WHRT init.)
 * this takes several milliseconds (i.e. much longer than
 * os_cpu_ClockFrequency) but delivers accurate measurements.
 **/
LIB_API double x86_x64_ClockFrequency();

#endif	// #ifndef INCLUDED_X86_X64
