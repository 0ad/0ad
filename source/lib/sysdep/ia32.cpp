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

#ifdef _M_IX86

#include "ia32.h"
#include "lib.h"
#include "detect.h"
#include "timer.h"

// HACK (see call to wtime_reset_impl)
#ifdef _WIN32
#include "win/wtime.h"
#endif

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <vector>
#include <algorithm>


// replace pathetic MS libc implementation
#ifdef _WIN32
double _ceil(double f)
{
	double r;

	const float _49 = 0.499999f;
__asm
{
	fld			[f]
	fadd		[_49]
	frndint
	fstp		[r]
}

	UNUSED(f)

	return r;
}
#endif


// return convention for 64 bits with VC7.1, ICC8 is in edx:eax,
// so temp variable is unnecessary, but we play it safe.
inline u64 rdtsc()
{
	u64 c;
__asm
{
	cpuid
	rdtsc
	mov			dword ptr [c], eax
	mov			dword ptr [c+4], edx
}
	return c;
}


// change FPU control word (used to set precision)
uint _control87(uint new_cw, uint mask)
{
__asm
{
	push		eax
	fnstcw		[esp]
	pop			eax								; old_cw
	mov			ecx, [new_cw]
	mov			edx, [mask]
	and			ecx, edx						; new_cw & mask
	not			edx								; ~mask
	and			eax, edx						; old_cw & ~mask
	or			eax, ecx						; (old_cw & ~mask) | (new_cw & mask)
	push		eax
	fldcw		[esp]
	pop			eax
}

	UNUSED(new_cw)
	UNUSED(mask)

	return 0;
}


//
// data returned by cpuid()
// each function using this data must call cpuid (no-op if already called)
//

static char vendor_str[13];
static int family, model, ext_family;
	// used in manual cpu_type detect
static u32 max_ext_func;

// caps
// treated as 128 bit field; order: std ecx, std edx, ext ecx, ext edx
// keep in sync with enum CpuCap and cpuid() code!
u32 caps[4];

static int have_brand_string = 0;
	// if false, need to detect cpu_type manually.
	// int instead of bool for easier setting from asm

enum Regs
{
	EAX,
	EBX,
	ECX,
	EDX
};

static bool cpuid(u32 func, u32* regs)
{
	if(func > max_ext_func)
		return false;

__asm
{
	mov			eax, [func]
	cpuid
	mov			edi, [regs]
	stosd
	xchg		eax, ebx
	stosd
	xchg		eax, ecx
	stosd
	xchg		eax, edx
	stosd
}

	return true;
}


// (optimized for size)
static void __declspec(naked) cpuid()
{
__asm
{
	pushad										; save ebx, esi, edi, ebp
		; ICC7: pusha is the 16-bit form!

; make sure CPUID is supported
	pushfd
	or			byte ptr [esp+2], 32
	popfd
	pushfd
	pop			eax
	shr			eax, 22							; bit 21 toggled?
	jnc			no_cpuid

; get vendor string
	xor			eax, eax
	cpuid
	mov			edi, offset vendor_str
	xchg		eax, ebx
	stosd
	xchg		eax, edx
	stosd
	xchg		eax, ecx
	stosd
	; (already 0 terminated)

; get CPU signature and std feature bits
	push		1
	pop			eax
	cpuid
	mov			[caps+0], ecx
	mov			[caps+4], edx
	movzx		edx, al
	shr			edx, 4
	mov			[model], edx					; eax[7:4]
	movzx		edx, ah
	and			edx, 0x0f
	mov			[family], edx					; eax[11:8]
	shr			eax, 20
	and			eax, 0x0f
	mov			[ext_family], eax				; eax[23:20]

; make sure CPUID ext functions are supported
	mov			esi, 0x80000000
	mov			eax, esi
	cpuid
	mov			[max_ext_func], eax
	cmp			eax, esi						; max ext <= 0x80000000?
	jbe			no_ext_funcs					; yes - no ext funcs at all
	lea			esi, [esi+4]					; esi = 0x80000004
	cmp			eax, esi						; max ext < 0x80000004?
	jb			no_brand_str					; yes - brand string not available, skip

; get CPU brand string (>= Athlon XP, P4)
	mov			edi, offset cpu_type
	push		-2
	pop			ebp								; loop counter: [-2, 0]
$1:	lea			eax, [ebp+esi]					; 0x80000002 .. 4
	cpuid
	stosd
	xchg		eax, ebx
	stosd
	xchg		eax, ecx
	stosd
	xchg		eax, edx
	stosd
	inc			ebp
	jle			$1
	; (already 0 terminated)

	mov			[have_brand_string], ebp		; ebp = 1 = true

no_brand_str:

; get extended feature flags
	lea			eax, [esi-3]					; 0x80000001
	cpuid
	mov			[caps+8], ecx
	mov			[caps+12], edx

no_ext_funcs:

no_cpuid:

	popad
	ret
}	// __asm
}	// cpuid()


bool ia32_cap(CpuCap cap)
{
	u32 idx = cap >> 5;
	if(idx > 3)
	{
		debug_warn("cap invalid");
		return false;
	}
	u32 bit = 1ul << (cap & 0x1f);

	return (caps[idx] & bit) != 0;
}



enum Vendor { UNKNOWN, INTEL, AMD };
static Vendor vendor = UNKNOWN;






static void get_cpu_type()
{
	// fall back to manual detect of CPU type if it didn't supply
	// a brand string, or if the brand string is useless (i.e. "Unknown").
	if(!have_brand_string || strncmp(cpu_type, "Unknow", 6) == 0)
		// we use an extra flag to detect if we got the brand string:
		// safer than comparing against the default name, which may change.
		//
		// some older boards reprogram the brand string with
		// "Unknow[n] CPU Type" on CPUs the BIOS doesn't recognize.
		// in that case, we ignore the brand string and detect manually.
	{
		if(vendor == AMD)
		{
			// everything else is either too old, or should have a brand string.
			if(family == 6)
			{
				if(model == 3 || model == 7)
					strcpy(cpu_type, "AMD Duron");
				else if(model <= 5)
					strcpy(cpu_type, "AMD Athlon");
				else
				{
					if(ia32_cap(MP))
						strcpy(cpu_type, "AMD Athlon MP");
					else
						strcpy(cpu_type, "AMD Athlon XP");
				}
			}
		}
		else if(vendor == INTEL)
		{
			// everything else is either too old, or should have a brand string.
			if(family == 6)
			{
				if(model == 1)
					strcpy(cpu_type, "Intel Pentium Pro");
				else if(model == 3 || model == 5)
					strcpy(cpu_type, "Intel Pentium II");
				else if(model == 6)
					strcpy(cpu_type, "Intel Celeron");
				else
					strcpy(cpu_type, "Intel Pentium III");
			}
		}
	}
	// we have a valid brand string; try to pretty it up some
	else
	{
		// strip (tm) from Athlon string
		if(!strncmp(cpu_type, "AMD Athlon(tm)", 14))
			memmove(cpu_type+10, cpu_type+14, 34);

		// remove 2x (R) and CPU freq from P4 string
		float freq;
		// the indicated frequency isn't necessarily correct - the CPU may be
		// overclocked. need to pass a variable though, since scanf returns
		// the number of fields actually stored.
		if(sscanf(cpu_type, " Intel(R) Pentium(R) 4 CPU %fGHz", &freq) == 1)
			strcpy(cpu_type, "Intel Pentium 4");
	}
}


static void measure_cpu_freq()
{
	// set max priority, to avoid interference while measuring.
	int old_policy;	static sched_param old_param;	// (static => 0-init)
	pthread_getschedparam(pthread_self(), &old_policy, &old_param);
	static sched_param max_param;
	max_param.sched_priority = sched_get_priority_max(SCHED_RR);
	pthread_setschedparam(pthread_self(), SCHED_RR, &max_param);

	if(ia32_cap(TSC))
		// we require the TSC to measure actual CPU cycles per clock tick.
		// counting loop iterations ("bogomips") is unreliable.
	{
		// rdtsc() uses cpuid to serialize instruction flow. the first
		// few calls of this instruction are documented to take longer
		// (no idea why), so we warm it up here.
		__asm cpuid __asm cpuid __asm cpuid

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

			// count # of clocks in max{1 tick, 1 ms}
			// .. wait for start of tick
			const double t0 = get_time();
			u64 c1; double t1;
			do
			{
				c1 = rdtsc();	// changes quickly
				t1 = get_time();
			}
			while(t1 == t0);
			// .. wait until start of next tick and at least 1 ms
			do
			{
				const u64 c2 = rdtsc();
				const double t2 = get_time();
				dc = (i64)(c2 - c1);
					// i64 rationale: see decl
				dt = t2 - t1;
			}
			while(dt < 1e-3);

			// .. freq = (delta_clocks) / (delta_seconds);
			//    cpuid/rdtsc/timer overhead is negligible
			const double freq = dc / dt;
			samples[i] = freq;
		}

		std::sort(samples.begin(), samples.end());

		// median filter (remove upper and lower 25% and average the rest)
		double sum = 0.0;
		const int lo = num_samples/4, hi = 3*num_samples/4;
		for(i = lo; i < hi; i++)
			sum += samples[i];
		cpu_freq = sum / (hi-lo);
	}
	// else: TSC not available, can't measure

	// restore previous policy and priority
	pthread_setschedparam(pthread_self(), old_policy, &old_param);
}


int get_cur_processor_id()
{
	int apic_id;
	__asm {
	push		1
	pop			eax
	cpuid
	shr			ebx, 24
	mov			[apic_id], ebx					; ebx[31:24]
	}

	return apic_id;
}


// set cpu_smp if there's more than 1 physical CPU -
// need to know this for wtime's TSC safety check.
static void check_smp()
{
	assert(cpus > 0 && "must know # CPUs (call OS-specific detect first)");

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


	//
	// still need to check if HT is actually enabled (BIOS and OS);
	// there might be 2 CPUs with HT supported but disabled.
	//

	// get number of logical CPUs per package
	// (the same for all packages on this system)
	int log_cpus_per_package;
	__asm {
	push		1
	pop			eax
	cpuid
	shr			ebx, 16
	and			ebx, 0xff
	mov			log_cpus_per_package, ebx		; ebx[23:16]
	}

	// logical CPUs are initialized after one another =>
	// they have the same physical ID.
	const int id = get_cur_processor_id();
	const int phys_shift = log2(log_cpus_per_package);
	const int phys_id = id >> phys_shift;

	// more than 1 physical CPU found
	static int last_phys_id = -1;
	if(last_phys_id != -1 && last_phys_id != phys_id)
		cpu_smp = 1;
	last_phys_id = phys_id;
}


static void check_speedstep()
{
	if(vendor == INTEL && ia32_cap(EST))
		cpu_speedstep = 1;

	if(vendor == AMD)
	{
		u32 regs[4];
		if(cpuid(0x80000007, regs))
			if(regs[EDX] & 2)	// frequency ID control
				cpu_speedstep = 1;
	}
}


void ia32_get_cpu_info()
{
	cpuid();
	if(family == 0)	// cpuid not supported - can't do the rest
		return;

	// (for easier comparison)
	if(!strcmp(vendor_str, "AuthenticAMD"))
		vendor = AMD;
	else if(!strcmp(vendor_str, "GenuineIntel"))
		vendor = INTEL;

	get_cpu_type();
	check_speedstep();
	on_each_cpu(check_smp);

	measure_cpu_freq();

	// HACK: if _WIN32, the HRT makes its final implementation choice
	// in the first calibrate call where cpu info is available.
	// call wtime_reset_impl here to have that happen now,
	// so app code isn't surprised by a timer change, although the HRT
	// does try to keep the timer continuous.
#ifdef _WIN32
	wtime_reset_impl();
#endif
}

#endif	// #ifndef _M_IX86
