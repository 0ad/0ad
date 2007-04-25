/**
 * =========================================================================
 * File        : ia32.cpp
 * Project     : 0 A.D.
 * Description : C++ and inline asm implementations for IA-32.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "precompiled.h"
#include "ia32.h"

#include <string.h>
#include <stdio.h>
#include <vector>
#include <algorithm>

#include "lib/posix/posix_pthread.h"
#include "lib/lib.h"
#include "lib/timer.h"
#include "lib/sysdep/cpu.h"

#if !HAVE_MS_ASM && !HAVE_GNU_ASM
#error ia32.cpp needs inline assembly support!
#endif

//-----------------------------------------------------------------------------
// capability bits

// set by ia32_cap_init, referenced by ia32_cap
// treated as 128 bit field; order: std ecx, std edx, ext ecx, ext edx
// keep in sync with enum CpuCap!
static u32 ia32_caps[4];


static void ia32_cap_init()
{
	u32 regs[4];
	if(ia32_asm_cpuid(1, regs))
	{
		ia32_caps[0] = regs[ECX];
		ia32_caps[1] = regs[EDX];
	}
	if(ia32_asm_cpuid(0x80000001, regs))
	{
		ia32_caps[2] = regs[ECX];
		ia32_caps[3] = regs[EDX];
	}
}

bool ia32_cap(IA32Cap cap)
{
	const uint tbl_idx = cap >> 5;
	const uint bit_idx = cap & 0x1f;
	if(tbl_idx > 3)
	{
		DEBUG_WARN_ERR(ERR::INVALID_PARAM);
		return false;
	}
	return (ia32_caps[tbl_idx] & BIT(bit_idx)) != 0;
}


// we only store enum Vendor rather than the string because that
// is easier to compare.
static enum Vendor
{
	UNKNOWN, INTEL, AMD
}
vendor = UNKNOWN;

static void detectVendor()
{
	u32 regs[4];
	if(!ia32_asm_cpuid(0, regs))
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
		DEBUG_WARN_ERR(ERR::CPU_UNKNOWN_VENDOR);
}




//-----------------------------------------------------------------------------

// this RDTSC implementation writes edx:eax to a temporary and returns that.
// rationale: this insulates against changing compiler calling conventions,
// at the cost of some efficiency.
// use ia32_asm_rdtsc_edx_eax instead if the return convention is known to be
// edx:eax (should be the case on all 32-bit x86).
u64 ia32_rdtsc_safe()
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
	// note: we save+restore EBX to avoid xcode complaining about a
	// "PIC register" being clobbered, whatever that means.
	__asm__ __volatile__ (
		"pushl %%ebx; cpuid; popl %%ebx; rdtsc"
		: "=A" (c)
		: /* no input */
		: "ecx" /* cpuid clobbers eax..edx, but the rest are covered */);
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
	__asm__ __volatile__ ("int $3");
#endif
}


//-----------------------------------------------------------------------------
// support code for lock-free primitives
//-----------------------------------------------------------------------------

// enforce strong memory ordering.
void ia32_mfence()
{
	// Pentium IV
	if(ia32_cap(IA32_CAP_SSE2))
#if HAVE_MS_ASM
		__asm mfence
#elif HAVE_GNU_ASM
		__asm__ __volatile__ ("mfence");
#endif
}

void ia32_serialize()
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

// returned in edx by CPUID 0x80000007.
enum AmdPowerNowFlags
{
	POWERNOW_FREQ_ID_CTRL = 2
};


const char* ia32_identifierString()
{
	// 3 calls x 4 registers x 4 bytes = 48
	static char identifier_string[48+1] = "";

	// get processor signature
	u32 regs[4];
	if(!ia32_asm_cpuid(1, regs))
		DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);
	const uint model  = bits(regs[EAX], 4, 7);
	const uint family = bits(regs[EAX], 8, 11);

	// get brand string (if available)
	// note: ia32_asm_cpuid writes 4 u32s directly to identifier_string -
	// be very careful with pointer arithmetic!
	u32* u32_string = (u32*)identifier_string;
	bool have_brand_string = false;
	if(ia32_asm_cpuid(0x80000002, u32_string+0 ) &&
	   ia32_asm_cpuid(0x80000003, u32_string+4) &&
	   ia32_asm_cpuid(0x80000004, u32_string+8))
		have_brand_string = true;

	// note: we previously verified max_chars is long enough, so copying
	// short literals into it is safe.

	// fall back to manual detect of CPU type because either:
	// - CPU doesn't support brand string (we use a flag to indicate this
	//   rather than comparing against a default value because it is safer);
	// - the brand string is useless, e.g. "Unknown". this happens on
	//   some older boards whose BIOS reprograms the string for CPUs it
	//   doesn't recognize.
	if(!have_brand_string || strncmp(identifier_string, "Unknow", 6) == 0)
	{
		if(vendor == AMD)
		{
			// everything else is either too old, or should have a brand string.
			if(family == 6)
			{
				if(model == 3 || model == 7)
					SAFE_STRCPY(identifier_string, "AMD Duron");
				else if(model <= 5)
					SAFE_STRCPY(identifier_string, "AMD Athlon");
				else
				{
					if(ia32_cap(IA32_CAP_AMD_MP))
						SAFE_STRCPY(identifier_string, "AMD Athlon MP");
					else
						SAFE_STRCPY(identifier_string, "AMD Athlon XP");
				}
			}
		}
		else if(vendor == INTEL)
		{
			// everything else is either too old, or should have a brand string.
			if(family == 6)
			{
				if(model == 1)
					SAFE_STRCPY(identifier_string, "Intel Pentium Pro");
				else if(model == 3 || model == 5)
					SAFE_STRCPY(identifier_string, "Intel Pentium II");
				else if(model == 6)
					SAFE_STRCPY(identifier_string, "Intel Celeron");	
				else
					SAFE_STRCPY(identifier_string, "Intel Pentium III");
			}
		}
	}
	// identifier_string already holds a valid brand string; pretty it up.
	else
	{
		// strip (tm) from Athlon string
		if(!strncmp(identifier_string, "AMD Athlon(tm)", 14))
			memmove(identifier_string+10, identifier_string+14, 35);

		// remove 2x (R) and CPU freq from P4 string
		float freq;
		// we can't use this because it isn't necessarily correct - the CPU
		// may be overclocked. a variable must be passed, though, since
		// scanf returns the number of fields actually stored.
		if(sscanf(identifier_string, " Intel(R) Pentium(R) 4 CPU %fGHz", &freq) == 1)
			SAFE_STRCPY(identifier_string, "Intel Pentium 4");
	}

	return identifier_string;
}


int ia32_isThrottlingPossible()
{
	if(vendor == INTEL)
	{
		if(ia32_cap(IA32_CAP_EST))
			return 1;
	}
	else if(vendor == AMD)
	{
		u32 regs[4];
		if(ia32_asm_cpuid(0x80000007, regs))
		{
			if(regs[EDX] & POWERNOW_FREQ_ID_CTRL)
				return 1;
		}
	}

	return 0;	// pretty much authoritative, so don't return -1.
}

double ia32_clockFrequency()
{
	double clock_frequency = 0.0;

	// set max priority, to reduce interference while measuring.
	int old_policy; static sched_param old_param;	// (static => 0-init)
	pthread_getschedparam(pthread_self(), &old_policy, &old_param);
	static sched_param max_param;
	max_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &max_param);

	// make sure the TSC is available, because we're going to
	// measure actual CPU clocks per known time interval.
	// counting loop iterations ("bogomips") is unreliable.
	if(ia32_cap(IA32_CAP_TSC))
	{
		// note: no need to "warm up" cpuid - it will already have been
		// called several times by the time this code is reached.
		// (background: it's used in ia32_rdtsc() to serialize instruction flow;
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
				// note: get_time effectively has a long delay (up to 5 us)
				// before returning the time. we call it before ia32_rdtsc to
				// minimize the delay between actually sampling time / TSC,
				// thus decreasing the chance for interference.
				// (if unavoidable background activity, e.g. interrupts,
				// delays the second reading, inaccuracy is introduced).
				t1 = get_time();
				c1 = ia32_rdtsc();
			}
			while(t1 == t0);
			// .. wait until start of next tick and at least 1 ms elapsed.
			do
			{
				const double t2 = get_time();
				const u64 c2 = ia32_rdtsc();
				dc = (i64)(c2 - c1);
				dt = t2 - t1;
			}
			while(dt < 1e-3);

			// .. freq = (delta_clocks) / (delta_seconds);
			//    ia32_rdtsc/timer overhead is negligible.
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
		clock_frequency = sum / (hi-lo);

	}
	// else: TSC not available, can't measure; cpu_freq remains unchanged.

	// restore previous policy and priority.
	pthread_setschedparam(pthread_self(), old_policy, &old_param);

	return clock_frequency;
}



// (these are uniform over all packages)


static uint logicalPerPackage()
{
	// not Hyperthreading capable
	if(!ia32_cap(IA32_CAP_HT))
		return 1;

	u32 regs[4];
	if(!ia32_asm_cpuid(1, regs))
		DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);
	const uint logical_per_package = bits(regs[EBX], 16, 23);
	return logical_per_package;
}

static uint coresPerPackage()
{
	// single core
	u32 regs[4];
	if(!ia32_asm_cpuid(4, regs))
		return 1;

	const uint cores_per_package = bits(regs[EAX], 26, 31)+1;
	return cores_per_package;
}


typedef std::vector<u8> Ids;
typedef std::set<u8> IdSet;

static void storeApicId(void* param)
{
	u32 regs[4];
	if(!ia32_asm_cpuid(1, regs))
		DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);
	const uint apic_id = bits(regs[EBX], 24, 31);

	Ids* apic_ids = (Ids*)param;
	apic_ids->push_back(apic_id);
}

static void extractFieldsIntoSet(const Ids& apic_ids, uint& bit_pos, uint num_values, IdSet& ids)
{
	const uint id_bits = log2(num_values);
	if(id_bits == 0)
		return;

	const uint mask = bit_mask(id_bits);

	for(size_t i = 0; i < apic_ids.size(); i++)
	{
		const u8 apic_id = apic_ids[i];
		const u8 field = (apic_id >> bit_pos) & mask;
		ids.insert(field);
	}

	bit_pos += id_bits;
}

static int num_packages = -1;	// i.e. sockets; > 1 => true SMP system
static int cores_per_package = -1;
static int logical_per_package = -1;	// hyperthreading units

// note: even though APIC IDs are assigned sequentially, we can't make any
// assumptions about the values/ordering because we get them according to
// the CPU affinity mask, which is unknown.

// HT and dual cores may be disabled, so see how many are actually enabled.
// (examine each APIC ID to determine the kind of CPU)
static void detectActualTopology()
{
	Ids apic_ids;
	if(cpu_callByEachCPU(storeApicId, &apic_ids) != INFO::OK)
		return;
	std::sort(apic_ids.begin(), apic_ids.end());
	debug_assert(std::unique(apic_ids.begin(), apic_ids.end()) == apic_ids.end());

	const uint max_cores_per_package = coresPerPackage();
	const uint logical_per_core = logicalPerPackage() / max_cores_per_package;

	// extract values from all 3 fields into separate sets
	// (we want to know how many logical CPUs per core, cores per package and packages exist)
	uint bit_pos = 0;
	IdSet logical_ids;
	extractFieldsIntoSet(apic_ids, bit_pos, logical_per_core, logical_ids);
	IdSet core_ids;
	extractFieldsIntoSet(apic_ids, bit_pos, max_cores_per_package, core_ids);
	IdSet package_ids;
	extractFieldsIntoSet(apic_ids, bit_pos, 0xFF, core_ids);

	num_packages = std::max((int)package_ids.size(), 1);
	cores_per_package = std::max((int)core_ids.size(), 1);
	logical_per_package = std::max((int)logical_ids.size(), 1) * cores_per_package;
}


uint ia32_logicalPerPackage()
{
#ifndef NDEBUG
	debug_assert(logical_per_package != -1);
#endif
	return (uint)logical_per_package;
}

uint ia32_coresPerPackage()
{
#ifndef NDEBUG
	debug_assert(cores_per_package != -1);
#endif
	return (uint)cores_per_package;
}

uint ia32_numPackages()
{
#ifndef NDEBUG
	debug_assert(num_packages != -1);
#endif
	return (uint)num_packages;
}


//-----------------------------------------------------------------------------


// checks if there is an IA-32 CALL instruction right before ret_addr.
// returns INFO::OK if so and ERR::FAIL if not.
// also attempts to determine the call target. if that is possible
// (directly addressed relative or indirect jumps), it is stored in
// target, which is otherwise 0.
//
// this is useful for walking the stack manually.
LibError ia32_get_call_target(void* ret_addr, void** target)
{
	*target = 0;

	// points to end of the CALL instruction (which is of unknown length)
	const u8* c = (const u8*)ret_addr;
	// this would allow for avoiding exceptions when accessing ret_addr
	// close to the beginning of the code segment. it's not currently set
	// because this is really unlikely and not worth the trouble.
	const size_t len = ~0u;

	// CALL rel32 (E8 cd)
	if(len >= 5 && c[-5] == 0xE8)
	{
		*target = (u8*)ret_addr + *(i32*)(c-4);
		return INFO::OK;
	}

	// CALL r/m32 (FF /2)
	// .. CALL [r32 + r32*s]          => FF 14 SIB
	if(len >= 3 && c[-3] == 0xFF && c[-2] == 0x14)
		return INFO::OK;
	// .. CALL [disp32]               => FF 15 disp32
	if(len >= 6 && c[-6] == 0xFF && c[-5] == 0x15)
	{
		void* addr_of_target = *(void**)(c-4);
		// there are no meaningful checks we can perform: we're called from
		// the stack trace code, so ring0 addresses may be legit.
		// even if the pointer is 0, it's better to pass its value on
		// (may help in tracking down memory corruption)
		*target = *(void**)addr_of_target;
		return INFO::OK;
	}
	// .. CALL [r32]                  => FF 00-3F(!14/15)
	if(len >= 2 && c[-2] == 0xFF && c[-1] < 0x40 && c[-1] != 0x14 && c[-1] != 0x15)
		return INFO::OK;
	// .. CALL [r32 + r32*s + disp8]  => FF 54 SIB disp8
	if(len >= 4 && c[-4] == 0xFF && c[-3] == 0x54)
		return INFO::OK;
	// .. CALL [r32 + disp8]          => FF 50-57(!54) disp8
	if(len >= 3 && c[-3] == 0xFF && (c[-2] & 0xF8) == 0x50 && c[-2] != 0x54)
		return INFO::OK;
	// .. CALL [r32 + r32*s + disp32] => FF 94 SIB disp32
	if(len >= 7 && c[-7] == 0xFF && c[-6] == 0x94)
		return INFO::OK;
	// .. CALL [r32 + disp32]         => FF 90-97(!94) disp32
	if(len >= 6 && c[-6] == 0xFF && (c[-5] & 0xF8) == 0x90 && c[-5] != 0x94)
		return INFO::OK;
	// .. CALL r32                    => FF D0-D7                 
	if(len >= 2 && c[-2] == 0xFF && (c[-1] & 0xF8) == 0xD0)
		return INFO::OK;

	WARN_RETURN(ERR::CPU_UNKNOWN_OPCODE);
}


//-----------------------------------------------------------------------------

void ia32_init()
{
	ia32_asm_cpuid_init();

	ia32_cap_init();

	detectVendor();

	detectActualTopology();
}
