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
	mov		dword ptr [c], eax
	mov		dword ptr [c+4], edx
}
	return c;
}


// change FPU control word (used to set precision)
uint _control87(uint new_cw, uint mask)
{
__asm
{
	push	eax
	fnstcw	[esp]
	pop		eax					; old_cw
	mov		ecx, [new_cw]
	mov		edx, [mask]
	and		ecx, edx			; new_cw & mask
	not		edx					; ~mask
	and		eax, edx			; old_cw & ~mask
	or		eax, ecx			; (old_cw & ~mask) | (new_cw & mask)
	push	eax
	fldcw	[esp]
	pop		eax
}

	UNUSED(new_cw)
	UNUSED(mask)

	return 0;
}


long cpu_caps = 0;
long cpu_ext_caps = 0;

int tsc_is_safe = -1;


static char cpu_vendor[13];
static int family, model;	// used to detect cpu_type

static int have_brand_string = 0;
	// int instead of bool for easier setting from asm


// optimized for size
static void __declspec(naked) cpuid()
{
__asm
{
	pushad
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
	mov			edi, offset cpu_vendor
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
	mov			[cpu_caps], edx
	mov			edx, eax
	shr			edx, 4
	and			edx, 0x0f
	mov			[model], edx
	shr			eax, 8
	and			eax, 0x0f
	mov			[family], eax

; make sure CPUID ext functions are supported
	mov			esi, 0x80000000
	mov			eax, esi
	cpuid
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
	mov			[cpu_ext_caps], edx

no_ext_funcs:

no_cpuid:

	popad
	ret
}
}


void ia32_get_cpu_info()
{
	cpuid();
	if(cpu_caps == 0)	// cpuid not supported - can't do the rest
		return;

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
		// AMD
		if(!strcmp(cpu_vendor, "AuthenticAMD"))
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
					if(cpu_ext_caps & EXT_MP_CAPABLE)
						strcpy(cpu_type, "AMD Athlon MP");
					else
                        strcpy(cpu_type, "AMD Athlon XP");
				}
			}
		}
		// Intel
		else if(!strcmp(cpu_vendor, "GenuineIntel"))
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
		float a;
			// the indicated frequency isn't necessarily correct - the CPU may be
			// overclocked. need to pass a variable though, since scanf returns
			// the number of fields actually stored.
		if(sscanf(cpu_type, " Intel(R) Pentium(R) 4 CPU %fGHz", &a) == 1)
			strcpy(cpu_type, "Intel Pentium 4");
	}


	//
	// measure CPU frequency
	//

	// .. get old policy and priority
	int old_policy;
	static sched_param old_param;
	pthread_getschedparam(pthread_self(), &old_policy, &old_param);
	// .. set max priority
	static sched_param max_param;
	max_param.sched_priority = sched_get_priority_max(SCHED_RR);
	pthread_setschedparam(pthread_self(), SCHED_RR, &max_param);

	// calculate CPU frequency.
	// balance measuring time (~ 10 ms) and accuracy (< 1 0/00 error -
	// ok for using the TSC as a time reference)
	if(cpu_caps & TSC)	// needed to calculate freq; bogomips are a WAG
	{
		// stabilize CPUID for timing (first few calls take longer)
		__asm cpuid __asm cpuid __asm cpuid

		u64 c0, c1;

		std::vector<double> samples;
		int num_samples = 20;
		// if clock is low-res, do less samples so it doesn't take too long
		if(timer_res() >= 1e-3)
			num_samples = 10;

		int i;
		for(i = 0; i < num_samples; i++)
		{
again:
			// count # of clocks in max{1 tick, 1 ms}
			double t0;
			double t1 = get_time();
			// .. wait for start of tick
			do
			{
				c0 = rdtsc();	// changes quickly
				t0 = get_time();
			}
			while(t0 == t1);
			// .. wait until start of next tick and at least 1 ms
			do
			{
				c1 = rdtsc();
				t1 = get_time();
			}
			while(t1 < t0 + 1e-3);

			double ds = t1 - t0;
			if(ds < 0.0)		// bogus time delta - take another sample
				goto again;

			// .. freq = (delta_clocks) / (delta_seconds);
			//    cpuid/rdtsc/timer overhead is negligible
			double freq = (i64)(c1-c0) / ds;
				// VC6 can't convert u64 -> double, and we don't need full range

			samples.push_back(freq);
		}

		std::sort(samples.begin(), samples.end());
//		double median = samples[num_samples/2];

		// median filter (remove upper and lower 25% and average the rest)
		double sum = 0.0;
		const int lo = num_samples/4, hi = 3*num_samples/4;
		for(i = lo; i < hi; i++)
			sum += samples[i];
		cpu_freq = sum / (hi-lo);

		// HACK: if _WIN32, the HRT makes its final implementation choice
		// in the first calibrate call where cpu_freq and cpu_caps are
		// available. call wtime_reset_impl here to have that happen now,
		// so app code isn't surprised by a timer change, although the HRT
		// does try to keep the timer continuous.
#ifdef _WIN32
		wtime_reset_impl();
#endif
	}
	// else: TSC not available, can't measure

	// restore previous policy and priority
	pthread_setschedparam(pthread_self(), old_policy, &old_param);
}

#endif	// #ifndef _M_IX86