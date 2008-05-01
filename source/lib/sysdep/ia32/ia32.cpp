/**
 * =========================================================================
 * File        : ia32.cpp
 * Project     : 0 A.D.
 * Description : C++ and inline asm implementations of IA-32 functions
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "ia32.h"

#include <string.h>
#include <stdio.h>
#include <vector>
#include <set>
#include <algorithm>

#include "lib/posix/posix.h"	// pthread
#include "lib/bits.h"
#include "lib/timer.h"
#include "lib/sysdep/cpu.h"
#include "ia32_memcpy.h"

#if !MSC_VERSION && !GCC_VERSION
#error ia32.cpp needs inline assembly support!
#endif


//-----------------------------------------------------------------------------
// capability bits

static void DetectFeatureFlags(u32 caps[4])
{
	u32 regs[4];
	if(ia32_asm_cpuid(1, regs))
	{
		caps[0] = regs[ECX];
		caps[1] = regs[EDX];
	}
	if(ia32_asm_cpuid(0x80000001, regs))
	{
		caps[2] = regs[ECX];
		caps[3] = regs[EDX];
	}
}

bool ia32_cap(IA32Cap cap)
{
	// treated as 128 bit field; order: std ecx, std edx, ext ecx, ext edx
	// keep in sync with enum CpuCap!
	static u32 ia32_caps[4];

	// (since relevant CPUs will surely advertise at least one standard flag,
	// they are zero iff we haven't been initialized yet)
	if(!ia32_caps[1])
		DetectFeatureFlags(ia32_caps);

	const uint tbl_idx = cap >> 5;
	const uint bit_idx = cap & 0x1f;
	if(tbl_idx > 3)
	{
		DEBUG_WARN_ERR(ERR::INVALID_PARAM);
		return false;
	}
	return (ia32_caps[tbl_idx] & BIT(bit_idx)) != 0;
}


//-----------------------------------------------------------------------------
// CPU identification

static Ia32Vendor DetectVendor()
{
	u32 regs[4];
	if(!ia32_asm_cpuid(0, regs))
		DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);

	// copy regs to string
	// note: 'strange' ebx,edx,ecx reg order is due to ModR/M encoding order.
	char vendor_str[13];
	u32* vendor_str_u32 = (u32*)vendor_str;
	vendor_str_u32[0] = regs[EBX];
	vendor_str_u32[1] = regs[EDX];
	vendor_str_u32[2] = regs[ECX];
	vendor_str[12] = '\0';	// 0-terminate

	if(!strcmp(vendor_str, "AuthenticAMD"))
		return IA32_VENDOR_AMD;
	else if(!strcmp(vendor_str, "GenuineIntel"))
		return IA32_VENDOR_INTEL;
	else
	{
		DEBUG_WARN_ERR(ERR::CPU_UNKNOWN_VENDOR);
		return IA32_VENDOR_UNKNOWN;
	}
}

Ia32Vendor ia32_Vendor()
{
	static Ia32Vendor vendor = IA32_VENDOR_UNKNOWN;
	if(vendor == IA32_VENDOR_UNKNOWN)
		vendor = DetectVendor();
	return vendor;
}


static void DetectSignature(uint* model, uint* family)
{
	u32 regs[4];
	if(!ia32_asm_cpuid(1, regs))
		DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);
	*model  = bits(regs[EAX], 4, 7);
	*family = bits(regs[EAX], 8, 11);
}


static uint DetectGeneration()
{
	uint model, family;
	DetectSignature(&model, &family);

	switch(ia32_Vendor())
	{
	case IA32_VENDOR_AMD:
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

	case IA32_VENDOR_INTEL:
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

uint ia32_Generation()
{
	static uint generation;
	if(!generation)
		generation = DetectGeneration();
	return generation;
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
	// note: ia32_asm_cpuid writes 4 u32s directly to identifierString -
	// be very careful with pointer arithmetic!
	u32* u32_string = (u32*)identifierString;
	bool have_brand_string = false;
	if(ia32_asm_cpuid(0x80000002, u32_string+0 ) &&
	   ia32_asm_cpuid(0x80000003, u32_string+4) &&
	   ia32_asm_cpuid(0x80000004, u32_string+8))
		have_brand_string = true;

	// fall back to manual detect of CPU type because either:
	// - CPU doesn't support brand string (we use a flag to indicate this
	//   rather than comparing against a default value because it is safer);
	// - the brand string is useless, e.g. "Unknown". this happens on
	//   some older boards whose BIOS reprograms the string for CPUs it
	//   doesn't recognize.
	if(!have_brand_string || strncmp(identifierString, "Unknow", 6) == 0)
	{
		uint model, family;
		DetectSignature(&model, &family);

		switch(ia32_Vendor())
		{
		case IA32_VENDOR_AMD:
			// everything else is either too old, or should have a brand string.
			if(family == 6)
			{
				if(model == 3 || model == 7)
					strcpy_s(identifierString, maxChars, "AMD Duron");
				else if(model <= 5)
					strcpy_s(identifierString, maxChars, "AMD Athlon");
				else
				{
					if(ia32_cap(IA32_CAP_AMD_MP))
						strcpy_s(identifierString, maxChars, "AMD Athlon MP");
					else
						strcpy_s(identifierString, maxChars, "AMD Athlon XP");
				}
			}
			break;

		case IA32_VENDOR_INTEL:
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
// whrt.cpp on Windows, which again calls ia32_Init. be careful that
// this function isn't called from there as well, else WHRT will be used
// before its init completes.
double ia32_ClockFrequency()
{
	// if the TSC isn't available, there's really no good way to count the
	// actual CPU clocks per known time interval, so bail.
	// note: loop iterations ("bogomips") are not a reliable measure due
	// to differing IPC and compiler optimizations.
	if(!ia32_cap(IA32_CAP_TSC))
		return -1.0;	// impossible value

	// increase priority to reduce interference while measuring.
	const int priority = sched_get_priority_max(SCHED_FIFO)-1;
	ScopedSetPriority ssp(priority);

	// note: no need to "warm up" cpuid - it will already have been
	// called several times by the time this code is reached.
	// (background: it's used in ia32_rdtsc() to serialize instruction flow;
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
			// before returning the time. we call it before ia32_rdtsc to
			// minimize the delay between actually sampling time / TSC,
			// thus decreasing the chance for interference.
			// (if unavoidable background activity, e.g. interrupts,
			// delays the second reading, inaccuracy is introduced).
			t1 = timer_Time();
			c1 = ia32_rdtsc();
		}
		while(t1 == t0);
		// .. wait until start of next tick and at least 1 ms elapsed.
		do
		{
			const double t2 = timer_Time();
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
	for(int i = lo; i < hi; i++)
		sum += samples[i];

	const double clock_frequency = sum / (hi-lo);
	return clock_frequency;
}


//-----------------------------------------------------------------------------
// processor topology

uint ia32_ApicId()
{
	u32 regs[4];
	if(!ia32_asm_cpuid(1, regs))
		DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);
	const uint apicId = bits(regs[EBX], 24, 31);
	return apicId;
}


// OSes report hyperthreading units and cores as "processors". we need to
// drill down and find out the exact counts (for thread pool dimensioning
// and cache sharing considerations).
// note: Intel Appnote 485 (CPUID) assures uniformity of coresPerPackage and
// logicalPerCore.

static uint DetectCoresPerPackage()
{
	u32 regs[4];
	switch(ia32_Vendor())
	{
	case IA32_VENDOR_INTEL:
		if(ia32_asm_cpuid(4, regs))
			return bits(regs[EAX], 26, 31)+1;
		break;

	case IA32_VENDOR_AMD:
		if(ia32_asm_cpuid(0x80000008, regs))
			return bits(regs[ECX], 0, 7)+1;
		break;
	}

	return 1;	// else: the CPU is single-core.
}

static uint CoresPerPackage()
{
	static uint coresPerPackage = 0;
	if(!coresPerPackage)
		coresPerPackage = DetectCoresPerPackage();
	return coresPerPackage;
}


static bool IsHyperthreadingCapable()
{
	// definitely not
	if(!ia32_cap(IA32_CAP_HT))
		return false;

	// AMD N-core systems falsely set the HT bit for compatibility reasons
	// (don't bother resetting it, might confuse callers)
	if(ia32_Vendor() == IA32_VENDOR_AMD && ia32_cap(IA32_CAP_AMD_CMP_LEGACY))
		return false;

	return true;
}

static uint DetectLogicalPerCore()
{
	if(!IsHyperthreadingCapable())
		return 1;

	u32 regs[4];
	if(!ia32_asm_cpuid(1, regs))
		DEBUG_WARN_ERR(ERR::CPU_FEATURE_MISSING);
	const uint logicalPerPackage = bits(regs[EBX], 16, 23);

	// cores ought to be uniform WRT # logical processors
	debug_assert(logicalPerPackage % CoresPerPackage() == 0);

	return logicalPerPackage / CoresPerPackage();
}

static uint LogicalPerCore()
{
	static uint logicalPerCore = 0;
	if(!logicalPerCore)
		logicalPerCore = DetectLogicalPerCore();
	return logicalPerCore;
}


// the above two functions give the maximum number of cores/logical units.
// however, some of them may actually be disabled by the BIOS!
// what we can do is to analyze the APIC IDs. they are allocated sequentially
// for all "processors". treating the IDs as variable-width bitfields
// (according to the number of cores/logical units present) allows
// determining the exact topology as well as number of packages.

// these are set by DetectProcessorTopology.
static uint numPackages = 0;	// i.e. sockets; > 1 => true SMP system
static uint enabledCoresPerPackage = 0;
static uint enabledLogicalPerCore = 0;	// hyperthreading units

typedef std::vector<u8> Ids;
typedef std::set<u8> IdSet;

// add the currently running processor's APIC ID to a list of IDs.
static void StoreApicId(void* param)
{
	Ids* apicIds = (Ids*)param;
	apicIds->push_back(ia32_ApicId());
}


// field := a range of bits sufficient to represent <num_values> integers.
// for each id in apicIds: extract the value of the field at offset bit_pos
// and insert it into ids. afterwards, adjust bit_pos to the next field.
// used to gather e.g. all core IDs from all APIC IDs.
static void ExtractFieldsIntoSet(const Ids& apicIds, uint& bit_pos, uint num_values, IdSet& ids)
{
	const uint id_bits = ceil_log2(num_values);
	if(id_bits == 0)
		return;

	const uint mask = bit_mask(id_bits);

	for(size_t i = 0; i < apicIds.size(); i++)
	{
		const u8 apic_id = apicIds[i];
		const u8 field = (apic_id >> bit_pos) & mask;
		ids.insert(field);
	}

	bit_pos += id_bits;
}


// @return false if unavailable / no information can be returned.
static bool DetectProcessorTopologyViaApicIds()
{
	// old APIC (see ia32_ApicId for details)
	if(ia32_Generation() < 8)
		return false;

	// get the set of all APIC IDs
	Ids apicIds;
	// .. OS affinity support is missing or excludes us from some processors
	if(cpu_CallByEachCPU(StoreApicId, &apicIds) != INFO::OK)
		return false;
	// .. if IDs aren't unique, cpu_CallByEachCPU is broken.
	std::sort(apicIds.begin(), apicIds.end());
	debug_assert(std::unique(apicIds.begin(), apicIds.end()) == apicIds.end());

	// extract values from all 3 ID bitfields into separate sets
	uint bit_pos = 0;
	IdSet logicalIds;
	ExtractFieldsIntoSet(apicIds, bit_pos, LogicalPerCore(), logicalIds);
	IdSet coreIds;
	ExtractFieldsIntoSet(apicIds, bit_pos, CoresPerPackage(), coreIds);
	IdSet packageIds;
	ExtractFieldsIntoSet(apicIds, bit_pos, 0xFF, packageIds);

	// (the set cardinality is representative of all packages/cores since
	// their numbers are uniform across the system.)
	numPackages            = std::max((uint)packageIds.size(), 1u);
	enabledCoresPerPackage = std::max((uint)coreIds   .size(), 1u);
	enabledLogicalPerCore  = std::max((uint)logicalIds.size(), 1u);

	// note: even though APIC IDs are assigned sequentially, we can't make any
	// assumptions about the values/ordering because we get them according to
	// the CPU affinity mask, which is unknown.

	return true;
}


static void GuessProcessorTopologyViaOsCount()
{
	const int numProcessors = cpu_NumProcessors();

	// note: we cannot hope to always return correct results since disabled
	// cores/logical units cannot be distinguished from the situation of the
	// OS simply not reporting them as "processors". unfortunately this
	// function won't always only be called for older (#core = #logical = 1)
	// systems because DetectProcessorTopologyViaApicIds may fail due to
	// lack of OS support. what we'll do is assume nothing is disabled; this
	// is reasonable because we care most about #packages. it's fine to assume
	// more cores (without inflating the total #processors) because that
	// count only indicates memory barriers etc. ought to be used.
	enabledCoresPerPackage = CoresPerPackage();
	enabledLogicalPerCore = LogicalPerCore();

	const long numPackagesTimesLogical = numProcessors / CoresPerPackage();
	debug_assert(numPackagesTimesLogical != 0);	// otherwise processors didn't include cores, which would be stupid

	numPackages = numPackagesTimesLogical / LogicalPerCore();
	if(!numPackages)	// processors didn't include logical units (reasonable)
		numPackages = numPackagesTimesLogical;
}


// determine how many CoresPerPackage and LogicalPerCore are
// actually enabled and also count numPackages.
static void DetectProcessorTopology()
{
	// authoritative, but requires newer CPU, and OS support.
	if(DetectProcessorTopologyViaApicIds())
		return;	// success, we're done.

	GuessProcessorTopologyViaOsCount();
}


uint cpu_NumPackages()
{
	if(!numPackages)
		DetectProcessorTopology();
	return (uint)numPackages;
}

uint cpu_CoresPerPackage()
{
	if(!enabledCoresPerPackage)
		DetectProcessorTopology();
	return (uint)enabledCoresPerPackage;
}

uint cpu_LogicalPerCore()
{
	if(!enabledLogicalPerCore)
		DetectProcessorTopology();
	return (uint)enabledLogicalPerCore;
}


//-----------------------------------------------------------------------------
// misc stateless functions

// this RDTSC implementation writes edx:eax to a temporary and returns that.
// rationale: this insulates against changing compiler calling conventions,
// at the cost of some efficiency.
// use ia32_asm_rdtsc_edx_eax instead if the return convention is known to be
// edx:eax (should be the case on all 32-bit x86).
u64 ia32_rdtsc_safe()
{
	u64 c;
#if MSC_VERSION
	__asm
	{
		cpuid
		rdtsc
		mov			dword ptr [c], eax
		mov			dword ptr [c+4], edx
	}
#elif GCC_VERSION
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


void ia32_DebugBreak()
{
#if MSC_VERSION
	__asm int 3
		// note: this probably isn't necessary, since unix_debug_break
		// (SIGTRAP) is most probably available if GCC_VERSION.
		// we include it for completeness, though.
#elif GCC_VERSION
	__asm__ __volatile__ ("int $3");
#endif
}


// enforce strong memory ordering.
void cpu_MemoryFence()
{
	// Pentium IV
	if(ia32_cap(IA32_CAP_SSE2))
#if MSC_VERSION
		__asm mfence
#elif GCC_VERSION
		__asm__ __volatile__ ("mfence");
#endif
}


// checks if there is an IA-32 CALL instruction right before ret_addr.
// returns INFO::OK if so and ERR::FAIL if not.
// also attempts to determine the call target. if that is possible
// (directly addressed relative or indirect jumps), it is stored in
// target, which is otherwise 0.
//
// this is useful for walking the stack manually.
LibError ia32_GetCallTarget(void* ret_addr, void** target)
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


void cpu_ConfigureFloatingPoint()
{
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


//-----------------------------------------------------------------------------
// thunk functions for ia32_asm to allow DLL export

void cpu_AtomicAdd(volatile intptr_t* location, intptr_t increment)
{
	ia32_asm_AtomicAdd(location, increment);
}

bool cpu_CAS(volatile uintptr_t* location, uintptr_t expected, uintptr_t new_value)
{
	return ia32_asm_CAS(location, expected, new_value);
}

void cpu_Serialize()
{
	return ia32_asm_Serialize();
}

void* cpu_memcpy(void* RESTRICT dst, const void* RESTRICT src, size_t size)
{
	return ia32_memcpy(dst, src, size);
}
