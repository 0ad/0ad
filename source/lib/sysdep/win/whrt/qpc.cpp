/**
 * =========================================================================
 * File        : qpc.cpp
 * Project     : 0 A.D.
 * Description : Timer implementation using QueryPerformanceCounter
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "qpc.h"

#include "lib/sysdep/win/win.h"
#include "lib/sysdep/win/wcpu.h"
#include "pit.h"	// PIT_FREQ
#include "pmt.h"	// PMT_FREQ


LibError CounterQPC::Activate()
{
	// note: QPC is observed to be universally supported, but the API
	// provides for failure, so play it safe.

	LARGE_INTEGER qpcFreq, qpcValue;
	const BOOL ok1 = QueryPerformanceFrequency(&qpcFreq);
	const BOOL ok2 = QueryPerformanceCounter(&qpcValue);
	WARN_RETURN_IF_FALSE(ok1 && ok2);
	if(!qpcFreq.QuadPart || !qpcValue.QuadPart)
		WARN_RETURN(ERR::FAIL);

	m_frequency = (i64)qpcFreq.QuadPart;
	return INFO::OK;
}

void CounterQPC::Shutdown()
{
}

bool CounterQPC::IsSafe() const
{
	// the PIT is entirely safe (even if annoyingly slow to read)
	if(m_frequency == PIT_FREQ)
		return true;

	// note: we have separate modules that directly access some of the
	// counters potentially used by QPC. marking them or QPC unsafe is
	// risky because users can override either of those decisions.
	// directly disabling them is ugly (increased coupling).
	// instead, we'll make sure our implementations can coexist with QPC and
	// verify the secondary reference timer has a different frequency.

	// the PMT is safe (see discussion in CounterPmt::IsSafe);
	if(m_frequency == PIT_FREQ)
		return true;

	// two other implementations have been observed: HPET
	// (on Vista) and RDTSC (on MP HAL).
	//
	// - the HPET is reliable but can't easily be recognized since its
	//   frequency is variable (the spec says > 10 MHz; the master 14.318 MHz
	//   oscillator is often used). note: considering frequencies between
	//   10..100 MHz to be a HPET would be dangerous because it may actually
	//   be faster or RDTSC slower.
	//
	// - the TSC implementation has been known to be buggy (even mentioned
	//   in MSDN) and we don't know which systems have been patched. it is
	//   therefore considered unsafe and recognized by comparing frequency
	//   against the CPU clock.

	// QPC frequency matches the CPU clock => it uses RDTSC => unsafe.
	if(IsSimilarMagnitude(m_frequency, wcpu_ClockFrequency()))
		return false;
	// unconfirmed reports indicate QPC sometimes uses 1/3 of the
	// CPU clock frequency, so check that as well.
	if(IsSimilarMagnitude(m_frequency, wcpu_ClockFrequency()/3))
		return false;

	// otherwise: it's apparently using the HPET => safe.
	return true;
}

u64 CounterQPC::Counter() const
{
	// fairly time-critical here, don't check the return value
	// (IsSupported made sure it succeeded initially)
	LARGE_INTEGER qpc_value;
	(void)QueryPerformanceCounter(&qpc_value);
	return qpc_value.QuadPart;
}

/**
 * WHRT uses this to ensure the counter (running at nominal frequency)
 * doesn't overflow more than once during CALIBRATION_INTERVAL_MS.
 **/
uint CounterQPC::CounterBits() const
{
	// there are reports of incorrect rollover handling in the PMT
	// implementation of QPC (see CounterPMT::IsSafe). however, other
	// counters would be used on those systems, so it's irrelevant.
	// we'll report the full 64 bits.
	return 64;
}

/**
 * initial measurement of the tick rate. not necessarily correct
 * (e.g. when using TSC: cpu_ClockFrequency isn't exact).
 **/
double CounterQPC::NominalFrequency() const
{
	return (double)m_frequency;
}
