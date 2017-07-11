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
 * Windows High Resolution Timer
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/whrt/whrt.h"

#include <process.h>	// _beginthreadex

#include "lib/sysdep/cpu.h"
#include "lib/sysdep/os/win/wutil.h"
#include "lib/sysdep/os/win/winit.h"
#include "lib/sysdep/acpi.h"
#include "lib/bits.h"

#include "lib/sysdep/os/win/whrt/counter.h"

WINIT_REGISTER_EARLY_INIT2(whrt_Init);	// wutil -> whrt -> wtime
WINIT_REGISTER_LATE_SHUTDOWN(whrt_Shutdown);


namespace ERR
{
	const Status WHRT_COUNTER_UNSAFE = 140000;
}


//-----------------------------------------------------------------------------
// choose best available safe counter

// (moved into a separate function to simplify error handling)
static inline Status ActivateCounter(ICounter* counter)
{
	RETURN_STATUS_IF_ERR(counter->Activate());

	if(!counter->IsSafe())
		return ERR::WHRT_COUNTER_UNSAFE;	// NOWARN (happens often)

	return INFO::OK;
}

/**
 * @return the newly created and unique instance of the next best counter
 * that is deemed safe, or 0 if all have already been created.
 **/
static ICounter* GetNextBestSafeCounter()
{
	for(;;)
	{
		static size_t nextCounterId = 0;
		ICounter* counter = CreateCounter(nextCounterId++);
		if(!counter)
			return 0;	// tried all, none were safe

		Status err = ActivateCounter(counter);
		if(err == INFO::OK)
		{
			debug_printf("HRT: using name=%s freq=%f\n", counter->Name(), counter->NominalFrequency());
			return counter;	// found a safe counter
		}
		else
		{
			wchar_t buf[100];
			debug_printf("HRT: activating %s failed: %s\n", counter->Name(), utf8_from_wstring(StatusDescription(err, buf, ARRAY_SIZE(buf))).c_str());
			DestroyCounter(counter);
		}
	}
}


//-----------------------------------------------------------------------------
// counter that drives the timer

static ICounter* counter;
// (these counter properties are cached for efficiency and convenience:)
static double nominalFrequency;
static double resolution;
static size_t counterBits;
static u64 counterMask;

static void InitCounter()
{
	// we used to support switching counters at runtime, but that's
	// unnecessarily complex. it need and should only be done once.
	ENSURE(counter == 0);
	counter = GetNextBestSafeCounter();
	ENSURE(counter != 0);

	nominalFrequency = counter->NominalFrequency();
	resolution       = counter->Resolution();
	counterBits      = counter->CounterBits();
	debug_printf("HRT: counter=%s freq=%g res=%g bits=%d\n", counter->Name(), nominalFrequency, resolution, counterBits);

	// sanity checks
	ENSURE(nominalFrequency >= 500.0-DBL_EPSILON);
	ENSURE(resolution <= 2e-3);
	ENSURE(8 <= counterBits && counterBits <= 64);

	counterMask = bit_mask<u64>(counterBits);
}

static void ShutdownCounter()
{
	DestroyCounter(counter);
}

static inline u64 Counter()
{
	return counter->Counter();
}

/**
 * @return difference [ticks], taking rollover into account.
 * (time-critical, so it's not called through ICounter.)
 **/
static inline u64 CounterDelta(u64 oldCounter, u64 newCounter)
{
	return (newCounter - oldCounter) & counterMask;
}

double whrt_Resolution()
{
	ENSURE(resolution != 0.0);
	return resolution;
}


//-----------------------------------------------------------------------------
// timer state

// we're not going to bother calibrating the counter (i.e. measuring its
// current frequency by means of a second timer). rationale:
// - all counters except the TSC are stable and run at fixed frequencies;
// - it's not clear that any other HRT or the tick count would be useful
//   as a stable time reference (if it were, we should be using it instead);
// - calibration would complicate the code (we'd have to make sure the
//   secondary counter is safe and can co-exist with the primary).

/**
 * stores all timer state shared between readers and the update thread.
 * (must be POD because it's used before static ctors run.)
 **/
struct TimerState
{
	// value of the counter at last update.
	u64 counter;

	// total elapsed time [seconds] since first update.
	// converted from tick deltas with the *then current* frequency
	// (this enables calibration, which is currently not implemented,
	// but leaving open the possibility costs nothing)
	double time;

	u8 padding[48];
};

// how do we detect when the old TimerState is no longer in use and can be
// freed? we use two static instances (avoids dynamic allocation headaches)
// and swap between them ('double-buffering'). it is assumed that all
// entered critical sections (the latching of TimerState fields) will have
// been exited before the next update comes around; if not, TimerState.time
// changes, the critical section notices and re-reads the new values.
static __declspec(align(64)) TimerState timerStates[2];
// note: exchanging pointers is easier than XORing an index.
static volatile TimerState* volatile ts  = &timerStates[0];
static volatile TimerState* volatile ts2 = &timerStates[1];

static void UpdateTimerState()
{
	// how can we synchronize readers and the update thread? locks are
	// preferably avoided since they're dangerous and can be slow. what we
	// need to ensure is that TimerState doesn't change while another thread is
	// accessing it. the first step is to linearize the update, i.e. have it
	// appear to happen in an instant (done by building a new TimerState and
	// having it go live by switching pointers). all that remains is to make
	// reads of the state variables consistent, done by latching them all and
	// retrying if an update came in the middle of this.

	const u64 counter = Counter();
	const u64 deltaTicks = CounterDelta(ts->counter, counter);
	ts2->counter = counter;
	ts2->time = ts->time + deltaTicks/nominalFrequency;
	ts = (volatile TimerState*)InterlockedExchangePointer((volatile PVOID*)&ts2, (PVOID)ts);
}

double whrt_Time()
{
	// latch timer state (counter and time must be from the same update)
    const volatile TimerState* state = ts;
	const double time = state->time;
	const u64 counter = state->counter;

	const u64 deltaTicks = CounterDelta(counter, Counter());
	return (time + deltaTicks/nominalFrequency);
}


//-----------------------------------------------------------------------------
// update thread

// note: we used to discipline the HRT timestamp to the system time, so it
// was advantageous to trigger updates via WinMM event (thus reducing
// instances where we're called in the middle of a scheduler tick).
// since that's no longer relevant, we prefer using a thread, because that
// avoids the dependency on WinMM and its lengthy startup time.

// rationale: (+ and - are reasons for longer and shorter lengths)
// + minimize CPU usage
// + ensure all threads currently using TimerState return from those
//   functions before the next interval
// - avoid more than 1 counter rollover per interval (InitUpdateThread makes
//   sure our interval is shorter than the current counter's rollover rate)
static const DWORD UPDATE_INTERVAL_MS = 1000;

static HANDLE hExitEvent;
static HANDLE hUpdateThread;

static unsigned __stdcall UpdateThread(void* UNUSED(data))
{
	debug_SetThreadName("whrt_UpdateThread");

	for(;;)
	{
		const DWORD ret = WaitForSingleObject(hExitEvent, UPDATE_INTERVAL_MS);
		// owner terminated or wait failed or exit event signaled - exit thread
		if(ret != WAIT_TIMEOUT)
			break;

		UpdateTimerState();
	}

	return 0;
}

static inline Status InitUpdateThread()
{
	WinScopedPreserveLastError s;	// CreateEvent

	// make sure our interval isn't too long
	// (counterBits can be 64 => Bit() would overflow => calculate period/2)
	const double period_2 = Bit<u64>(counterBits-1) / nominalFrequency;
	const size_t rolloversPerInterval = size_t(UPDATE_INTERVAL_MS / i64(period_2*2.0*1000.0));
	ENSURE(rolloversPerInterval <= 1);

	hExitEvent = CreateEvent(0, TRUE, FALSE, 0);	// manual reset, initially false
	if(hExitEvent == INVALID_HANDLE_VALUE)
		WARN_RETURN(ERR::LIMIT);

	hUpdateThread = (HANDLE)_beginthreadex(0, 0, UpdateThread, 0, 0, 0);
	if(!hUpdateThread)
		WARN_RETURN(ERR::LIMIT);

	return INFO::OK;
}

static inline void ShutdownUpdateThread()
{
	// signal thread
	BOOL ok = SetEvent(hExitEvent);
	WARN_IF_FALSE(ok);
	// the nice way is to wait for it to exit
	if(WaitForSingleObject(hUpdateThread, 100) != WAIT_OBJECT_0)
		TerminateThread(hUpdateThread, 0);	// forcibly exit (dangerous)
	CloseHandle(hExitEvent);
	CloseHandle(hUpdateThread);
}


//-----------------------------------------------------------------------------

static Status whrt_Init()
{
	InitCounter();

	// latch initial counter value so that timer starts at 0
	ts->counter = Counter();	// must come before UpdateTimerState

	UpdateTimerState();	// must come before InitUpdateThread to avoid race

	RETURN_STATUS_IF_ERR(InitUpdateThread());

	return INFO::OK;
}


static Status whrt_Shutdown()
{
	ShutdownUpdateThread();

	ShutdownCounter();

	acpi_Shutdown();

	return INFO::OK;
}
