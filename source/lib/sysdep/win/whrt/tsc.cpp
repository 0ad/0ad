/**
 * =========================================================================
 * File        : tsc.cpp
 * Project     : 0 A.D.
 * Description : Timer implementation using RDTSC
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "tsc.h"

#include "lib/sysdep/cpu.h"
#include "lib/sysdep/win/win.h"
#include "lib/bits.h"

#if ARCH_IA32 || ARCH_AMD64
# include "lib/sysdep/x86_x64/x86_x64.h"	// x86_x64_rdtsc
# include "lib/sysdep/x86_x64/topology.h"
#endif


//-----------------------------------------------------------------------------
// detect throttling

enum AmdPowerNowFlags
{
	PN_FREQ_ID_CTRL    = BIT(1),
	PN_SW_THERMAL_CTRL = BIT(5),
	PN_INVARIANT_TSC   = BIT(8)
};

static bool IsThrottlingPossible()
{
#if ARCH_IA32 || ARCH_AMD64
	x86_x64_CpuidRegs regs;
	switch(x86_x64_Vendor())
	{
	case X86_X64_VENDOR_INTEL:
		if(x86_x64_cap(X86_X64_CAP_TM_SCC) || x86_x64_cap(X86_X64_CAP_EST))
			return true;
		break;

	case X86_X64_VENDOR_AMD:
		regs.eax = 0x80000007;
		if(x86_x64_cpuid(&regs))
		{
			if(regs.edx & (PN_FREQ_ID_CTRL|PN_SW_THERMAL_CTRL))
				return true;
		}
		break;
	}
	return false;
#endif
}


//-----------------------------------------------------------------------------

LibError CounterTSC::Activate()
{
#if ARCH_IA32 || ARCH_AMD64
	if(!x86_x64_cap(X86_X64_CAP_TSC))
		return ERR::NO_SYS;		// NOWARN (CPU doesn't support RDTSC)
#endif

	return INFO::OK;
}

void CounterTSC::Shutdown()
{
}

bool CounterTSC::IsSafe() const
{
	// use of the TSC for timing is subject to a litany of potential problems:
	// - separate, unsynchronized counters with offset and drift;
	// - frequency changes (P-state transitions and STPCLK throttling);
	// - failure to increment in C3 and C4 deep-sleep states.
	// we will discuss the specifics below.

	// SMP or multi-core => counters are unsynchronized. this could be
	// solved by maintaining separate per-core counter states, but that
	// requires atomic reads of the TSC and the current processor number.
	//
	// (otherwise, we have a subtle race condition: if preempted while
	// reading the time and rescheduled on a different core, incorrect
	// results may be returned, which would be unacceptable.)
	//
	// unfortunately this isn't possible without OS support or the
	// as yet unavailable RDTSCP instruction => unsafe.
	//
	// (note: if the TSC is invariant, drift is no longer a concern.
	// we could synchronize the TSC MSRs during initialization and avoid
	// per-core counter state and the abovementioned race condition.
	// however, we won't bother, since such platforms aren't yet widespread
	// and would surely support the nice and safe HPET, anyway)
	if(cpu_NumPackages() != 1 || cpu_CoresPerPackage() != 1)
		return false;

#if ARCH_IA32 || ARCH_AMD64
	// recent CPU:
	if(x86_x64_Generation() >= 7)
	{
		// note: 8th generation CPUs support C1-clock ramping, which causes
		// drift on multi-core systems, but those were excluded above.

		x86_x64_CpuidRegs regs;
		regs.eax = 0x80000007;
		if(x86_x64_cpuid(&regs))
		{
			// TSC is invariant WRT P-state, C-state and STPCLK => safe.
			if(regs.edx & PN_INVARIANT_TSC)
				return true;
		}

		// in addition to P-state transitions, we're also subject to
		// STPCLK throttling. this happens when the chipset thinks the
		// system is dangerously overheated; the OS isn't even notified.
		// it may be rare, but could cause incorrect results => unsafe.
		return false;

		// newer systems also support the C3 Deep Sleep state, in which
		// the TSC isn't incremented. that's not nice, but irrelevant
		// since STPCLK dooms the TSC on those systems anyway.
	}
#endif

	// we're dealing with a single older CPU; the only problem there is
	// throttling, i.e. changes to the TSC frequency. we don't want to
	// disable this because it may be important for cooling. the OS
	// initiates changes but doesn't notify us; jumps are too frequent
	// and drastic to detect and account for => unsafe.
	if(IsThrottlingPossible())
		return false;

	return true;
}

u64 CounterTSC::Counter() const
{
	return x86_x64_rdtsc();
}

/**
 * WHRT uses this to ensure the counter (running at nominal frequency)
 * doesn't overflow more than once during CALIBRATION_INTERVAL_MS.
 **/
size_t CounterTSC::CounterBits() const
{
	return 64;
}

/**
 * initial measurement of the tick rate. not necessarily correct
 * (e.g. when using TSC: cpu_ClockFrequency isn't exact).
 **/
double CounterTSC::NominalFrequency() const
{
	return cpu_ClockFrequency();
}
