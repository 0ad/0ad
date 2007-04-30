/**
 * =========================================================================
 * File        : cpu.h
 * Project     : 0 A.D.
 * Description : CPU and memory detection.
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

#ifndef CPU_H__
#define CPU_H__

#ifdef __cplusplus
extern "C" {
#endif

namespace ERR
{
	const LibError CPU_FEATURE_MISSING     = -130000;
	const LibError CPU_UNKNOWN_OPCODE      = -130001;
	const LibError CPU_UNKNOWN_VENDOR      = -130002;
	const LibError CPU_RESTRICTED_AFFINITY = -130003;
}


// must be called before any of the below accessors.
extern void cpu_init(void);

extern bool cpu_isModuleInitialized();


extern const char* cpu_identifierString();
extern double cpu_clockFrequency();
extern int cpu_numPackages();	// i.e. sockets
extern int cpu_coresPerPackage();
extern int cpu_logicalPerCore();
extern bool cpu_isThrottlingPossible();


//
// memory
//

enum CpuMemoryIndicators
{
	CPU_MEM_TOTAL, CPU_MEM_AVAILABLE
};

extern size_t cpu_memorySize(CpuMemoryIndicators mem_type);

// faster than cpu_memorySize (caches total size determined during init),
// returns #Mebibytes (cleaned up to account e.g. for nonpaged pool)
extern size_t cpu_memoryTotalMiB();


//
// misc
//

// atomic "compare and swap". compare the machine word at <location> against
// <expected>; if not equal, return false; otherwise, overwrite it with
// <new_value> and return true.
extern bool cpu_CAS(uintptr_t* location, uintptr_t expected, uintptr_t new_value);

// this is often used for pointers, so the macro coerces parameters to
// uinptr_t. invalid usage unfortunately also goes through without warnings.
// to catch cases where the caller has passed <expected> as <location> or
// similar mishaps, the implementation verifies <location> is a valid pointer.
#define CAS(l,o,n) cpu_CAS((uintptr_t*)l, (uintptr_t)o, (uintptr_t)n)

/**
 * add a signed value to a variable without the possibility of interference
 * from other threads/CPUs.
 **/
extern void cpu_atomic_add(intptr_t* location, intptr_t increment);

// enforce strong memory ordering.
extern void cpu_mfence();

extern void cpu_serialize();


// drop-in replacement for libc memcpy(). only requires CPU support for
// MMX (by now universal). highly optimized for Athlon and Pentium III
// microarchitectures; significantly outperforms VC7.1 memcpy and memcpy_amd.
// for details, see accompanying article.
extern void* cpu_memcpy(void* RESTRICT dst, const void* RESTRICT src, size_t size);


// execute the specified function once on each CPU.
// this includes logical HT units and proceeds serially (function
// is never re-entered) in order of increasing OS CPU ID.
// note: implemented by switching thread affinity masks and forcing
// a reschedule, which is apparently not possible with POSIX.
//
// may fail if e.g. OS is preventing us from running on some CPUs.
// called from ia32.cpp get_cpu_count.

typedef void (*CpuCallback)(void* param);
extern LibError cpu_callByEachCPU(CpuCallback cb, void* param);


// convert float to int much faster than _ftol2, which would normally be
// used by (int) casts.
extern i32 cpu_i32_from_float(float f);
extern i32 cpu_i32_from_double(double d);
extern i64 cpu_i64_from_double(double d);




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

#ifdef __cplusplus
}
#endif

#endif	// #ifndef CPU_H__
