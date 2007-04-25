#ifndef INCLUDED_UCPU
#define INCLUDED_UCPU

#include "lib/sysdep/cpu.h"

extern int ucpu_isThrottlingPossible();
extern int ucpu_numPackages();
extern double ucpu_clockFrequency();
extern LibError ucpu_callByEachCPU(CpuCallback cb, void* param);

#endif	// #ifndef INCLUDED_UCPU
