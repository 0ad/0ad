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


namespace ERR
{
	const LibError CPU_FEATURE_MISSING     = -130000;
	const LibError CPU_UNKNOWN_OPCODE      = -130001;
	const LibError CPU_RESTRICTED_AFFINITY = -130002;
}


const size_t CPU_TYPE_LEN = 49;	// IA32 processor brand string is <= 48 chars
extern char cpu_type[CPU_TYPE_LEN];

extern double cpu_freq;


// -1 if detect not yet called, or cannot be determined:

extern "C" int cpus;			// # packages (i.e. sockets; > 1 => SMP system)
extern int cpu_ht_units;	// degree of hyperthreading, typically 2
extern int cpu_cores;		// cores per package, typically 2

extern int cpu_speedstep;


extern void cpu_init(void);


extern size_t tot_mem;
extern size_t avl_mem;

// updates *_mem above
extern void get_mem_status(void);


// atomic "compare and swap". compare the machine word at <location> against
// <expected>; if not equal, return false; otherwise, overwrite it with
// <new_value> and return true.
extern "C" bool CAS_(uintptr_t* location, uintptr_t expected, uintptr_t new_value);

// this is often used for pointers, so the macro coerces parameters to
// uinptr_t. invalid usage unfortunately also goes through without warnings.
// to catch cases where the caller has passed <expected> as <location> or
// similar mishaps, the implementation verifies <location> is a valid pointer.
#define CAS(l,o,n) CAS_((uintptr_t*)l, (uintptr_t)o, (uintptr_t)n)

extern "C" void atomic_add(intptr_t* location, intptr_t increment);

// enforce strong memory ordering.
extern "C" void mfence();

extern "C" void serialize();


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

#endif	// #ifndef CPU_H__
