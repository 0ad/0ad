/**
 * =========================================================================
 * File        : whrt.cpp
 * Project     : 0 A.D.
 * Description : Windows High Resolution Timer
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "whrt.h"

#include <process.h>	// _beginthreadex

#include "lib/sysdep/win/win.h"
#include "lib/sysdep/win/winit.h"
#include "lib/sysdep/win/wcpu.h"
#include "lib/adts.h"
#include "lib/bits.h"

#include "tsc.h"
#include "hpet.h"
#include "pmt.h"
#include "qpc.h"
#include "tgt.h"
// to add a new counter type, simply include its header here and
// insert a case in ConstructCounterAt's switch statement.


#pragma SECTION_PRE_LIBC(D)	// wposix depends on us
WIN_REGISTER_FUNC(whrt_Init);
#pragma FORCE_INCLUDE(whrt_Init)
#pragma SECTION_POST_ATEXIT(V)
WIN_REGISTER_FUNC(whrt_Shutdown);
#pragma FORCE_INCLUDE(whrt_Shutdown)
#pragma SECTION_RESTORE


namespace ERR
{
	const LibError WHRT_COUNTER_UNSAFE = 140000;
}


//-----------------------------------------------------------------------------
// create/destroy counters

/**
 * @return pointer to a newly constructed ICounter subclass of type <id> at
 * the given address, or 0 iff the ID is invalid.
 * @param size receives the size [bytes] of the created instance.
 **/
static ICounter* ConstructCounterAt(uint id, void* address, size_t& size)
{
	// rationale for placement new: see call site.
#define CREATE(impl)\
	size = sizeof(Counter##impl);\
	return new(address) Counter##impl();

#include "lib/nommgr.h"	// MMGR interferes with placement new

	// counters are chosen according to the following order. rationale:
	// - TSC must come before QPC and PMT to make sure a bug in the latter on
	//   Pentium systems doesn't come up.
	// - TGT really isn't as safe as the others, so it should be last.
	// - low-overhead and high-resolution counters are preferred.
	switch(id)
	{
	case 0:
		CREATE(TSC)
	case 1:
		CREATE(HPET)
	case 2:
		CREATE(PMT)
	case 3:
		CREATE(QPC)
	case 4:
		CREATE(TGT)
	default:
		size = 0;
		return 0;
	}

#include "lib/mmgr.h"

#undef CREATE
}

/**
 * @return a newly created Counter of type <id> or 0 iff the ID is invalid.
 **/
static ICounter* CreateCounter(uint id)
{
	// we placement-new the Counter classes in a static buffer.
	// this is dangerous, but we are careful to ensure alignment. it is
	// unusual and thus bad, but there's also one advantage: we avoid
	// using global operator new before the CRT is initialized (risky).
	//
	// note: we can't just define these classes as static because their
	// ctors (necessary for vptr initialization) will run during _cinit,
	// which is already after our use of them.
	static const size_t MEM_SIZE = 200;	// checked below
	static u8 mem[MEM_SIZE];
	static u8* nextMem = mem;

	u8* addr = (u8*)round_up((uintptr_t)nextMem, 16);
	size_t size;
	ICounter* counter = ConstructCounterAt(id, addr, size);

	nextMem = addr+size;
	debug_assert(nextMem < mem+MEM_SIZE);	// had enough room?

	return counter;
}


static inline void DestroyCounter(ICounter*& counter)
{
	if(!counter)
		return;

	counter->Shutdown();
	counter->~ICounter();	// must be called due to placement new
	counter = 0;
}


//-----------------------------------------------------------------------------
// choose best available counter

// (moved into a separate function to simplify error handling)
static inline LibError ActivateCounter(ICounter* counter)
{
	RETURN_ERR(counter->Activate());

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
		static uint nextCounterId = 0;
		ICounter* counter = CreateCounter(nextCounterId++);
		if(!counter)
			return 0;	// tried all, none were safe

		LibError err = ActivateCounter(counter);
		if(err == INFO::OK)
		{
			debug_printf("HRT/ using name=%s freq=%f\n", counter->Name(), counter->NominalFrequency());
			return counter;	// found a safe counter
		}
		else
		{
			char buf[100];
			debug_printf("HRT/ activating %s failed: %s\n", counter->Name(), error_description_r(err, buf, ARRAY_SIZE(buf)));
			DestroyCounter(counter);
		}
	}
}


//-----------------------------------------------------------------------------
// counter that drives the timer

static ICounter* counter;
static double nominalFrequency;
static double resolution;
static uint counterBits;
static u64 counterMask;

static void InitCounter()
{
	// we used to support switching counters at runtime, but that's
	// unnecessarily complex. it need and should only be done once.
	debug_assert(counter == 0);
	counter = GetNextBestSafeCounter();
	debug_assert(counter != 0);

	nominalFrequency = counter->NominalFrequency();
	resolution       = counter->Resolution();
	counterBits      = counter->CounterBits();

	counterMask = bit_mask64(counterBits);

	// sanity checks
	debug_assert(nominalFrequency >= 500.0);
	debug_assert(resolution <= 2e-3);
	debug_assert(8 <= counterBits && counterBits <= 64);
}

static void ShutdownCounter()
{
	DestroyCounter(counter);
}

static inline u64 Counter()
{
	return counter->Counter();
}

/// @return difference [ticks], taking rollover into account.
static inline u64 CounterDelta(u64 oldCounter, u64 newCounter)
{
	return (newCounter - oldCounter) & counterMask;
}

double whrt_Resolution()
{
	return resolution;
}


//-----------------------------------------------------------------------------
// timer state

/**
 * stores all timer state shared between readers and the update thread.
 * (must be POD because it's used before static ctors run.)
 **/
struct TimerState
{
	// current value of the counter.
	u64 counter;

	// sum of all counter ticks since first update.
	// rollover is not an issue (even at a high frequency of 10 GHz,
	// it'd only happen after 58 years)
	u64 ticks;

	// total elapsed time [seconds] since first update.
	// converted from tick deltas with the *then current* frequency
	// (avoids retroactive changes when then frequency changes)
	double time;

	// current frequency that will be used to convert ticks to seconds.
	double frequency;
};

// how do we detect when the old TimerState is no longer in use and can be
// freed? we use two static instances (avoids dynamic allocation headaches)
// and swap between them ('double-buffering'). it is assumed that all
// entered critical sections (the latching of TimerState fields) will have
// been exited before the next update comes around; if not, TimerState.time
// changes, the critical section notices and re-reads the new values.
static TimerState timerStates[2];
// note: exchanging pointers is easier than XORing an index.
static TimerState* volatile ts  = &timerStates[0];
static TimerState* volatile ts2 = &timerStates[1];

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
	ts2->frequency = nominalFrequency;
	ts2->ticks = ts->ticks + deltaTicks;
	ts2->time  = ts->time  + deltaTicks/ts2->frequency;
	ts = (TimerState*)InterlockedExchangePointer(&ts2, ts);
}

double whrt_Time()
{
retry:
	// latch timer state (counter and time must be from the same update)
	const double time = ts->time;
	const u64 counter = ts->counter;
	// ts changed after reading time. note: don't compare counter because
	// it _might_ have the same value after two updates.
	if(time != ts->time)
		goto retry;

	const u64 deltaTicks = CounterDelta(counter, Counter());
	return (time + deltaTicks/ts->frequency);
}



#if 0



class Calibrator
{
	double LastFreqs[8];	// ring buffer

	// current ticks per second; average of last few values measured in
	// calibrate(). needed to prevent long-term drift, and because
	// hrt_nominal_freq isn't necessarily correct. only affects the ticks since
	// last calibration - don't want to retroactively change the time.
	double CurFreq;
};

calibrationCounter = DetermineBestSafeCounter(counter);
IsSimilarMagnitude(counter->NominalFrequency(), counter2->NominalFrequency()

// measure current HRT freq - prevents long-term drift; also useful because
// hrt_nominal_freq isn't necessarily exact.
static void calibrate_lk()
{
	debug_assert(hrt_cal_ticks > 0);

	// we're called from a WinMM event or after thread wakeup,
	// so the timer has just been updated. no need to determine tick / compensate.

	// get elapsed HRT ticks
	const i64 hrt_cur = ticks_lk();
	const i64 hrt_d = hrt_cur - hrt_cal_ticks;
	hrt_cal_ticks = hrt_cur;

	hrt_cal_time += hrt_d / hrt_cur_freq;

	// get elapsed time from safe millisecond timer
	static long safe_last = LONG_MAX;
	// chosen so that dt and therefore hrt_est_freq will be negative
	// on first call => it won't be added to buffer
	const long safe_cur = safe_time();
	const double dt = (safe_cur - safe_last) / safe_timer_freq;
	safe_last = safe_cur;

	double hrt_est_freq = hrt_d / dt;

	// past couple of calculated hrt freqs, for averaging
	typedef RingBuf<double, 8> SampleBuf;
	static SampleBuf samples;

	// only add to buffer if within 10% of nominal
	// (don't want to pollute buffer with flukes / incorrect results)
	if(fabs(hrt_est_freq/hrt_nominal_freq - 1.0) < 0.10)
	{
		samples.push_back(hrt_est_freq);

		// average all samples in buffer
		double freq_sum = std::accumulate(samples.begin(), samples.end(), 0.0);
		hrt_cur_freq = freq_sum / (int)samples.size();
	}
	else
	{
		samples.clear();

		hrt_cur_freq = hrt_nominal_freq;
	}

	debug_assert(hrt_cur_freq > 0.0);
}

#endif




















//-----------------------------------------------------------------------------
// update thread

// note: we used to discipline the HRT timestamp to the system time, so it
// was advantageous to perform updates triggered by a WinMM event
// (reducing instances where we're called in the middle of a scheduler tick).
// since that's no longer relevant, we prefer using a thread, because that
// avoids the dependency on WinMM and its lengthy startup time.

// rationale: (+ and - are reasons for longer and shorter lengths)
// + minimize CPU usage
// + tolerate possibly low secondary counter resolution
// + ensure all threads currently using TimerState return from those
//   functions before the next interval
// - notice frequency drift quickly enough
// - ensure there's no more than 1 counter rollover per interval (this is
//   checked via RolloversPerCalibrationInterval)
static const DWORD UPDATE_INTERVAL_MS = 1000;

static HANDLE hExitEvent;
static HANDLE hUpdateThread;

static unsigned __stdcall UpdateThread(void* UNUSED(data))
{
	debug_set_thread_name("whrt_UpdateThread");

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

static inline LibError InitUpdateThread()
{
	// make sure our interval isn't too long
	// (counterBits can be 64 => BIT64 would overflow => calculate period/2
	const double period_2 = BIT64(counterBits-1) / nominalFrequency;
	const uint rolloversPerInterval = UPDATE_INTERVAL_MS / cpu_i64FromDouble(period_2*2.0*1000.0);
	debug_assert(rolloversPerInterval <= 1);

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

static LibError whrt_Init()
{
	InitCounter();

	UpdateTimerState();	// must come before InitUpdateThread to avoid race

	RETURN_ERR(InitUpdateThread());

	return INFO::OK;
}


static LibError whrt_Shutdown()
{
	ShutdownUpdateThread();

	ShutdownCounter();

	return INFO::OK;
}
