/**
 * =========================================================================
 * File        : cpu.cpp
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

#include "precompiled.h"

#include "lib.h"

#if CPU_IA32
# include "sysdep/ia32.h"
#endif

char cpu_type[CPU_TYPE_LEN] = "";
double cpu_freq = 0.f;

// -1 if detect not yet called, or cannot be determined
int cpus = -1;
int cpu_ht_units = -1;
int cpu_cores = -1;
int cpu_speedstep = -1;


void cpu_init()
{
#if CPU_IA32
	// must come before any uses of ia32.asm, e.g. by get_cpu_info
	ia32_init();

	// no longer set 24 bit (float) precision by default: for
	// very long game uptimes (> 1 day; e.g. dedicated server),
	// we need full precision when calculating the time.
	// if there's a spot where we want to speed up divides|sqrts,
	// we can temporarily change precision there.
	//ia32_control87(IA32_PC_24, IA32_MCW_PC);

	// to help catch bugs, enable as many floating-point exceptions as
	// possible. that means only zero-divide, because the JS engine is
	// triggering the rest.
	// note: passing a flag *disables* that exception.
	ia32_control87(IA32_EM_ZERODIVIDE|IA32_EM_INVALID|IA32_EM_DENORMAL|IA32_EM_OVERFLOW|IA32_EM_UNDERFLOW|IA32_EM_INEXACT, IA32_MCW_EM);

	// no longer round toward zero (truncate). changing this setting
	// resulted in much faster float->int casts, because the compiler
	// could be told (via /QIfist) to use FISTP while still truncating
	// the result as required by ANSI C. however, FPU calculation
	// results were changed significantly, so it had to be disabled.
	//ia32_control87(IA32_RC_CHOP, IA32_MCW_RC);
#endif

	// detects CPU clock frequency and capabilities, which are prerequisites
	// for using the TSC as a timer (desirable due to its high resolution).
	// do this before lengthy init so we can time those accurately.
#if OS_WIN
	extern LibError win_get_cpu_info();
	win_get_cpu_info();
#elif OS_UNIX
	extern LibError unix_get_cpu_info();
	unix_get_cpu_info();
#endif

#if CPU_IA32
	ia32_get_cpu_info();
#endif
}


//
// memory
//

size_t tot_mem = 0;
size_t avl_mem = 0;

size_t page_size = 0;

void get_mem_status()
{
#ifdef _SC_PAGESIZE
	if(!page_size)
		page_size = (size_t)sysconf(_SC_PAGESIZE);
#endif

	// Sys V derived (GNU/Linux, Solaris)
#if defined(_SC_AVPHYS_PAGES)

	tot_mem = sysconf(_SC_PHYS_PAGES  ) * page_size;
	avl_mem = sysconf(_SC_AVPHYS_PAGES) * page_size;

	// BSD / Mac OS X
#elif defined(HW_PHYSMEM)

	size_t len = sizeof(tot_mem);
	int mib[2] = { CTL_HW, HW_PHYSMEM };
	sysctl(mib, 2, &tot_mem, &len, 0, 0);
	mib[1] = HW_USERMEM;
	sysctl(mib, 2, &avl_mem, &len, 0, 0);

#endif
}
