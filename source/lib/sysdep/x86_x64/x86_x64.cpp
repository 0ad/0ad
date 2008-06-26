/**
 * =========================================================================
 * File        : x86_x64.cpp
 * Project     : 0 A.D.
 * Description : CPU-specific routines common to 32 and 64-bit x86
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "x86_x64.h"

#include <string.h>
#include <stdio.h>
#include <vector>
#include <set>
#include <algorithm>

#include "lib/posix/posix.h"	// pthread
#include "lib/bits.h"
#include "lib/timer.h"
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/os_cpu.h"

#if ARCH_IA32
# include "../ia32/ia32_asm.h"
#else
#include "../amd64/amd64_asm.h"
# endif

#if MSC_VERSION
# include <intrin.h>	// __rdtsc
#elif GCC_VERSION
# include <emmintrin.h>
#else
# error compiler not supported
#endif


// note: unfortunately the MSC __cpuid intrinsic does not allow passing
// additional inputs (e.g. ecx = count), so we need to implement this
// in assembly for both IA-32 and AMD64.
static void cpuid_impl(x86_x64_CpuidRegs* regs)
{
#if ARCH_IA32
	ia32_asm_cpuid(regs);
#else
	amd64_asm_cpuid(regs);
#endif
}

bool x86_x64_cpuid(x86_x64_CpuidRegs* regs)
{
	static u32 maxFunction;
	static u32 maxExtendedFunction;
	if(!maxFunction)
	{
		x86_x64_CpuidRegs regs2;
		regs2.eax = 0;
		cpuid_impl(&regs2);
		maxFunction = regs2.eax;
		regs2.eax = 0x80000000;
		cpuid_impl(&regs2);
		maxExtendedFunction = regs2.eax;
	}

	const u32 function = regs->eax;
	if(function > maxExtendedFunction)
		return false;
	if(function < 0x80000000 && function > maxFunction)
		return false;

	cpuid_impl(regs);
	return true;
}


//-----------------------------------------------------------------------------
// capability bits

static void DetectFeatureFlags(u32 caps[4])
{
	x86_x64_CpuidRegs regs;
	regs.eax = 1;
	if(x86_x64_cpuid(&regs))
	{
		caps[0] = regs.ecx;
		caps[1] = regs.edx;
	}
	regs.eax = 0x80000001;
	if(x86_x64_cpuid(&regs))
	{
		caps[2] = regs.ecx;
		caps[3] = regs.edx;
	}
}

bool x86_x64_cap(x86_x64_Cap cap)
{
	// treated as 128 bit field; order: std ecx, std edx, ext ecx, ext edx
	// keep in sync with enum CpuCap!
	static u32 x86_x64_caps[4];

	// (since relevant CPUs will surely advertise at least one standard flag,
	// they are zero iff we haven't been initialized yet)
	if(!x86_x64_caps[1])
		DetectFeatureFlags(x86_x64_caps);

	const size_t tbl_idx = cap >> 5;
	const size_t bit_idx = cap & 0x1f;
	if(tbl_idx > 3)
	{
		DEBUG_WARN_ERR(ERR::INVALID_PARAM);
		return false;
	}
	return (x86_x64_caps[tbl_idx] & BIT(bit_idx)) != 0;
}


//-----------------------------------------------------------------------------
// CPU identification

static x86_x64_Vendors DetectVendor()
{
	x86_x64_CpuidRegs regs;
	regs.eax = 0;
	if(!x86_x64_cpuid(&regs))
		DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);

	// copy regs to string
	// note: 'strange' ebx,edx,ecx reg order is due to ModR/M encoding order.
	char vendor_str[13];
	u32* vendor_str_u32 = (u32*)vendor_str;
	vendor_str_u32[0] = regs.ebx;
	vendor_str_u32[1] = regs.edx;
	vendor_str_u32[2] = regs.ecx;
	vendor_str[12] = '\0';	// 0-terminate

	if(!strcmp(vendor_str, "AuthenticAMD"))
		return X86_X64_VENDOR_AMD;
	else if(!strcmp(vendor_str, "GenuineIntel"))
		return X86_X64_VENDOR_INTEL;
	else
	{
		DEBUG_WARN_ERR(ERR::CPU_UNKNOWN_VENDOR);
		return X86_X64_VENDOR_UNKNOWN;
	}
}

x86_x64_Vendors x86_x64_Vendor()
{
	static x86_x64_Vendors vendor = X86_X64_VENDOR_UNKNOWN;
	if(vendor == X86_X64_VENDOR_UNKNOWN)
		vendor = DetectVendor();
	return vendor;
}


static void DetectSignature(size_t* model, size_t* family)
{
	x86_x64_CpuidRegs regs;
	regs.eax = 1;
	if(!x86_x64_cpuid(&regs))
		DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);
	*model  = bits(regs.eax, 4, 7);
	*family = bits(regs.eax, 8, 11);
}


static size_t DetectGeneration()
{
	size_t model, family;
	DetectSignature(&model, &family);

	switch(x86_x64_Vendor())
	{
	case X86_X64_VENDOR_AMD:
		switch(family)
		{
		case 5:
			if(model < 6)
				return 5;	// K5
			else
				return 6;	// K6

		case 6:
			return 7;	// K7 (Athlon)

		case 0xF:
			return 8;	// K8 (Opteron)
		}
		break;

	case X86_X64_VENDOR_INTEL:
		switch(family)
		{
		case 5:
			return 5;	// Pentium

		case 6:
			if(model <= 0xD)
				return 6;	// Pentium Pro/II/III/M
			else
				return 8;	// Core2Duo

		case 0xF:
			if(model <= 6)
				return 7;	// Pentium 4/D
		}
		break;
	}

	debug_assert(0);	// unknown CPU generation
	return family;
}

size_t x86_x64_Generation()
{
	static size_t generation;
	if(!generation)
		generation = DetectGeneration();
	return generation;
}


//-----------------------------------------------------------------------------
// cache

// AMD reports maxCpuidIdFunction > 4 but consider functions 2..4 to be
// "reserved". cache characteristics are returned via ext. functions.
static void EnumerateCaches_AMD(x86_x64_CacheCallback callback)
{
	x86_x64_CacheParameters cache;

	x86_x64_CpuidRegs regs;
	regs.eax = 0x80000005;
	if(x86_x64_cpuid(&regs))
	{
		// L1D
		cache.type = X86_X64_CACHE_TYPE_DATA;
		cache.level = 1;
		cache.associativity = bits(regs.ecx, 16, 23);
		cache.lineSize      = bits(regs.ecx,  0,  7);
		cache.sharedBy      = 1;
		cache.size          = bits(regs.ecx, 24, 31)*KiB;
		callback(&cache);

		// L1I
		cache.type = X86_X64_CACHE_TYPE_INSTRUCTION;
		cache.level = 1;
		cache.associativity = bits(regs.edx, 16, 23);
		cache.lineSize      = bits(regs.edx,  0,  7);
		cache.sharedBy      = 1;
		cache.size          = bits(regs.edx, 24, 31)*KiB;
		callback(&cache);	
	}

	regs.eax = 0x80000006;
	if(x86_x64_cpuid(&regs))
	{
		// (zeroes are "reserved" encodings)
		const size_t encodedAssociativity[16] =
		{
			0,1,2,0,4,0,8,0,16,0,
			32, 48, 64, 96, 128,
			0xFF	// fully associative
		};

		// L2
		cache.type = X86_X64_CACHE_TYPE_UNIFIED;
		cache.level = 2;
		cache.associativity = encodedAssociativity[bits(regs.ecx, 12, 15)];
		cache.lineSize      = bits(regs.ecx,  0,  7);
		cache.sharedBy      = 1;
		cache.size          = bits(regs.ecx, 16, 31)*KiB;
		callback(&cache);

		// L3
		cache.type = X86_X64_CACHE_TYPE_UNIFIED;
		cache.level = 3;
		cache.associativity = encodedAssociativity[bits(regs.edx, 12, 15)];
		cache.lineSize      = bits(regs.edx,  0,  7);
		cache.sharedBy      = 1;
		cache.size          = bits(regs.edx, 18, 31)*512*KiB;	// (is rounded down to 512 KiB)
		callback(&cache);
	}
}

void x86_x64_EnumerateCaches(x86_x64_CacheCallback callback)
{
	if(x86_x64_Vendor() == X86_X64_VENDOR_AMD)
	{
		EnumerateCaches_AMD(callback);
		return;
	}

	for(u32 count = 0; ; count++)
	{
		x86_x64_CpuidRegs regs;
		regs.eax = 4;
		regs.ecx = count;
		if(!x86_x64_cpuid(&regs))
		{
			debug_assert(0);
			break;
		}

		x86_x64_CacheParameters cache;
		cache.type = (x86_x64_CacheType)bits(regs.eax, 0, 4);
		if(cache.type == X86_X64_CACHE_TYPE_NULL)	// no more remaining
			break;
		cache.level = (size_t)bits(regs.eax, 5, 7);
		cache.associativity = (size_t)bits(regs.ebx, 22, 31)+1;
		cache.lineSize = (size_t)bits(regs.ebx, 0, 11)+1;	// (yes, this also uses +1 encoding)
		cache.sharedBy = (size_t)bits(regs.eax, 14, 25)+1;
		{
			const size_t partitions = (size_t)bits(regs.ebx, 12, 21)+1;
			const size_t sets = (size_t)bits(regs.ecx, 0, 31)+1;
			cache.size = cache.associativity * partitions * cache.lineSize * sets;
		}

		callback(&cache);
	}
}


size_t x86_x64_L1CacheLineSize()
{
	static size_t l1CacheLineSize;

	if(!l1CacheLineSize)
	{
		l1CacheLineSize = 64;	// (default in case DetectL1CacheLineSize fails)

		struct DetectL1CacheLineSize
		{
			static void Callback(const x86_x64_CacheParameters* cache)
			{
				if(cache->type != X86_X64_CACHE_TYPE_DATA && cache->type != X86_X64_CACHE_TYPE_UNIFIED)
					return;
				if(cache->level != 1)
					return;
				l1CacheLineSize = cache->lineSize;
			}
		};
		x86_x64_EnumerateCaches(DetectL1CacheLineSize::Callback);
	}

	return l1CacheLineSize;
}


//-----------------------------------------------------------------------------
// identifier string

/// functor to remove substrings from the CPU identifier string
class StringStripper
{
	char* m_string;
	size_t m_max_chars;

public:
	StringStripper(char* string, size_t max_chars)
	: m_string(string), m_max_chars(max_chars)
	{
	}

	// remove all instances of substring from m_string
	void operator()(const char* substring)
	{
		const size_t substring_length = strlen(substring);
		for(;;)
		{
			char* substring_pos = strstr(m_string, substring);
			if(!substring_pos)
				break;
			const size_t substring_ofs = substring_pos - m_string;
			const size_t num_chars = m_max_chars - substring_ofs - substring_length;
			memmove(substring_pos, substring_pos+substring_length, num_chars);
		}
	}
};

static void DetectIdentifierString(char* identifierString, size_t maxChars)
{
	// get brand string (if available)
	char* pos = identifierString;
	bool have_brand_string = true;
	for(u32 function = 0x80000002; function <= 0x80000004; function++)
	{
		x86_x64_CpuidRegs regs;
		regs.eax = function;
		have_brand_string &= x86_x64_cpuid(&regs);
		memcpy(pos, &regs, 16);
		pos += 16;
	}

	// fall back to manual detect of CPU type because either:
	// - CPU doesn't support brand string (we use a flag to indicate this
	//   rather than comparing against a default value because it is safer);
	// - the brand string is useless, e.g. "Unknown". this happens on
	//   some older boards whose BIOS reprograms the string for CPUs it
	//   doesn't recognize.
	if(!have_brand_string || strncmp(identifierString, "Unknow", 6) == 0)
	{
		size_t model, family;
		DetectSignature(&model, &family);

		switch(x86_x64_Vendor())
		{
		case X86_X64_VENDOR_AMD:
			// everything else is either too old, or should have a brand string.
			if(family == 6)
			{
				if(model == 3 || model == 7)
					strcpy_s(identifierString, maxChars, "AMD Duron");
				else if(model <= 5)
					strcpy_s(identifierString, maxChars, "AMD Athlon");
				else
				{
					if(x86_x64_cap(X86_X64_CAP_AMD_MP))
						strcpy_s(identifierString, maxChars, "AMD Athlon MP");
					else
						strcpy_s(identifierString, maxChars, "AMD Athlon XP");
				}
			}
			break;

		case X86_X64_VENDOR_INTEL:
			// everything else is either too old, or should have a brand string.
			if(family == 6)
			{
				if(model == 1)
					strcpy_s(identifierString, maxChars, "Intel Pentium Pro");
				else if(model == 3 || model == 5)
					strcpy_s(identifierString, maxChars, "Intel Pentium II");
				else if(model == 6)
					strcpy_s(identifierString, maxChars, "Intel Celeron");	
				else
					strcpy_s(identifierString, maxChars, "Intel Pentium III");
			}
			break;
		}
	}
	// identifierString already holds a valid brand string; pretty it up.
	else
	{
		const char* const undesired_strings[] = { "(tm)", "(TM)", "(R)", "CPU " };
		std::for_each(undesired_strings, undesired_strings+ARRAY_SIZE(undesired_strings),
			StringStripper(identifierString, strlen(identifierString)+1));

		// note: Intel brand strings include a frequency, but we can't rely
		// on it because the CPU may be overclocked. we'll leave it in the
		// string to show measurement accuracy and if SpeedStep is active.
	}
}

const char* cpu_IdentifierString()
{
	// 3 calls x 4 registers x 4 bytes = 48
	static char identifierString[48+1] = {'\0'};
	if(identifierString[0] == '\0')
		DetectIdentifierString(identifierString, ARRAY_SIZE(identifierString));
	return identifierString;
}


//-----------------------------------------------------------------------------
// misc stateless functions

u8 x86_x64_ApicId()
{
	x86_x64_CpuidRegs regs;
	regs.eax = 1;
	// note: CPUID function 1 should be available everywhere, but only
	// processors with an xAPIC (8th generation or above, e.g. P4/Athlon XP)
	// will return a nonzero value.
	if(!x86_x64_cpuid(&regs))
		DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);
	const u8 apicId = (u8)bits(regs.ebx, 24, 31);
	return apicId;
}


u64 x86_x64_rdtsc()
{
#if MSC_VERSION
	return (u64)__rdtsc();
#elif GCC_VERSION
	// GCC supports "portable" assembly for both x86 and x64
	volatile u32 lo, hi;
	asm volatile ("rdtsc" : "=a" (lo), "=d" (hi));
	return u64_from_u32(hi, lo);
#endif
}


void x86_x64_DebugBreak()
{
#if MSC_VERSION
	__debugbreak();
#elif GCC_VERSION
	// note: this probably isn't necessary, since unix_debug_break
	// (SIGTRAP) is most probably available if GCC_VERSION.
	// we include it for completeness, though.
	__asm__ __volatile__ ("int $3");
#endif
}


void cpu_Serialize()
{
	x86_x64_CpuidRegs regs;
	regs.eax = 1;
	x86_x64_cpuid(&regs);	// CPUID serializes execution.
}


//-----------------------------------------------------------------------------
// CPU frequency

// set scheduling priority and restore when going out of scope.
class ScopedSetPriority
{
	int m_old_policy;
	sched_param m_old_param;

public:
	ScopedSetPriority(int new_priority)
	{
		// get current scheduling policy and priority
		pthread_getschedparam(pthread_self(), &m_old_policy, &m_old_param);

		// set new priority
		sched_param new_param = {0};
		new_param.sched_priority = new_priority;
		pthread_setschedparam(pthread_self(), SCHED_FIFO, &new_param);
	}

	~ScopedSetPriority()
	{
		// restore previous policy and priority.
		pthread_setschedparam(pthread_self(), m_old_policy, &m_old_param);
	}
};

// note: this function uses timer.cpp!timer_Time, which is implemented via
// whrt.cpp on Windows.
double x86_x64_ClockFrequency()
{
	// if the TSC isn't available, there's really no good way to count the
	// actual CPU clocks per known time interval, so bail.
	// note: loop iterations ("bogomips") are not a reliable measure due
	// to differing IPC and compiler optimizations.
	if(!x86_x64_cap(X86_X64_CAP_TSC))
		return -1.0;	// impossible value

	// increase priority to reduce interference while measuring.
	const int priority = sched_get_priority_max(SCHED_FIFO)-1;
	ScopedSetPriority ssp(priority);

	// note: no need to "warm up" cpuid - it will already have been
	// called several times by the time this code is reached.
	// (background: it's used in x86_x64_rdtsc() to serialize instruction flow;
	// the first call is documented to be slower on Intel CPUs)

	int num_samples = 16;
	// if clock is low-res, do less samples so it doesn't take too long.
	// balance measuring time (~ 10 ms) and accuracy (< 1 0/00 error -
	// ok for using the TSC as a time reference)
	if(timer_Resolution() >= 1e-3)
		num_samples = 8;
	std::vector<double> samples(num_samples);

	for(int i = 0; i < num_samples; i++)
	{
		double dt;
		i64 dc; // i64 because VC6 can't convert u64 -> double,
		        // and we don't need all 64 bits.

		// count # of clocks in max{1 tick, 1 ms}:
		// .. wait for start of tick.
		const double t0 = timer_Time();
		u64 c1; double t1;
		do
		{
			// note: timer_Time effectively has a long delay (up to 5 us)
			// before returning the time. we call it before x86_x64_rdtsc to
			// minimize the delay between actually sampling time / TSC,
			// thus decreasing the chance for interference.
			// (if unavoidable background activity, e.g. interrupts,
			// delays the second reading, inaccuracy is introduced).
			t1 = timer_Time();
			c1 = x86_x64_rdtsc();
		}
		while(t1 == t0);
		// .. wait until start of next tick and at least 1 ms elapsed.
		do
		{
			const double t2 = timer_Time();
			const u64 c2 = x86_x64_rdtsc();
			dc = (i64)(c2 - c1);
			dt = t2 - t1;
		}
		while(dt < 1e-3);

		// .. freq = (delta_clocks) / (delta_seconds);
		//    x86_x64_rdtsc/timer overhead is negligible.
		const double freq = dc / dt;
		samples[i] = freq;
	}

	std::sort(samples.begin(), samples.end());

	// median filter (remove upper and lower 25% and average the rest).
	// note: don't just take the lowest value! it could conceivably be
	// too low, if background processing delays reading c1 (see above).
	double sum = 0.0;
	const int lo = num_samples/4, hi = 3*num_samples/4;
	for(int i = lo; i < hi; i++)
		sum += samples[i];

	const double clock_frequency = sum / (hi-lo);
	return clock_frequency;
}
