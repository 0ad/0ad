#ifndef INCLUDED_WCPU
#define INCLUDED_WCPU

#include "lib/sysdep/cpu.h"

extern int wcpu_numProcessors();
extern int wcpu_isThrottlingPossible();
extern double wcpu_clockFrequency();
extern LibError wcpu_callByEachCPU(CpuCallback cb, void* param);

#endif	// #ifndef INCLUDED_WCPU
