// Windows-specific high resolution timer
// Copyright (c) 2003 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include "hrt.h"
#include "lib.h"
#include "adts.h"
#include "sysdep/ia32.h"
#include "detect.h"

#include "win_internal.h"

// we no longer use TGT, due to issues on Win9x; GTC is just as good.
// still need the header for the event timer (triggers periodic recalibration)
#include <mmsystem.h>
	// not included by win_internal due to its WIN32_LEAN_AND_MEAN define
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif


// ticks per second; average of last few values measured in calibrate()
static double hrt_freq = -1.0;

// used to rebase the hrt tick values to 0
static i64 hrt_origin = 0;

static HRTImpl hrt_impl = HRT_NONE;
static HRTOverride overrides[HRT_NUM_IMPLS];
	// HRTImpl enums as index
	// HACK: no init needed - static data is zeroed (= HRT_DEFAULT)
cassert(HRT_DEFAULT == 0);

static i64 hrt_nominal_freq = -1;


#define lock() win_lock(HRT_CS)
#define unlock() win_unlock(HRT_CS)


// decide upon a HRT implementation, checking if we can work around
// each timer's issues on this platform, but allow user override
// in case there are unforeseen problems with one of them.
// order of preference (due to resolution and speed): TSC, QPC, GTC.
// split out of reset_impl so we can just return when impl is chosen.
static void choose_impl()
{
	bool safe;
#define SAFETY_OVERRIDE(impl)\
	if(overrides[impl] == HRT_DISABLE)\
		safe = false;\
	if(overrides[impl] == HRT_FORCE)\
		safe = true;

#if defined(_M_IX86) && !defined(NO_TSC)
	// CPU Timestamp Counter (incremented every clock)
	// ns resolution, moderate precision (poor clock crystal?)
	//
	// issues:
	// - multiprocessor systems: may be inconsistent across CPUs.
	//   could fix by keeping per-CPU timer state, but we'd need
	//   GetCurrentProcessorNumber (only available on Win Server 2003).
	//   spinning off a thread with set CPU affinity is too slow
	//   (we may have to wait until the next timeslice).
	//   we could discard really bad values, but that's still inaccurate.
	//   => unsafe.
	// - deep sleep modes: TSC may not be advanced.
	//   not a problem though, because if the TSC is disabled, the CPU
	//   isn't doing any other work, either.
	// - SpeedStep/'gearshift' CPUs: frequency may change.
	//   this happens on notebooks now, but eventually desktop systems
	//   will do this as well (if not to save power, for heat reasons).
	//   frequency changes are too often and drastic to correct,
	//   and we don't want to mess with the system power settings.
	//   => unsafe.
	if(cpu_caps & TSC && cpu_freq > 0.0)
	{
		safe = (cpus == 1 && !cpu_speedstep);
		SAFETY_OVERRIDE(HRT_TSC);
		if(safe)
		{
			hrt_impl = HRT_TSC;
			hrt_nominal_freq = (i64)cpu_freq;
			return;
		}
	}
#endif	// TSC

#if defined(_WIN32) && !defined(NO_QPC)
	// Windows QueryPerformanceCounter API
	// implementations:
	// - PIT on Win2k - 838 ns resolution, slow to read (~3 µs)
	// - PMT on WinXP - 279 ns ", moderate overhead (700 ns?)
	//   issues:
	//   1) Q274323: may jump several seconds under heavy PCI bus load.
	//      not a problem, because the older systems on which this occurs
	//      have safe TSCs, so that is used instead.
	//   2) "System clock problem can inflate benchmark scores":
	//      incorrect value if not polled every 4.5 seconds? solved
	//      by calibration thread, which reads timer every second anyway.
	// - TSC on MP HAL - see TSC above.

	// cache freq because QPF is fairly slow.
	static i64 qpc_freq = -1;

	// first call - check if QPC is supported
	if(qpc_freq == -1)
	{
		LARGE_INTEGER i;
		BOOL qpc_ok = QueryPerformanceFrequency(&i);
		qpc_freq = qpc_ok? i.QuadPart : 0;
	}

	// QPC is available
	if(qpc_freq > 0)
	{
		// PIT and PMT are safe.
		if(qpc_freq == 1193182 || qpc_freq == 3579545)
			safe = true;
		// make sure QPC doesn't use the TSC
		// (if it were safe, we would have chosen it above)
		else
		{
			// can't decide yet - assume unsafe
			if(cpu_freq == 0.0)
				safe = false;
			else
			{
				// compare QPC freq to CPU clock freq - can't rule out HPET,
				// because its frequency isn't known (it's at least 10 MHz).
				double freq_dist = fabs(cpu_freq / qpc_freq - 1.0);
				safe = freq_dist > 0.05;
					// safe if freqs not within 5% (i.e. it doesn't use TSC)
			}
		}

		SAFETY_OVERRIDE(HRT_QPC);
		if(safe)
		{
			hrt_impl = HRT_QPC;
			hrt_nominal_freq = qpc_freq;
			return;
		}
	}
#endif	// QPC

	//
	// GTC
	//
	if(1)
	{
		hrt_impl = HRT_GTC;
		hrt_nominal_freq = 1000;
		return;
	}

	// no warning here - doesn't inspire confidence in VC dead code removal.
	debug_warn("hrt_choose_impl: no safe timer found!");
	hrt_impl = HRT_NONE;
	hrt_nominal_freq = -1;
	return;
}


// return ticks since first call. lock must be held.
//
// split to allow calling from reset_impl_lk without recursive locking.
// (not a problem, but avoids a BoundsChecker warning)
static i64 ticks_lk()
{
	i64 t;

	switch(hrt_impl)
	{
// TSC
#if defined(_M_IX86) && !defined(NO_TSC)
	case HRT_TSC:
		t = rdtsc();
		break;
#endif

// QPC
#if defined(_WIN32) && !defined(NO_QPC)
	case HRT_QPC:
		LARGE_INTEGER i;
		QueryPerformanceCounter(&i);
		t = i.QuadPart;
		break;
#endif

// TGT
#ifdef _WIN32
	case HRT_GTC:
		t = (i64)GetTickCount();
		break;
#endif

	// add further timers here.

	default:
		debug_warn("hrt_ticks: invalid impl");
		// fall through

	case HRT_NONE:
		t = 0;
	}	// switch(impl)

	return t - hrt_origin;
}


// this module is dependent upon detect (supplies system information needed to
// choose a HRT), which in turn uses our timer to detect the CPU clock
// when running on Windows (clock(), the only cross platform HRT available on
// Windows, isn't good enough - only 10..15 ms resolution).
//
// we first use a safe timer, and choose again after client code calls
// hrt_override_impl when system information is available.
// the timer will work without this call, but it won't use certain
// implementations. we do it this way, instead of polling every hrt_ticks,
// because a timer implementation change causes hrt_ticks to jump.


// choose a HRT implementation. lock must be held.
//
// don't want to saddle timer module with the problem of initializing
// us on first call - it wouldn't otherwise need to be thread-safe.
static void reset_impl_lk()
{
	HRTImpl old_impl = hrt_impl;
	double old_time = 0.0;

	// if not first time: want to reset tick origin
	if(hrt_nominal_freq > 0)
		old_time = ticks_lk() / hrt_freq;
			// don't call hrt_time to avoid recursive lock.

	choose_impl();
	// post: hrt_impl != HRT_NONE, hrt_nominal_freq > 0

	hrt_freq = (double)hrt_nominal_freq;

	// if impl has changed, re-base tick counter.
	// want it 0-based, but it must not go backwards WRT previous reading.
	if(old_impl != hrt_impl)
		hrt_origin = ticks_lk() - (i64)(old_time * hrt_freq);
}


// multiple entry points, can't use ONCE.
static bool initialized;

static void init_calibration_thread();

// call iff !initialized. lock must be held.
static void init_lk()
{
	assert(!initialized && "init_lk called more than once!");

	reset_impl_lk();
	init_calibration_thread();

	initialized = true;
}


// return ticks since first call.
i64 hrt_ticks()
{
	lock();

	// ugly, but it'll fall-through in common case.
	if(!initialized)
		goto init;

ready:
	{	// VC6 goto fix
	i64 t = ticks_lk();

	unlock();

	return t;
	}

// reached from first call if init_lk hasn't been called yet. lock is held.
init:
	init_lk();
	goto ready;
}


// return seconds since first call.
double hrt_time()
{
	lock();

	// ugly, but it'll fall-through in common case.
	if(!initialized)
		goto init;

ready:
	{	// VC6 goto fix
	double t = ticks_lk() / hrt_freq;

	unlock();

	return t;
	}

// reached from first call if init_lk hasn't been called yet. lock is held.
init:
	init_lk();
	goto ready;
}


// return seconds between start and end timestamps (returned by hrt_ticks).
// negative if end comes before start.
double hrt_delta_s(i64 start, i64 end)
{
	// paranoia: reading double may not be atomic.
	lock();
	double freq = hrt_freq;
	unlock();

	assert(freq != -1.0 && "hrt_delta_s called before hrt_ticks");
	return (end - start) / freq;
}


// return current timer implementation and its nominal (rated) frequency.
// nominal_freq is never 0.
// implementation only changes after hrt_override_impl.
//
// may be called before first hrt_ticks / hrt_time, so do init here also.
void hrt_query_impl(HRTImpl& impl, i64& nominal_freq)
{
lock();

	if(!initialized)
		init_lk();

	impl = hrt_impl;
	nominal_freq = hrt_nominal_freq;

unlock();

	assert(nominal_freq > 0 && "hrt_query_impl: invalid hrt_nominal_freq");
}


// override our 'safe to use' decision.
// resets (and chooses another, if applicable) implementation;
// the timer may jump after doing so.
// call with HRT_DEFAULT, HRT_NONE to re-evaluate implementation choice
// after system info becomes available.
int hrt_override_impl(HRTOverride ovr, HRTImpl impl)
{
	if((ovr != HRT_DISABLE && ovr != HRT_FORCE && ovr != HRT_DEFAULT) ||
	   (impl != HRT_TSC && impl != HRT_QPC && impl != HRT_GTC && impl != HRT_NONE))
	{
		debug_warn("hrt_override: invalid ovr or impl param");
		return -1;
	}

lock();

	overrides[impl] = ovr;
	reset_impl_lk();

unlock();

	return 0;
}




// 'safe' millisecond timer, used to measure HRT freq
static long ms_time()
{
#ifdef _WIN32
	return (long)timeGetTime();
#else
	return (long)clock();
#endif
}


static void calibrate()
{
lock();

	// past couple of calculated hrt freqs, for averaging
	typedef RingBuf<double, 8> SampleBuf;
	static SampleBuf samples;

	const i64 hrt_cur = ticks_lk();
	const long ms_cur = ms_time();

	// get elapsed times since last call
	static long ms_cal_time;
	static i64 hrt_cal_time;
	double hrt_ds = (hrt_cur - hrt_cal_time) / hrt_freq;
	double ms_ds = (ms_cur - ms_cal_time) / 1e3;
	hrt_cal_time = hrt_cur;
	ms_cal_time = ms_cur;

	// we're called from a WinMM event, so the timer has just been updated.
	// no need to determine tick / compensate.

//	double dt = ms_ds;	// actual elapsed time since last calibration
//	double hrt_err = ms_ds - hrt_ds;
//	double hrt_abs_err = fabs(hrt_err);
//	double hrt_rel_err = hrt_abs_err / ms_ds;

	double hrt_est_freq = hrt_ds / ms_ds;
	// only add to buffer if within 10% of nominal
	// (don't want to pollute buffer with flukes / incorrect results)
	if(fabs(hrt_est_freq / hrt_nominal_freq - 1.0) < 0.10)
	{
		samples.push_back(hrt_est_freq);

		// average all samples in buffer
		double freq_sum = std::accumulate(samples.begin(), samples.end(), 0.0);
		hrt_freq = freq_sum / (int)samples.size();
	}
	else
	{
		samples.clear();

		hrt_freq = (double)hrt_nominal_freq;
	}

unlock();
}


#ifdef _WIN32

// setup calibration thread
// note: winmm event is better than a thread or just checking elapsed time
// in hrt_ticks, because it's called right after GTC is updated;
// otherwise, we may be in the middle of a tick.

static UINT mm_event;

// keep calibrate() portable, don't need args anyway
static void CALLBACK trampoline(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	UNUSED(uTimerID);
	UNUSED(uMsg);
	UNUSED(dwUser);
	UNUSED(dw1);
	UNUSED(dw2);

	calibrate();
}

#endif


static void init_calibration_thread()
{
#ifdef _WIN32

	// choosing resolution of winmm timer. don't want to increase the
	// system clock interrupt rate (=> higher system load),
	// so set res to current tick rate.
	DWORD adj, incr;
	BOOL adj_disabled;
	GetSystemTimeAdjustment(&adj, &incr, &adj_disabled);
	DWORD res = adj / 10000;
	mm_event = timeSetEvent(1000, res, trampoline, 0, TIME_PERIODIC);
	atexit2(timeKillEvent, mm_event, CC_STDCALL_1);

#else

	// TODO: port thread. it's no big deal, but the timer should work without.

#endif
}
