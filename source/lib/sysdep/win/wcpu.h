#ifndef INCLUDED_WCPU
#define INCLUDED_WCPU

#include "lib/sysdep/cpu.h"

extern uint wcpu_NumProcessors();
extern int wcpu_IsThrottlingPossible();
extern double wcpu_ClockFrequency();
extern LibError wcpu_CallByEachCPU(CpuCallback cb, void* param);

#endif	// #ifndef INCLUDED_WCPU
