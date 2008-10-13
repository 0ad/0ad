/**
 * =========================================================================
 * File        : x86_x64.h
 * Project     : 0 A.D.
 * Description : CPU-specific routines common to 32 and 64-bit x86
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_X86_X64
#define INCLUDED_X86_X64

#if !ARCH_X86_X64
#error "including x86_x64.h without ARCH_X86_X64=1"
#endif

/**
 * registers used/returned by x86_x64_cpuid
 **/
struct x86_x64_CpuidRegs
{
	u32 eax;
	u32 ebx;
	u32 ecx;
	u32 edx;
};

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
extern bool x86_x64_cpuid(x86_x64_CpuidRegs* regs);

/**
 * CPU vendor.
 * (this is exposed because some CPUID functions are vendor-specific.)
 * (an enum is easier to compare than the original string values.)
 **/
enum x86_x64_Vendors
{
	X86_X64_VENDOR_UNKNOWN,
	X86_X64_VENDOR_INTEL,
	X86_X64_VENDOR_AMD,
};

LIB_API x86_x64_Vendors x86_x64_Vendor();


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

	// standard (edx)
	X86_X64_CAP_FPU             = 32+0,	// Floating Point Unit
	X86_X64_CAP_TSC             = 32+4,	// TimeStamp Counter
	X86_X64_CAP_CMOV            = 32+15,	// Conditional MOVe
	X86_X64_CAP_TM_SCC          = 32+22,	// Thermal Monitoring and Software Controlled Clock
	X86_X64_CAP_MMX             = 32+23,	// MultiMedia eXtensions
	X86_X64_CAP_SSE             = 32+25,	// Streaming SIMD Extensions
	X86_X64_CAP_SSE2            = 32+26,	// Streaming SIMD Extensions 2
	X86_X64_CAP_HT              = 32+28,	// HyperThreading

	// extended (ecx)
	X86_X64_CAP_AMD_CMP_LEGACY  = 64+1,	// N-core and X86_X64_CAP_HT is falsely set

	// extended (edx)
	X86_X64_CAP_AMD_MP          = 96+19,	// MultiProcessing capable; reserved on AMD64
	X86_X64_CAP_AMD_MMX_EXT     = 96+22,
	X86_X64_CAP_AMD_3DNOW_PRO   = 96+30,
	X86_X64_CAP_AMD_3DNOW       = 96+31
};

/**
 * @return whether the CPU supports the indicated x86_x64_Cap / feature flag.
 **/
LIB_API bool x86_x64_cap(x86_x64_Cap cap);


//-----------------------------------------------------------------------------
// cache

enum x86_x64_CacheType
{
	X86_X64_CACHE_TYPE_NULL,	// never passed to the callback
	X86_X64_CACHE_TYPE_DATA,
	X86_X64_CACHE_TYPE_INSTRUCTION,
	X86_X64_CACHE_TYPE_UNIFIED
	// note: further values are "reserved"
};

struct x86_x64_CacheParameters
{
	x86_x64_CacheType type;
	size_t level;
	size_t associativity;
	size_t lineSize;
	size_t sharedBy;
	size_t size;
};

typedef void (CALL_CONV *x86_x64_CacheCallback)(const x86_x64_CacheParameters*);

/**
 * call back for each cache reported by CPUID.
 *
 * note: ordering is undefined (see Intel AP-485)
 **/
LIB_API void x86_x64_EnumerateCaches(x86_x64_CacheCallback callback);

LIB_API size_t x86_x64_L1CacheLineSize();
LIB_API size_t x86_x64_L2CacheLineSize();


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
 **/
LIB_API u64 x86_x64_rdtsc();

/**
 * trigger a breakpoint inside this function when it is called.
 **/
LIB_API void x86_x64_DebugBreak(void);

/**
 * measure the CPU clock frequency via x86_x64_rdtsc and timer_Time.
 * (it follows that this must not be called from WHRT init.)
 * this takes several milliseconds (i.e. much longer than
 * os_cpu_ClockFrequency) but delivers accurate measurements.
 **/
LIB_API double x86_x64_ClockFrequency();

#endif	// #ifndef INCLUDED_X86_X64
