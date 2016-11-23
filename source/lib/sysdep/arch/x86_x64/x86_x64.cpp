/* Copyright (c) 2011 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * CPU-specific routines common to 32 and 64-bit x86
 */

#include "precompiled.h"
#include "lib/sysdep/arch/x86_x64/x86_x64.h"

#include <cstring>
#include <cstdio>
#include <vector>
#include <set>
#include <algorithm>

#include "lib/posix/posix_pthread.h"
#include "lib/bits.h"
#include "lib/timer.h"
#include "lib/module_init.h"
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/os_cpu.h"

#if MSC_VERSION
# include <intrin.h>	// __rdtsc
#endif

namespace x86_x64 {

#if defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 150030729
// VC10+ and VC9 SP1: __cpuidex is already available
#elif GCC_VERSION
# if defined(__i386__) && defined(__PIC__)
#  define __cpuidex(regsArray, level, index)\
	__asm__ __volatile__ ("pushl %%ebx\n"\
		"cpuid\n"\
		"mov %%ebx,%1\n"\
		"popl %%ebx"\
		: "=a" ((regsArray)[0]), "=r" ((regsArray)[1]), "=c" ((regsArray)[2]), "=d" ((regsArray)[3])\
		: "0" (level), "2" (index));
# else
#  define __cpuidex(regsArray, level, index)\
	__asm__ __volatile__ ("cpuid"\
		: "=a" ((regsArray)[0]), "=b" ((regsArray)[1]), "=c" ((regsArray)[2]), "=d" ((regsArray)[3])\
		: "0" (level), "2" (index));
# endif
#else
# error "compiler not supported"
#endif


// some of this module's functions are frequently called but require
// non-trivial initialization, so caching is helpful. isInitialized
// flags aren't thread-safe, so we use ModuleInit. calling it from
// every function is a bit wasteful, but it is convenient to avoid
// requiring users to pass around a global state object.
// one big Init() would be prone to deadlock if its subroutines also
// call a public function (that re-enters ModuleInit), so each
// function gets its own initState.

//-----------------------------------------------------------------------------
// CPUID

static void Invoke_cpuid(CpuidRegs* regs)
{
	cassert(sizeof(regs->eax) == sizeof(int));
	cassert(sizeof(*regs) == 4*sizeof(int));
	__cpuidex((int*)regs, regs->eax, regs->ecx);
}

static u32 cpuid_maxFunction;
static u32 cpuid_maxExtendedFunction;

static Status InitCpuid()
{
	CpuidRegs regs = { 0 };

	regs.eax = 0;
	Invoke_cpuid(&regs);
	cpuid_maxFunction = regs.eax;

	regs.eax = 0x80000000;
	Invoke_cpuid(&regs);
	cpuid_maxExtendedFunction = regs.eax;

	return INFO::OK;
}

bool cpuid(CpuidRegs* regs)
{
	static ModuleInitState initState;
	ModuleInit(&initState, InitCpuid);

	const u32 function = regs->eax;
	if(function > cpuid_maxExtendedFunction)
		return false;
	if(function < 0x80000000 && function > cpuid_maxFunction)
		return false;

	Invoke_cpuid(regs);
	return true;
}


//-----------------------------------------------------------------------------
// capability bits

// treated as 128 bit field; order: std ecx, std edx, ext ecx, ext edx
// keep in sync with enum Cap!
static u32 caps[4];

static ModuleInitState capsInitState;

static Status InitCaps()
{
	CpuidRegs regs = { 0 };
	regs.eax = 1;
	if(cpuid(&regs))
	{
		caps[0] = regs.ecx;
		caps[1] = regs.edx;
	}
	regs.eax = 0x80000001;
	if(cpuid(&regs))
	{
		caps[2] = regs.ecx;
		caps[3] = regs.edx;
	}

	return INFO::OK;
}

bool Cap(Caps cap)
{
	ModuleInit(&capsInitState, InitCaps);

	const size_t index = cap >> 5;
	const size_t bit = cap & 0x1F;
	if(index >= ARRAY_SIZE(caps))
	{
		DEBUG_WARN_ERR(ERR::INVALID_PARAM);
		return false;
	}
	return IsBitSet(caps[index], bit);
}

void GetCapBits(u32* d0, u32* d1, u32* d2, u32* d3)
{
	ModuleInit(&capsInitState, InitCaps);

	*d0 = caps[0];
	*d1 = caps[1];
	*d2 = caps[2];
	*d3 = caps[3];
}


//-----------------------------------------------------------------------------
// vendor

static Vendors vendor;

static Status InitVendor()
{
	CpuidRegs regs = { 0 };
	regs.eax = 0;
	if(!cpuid(&regs))
		DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);

	// copy regs to string
	// note: 'strange' ebx,edx,ecx reg order is due to ModR/M encoding order.
	char vendorString[13];
	memcpy(&vendorString[0], &regs.ebx, 4);
	memcpy(&vendorString[4], &regs.edx, 4);
	memcpy(&vendorString[8], &regs.ecx, 4);
	vendorString[12] = '\0';	// 0-terminate

	if(!strcmp(vendorString, "AuthenticAMD"))
		vendor = x86_x64::VENDOR_AMD;
	else if(!strcmp(vendorString, "GenuineIntel"))
		vendor = x86_x64::VENDOR_INTEL;
	else
	{
		DEBUG_WARN_ERR(ERR::CPU_UNKNOWN_VENDOR);
		vendor = x86_x64::VENDOR_UNKNOWN;
	}

	return INFO::OK;
}

Vendors Vendor()
{
	static ModuleInitState initState;
	ModuleInit(&initState, InitVendor);
	return vendor;
}


//-----------------------------------------------------------------------------
// signature

static size_t model;
static size_t family;
static ModuleInitState signatureInitState;

static Status InitSignature()
{
	CpuidRegs regs = { 0 };
	regs.eax = 1;
	if(!cpuid(&regs))
		DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);
	model = bits(regs.eax, 4, 7);
	family = bits(regs.eax, 8, 11);
	const size_t extendedModel = bits(regs.eax, 16, 19);
	const size_t extendedFamily = bits(regs.eax, 20, 27);
	if(family == 0xF)
		family += extendedFamily;
	if(family == 0xF || (Vendor() == x86_x64::VENDOR_INTEL && family == 6))
		model += extendedModel << 4;
	return INFO::OK;
}

size_t Model()
{
	ModuleInit(&signatureInitState, InitSignature);
	return model;
}

size_t Family()
{
	ModuleInit(&signatureInitState, InitSignature);
	return family;
}




//-----------------------------------------------------------------------------
// identifier string

/// functor to remove substrings from the CPU identifier string
class StringStripper
{
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

private:
	char* m_string;
	size_t m_max_chars;
};

// 3 calls x 4 registers x 4 bytes = 48 + 0-terminator
static char identifierString[48+1];

static Status InitIdentifierString()
{
	// get brand string (if available)
	char* pos = identifierString;
	bool gotBrandString = true;
	for(u32 function = 0x80000002; function <= 0x80000004; function++)
	{
		CpuidRegs regs = { 0 };
		regs.eax = function;
		gotBrandString &= cpuid(&regs);
		memcpy(pos, &regs, 16);
		pos += 16;
	}

	// fall back to manual detect of CPU type because either:
	// - CPU doesn't support brand string (we use a flag to indicate this
	//   rather than comparing against a default value because it is safer);
	// - the brand string is useless, e.g. "Unknown". this happens on
	//   some older boards whose BIOS reprograms the string for CPUs it
	//   doesn't recognize.
	if(!gotBrandString || strncmp(identifierString, "Unknow", 6) == 0)
	{
		const size_t family = Family();
		const size_t model = Model();
		switch(Vendor())
		{
		case x86_x64::VENDOR_AMD:
			// everything else is either too old, or should have a brand string.
			if(family == 6)
			{
				if(model == 3 || model == 7)
					strcpy_s(identifierString, ARRAY_SIZE(identifierString), "AMD Duron");
				else if(model <= 5)
					strcpy_s(identifierString, ARRAY_SIZE(identifierString), "AMD Athlon");
				else
				{
					if(Cap(x86_x64::CAP_AMD_MP))
						strcpy_s(identifierString, ARRAY_SIZE(identifierString), "AMD Athlon MP");
					else
						strcpy_s(identifierString, ARRAY_SIZE(identifierString), "AMD Athlon XP");
				}
			}
			break;

		case x86_x64::VENDOR_INTEL:
			// everything else is either too old, or should have a brand string.
			if(family == 6)
			{
				if(model == 1)
					strcpy_s(identifierString, ARRAY_SIZE(identifierString), "Intel Pentium Pro");
				else if(model == 3 || model == 5)
					strcpy_s(identifierString, ARRAY_SIZE(identifierString), "Intel Pentium II");
				else if(model == 6)
					strcpy_s(identifierString, ARRAY_SIZE(identifierString), "Intel Celeron");
				else
					strcpy_s(identifierString, ARRAY_SIZE(identifierString), "Intel Pentium III");
			}
			break;

		default:
			strcpy_s(identifierString, ARRAY_SIZE(identifierString), "Unknown, non-Intel/AMD");
			break;
		}
	}
	// identifierString already holds a valid brand string; pretty it up.
	else
	{
		const char* const undesiredStrings[] = { "(tm)", "(TM)", "(R)", "CPU ", "          " };
		std::for_each(undesiredStrings, undesiredStrings+ARRAY_SIZE(undesiredStrings),
			StringStripper(identifierString, strlen(identifierString)+1));

		// note: Intel brand strings include a frequency, but we can't rely
		// on it because the CPU may be overclocked. we'll leave it in the
		// string to show measurement accuracy and if SpeedStep is active.
	}

	return INFO::OK;
}

static const char* IdentifierString()
{
	static ModuleInitState initState;
	ModuleInit(&initState, InitIdentifierString);
	return identifierString;
}


//-----------------------------------------------------------------------------
// miscellaneous stateless functions

#if !MSC_VERSION	// ensure not already defined in header
u64 rdtsc()
{
#if GCC_VERSION
	// GCC supports "portable" assembly for both x86 and x64
	volatile u32 lo, hi;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return u64_from_u32(hi, lo);
#endif
}
#endif


void DebugBreak()
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


//-----------------------------------------------------------------------------
// CPU frequency

// set scheduling priority and restore when going out of scope.
class ScopedSetPriority
{
public:
	ScopedSetPriority(int newPriority)
	{
		// get current scheduling policy and priority
		pthread_getschedparam(pthread_self(), &m_oldPolicy, &m_oldParam);

		// set new priority
		sched_param newParam = {0};
		newParam.sched_priority = newPriority;
		pthread_setschedparam(pthread_self(), SCHED_FIFO, &newParam);
	}

	~ScopedSetPriority()
	{
		// restore previous policy and priority.
		pthread_setschedparam(pthread_self(), m_oldPolicy, &m_oldParam);
	}

private:
	int m_oldPolicy;
	sched_param m_oldParam;
};

// note: this function uses timer.cpp!timer_Time, which is implemented via
// whrt.cpp on Windows.
double ClockFrequency()
{
	// if the TSC isn't available, there's really no good way to count the
	// actual CPU clocks per known time interval, so bail.
	// note: loop iterations ("bogomips") are not a reliable measure due
	// to differing IPC and compiler optimizations.
	if(!Cap(x86_x64::CAP_TSC))
		return -1.0;	// impossible value

	// increase priority to reduce interference while measuring.
	const int priority = sched_get_priority_max(SCHED_FIFO)-1;
	ScopedSetPriority ssp(priority);

	// note: no need to "warm up" cpuid - it will already have been
	// called several times by the time this code is reached.
	// (background: it's used in rdtsc() to serialize instruction flow;
	// the first call is documented to be slower on Intel CPUs)

	size_t numSamples = 16;
	// if clock is low-res, do less samples so it doesn't take too long.
	// balance measuring time (~ 10 ms) and accuracy (< 0.1% error -
	// ok for using the TSC as a time reference)
	if(timer_Resolution() >= 1e-3)
		numSamples = 8;
	std::vector<double> samples(numSamples);

	for(size_t i = 0; i < numSamples; i++)
	{
		double dt;
		i64 dc;	// (i64 instead of u64 for faster conversion to double)

		// count # of clocks in max{1 tick, 1 ms}:
		// .. wait for start of tick.
		const double t0 = timer_Time();
		u64 c1; double t1;
		do
		{
			// note: timer_Time effectively has a long delay (up to 5 us)
			// before returning the time. we call it before rdtsc to
			// minimize the delay between actually sampling time / TSC,
			// thus decreasing the chance for interference.
			// (if unavoidable background activity, e.g. interrupts,
			// delays the second reading, inaccuracy is introduced).
			t1 = timer_Time();
			c1 = rdtsc();
		}
		while(t1 == t0);
		// .. wait until start of next tick and at least 1 ms elapsed.
		do
		{
			const double t2 = timer_Time();
			const u64 c2 = rdtsc();
			dc = (i64)(c2 - c1);
			dt = t2 - t1;
		}
		while(dt < 1e-3);

		// .. freq = (delta_clocks) / (delta_seconds);
		//    rdtsc/timer overhead is negligible.
		const double freq = dc / dt;
		samples[i] = freq;
	}

	std::sort(samples.begin(), samples.end());

	// median filter (remove upper and lower 25% and average the rest).
	// note: don't just take the lowest value! it could conceivably be
	// too low, if background processing delays reading c1 (see above).
	double sum = 0.0;
	const size_t lo = numSamples/4, hi = 3*numSamples/4;
	for(size_t i = lo; i < hi; i++)
		sum += samples[i];

	const double clockFrequency = sum / (hi-lo);
	return clockFrequency;
}

}	// namespace x86_x64


const char* cpu_IdentifierString()
{
	return x86_x64::IdentifierString();
}
