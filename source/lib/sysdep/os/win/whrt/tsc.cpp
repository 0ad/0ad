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

#include "counter.h"

#include "lib/bits.h"
#include "lib/sysdep/os_cpu.h"
#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/wutil.h"

#if ARCH_IA32 || ARCH_AMD64
# include "lib/sysdep/arch/x86_x64/x86_x64.h"	// x86_x64_rdtsc
# include "lib/sysdep/arch/x86_x64/topology.h"
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

class CounterTSC : public ICounter
{
public:
	virtual const char* Name() const
	{
		return "TSC";
	}
		
	LibError Activate()
	{
#if ARCH_IA32 || ARCH_AMD64
		if(!x86_x64_cap(X86_X64_CAP_TSC))
			return ERR::NO_SYS;		// NOWARN (CPU doesn't support RDTSC)
#endif

		return INFO::OK;
	}

	void Shutdown()
	{
	}

	bool IsSafe() const
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
		{
			WinScopedLock lock(WHRT_CS);
			const CpuTopology* topology = cpu_topology_Detect();
			if(cpu_topology_NumPackages(topology) != 1 || cpu_topology_CoresPerPackage(topology) != 1)
				return false;
		}

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

	u64 Counter() const
	{
		return x86_x64_rdtsc();
	}

	size_t CounterBits() const
	{
		return 64;
	}

	double NominalFrequency() const
	{
		// WARNING: do not call x86_x64_ClockFrequency because it uses the
		// HRT, which we're currently in the process of initializing.
		// instead query CPU clock frequency via OS.
		//
		// note: even here, initial accuracy isn't critical because the
		// clock is subject to thermal drift and would require continual
		// recalibration anyway.
		return os_cpu_ClockFrequency();
	}

	double Resolution() const
	{
		return 1.0 / NominalFrequency();
	}
};

ICounter* CreateCounterTSC(void* address, size_t size)
{
	debug_assert(sizeof(CounterTSC) <= size);
	return new(address) CounterTSC();
}
