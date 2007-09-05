/**
 * =========================================================================
 * File        : cpu.cpp
 * Project     : 0 A.D.
 * Description : CPU and memory detection.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "cpu.h"

#include "lib/bits.h"
#include "lib/module_init.h"
#include "lib/posix/posix.h"
#if CPU_IA32
# include "lib/sysdep/ia32/ia32.h"
# include "lib/sysdep/ia32/ia32_memcpy.h"
#endif
#if OS_BSD
# include "lib/sysdep/unix/bsd.h"
#endif
#if OS_UNIX
# include "lib/sysdep/unix/ucpu.h"
#endif
#if OS_WIN
# include "lib/sysdep/win/wcpu.h"
#endif


ERROR_ASSOCIATE(ERR::CPU_FEATURE_MISSING, "This CPU doesn't support a required feature", -1);
ERROR_ASSOCIATE(ERR::CPU_UNKNOWN_OPCODE, "Disassembly failed", -1);
ERROR_ASSOCIATE(ERR::CPU_UNKNOWN_VENDOR, "CPU vendor unknown", -1);
ERROR_ASSOCIATE(ERR::CPU_RESTRICTED_AFFINITY, "Cannot set desired CPU affinity", -1);


//-----------------------------------------------------------------------------
#pragma region Accessor functions
// insulate caller from the system-specific modules and cache results.
// note: the providers sometimes need to store the results anyway, so we
// don't need to do caching in those cases.
// these are set once during cpu_Init since they're usually all used and
// we thus avoid needing if(already_called) return old_result.
// initially set to 'impossible' values to catch uses before cpu_Init.

static double clockFrequency = -1.0;

double cpu_ClockFrequency()
{
	debug_assert(clockFrequency > 0.0);
	return clockFrequency;
}

static void DetectClockFrequency()
{
#if OS_WIN
	clockFrequency = wcpu_ClockFrequency();
	// success; we stick with this value because it either doesn't matter
	// (WHRT isn't using the TSC), or cannot be determined more accurately
	// (ia32 will use WHRT's TSC to measure its own frequency).
	// bonus: the wcpu function is much faster than ia32's measurement loop.
	if(clockFrequency > 0.0)
		return;
#endif

#if CPU_IA32
	clockFrequency = ia32_ClockFrequency();	// authoritative, precise
#endif
}


static size_t memoryTotalMib = 1;

size_t cpu_MemoryTotalMiB()
{
	debug_assert(memoryTotalMib > 1);
	return memoryTotalMib;
}

static void DetectMemory()
{
	size_t memoryTotal = cpu_MemorySize(CPU_MEM_TOTAL);

	// account for inaccurate reporting by rounding up (see wposix sysconf)
	const size_t memoryTotalPow2 = (size_t)round_up_to_pow2((uint)memoryTotal);
	// .. difference too great, just round up to 1 MiB
	if(memoryTotalPow2 - memoryTotal > 3*MiB)
		memoryTotal = round_up(memoryTotal, 1*MiB);
	// .. difference acceptable, use next power of two
	else
		memoryTotal = memoryTotalPow2;

	memoryTotalMib = memoryTotal / MiB;
}


const char* cpu_IdentifierString()
{
#if CPU_IA32
	return ia32_IdentifierString();	// cached
#endif
}

uint cpu_NumPackages()
{
#if CPU_IA32
	return ia32_NumPackages();	// cached
#endif
}

uint cpu_CoresPerPackage()
{
#if CPU_IA32
	return ia32_CoresPerPackage();	// cached
#endif
}

uint cpu_LogicalPerCore()
{
#if CPU_IA32
	return ia32_LogicalPerCore();	// cached
#endif
}

#pragma endregion
//-----------------------------------------------------------------------------

#if CPU_IA32

static void InitAndConfigureIA32()
{
	ia32_Init();	// must come before any use of ia32*

	ia32_memcpy_init();

	// no longer set 24 bit (float) precision by default: for
	// very long game uptimes (> 1 day; e.g. dedicated server),
	// we need full precision when calculating the time.
	// if there's a spot where we want to speed up divides|sqrts,
	// we can temporarily change precision there.
	//ia32_asm_control87(IA32_PC_24, IA32_MCW_PC);

	// to help catch bugs, enable as many floating-point exceptions as
	// possible. unfortunately SpiderMonkey triggers all of them.
	// note: passing a flag *disables* that exception.
	ia32_asm_control87(IA32_EM_ZERODIVIDE|IA32_EM_INVALID|IA32_EM_DENORMAL|IA32_EM_OVERFLOW|IA32_EM_UNDERFLOW|IA32_EM_INEXACT, IA32_MCW_EM);

	// no longer round toward zero (truncate). changing this setting
	// resulted in much faster float->int casts, because the compiler
	// could be told (via /QIfist) to use FISTP while still truncating
	// the result as required by ANSI C. however, FPU calculation
	// results were changed significantly, so it had to be disabled.
	//ia32_asm_control87(IA32_RC_CHOP, IA32_MCW_RC);
}

#endif


//-----------------------------------------------------------------------------

static ModuleInitState initState;

void cpu_Init()
{
	if(!ModuleShouldInitialize(&initState))
		return;

#if CPU_IA32
	InitAndConfigureIA32();
#endif

	DetectMemory();
	DetectClockFrequency();
}


void cpu_Shutdown()
{
	if(!ModuleShouldShutdown(&initState))
		return;

	// currently nothing to do
}


//-----------------------------------------------------------------------------
// stateless routines

bool cpu_CAS(volatile uintptr_t* location, uintptr_t expected, uintptr_t new_value)
{
#if CPU_IA32
	return ia32_asm_CAS(location, expected, new_value);
#endif
}

void cpu_AtomicAdd(volatile intptr_t* location, intptr_t increment)
{
#if CPU_IA32
	return ia32_asm_AtomicAdd(location, increment);
#endif
}

void cpu_MemoryFence()
{
#if CPU_IA32
	return ia32_MemoryFence();
#endif
}

void cpu_Serialize()
{
#if CPU_IA32
	return ia32_Serialize();
#endif
}

void* cpu_memcpy(void* RESTRICT dst, const void* RESTRICT src, size_t nbytes)
{
#if CPU_IA32
	return ia32_memcpy(dst, src, nbytes);
#else
	return memcpy(dst, src, nbytes);
#endif
}


int cpu_OsNumProcessors()
{
#if OS_WIN
	return wcpu_NumProcessors();
#elif OS_UNIX
	return ucpu_NumPackages();
#else
#error "port"
#endif
}


LibError cpu_CallByEachCPU(CpuCallback cb, void* param)
{
#if OS_WIN
	return wcpu_CallByEachCPU(cb, param);
#else
	return ucpu_CallByEachCPU(cb, param);
#endif
}


i32 cpu_i32FromFloat(float f)
{
#if USE_IA32_FLOAT_TO_INT
	return ia32_asm_i32FromFloat(f);
#else
	return (i32)f;
#endif
}

i32 cpu_i32FromDouble(double d)
{
#if USE_IA32_FLOAT_TO_INT
	return ia32_asm_i32FromDouble(d);
#else
	return (i32)d;
#endif
}

i64 cpu_i64FromDouble(double d)
{
#if USE_IA32_FLOAT_TO_INT
	return ia32_asm_i64FromDouble(d);
#else
	return (i64)d;
#endif
}


// System V derived (GNU/Linux, Solaris)
#if defined(_SC_AVPHYS_PAGES)

static int SysconfFromMemType(CpuMemoryIndicators mem_type)
{
	switch(mem_type)
	{
	case CPU_MEM_TOTAL:
		return _SC_PHYS_PAGES;
	case CPU_MEM_AVAILABLE:
		return _SC_AVPHYS_PAGES;
	}
	UNREACHABLE;
}

#endif

size_t cpu_MemorySize(CpuMemoryIndicators mem_type)
{
	// quasi-POSIX
#if defined(_SC_AVPHYS_PAGES)
	const int sc_name = SysconfFromMemType(mem_type);
	const size_t pageSize = sysconf(_SC_PAGESIZE);
	const size_t memory_size = sysconf(sc_name) * pageSize;
	return memory_size;
	// BSD / Mac OS X
#else
	return bsd_MemorySize(mem_type);
#endif
}
