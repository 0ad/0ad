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

#include "lib/sysdep/win/win.h"
#include "lib/sysdep/win/wcpu.h"
#include "lib/sysdep/ia32/ia32.h"
#include "lib/sysdep/cpu.h"		// cpu_CAS


//-----------------------------------------------------------------------------
// per-CPU state

// necessary because CPUs are initialized one-by-one and the TSC values
// differ significantly. (while at it, we also keep per-CPU frequency values
// in case the clocks aren't exactly synced)
//
// note: only reading the TSC from one CPU (possible via thread affinity)
// would work but take much longer (context switch).

struct PerCpuTscState
{
	u64 calibrationTicks;
	double calibrationTime;
	// mark this struct used just in case cpu_CallByEachCPU doesn't ensure
	// only one thread is running. a flag is safer than a magic APIC ID value.
	uintptr_t isInitialized;
	uint apicId;
	float observedFrequency;
};

static const size_t MAX_CPUS = 32;	// Win32 also imposes this limit
static PerCpuTscState cpuTscStates[MAX_CPUS];

static PerCpuTscState& NextUnusedPerCpuTscState()
{
	for(size_t i = 0; i < MAX_CPUS; i++)
	{
		PerCpuTscState& cpuTscState = cpuTscStates[i];
		if(cpu_CAS(&cpuTscState.isInitialized, 0, 1))
			return cpuTscState;
	}

	throw std::runtime_error("allocated too many PerCpuTscState");
}

static PerCpuTscState& CurrentCpuTscState()
{
	const uint apicId = ia32_ApicId();
	for(size_t i = 0; i < MAX_CPUS; i++)
	{
		PerCpuTscState& cpuTscState = cpuTscStates[i];
		if(cpuTscState.isInitialized && cpuTscState.apicId == apicId)
			return cpuTscState;
	}

	throw std::runtime_error("no matching PerCpuTscState found");
}

static void InitPerCpuTscState(void* param)	// callback
{
	const double cpuClockFrequency = *(double*)param;

	PerCpuTscState& cpuTscState = NextUnusedPerCpuTscState();
	cpuTscState.apicId            = ia32_ApicId();
	cpuTscState.calibrationTicks  = ia32_rdtsc();
	cpuTscState.calibrationTime   = 0.0;
	cpuTscState.observedFrequency = cpuClockFrequency;
}

static LibError InitPerCpuTscStates(double cpuClockFrequency)
{
	LibError ret = cpu_CallByEachCPU(InitPerCpuTscState, &cpuClockFrequency);
	CHECK_ERR(ret);
	return INFO::OK;
}

//-----------------------------------------------------------------------------

// note: calibration is necessary due to long-term thermal drift
// (oscillator is usually poor quality) and inaccurate initial measurement.

//-----------------------------------------------------------------------------


TickSourceTsc::TickSourceTsc()
{
	if(!ia32_cap(IA32_CAP_TSC))
		throw TickSourceUnavailable("TSC: unsupported");

	if(InitPerCpuTscStates(wcpu_ClockFrequency()) != INFO::OK)
		throw TickSourceUnavailable("TSC: per-CPU init failed");
}

TickSourceTsc::~TickSourceTsc()
{
}

bool TickSourceTsc::IsSafe() const
{
return false;

	u32 regs[4];
	if(ia32_asm_cpuid(0x80000007, regs))
	{
	//	if(regs[EDX] & POWERNOW_FREQ_ID_CTRL)
	}


	/*
	AMD  has  defined  a CPUID  feature  bit  that
	software   can   test   to   determine   if   the   TSC   is
	invariant. Issuing a CPUID instruction with an %eax register
	value of  0x8000_0007, on a  processor whose base  family is
	0xF, returns "Advanced  Power Management Information" in the
	%eax, %ebx, %ecx,  and %edx registers.  Bit 8  of the return
	%edx is  the "TscInvariant" feature  flag which is  set when
	TSC is P-state, C-state, and STPCLK-throttling invariant; it
	is clear otherwise.
	*/

#if 0
	if (CPUID.base_family < 0xf) {
	// TSC drift doesn't exist on 7th Gen or less
	// However, OS still needs to consider effects
	// of P-state changes on TSC
	return TRUE;

	} else if (CPUID.AdvPowerMgmtInfo.TscInvariant) {
	// Invariant TSC on 8th Gen or newer, use it
	// (assume all cores have invariant TSC)
	return TRUE;

	// - deep sleep modes: TSC may not be advanced.
	//   not a problem though, because if the TSC is disabled, the CPU
	//   isn't doing any other work, either.
	// - SpeedStep/'gearshift' CPUs: frequency may change.
	//   this happens on notebooks now, but eventually desktop systems
	//   will do this as well (if not to save power, for heat reasons).
	//   frequency changes are too often and drastic to correct,
	//   and we don't want to mess with the system power settings => unsafe.
	if(cpu_IsThrottlingPossible() == 0)
		return true;


		/* But TSC doesn't tick in C3 so don't use it there */
	957                 if (acpi_fadt.length > 0 && acpi_fadt.plvl3_lat < 1000)
		958                         return 1;

#endif
	return false;
}

u64 TickSourceTsc::Ticks() const
{
	return ia32_rdtsc();
}

/**
 * WHRT uses this to ensure the counter (running at nominal frequency)
 * doesn't overflow more than once during CALIBRATION_INTERVAL_MS.
 **/
uint TickSourceTsc::CounterBits() const
{
	return 64;
}

/**
 * initial measurement of the tick rate. not necessarily correct
 * (e.g. when using TSC: cpu_ClockFrequency isn't exact).
 **/
double TickSourceTsc::NominalFrequency() const
{
	return wcpu_ClockFrequency();
}
