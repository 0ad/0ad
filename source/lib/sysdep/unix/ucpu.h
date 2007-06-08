#ifndef INCLUDED_UCPU
#define INCLUDED_UCPU

#include "lib/sysdep/cpu.h"

extern int ucpu_IsThrottlingPossible();
extern int ucpu_NumPackages();
extern double ucpu_ClockFrequency();
extern LibError ucpu_CallByEachCPU(CpuCallback cb, void* param);

#endif	// #ifndef INCLUDED_UCPU
