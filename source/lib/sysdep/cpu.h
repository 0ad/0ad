/**
 * =========================================================================
 * File        : cpu.h
 * Project     : 0 A.D.
 * Description : CPU and memory detection.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_CPU
#define INCLUDED_CPU

namespace ERR
{
	const LibError CPU_FEATURE_MISSING     = -130000;
	const LibError CPU_UNKNOWN_OPCODE      = -130001;
	const LibError CPU_UNKNOWN_VENDOR      = -130002;
	const LibError CPU_RESTRICTED_AFFINITY = -130003;
}


// must be called before any of the below accessors.
extern void cpu_Init();
extern void cpu_Shutdown();

extern const char* cpu_IdentifierString();
extern double cpu_ClockFrequency();
extern uint cpu_NumPackages();	// i.e. sockets
extern uint cpu_CoresPerPackage();
extern uint cpu_LogicalPerCore();

// faster than cpu_MemorySize (caches total size determined during init),
// returns #Mebibytes (cleaned up to account e.g. for nonpaged pool)
extern size_t cpu_MemoryTotalMiB();


//
// misc (stateless)
//

// atomic "compare and swap". compare the machine word at <location> against
// <expected>; if not equal, return false; otherwise, overwrite it with
// <new_value> and return true.
extern bool cpu_CAS(volatile uintptr_t* location, uintptr_t expected, uintptr_t new_value);

// this is often used for pointers, so the macro coerces parameters to
// uintptr_t. invalid usage unfortunately also goes through without warnings.
// to catch cases where the caller has passed <expected> as <location> or
// similar mishaps, the implementation verifies <location> is a valid pointer.
#define CAS(l,o,n) cpu_CAS((volatile uintptr_t*)l, (uintptr_t)o, (uintptr_t)n)

/**
 * add a signed value to a variable without the possibility of interference
 * from other threads/CPUs.
 **/
extern void cpu_AtomicAdd(volatile intptr_t* location, intptr_t increment);

extern void cpu_Serialize();

// enforce strong memory ordering.
extern void cpu_MemoryFence();


enum CpuMemoryIndicators
{
	CPU_MEM_TOTAL, CPU_MEM_AVAILABLE
};

extern size_t cpu_MemorySize(CpuMemoryIndicators mem_type);


// drop-in replacement for libc memcpy(). only requires CPU support for
// MMX (by now universal). highly optimized for Athlon and Pentium III
// microarchitectures; significantly outperforms VC7.1 memcpy and memcpy_amd.
// for details, see accompanying article.
extern void* cpu_memcpy(void* RESTRICT dst, const void* RESTRICT src, size_t size);


/**
 * @return the number of what the OS deems "processors" or -1 on failure.
 *
 * this is used by ia32 when it cannot determine the number via APIC IDs.
 * in other situations, the cpu_NumPackages et al. functions are preferable
 * since they are more specific.
 *
 * note: this function is necessary because POSIX sysconf _SC_NPROCESSORS_CONF
 * is not suppored on MacOSX, else we would use that.
 **/
extern int cpu_OsNumProcessors();

// execute the specified function once on each CPU.
// this includes logical HT units and proceeds serially (function
// is never re-entered) in order of increasing OS CPU ID.
// note: implemented by switching thread affinity masks and forcing
// a reschedule, which is apparently not possible with POSIX.
//
// may fail if e.g. OS is preventing us from running on some CPUs.
// called from ia32.cpp get_cpu_count.

typedef void (*CpuCallback)(void* param);
extern LibError cpu_CallByEachCPU(CpuCallback cb, void* param);


// convert float to int much faster than _ftol2, which would normally be
// used by (int) casts.
extern i32 cpu_i32FromFloat(float f);
extern i32 cpu_i32FromDouble(double d);
extern i64 cpu_i64FromDouble(double d);




// Win32 CONTEXT field abstraction
// (there's no harm also defining this for other platforms)
#if CPU_AMD64
# define PC_ Rip
# define FP_ Rbp
# define SP_ Rsp
#elif CPU_IA32
# define PC_ Eip
# define FP_ Ebp
# define SP_ Esp
#endif

#endif	// #ifndef INCLUDED_CPU
