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
#include "lib/posix/posix.h"
#if CPU_IA32
# include "lib/sysdep/ia32/ia32.h"
# include "lib/sysdep/ia32/ia32_memcpy.h"
#endif
#if OS_BSD
# include "lib/sysdep/unix/bsd.h"
#endif
#if OS_WIN
# include "lib/sysdep/win/wcpu.h"
# include "lib/sysdep/win/wposix/wtime_internal.h"	// HACK (see call to wtime_reset_impl)
#endif


AT_STARTUP(\
	error_setDescription(ERR::CPU_FEATURE_MISSING, "This CPU doesn't support a required feature");\
	error_setDescription(ERR::CPU_UNKNOWN_OPCODE, "Disassembly failed");\
	error_setDescription(ERR::CPU_UNKNOWN_VENDOR, "CPU vendor unknown");\
	error_setDescription(ERR::CPU_RESTRICTED_AFFINITY, "Cannot set desired CPU affinity");\
)


//-----------------------------------------------------------------------------
#pragma region Accessor functions
// insulate caller from the system-specific modules and cache results.
// note: the providers sometimes need to store the results anyway, so we
// don't need to do caching in those cases.
// these are set once during cpu_Init since they're usually all used and
// we thus avoid needing if(already_called) return old_result.
// initially set to 'impossible' values to catch uses before cpu_Init.

static double clock_frequency = -1.0;
static bool is_throttling_possible = true;
static size_t page_size = 1;
static size_t memory_total_mib = 1;


static void DetectClockFrequency()
{
#if CPU_IA32
	clock_frequency = ia32_ClockFrequency();	// authoritative, precise
#endif
}


static void DetectIfThrottlingPossible()
{
#if CPU_IA32
	if(ia32_IsThrottlingPossible() == 1)
	{
		is_throttling_possible = true;
		return;
	}
#endif

#if OS_WIN
	if(wcpu_IsThrottlingPossible() == 1)
	{
		is_throttling_possible = true;
		return;
	}
#endif

	is_throttling_possible = false;
}


static void DetectMemory()
{
	page_size = (size_t)sysconf(_SC_PAGESIZE);

	size_t memory_total = cpu_MemorySize(CPU_MEM_TOTAL);

	// account for inaccurate reporting by rounding up (see wposix sysconf)
	const size_t memory_total_pow2 = (size_t)round_up_to_pow2((uint)memory_total);
	// .. difference too great, just round up to 1 MiB
	if(memory_total_pow2 - memory_total > 3*MiB)
		memory_total = round_up(memory_total, 1*MiB);
	// .. difference acceptable, use next power of two
	else
		memory_total = memory_total_pow2;

	memory_total_mib = memory_total / MiB;
}

double cpu_ClockFrequency()
{
	return clock_frequency;
}

bool cpu_IsThrottlingPossible()
{
	return is_throttling_possible;
}

size_t cpu_MemoryTotalMiB()
{
	return memory_total_mib;
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

// note: can't use ModuleInitState for this because it changes as soon as
// init has *begun*, which isn't what we want.
static bool isDetectFinished = false;

bool cpu_IsDetectFinished()
{
	return isDetectFinished;
}


void cpu_Init()
{

#if CPU_IA32
	InitAndConfigureIA32();
#endif

	DetectMemory();
	DetectIfThrottlingPossible();
	DetectClockFrequency();

	// must be set before wtime_reset_impl since it queries this flag via
	// cpu_IsDetectFinished.
	isDetectFinished = true;

	// HACK: on Windows, the HRT makes its final implementation choice
	// in the first calibrate call where CPU info is available.
	// call wtime_reset_impl here to have that happen now so app code isn't
	// surprised by a timer change, although the HRT does try to
	// keep the timer continuous.
#if OS_WIN
	wtime_reset_impl();
#endif
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

LibError cpu_CallByEachCPU(CpuCallback cb, void* param)
{
#if OS_WIN
	return wcpu_CallByEachCPU(cb, param);
#else
	UNUSED2(cb);
	UNUSED2(param);
	return ERR::NOT_IMPLEMENTED;
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
	const size_t memory_size = sysconf(sc_name) * page_size;
	return memory_size;
	// BSD / Mac OS X
#else
	return bsd_MemorySize(mem_type);
#endif
}
