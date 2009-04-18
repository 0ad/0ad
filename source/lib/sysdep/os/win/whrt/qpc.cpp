/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Timer implementation using QueryPerformanceCounter
 */

#include "precompiled.h"
#include "qpc.h"

#include "counter.h"

#include "lib/sysdep/os_cpu.h"
#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/wutil.h"	// wutil_argv
#include "pit.h"	// PIT_FREQ
#include "pmt.h"	// PMT_FREQ


class CounterQPC : public ICounter
{
public:
	CounterQPC()
		: m_frequency(-1)
	{
	}

	virtual const char* Name() const
	{
		return "QPC";
	}

	LibError Activate()
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

	void Shutdown()
	{
	}

	bool IsSafe() const
	{
		// note: we have separate modules that directly access some of the
		// counters potentially used by QPC. disabling the redundant counters
		// would be ugly (increased coupling). instead, we'll make sure our
		// implementations could (if necessary) coexist with QPC, but it
		// shouldn't come to that since only one counter is needed/used.

		// the PIT is entirely safe (even if annoyingly slow to read)
		if(m_frequency == PIT_FREQ)
			return true;

		// the PMT is generally safe (see discussion in CounterPmt::IsSafe),
		// but older QPC implementations had problems with 24-bit rollover.
		// "System clock problem can inflate benchmark scores"
		// (http://www.lionbridge.com/bi/cont2000/200012/perfcnt.asp ; no longer
		// online, nor findable in Google Cache / archive.org) tells of
		// incorrect values every 4.6 seconds (i.e. 24 bits @ 3.57 MHz) unless
		// the timer is polled in the meantime. fortunately, this is guaranteed
		// by our periodic updates (which come at least that often).
		if(m_frequency == PMT_FREQ)
			return true;

		// the TSC has been known to be buggy (even mentioned in MSDN). it is
		// used on MP HAL systems and can be detected by comparing QPF with the
		// CPU clock. we consider it unsafe unless the user promises (via
		// command line) that it's patched and thus reliable on their system.
		bool usesTsc = IsSimilarMagnitude((double)m_frequency, os_cpu_ClockFrequency());
		// unconfirmed reports indicate QPC sometimes uses 1/3 of the
		// CPU clock frequency, so check that as well.
		usesTsc |= IsSimilarMagnitude((double)m_frequency, os_cpu_ClockFrequency()/3);
		if(usesTsc)
		{
			const bool isTscSafe = wutil_HasCommandLineArgument("-wQpcTscSafe");
			return isTscSafe;
		}

		// the HPET is reliable and used on Vista. it can't easily be recognized
		// since its frequency is variable (the spec says > 10 MHz; the master
		// 14.318 MHz oscillator is often used). considering frequencies in
		// [10, 100 MHz) to be a HPET would be dangerous because it may actually
		// be faster or RDTSC slower. we have to exclude all other cases and
		// assume it's a HPET - and thus safe - if we get here.
		return true;
	}

	u64 Counter() const
	{
		// fairly time-critical here, don't check the return value
		// (IsSupported made sure it succeeded initially)
		LARGE_INTEGER qpc_value;
		(void)QueryPerformanceCounter(&qpc_value);
		return qpc_value.QuadPart;
	}

	size_t CounterBits() const
	{
		// there are reports of incorrect rollover handling in the PMT
		// implementation of QPC (see CounterPMT::IsSafe). however, other
		// counters would be used on those systems, so it's irrelevant.
		// we'll report the full 64 bits.
		return 64;
	}

	double NominalFrequency() const
	{
		return (double)m_frequency;
	}

	double Resolution() const
	{
		return 1.0 / m_frequency;
	}

private:
	// used in several places and QPF is a bit slow+cumbersome.
	// (i64 allows easier conversion to double)
	i64 m_frequency;
};

ICounter* CreateCounterQPC(void* address, size_t size)
{
	debug_assert(sizeof(CounterQPC) <= size);
	return new(address) CounterQPC();
}
