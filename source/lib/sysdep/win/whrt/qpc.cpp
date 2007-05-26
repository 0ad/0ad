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


TickSourceQpc::TickSourceQpc()
{
	// note: QPC is observed to be universally supported, but the API
	// provides for failure, so play it safe.

	LARGE_INTEGER qpcFreq, qpcValue;
	const BOOL ok1 = QueryPerformanceFrequency(&qpcFreq);
	const BOOL ok2 = QueryPerformanceCounter(&qpcValue);
	if(!ok1 || !ok2 || !qpcFreq.QuadPart || !qpcValue.QuadPart)
		throw TickSourceUnavailable("QPC not supported?!");

	m_frequency = (i64)qpcFreq.QuadPart;
}

TickSourceQpc::~TickSourceQpc()
{
}

bool TickSourceQpc::IsSafe() const
{
	// the PIT is entirely safe (even if annoyingly slow to read)
	if(m_frequency == PIT_FREQ)
		return true;

	// note: we have separate modules that directly access some of the
	// tick sources potentially used by QPC. marking them or QPC unsafe is
	// risky because users can override either of those decisions.
	// directly disabling them is ugly (increased coupling).
	// instead, we'll make sure our implementations can coexist with QPC and
	// verify the secondary reference timer has a different frequency.

	// the PMT is safe (see discussion in TickSourcePmt::IsSafe);
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

	const double cpuClockFrequency = wcpu_ClockFrequency();
	// failed for some reason => can't tell if RDTSC is being used
	// => assume unsafe
	if(cpuClockFrequency == 0.0)
		return false;

	// QPC frequency matches the CPU clock => it uses RDTSC => unsafe.
	if(IsSimilarMagnitude(m_frequency, cpuClockFrequency))
		return false;
	// unconfirmed reports indicate QPC sometimes uses 1/3 of the
	// CPU clock frequency, so check that as well.
	if(IsSimilarMagnitude(m_frequency, cpuClockFrequency/3))
		return false;

	// otherwise: it's apparently using the HPET => safe.
	return true;
}

u64 TickSourceQpc::Ticks() const
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
uint TickSourceQpc::CounterBits() const
{
	// note: the PMT is either 24 or 32 bits; older QPC implementations
	// apparently had troubles with rollover.
	// "System clock problem can inflate benchmark scores"
	// (http://www.lionbridge.com/bi/cont2000/200012/perfcnt.asp ; no longer
	// online, nor findable in Google Cache / archive.org) reports
	// incorrect values every 4.6 seconds unless the timer is polled in
	// the meantime. the given timeframe corresponds to 24 bits @ 3.57 MHz.
	//
	// we will therefore return the worst case value of 24 when using PMT
	// (don't bother checking if it's 32-bit because there's no harm in
	// ignoring the upper bits since we read it often enough)
	if(m_frequency == PMT_FREQ)
		return 24;

	// no reports of trouble with the other implementations have surfaced,
	// so we'll assume Windows correctly handles rollover and that we
	// have the full 64 bits.
	return 64;
}

/**
 * initial measurement of the tick rate. not necessarily correct
 * (e.g. when using TSC: cpu_ClockFrequency isn't exact).
 **/
double TickSourceQpc::NominalFrequency() const
{
	return (double)m_frequency;
}
