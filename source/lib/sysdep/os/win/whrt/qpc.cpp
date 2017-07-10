/* Copyright (C) 2010 Wildfire Games.
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
 * Timer implementation using QueryPerformanceCounter
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/whrt/qpc.h"

#include "lib/sysdep/os/win/whrt/counter.h"

#include "lib/sysdep/os_cpu.h"
#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/wutil.h"	// wutil_argv
#include "lib/sysdep/os/win/whrt/pit.h"	// PIT_FREQ
#include "lib/sysdep/os/win/whrt/pmt.h"	// PMT_FREQ


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

	Status Activate()
	{
		// note: QPC is observed to be universally supported, but the API
		// provides for failure, so play it safe.

		LARGE_INTEGER qpcFreq, qpcValue;
		const BOOL ok1 = QueryPerformanceFrequency(&qpcFreq);
		const BOOL ok2 = QueryPerformanceCounter(&qpcValue);
		if(!ok1 || !ok2)
			WARN_RETURN(ERR::FAIL);
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
			const bool isTscSafe = wutil_HasCommandLineArgument(L"-wQpcTscSafe");
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
	ENSURE(sizeof(CounterQPC) <= size);
	return new(address) CounterQPC();
}
