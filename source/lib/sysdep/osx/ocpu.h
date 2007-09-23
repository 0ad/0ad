#ifndef INCLUDED_OCPU
#define INCLUDED_OCPU

#include "lib/sysdep/cpu.h"

extern size_t ocpu_MemorySize(CpuMemoryIndicators mem_type);

extern int ocpu_IsThrottlingPossible();
extern int ocpu_NumPackages();
extern double ocpu_ClockFrequency();
extern LibError ocpu_CallByEachCPU(CpuCallback cb, void* param);

#endif	// #ifndef INCLUDED_OCPU
