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
 * Timer implementation using timeGetTime
 */

// note: WinMM is delay-loaded to avoid dragging it in when this timer
// implementation isn't used. (this is relevant because its startup is
// fairly slow)

#include "precompiled.h"
#include "lib/sysdep/os/win/whrt/tgt.h"

#include "lib/sysdep/os/win/whrt/counter.h"

#include "lib/sysdep/os/win/win.h"
#include <mmsystem.h>

#if MSC_VERSION
#pragma comment(lib, "winmm.lib")
#endif


// "Guidelines For Providing Multimedia Timer Support" claims that
// speeding the timer up to 2 ms has little impact, while 1 ms
// causes significant slowdown.
static const UINT PERIOD_MS = 2;

class CounterTGT : public ICounter
{
public:
	virtual const char* Name() const
	{
		return "TGT";
	}

	Status Activate()
	{
		// note: timeGetTime is always available and cannot fail.

		MMRESULT ret = timeBeginPeriod(PERIOD_MS);
		ENSURE(ret == TIMERR_NOERROR);

		return INFO::OK;
	}

	void Shutdown()
	{
		timeEndPeriod(PERIOD_MS);
	}

	bool IsSafe() const
	{
		// the only point of criticism is the possibility of falling behind
		// due to lost interrupts. this can happen to any interrupt-based timer
		// and some systems may lack a counter-based timer, so consider TGT
		// 'safe'. note that it is still only chosen when all other timers fail.
		return true;
	}

	u64 Counter() const
	{
		return timeGetTime();
	}

	size_t CounterBits() const
	{
		return 32;
	}

	double NominalFrequency() const
	{
		return 1000.0;
	}

	double Resolution() const
	{
		return PERIOD_MS*1e-3;
	}
};

ICounter* CreateCounterTGT(void* address, size_t size)
{
	ENSURE(sizeof(CounterTGT) <= size);
	return new(address) CounterTGT();
}
