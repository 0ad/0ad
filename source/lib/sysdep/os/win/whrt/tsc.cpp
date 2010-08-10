/* Copyright (c) 2010 Wildfire Games
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
 * Timer implementation using RDTSC
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/whrt/tsc.h"

#include "lib/sysdep/os/win/whrt/counter.h"

#include "lib/bits.h"
#include "lib/sysdep/acpi.h"
#include "lib/sysdep/os_cpu.h"
#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/wutil.h"

#if ARCH_X86_X64
# include "lib/sysdep/arch/x86_x64/x86_x64.h"	// x86_x64_rdtsc
# include "lib/sysdep/arch/x86_x64/topology.h"
# include "lib/sysdep/arch/x86_x64/msr.h"
#endif


//-----------------------------------------------------------------------------

static bool IsUniprocessor()
{
	if(cpu_topology_NumPackages() != 1)
		return false;
	if(cpu_topology_CoresPerPackage() != 1)
		return false;
	return true;
}


static bool IsInvariantTSC()
{
#if ARCH_X86_X64
	// (we no longer need to check x86_x64_Vendor - Intel and AMD
	// agreed on the definition of this feature check)
	x86_x64_CpuidRegs regs = { 0 };
	regs.eax = 0x80000007;
	if(x86_x64_cpuid(&regs))
	{
		// TSC is invariant across P-state, C-state, turbo, and
		// stop grant transitions (e.g. STPCLK)
		if(regs.edx & BIT(8))
			return true;
	}
#endif

	return false;
}


static bool IsThrottlingPossible()
{
#if ARCH_X86_X64
	x86_x64_CpuidRegs regs = { 0 };
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
			enum AmdPowerNowFlags
			{
				PN_FREQ_ID_CTRL    = BIT(1),
				PN_HW_THERMAL_CTRL = BIT(4),
				PN_SW_THERMAL_CTRL = BIT(5)
			};
			if(regs.edx & (PN_FREQ_ID_CTRL|PN_HW_THERMAL_CTRL|PN_SW_THERMAL_CTRL))
				return true;
		}
		break;
	}
#endif

	return false;
}


//-----------------------------------------------------------------------------

class CounterTSC : public ICounter
{
public:
	virtual const wchar_t* Name() const
	{
		return L"TSC";
	}

	LibError Activate()
	{
#if ARCH_X86_X64
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
		// using the TSC for timing is subject to a litany of
		// potential problems, discussed below:

		if(IsInvariantTSC())
			return true;

		// SMP or multi-core => counters are unsynchronized. both offset and
		// drift could be solved by maintaining separate per-core
		// counter states, but that requires atomic reads of the TSC and
		// the current processor number.
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
		// per-core counter state and the race condition mentioned above.
		// however, we won't bother, since such platforms aren't yet widespread
		// and would surely support the nice and safe HPET, anyway)
		if(!IsUniprocessor())
			return false;

		const FADT* fadt = (const FADT*)acpi_GetTable("FACP");
		if(fadt)
		{
			debug_assert(fadt->header.size >= sizeof(FADT));

			// TSC isn't incremented in deep-sleep states => unsafe.
			if(fadt->IsC3Supported())
				return false;

			// frequency throttling possible => unsafe.
			if(fadt->IsDutyCycleSupported())
				return false;
		}

#if ARCH_X86_X64
		// recent CPU:
		//if(x86_x64_Generation() >= 7)
		{
			// note: 8th generation CPUs support C1-clock ramping, which causes
			// drift on multi-core systems, but those were excluded above.

			// in addition to frequency changes due to P-state transitions,
			// we're also subject to STPCLK throttling. this happens when
			// the chipset thinks the system is dangerously overheated; the
			// OS isn't even notified. this may be rare, but could cause
			// incorrect results => unsafe.
			//return false;
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
#if ARCH_X86_X64
		if(MSR::IsAccessible && MSR::HasNehalem())
		{
			const u64 platformInfo = MSR::Read(MSR::NHM_PLATFORM_INFO);
			const u8 maxNonTurboRatio = bits(platformInfo, 8, 15);
			return maxNonTurboRatio * 133.33e6f;
		}
		else
#endif
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
