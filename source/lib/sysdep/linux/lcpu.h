#include "lib/sysdep/cpu.h"

extern size_t lcpu_MemorySize(CpuMemoryIndicators mem_type);
extern LibError lcpu_CallByEachCPU(CpuCallback cb, void* param);
