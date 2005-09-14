// IA-32 (x86) specific code
// Copyright (c) 2003 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include "lib.h"
#include "posix.h"
#include "ia32.h"
#include "detect.h"
#include "timer.h"

// HACK (see call to wtime_reset_impl)
#if OS_WIN
#include "win/wtime.h"
#endif


#include <string.h>
#include <stdio.h>

#include <vector>
#include <algorithm>

#if !HAVE_MS_ASM && !HAVE_GNU_ASM
#error ia32.cpp needs inline assembly support!
#endif

// replace pathetic MS libc implementation.
// not needed on non-Win32, so don't bother converting from MS inline asm.
#if HAVE_MS_ASM
double _ceil(double f)
{
	UNUSED2(f);	// avoid bogus warning
	const float _49 = 0.499999f;
	double r;
__asm
{
	fld			[f]
	fadd		[_49]
	frndint
	fstp		[r]
}
	return r;
}
#endif


// rationale: this function should return its output (instead of setting
// out params) to simplify its callers. it is written in inline asm
// (instead of moving to ia32.asm) to insulate from changing compiler
// calling conventions.
// MSC, ICC and GCC currently return 64 bits in edx:eax, which even
// matches rdtsc output, but we play it safe and return a temporary.
inline u64 rdtsc()
{
	u64 c;
#if HAVE_MS_ASM
__asm
{
	cpuid
	rdtsc
	mov			dword ptr [c], eax
	mov			dword ptr [c+4], edx
}
#elif HAVE_GNU_ASM
	__asm__ __volatile__ (
		"cpuid; rdtsc"
		: "=A" (c)
		: /* no input */
		: "ebx", "ecx" /* cpuid clobbers ebx and ecx */);
#endif
	return c;
}


void ia32_debug_break()
{
#if HAVE_MS_ASM
	__asm int 3
// note: this probably isn't necessary, since unix_debug_break
// (SIGTRAP) is most probably available if HAVE_GNU_ASM.
// we include it for completeness, though.
#elif HAVE_GNU_ASM
	__asm__ __volatile__ ("mfence");
#endif
}


void ia32_memcpy(void* dst, const void* src, size_t nbytes)
{
	memcpy(dst, src, nbytes);
}


//-----------------------------------------------------------------------------
// support code for lock-free primitives
//-----------------------------------------------------------------------------

// CAS does a sanity check on the location parameter to see if the caller
// actually is passing an address (instead of a value, e.g. 1). this is
// important because the call is via a macro that coerces parameters.
//
// reporting is done with the regular CRT assert instead of debug_assert
// because the wdbg code relies on CAS internally (e.g. to avoid
// nested stack traces). a bug such as VC's incorrect handling of params
// in __declspec(naked) functions would then cause infinite recursion,
// which is difficult to debug (since wdbg is hosed) and quite fatal.
#define ASSERT(x) assert(x)

// note: a 486 or later processor is required since we use CMPXCHG.
// there's no feature flag we can check, and the ia32 code doesn't
// bother detecting anything < Pentium, so this'll crash and burn if
// run on 386. we could replace cmpxchg with a simple mov (since 386
// CPUs aren't MP-capable), but it's not worth the trouble.

// note: don't use __declspec(naked) because we need to access one parameter
// from C code and VC can't handle that correctly.
#if HAVE_MS_ASM

bool __cdecl CAS_(uintptr_t* location, uintptr_t expected, uintptr_t new_value)
{
	// try to see if caller isn't passing in an address
	// (CAS's arguments are silently casted)
	ASSERT(!debug_is_pointer_bogus(location));

	bool was_updated;
__asm
{
	cmp		byte ptr [cpus], 1
	mov		eax, [expected]
	mov		edx, [location]
	mov		ecx, [new_value]
	je		$no_lock
_emit 0xf0	// LOCK prefix
$no_lock:
	cmpxchg	[edx], ecx
	sete	al
	mov		[was_updated], al
}
	return was_updated;
}


void atomic_add(intptr_t* location, intptr_t increment)
{
__asm
{
	cmp		byte ptr [cpus], 1
	mov		edx, [location]
	mov		eax, [increment]
	je		$no_lock
_emit 0xf0	// LOCK prefix
$no_lock:
	add		[edx], eax
}
}

#else // #if HAVE_MS_ASM

bool CAS_(uintptr_t* location, uintptr_t expected, uintptr_t new_value)
{
	uintptr_t prev;

	ASSERT(location >= (uintptr_t*)0x10000);

	__asm__ __volatile__(
		"lock; cmpxchgl %1,%2"
		: "=a"(prev) // %0: Result in eax should be stored in prev
		: "q"(new_value), // %1: new_value -> e[abcd]x
		  "m"(*location), // %2: Memory operand
		  "0"(expected) // Stored in same place as %0
		: "memory"); // We make changes in memory
	return prev == expected;
}

void atomic_add(intptr_t* location, intptr_t increment)
{
	__asm__ __volatile__ (
			"cmpb $1, %1;"
			"je 1f;"
			"lock;"
			"1: addl %3, %0"
		: "=m" (*location) /* %0: Output into *location */
		: "m" (cpus), /* %1: Input for cpu check */
		  "m" (*location), /* %2: *location is also an input */
		  "r" (increment) /* %3: Increment (store in register) */
		: "memory"); /* clobbers memory (*location) */
}

#endif	// #if HAVE_MS_ASM


// enforce strong memory ordering.
void mfence()
{
	// Pentium IV
	if(ia32_cap(SSE2))
#if HAVE_MS_ASM
		__asm mfence
#elif HAVE_GNU_ASM
		__asm__ __volatile__ ("mfence");
#endif
}

void serialize()
{
#if HAVE_MS_ASM
	__asm cpuid
#elif HAVE_GNU_ASM
	__asm__ __volatile__ ("cpuid");
#endif
}


//-----------------------------------------------------------------------------
// CPU / feature detect
//-----------------------------------------------------------------------------

bool ia32_cap(CpuCap cap)
{
	// treated as 128 bit field; order: std ecx, std edx, ext ecx, ext edx
	// keep in sync with enum CpuCap!
	static u32 caps[4];
	ONCE(\
		u32 regs[4];
		if(ia32_cpuid(1, regs))\
		{\
			caps[0] = regs[ECX];\
			caps[1] = regs[EDX];\
		}\
		if(ia32_cpuid(0x80000001, regs))\
		{\
			caps[2] = regs[ECX];\
			caps[3] = regs[EDX];\
		}\
	);

	const uint tbl_idx = cap >> 5;
	const uint bit_idx = cap & 0x1f;
	if(tbl_idx > 3)
	{
		debug_warn("cap invalid");
		return false;
	}
	return (caps[tbl_idx] & BIT(bit_idx)) != 0;
}





// we only store enum Vendor rather than the string because that
// is easier to compare.
enum Vendor { UNKNOWN, INTEL, AMD };
static Vendor vendor = UNKNOWN;



enum MiscCpuCapBits
{
	// AMD PowerNow! flags (returned in edx by CPUID 0x80000007)
	POWERNOW_FREQ_ID_CTRL = 2
};



static void get_cpu_vendor()
{
	u32 regs[4];
	if(!ia32_cpuid(0, regs))
		return;

	// copy regs to string
	// note: 'strange' ebx,edx,ecx reg order is due to ModR/M encoding order.
	char vendor_str[13];
	u32* vendor_str_u32 = (u32*)vendor_str;
	vendor_str_u32[0] = regs[EBX];
	vendor_str_u32[1] = regs[EDX];
	vendor_str_u32[2] = regs[ECX];
	vendor_str[12] = '\0';	// 0-terminate

	if(!strcmp(vendor_str, "AuthenticAMD"))
		vendor = AMD;
	else if(!strcmp(vendor_str, "GenuineIntel"))
		vendor = INTEL;
	else
		debug_warn("unknown vendor");
}


static void get_cpu_type()
{
	// get processor signature
	u32 regs[4];
	if(!ia32_cpuid(1, regs))
		debug_warn("cpuid 1 failed");
	const uint model  = bits(regs[EAX], 4, 7);
	const uint family = bits(regs[EAX], 8, 11);

	// get brand string (if available)
	u32* cpu_type_u32 = (u32*)cpu_type;
	bool have_brand_string = false;
	if(ia32_cpuid(0x80000002, cpu_type_u32+0 ) &&
	   ia32_cpuid(0x80000003, cpu_type_u32+16) &&
	   ia32_cpuid(0x80000004, cpu_type_u32+32))
		have_brand_string = true;


	// note: cpu_type is guaranteed to hold 48+1 chars, since that's the
	// length of the CPU brand string. strcpy(cpu_type, literal) is safe.
#define SAFE_STRCPY strcpy

	// fall back to manual detect of CPU type because either:
	// - CPU doesn't support brand string (we use a flag to indicate this
	//   rather than comparing against a default value because it is safer);
	// - the brand string is useless, e.g. "Unknown". this happens on
	//   some older boards whose BIOS reprograms the string for CPUs it
	//   doesn't recognize.
	if(!have_brand_string || strncmp(cpu_type, "Unknow", 6) == 0)
	{
		if(vendor == AMD)
		{
			// everything else is either too old, or should have a brand string.
			if(family == 6)
			{
				if(model == 3 || model == 7)
					SAFE_STRCPY(cpu_type, "AMD Duron");
				else if(model <= 5)
					SAFE_STRCPY(cpu_type, "AMD Athlon");
				else
				{
					if(ia32_cap(AMD_MP))
						SAFE_STRCPY(cpu_type, "AMD Athlon MP");
					else
						SAFE_STRCPY(cpu_type, "AMD Athlon XP");
				}
			}
		}
		else if(vendor == INTEL)
		{
			// everything else is either too old, or should have a brand string.
			if(family == 6)
			{
				if(model == 1)
					SAFE_STRCPY(cpu_type, "Intel Pentium Pro");
				else if(model == 3 || model == 5)
					SAFE_STRCPY(cpu_type, "Intel Pentium II");
				else if(model == 6)
					SAFE_STRCPY(cpu_type, "Intel Celeron");	
				else
					SAFE_STRCPY(cpu_type, "Intel Pentium III");
			}
		}
	}
	// cpu_type already holds a valid brand string; pretty it up.
	else
	{
		// strip (tm) from Athlon string
		if(!strncmp(cpu_type, "AMD Athlon(tm)", 14))
			memmove(cpu_type+10, cpu_type+14, 34);

		// remove 2x (R) and CPU freq from P4 string
		float freq;
		// we can't use this because it isn't necessarily correct - the CPU
		// may be overclocked. a variable must be passed, though, since
		// scanf returns the number of fields actually stored.
		if(sscanf(cpu_type, " Intel(R) Pentium(R) 4 CPU %fGHz", &freq) == 1)
			SAFE_STRCPY(cpu_type, "Intel Pentium 4");
	}
}



static void measure_cpu_freq()
{
	// set max priority, to reduce interference while measuring.
	int old_policy; static sched_param old_param;	// (static => 0-init)
	pthread_getschedparam(pthread_self(), &old_policy, &old_param);
	static sched_param max_param;
	max_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &max_param);

	if(ia32_cap(TSC))
		// make sure the TSC is available, because we're going to
		// measure actual CPU clocks per known time interval.
		// counting loop iterations ("bogomips") is unreliable.
	{
		// note: no need to "warm up" cpuid - it will already have been
		// called several times by the time this code is reached.
		// (background: it's used in rdtsc() to serialize instruction flow;
		// the first call is documented to be slower on Intel CPUs)

		int num_samples = 16;
		// if clock is low-res, do less samples so it doesn't take too long.
		// balance measuring time (~ 10 ms) and accuracy (< 1 0/00 error -
		// ok for using the TSC as a time reference)
		if(timer_res() >= 1e-3)
			num_samples = 8;
		std::vector<double> samples(num_samples);

		int i;
		for(i = 0; i < num_samples; i++)
		{
			double dt;
			i64 dc;
				// i64 because VC6 can't convert u64 -> double,
				// and we don't need all 64 bits.

			// count # of clocks in max{1 tick, 1 ms}:
			// .. wait for start of tick.
			const double t0 = get_time();
			u64 c1; double t1;
			do
			{
				// note: get_time effectively has a long delay (up to 5 µs)
				// before returning the time. we call it before rdtsc to
				// minimize the delay between actually sampling time / TSC,
				// thus decreasing the chance for interference.
				// (if unavoidable background activity, e.g. interrupts,
				// delays the second reading, inaccuracy is introduced).
				t1 = get_time();
				c1 = rdtsc();
			}
			while(t1 == t0);
			// .. wait until start of next tick and at least 1 ms elapsed.
			do
			{
				const double t2 = get_time();
				const u64 c2 = rdtsc();
				dc = (i64)(c2 - c1);
				dt = t2 - t1;
			}
			while(dt < 1e-3);

			// .. freq = (delta_clocks) / (delta_seconds);
			//    cpuid/rdtsc/timer overhead is negligible.
			const double freq = dc / dt;
			samples[i] = freq;
		}

		std::sort(samples.begin(), samples.end());

		// median filter (remove upper and lower 25% and average the rest).
		// note: don't just take the lowest value! it could conceivably be
		// too low, if background processing delays reading c1 (see above).
		double sum = 0.0;
		const int lo = num_samples/4, hi = 3*num_samples/4;
		for(i = lo; i < hi; i++)
			sum += samples[i];
		cpu_freq = sum / (hi-lo);

	}
	// else: TSC not available, can't measure; cpu_freq remains unchanged.

	// restore previous policy and priority.
	pthread_setschedparam(pthread_self(), old_policy, &old_param);
}


// set cpu_smp if there's more than 1 physical CPU -
// need to know this for wtime's TSC safety check.
// called on each CPU by on_each_cpu.
static void check_smp()
{
	u32 regs[4];

	debug_assert(cpus > 0 && "must know # CPUs (call OS-specific detect first)");

/*
	if single-core and no HT
		no change
	if multi-core and no HT
		phys = windows_cpus / cores_per_package


*/

	// we don't check if it's Intel and P4 or above - HT may be supported
	// on other CPUs in future. haven't come across a processor that
	// incorrectly sets the HT feature bit.
	if(!ia32_cap(HT))
	{
		// no HT supported, just check number of CPUs as reported by OS.
		cpu_smp = (cpus > 1);
		return;
	}

	// first call. we set cpu_smp below if more than 1 physical CPU is found,
	// so clear it until then.
	if(cpu_smp == -1)
		cpu_smp = 0;

	// multicore count
	uint num_cores_per_package = 1;
	if(ia32_cpuid(4, regs))
		num_cores_per_package = bits(regs[EBX], 26, 31)+1;



	//
	// still need to check if HT is actually enabled (BIOS and OS);
	// there might be 2 CPUs with HT supported but disabled.
	//

	// get number of logical CPUs per package
	// (the same for all packages on this system)
	if(!ia32_cpuid(1, regs))
		debug_warn("cpuid 1 failed");
	const uint log_cpus_per_package = bits(regs[EBX], 16, 23);
	// logical CPUs are initialized after one another =>
	// they have the same physical ID.
	const uint cur_id = bits(regs[EBX], 24, 31);

	const int phys_shift = ilog2(log_cpus_per_package);
	const int phys_id = cur_id >> phys_shift;

	// more than 1 physical CPU found
	static int last_phys_id = -1;
	if(last_phys_id != -1 && last_phys_id != phys_id)
		cpu_smp = 1;
	last_phys_id = phys_id;
}


static void check_speedstep()
{
	if(vendor == INTEL)
	{
		if(ia32_cap(INTEL_EST))
			cpu_speedstep = 1;
	}
	else if(vendor == AMD)
	{
		u32 regs[4];
		if(ia32_cpuid(0x80000007, regs))
			if(regs[EDX] & POWERNOW_FREQ_ID_CTRL)
				cpu_speedstep = 1;
	}
}


void ia32_get_cpu_info()
{
	get_cpu_vendor();
	get_cpu_type();

	check_speedstep();
	// linux doesn't have CPU affinity API:s (that we've found...)
#if OS_WIN
	on_each_cpu(check_smp);
#endif

	measure_cpu_freq();

	// HACK: on Windows, the HRT makes its final implementation choice
	// in the first calibrate call where cpu info is available.
	// call wtime_reset_impl here to have that happen now,
	// so app code isn't surprised by a timer change, although the HRT
	// does try to keep the timer continuous.
#if OS_WIN
	wtime_reset_impl();
#endif
}
