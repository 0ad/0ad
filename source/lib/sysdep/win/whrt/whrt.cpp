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

#include "hpet.h"
#include "pmt.h"
#include "qpc.h"
#include "tgt.h"
#include "tsc.h"


#pragma SECTION_PRE_LIBC(L)	// dependencies: wposix
WIN_REGISTER_FUNC(whrt_Init);
#pragma FORCE_INCLUDE(whrt_Init)
#pragma SECTION_POST_ATEXIT(D)
WIN_REGISTER_FUNC(whrt_Shutdown);
#pragma FORCE_INCLUDE(whrt_Shutdown)
#pragma SECTION_RESTORE


// see http://www.gamedev.net/reference/programming/features/timing/ .


static bool IsTickSourceEstablished();
static int RolloversPerCalibrationInterval(double frequency, uint counterBits);


//-----------------------------------------------------------------------------
// safety recommendation / override

// while we do our best to work around timer problems or avoid them if unsafe,
// future requirements and problems may be different. allow the user or app
// override TickSource::IsSafe decisions.

cassert(WHRT_DEFAULT == 0);	// ensure 0 is the correct initializer
static WhrtOverride overrides[WHRT_NUM_TICK_SOURCES];	// indexed by WhrtTickSourceId

void whrt_OverrideRecommendation(WhrtTickSourceId id, WhrtOverride override)
{
	// calling this function only makes sense when tick source hasn't
	// been chosen yet
	debug_assert(!IsTickSourceEstablished());

	debug_assert(id < WHRT_NUM_TICK_SOURCES);
	overrides[id] = override;
}

static bool IsSafe(const TickSource* tickSource, WhrtTickSourceId id)
{
	debug_assert(id < WHRT_NUM_TICK_SOURCES);
	if(overrides[id] == WHRT_DISABLE)
		return false;
	if(overrides[id] == WHRT_FORCE)
		return true;

	return tickSource->IsSafe();
}

//-----------------------------------------------------------------------------
// manage tick sources

// use static array to avoid allocations (max #implementations is known)
static TickSource* tickSources[WHRT_NUM_TICK_SOURCES];
static uint nextTickSourceId = 0;

// factory
static TickSource* CreateTickSource(WhrtTickSourceId id)
{
	switch(id)
	{
	case WHRT_TSC:
		return new TickSourceTsc();
	case WHRT_QPC:
		return new TickSourceQpc();
	case WHRT_HPET:
		return new TickSourceHpet();
	case WHRT_PMT:
		return new TickSourcePmt();
	case WHRT_TGT:
		return new TickSourceTgt();
	NODEFAULT;
	}
}

/**
 * @return the newly created and unique instance of the next tick source,
 * or 0 if all have already been created.
 *
 * notes:
 * - stores the tick source in tickSources[] with index = id.
 * - don't always create all tick sources - some require 'lengthy' init.
 **/
static TickSource* CreateNextBestTickSource()
{
	for(;;)
	{
		if(nextTickSourceId == WHRT_NUM_TICK_SOURCES)
			return 0;
		WhrtTickSourceId id = (WhrtTickSourceId)nextTickSourceId++;

		try
		{
			TickSource* tickSource = CreateTickSource(id);
			debug_printf("HRT/ create id=%d name=%s freq=%f\n", id, tickSource->Name(), tickSource->NominalFrequency());
			debug_assert(tickSources[id] == 0);
			tickSources[id] = tickSource;
			return tickSource;
		}
		catch(TickSourceUnavailable& e)
		{
			debug_printf("HRT/ create id=%d failed: %s\n", id, e.what());
		}
	}
}

static bool IsTickSourceAcceptable(TickSource* tickSource, WhrtTickSourceId id, TickSource* undesiredTickSource = 0)
{
	// not (yet|successfully) created
	if(!tickSource)
		return false;

	// it's the one we don't want (typically the primary source)
	if(tickSource == undesiredTickSource)
		return false;

	// unsafe
	if(!IsSafe(tickSource, id))
		return false;

	// duplicate source (i.e. frequency matches that of another)
	for(uint id = 0; ; id++)
	{
		TickSource* tickSource2 = tickSources[id];
		// not (yet|successfully) created
		if(!tickSource2)
			continue;
		// if there are two sources with the same frequency, the one with
		// higher precedence (lower ID) should be taken, so stop when we
		// reach tickSource's ID.
		if(tickSource == tickSource2)
			break;
		if(IsSimilarMagnitude(tickSource->NominalFrequency(), tickSource2->NominalFrequency()))
			return false;
	}

	return true;
}

static TickSource* DetermineBestSafeTickSource(TickSource* undesiredTickSource = 0)
{
	// until one is found or all have been created:
	for(;;)
	{
		// check all existing sources in decreasing order of precedence
		for(uint id = 0; id < WHRT_NUM_TICK_SOURCES; id++)
		{
			TickSource* tickSource = tickSources[id];
			if(IsTickSourceAcceptable(tickSource, (WhrtTickSourceId)id, undesiredTickSource))
				return tickSource;
		}

		// no acceptable source found; create the next one
		if(!CreateNextBestTickSource())
			return 0;	// have already created all sources
	}
}

static void ShutdownTickSources()
{
	for(uint i = 0; i < WHRT_NUM_TICK_SOURCES; i++)
	{
		SAFE_DELETE(tickSources[i]);
	}
}


//-----------------------------------------------------------------------------
// (primary) tick source

static TickSource* primaryTickSource;

static bool IsTickSourceEstablished()
{
	return (primaryTickSource != 0);
};

static void ChooseTickSource()
{
	// we used to support switching tick sources at runtime, but that's
	// unnecessarily complex. it need and should only be done once.
	debug_assert(!IsTickSourceEstablished());

	primaryTickSource = DetermineBestSafeTickSource();

	const int rollovers = RolloversPerCalibrationInterval(primaryTickSource->NominalFrequency(), primaryTickSource->CounterBits());
	debug_assert(rollovers <= 1);
}

/// @return ticks (unspecified start point)
i64 whrt_Ticks()
{
	const u64 ticks = primaryTickSource->Ticks();
	return (i64)ticks;
}

double whrt_NominalFrequency()
{
	const double frequency = primaryTickSource->NominalFrequency();
	return frequency;
}

double whrt_Resolution()
{
	const double resolution = primaryTickSource->Resolution();
	return resolution;
}






//-----------------------------------------------------------------------------


static u64 initialTicks;

double whrt_Time()
{
	i64 deltaTicks = whrt_Ticks() - initialTicks;
	double seconds = deltaTicks / whrt_NominalFrequency();
	return seconds;
}


// must be an object so we can CAS-in the pointer to it

#if 0

class Calibrator
{
	// ticks at init or last calibration.
	// ticks since then are scaled by 1/hrt_cur_freq and added to hrt_cal_time
	// to yield the current time.
	u64 lastTicks;

	//IHighResTimer safe;
	u64 safe_last;

	double LastFreqs[8];	// ring buffer

	// used to calibrate and second-guess the primary
	static TickSource* secondaryTickSource;


	// value of hrt_time() at last calibration. needed so that changes to
	// hrt_cur_freq don't affect the previous ticks (example: 72 ticks elapsed,
	// nominal freq = 8 => time = 9.0. if freq is calculated as 9, time would
	// go backwards to 8.0).
	static double hrt_cal_time = 0.0;

	// current ticks per second; average of last few values measured in
	// calibrate(). needed to prevent long-term drift, and because
	// hrt_nominal_freq isn't necessarily correct. only affects the ticks since
	// last calibration - don't want to retroactively change the time.
	double CurFreq;
};

calibrationTickSource = DetermineBestSafeTickSource(primaryTickSource);


// return seconds since init.
//
// split to allow calling from calibrate without recursive locking.
// (not a problem, but avoids a BoundsChecker warning)
static double time_lk()
{
	debug_assert(hrt_cur_freq > 0.0);
	debug_assert(hrt_cal_ticks > 0);

	// elapsed ticks and time since last calibration
	const i64 delta_ticks = ticks_lk() - hrt_cal_ticks;
	const double delta_time = delta_ticks / hrt_cur_freq;

	return hrt_cal_time + delta_time;
}


// measure current HRT freq - prevents long-term drift; also useful because
// hrt_nominal_freq isn't necessarily exact.
//
// lock must be held.
static void calibrate_lk()
{
	debug_assert(hrt_cal_ticks > 0);

	// we're called from a WinMM event or after thread wakeup,
	// so the timer has just been updated.
	// no need to determine tick / compensate.

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
// calibration thread

// note: we used to discipline the HRT timestamp to the system time, so it
// was advantageous to wake up the calibration thread via WinMM event
// (reducing instances where we're called in the middle of a scheduler tick).
// since that's no longer relevant, we prefer using a thread, because that
// avoids the dependency on WinMM and its lengthy startup time.

// rationale: (+ and - are reasons for longer and shorter lengths)
// + minimize CPU usage
// + tolerate possibly low secondary tick source resolution
// - notice frequency drift quickly enough
// - no more than 1 counter rollover per interval (this is checked via
//   RolloversPerCalibrationInterval)
static const DWORD CALIBRATION_INTERVAL_MS = 1000;

static int RolloversPerCalibrationInterval(double frequency, uint counterBits)
{
	const double period = BIT64(counterBits) / frequency;
	const i64 period_ms = cpu_i64FromDouble(period*1000.0);
	return CALIBRATION_INTERVAL_MS / period_ms;
}

static HANDLE hExitEvent;
static HANDLE hCalibrationThread;

static unsigned __stdcall CalibrationThread(void* UNUSED(data))
{
	debug_set_thread_name("whrt_calibrate");

	for(;;)
	{
		const DWORD ret = WaitForSingleObject(hExitEvent, CALIBRATION_INTERVAL_MS);
		// owner terminated or wait failed or exit event signaled - exit thread
		if(ret != WAIT_TIMEOUT)
			break;

///		Calibrate();
	}

	return 0;
}

static inline LibError InitCalibrationThread()
{
	hExitEvent = CreateEvent(0, TRUE, FALSE, 0);	// manual reset, initially false
	if(hExitEvent == INVALID_HANDLE_VALUE)
		WARN_RETURN(ERR::LIMIT);

	hCalibrationThread = (HANDLE)_beginthreadex(0, 0, CalibrationThread, 0, 0, 0);
	if(!hCalibrationThread)
		WARN_RETURN(ERR::LIMIT);

	return INFO::OK;
}

static inline void ShutdownCalibrationThread()
{
	// signal thread
	BOOL ok = SetEvent(hExitEvent);
	WARN_IF_FALSE(ok);
	// the nice way is to wait for it to exit
	if(WaitForSingleObject(hCalibrationThread, 100) != WAIT_OBJECT_0)
		TerminateThread(hCalibrationThread, 0);	// forcibly exit (dangerous)
	CloseHandle(hExitEvent);
	CloseHandle(hCalibrationThread);
}


//-----------------------------------------------------------------------------

static LibError whrt_Init()
{
	ChooseTickSource();

	// latch start times
	initialTicks = whrt_Ticks();

//	RETURN_ERR(InitCalibrationThread());

	return INFO::OK;
}


static LibError whrt_Shutdown()
{
//	ShutdownCalibrationThread();
	ShutdownTickSources();

	return INFO::OK;
}
