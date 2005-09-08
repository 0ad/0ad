// Windows-specific high resolution timer
// Copyright (c) 2004 Jan Wassenberg
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

#include "lib.h"
#include "posix.h"
#include "adts.h"
#include "sysdep/ia32.h"
#include "detect.h"

#include "win_internal.h"

#include <math.h>
#include <process.h>	// _beginthreadex
#include <time.h>

#include <algorithm>
#include <numeric>


// define to disable time sources (useful for simulating other systems)
//#define NO_QPC
//#define NO_TSC

static const int CALIBRATION_FREQ = 1;


// automatic module init (before main) and shutdown (before termination)
#pragma data_seg(WIN_CALLBACK_PRE_LIBC(b))
WIN_REGISTER_FUNC(wtime_init);
#pragma data_seg(WIN_CALLBACK_POST_ATEXIT(b))
WIN_REGISTER_FUNC(wtime_shutdown);
#pragma data_seg()


// see http://www.gamedev.net/reference/programming/features/timing/ .

// rationale:
// we no longer use TGT, due to issues on Win9x; GTC is just as good.
// (don't want to accelerate the tick rate, because performance will suffer).
// avoid dependency on WinMM (event timer) to shorten startup time.
//
// we go to the trouble of allowing switching time sources at runtime
// (=> have to be careful to keep the timer continuous) because we want
// to allow overriding the implementation choice via command line switch,
// in case a time source turns out to have a serious problem.


// (default values for HRT_NONE impl)

// initial measurement of the time source's tick rate. not necessarily
// correct (e.g. when using TSC; cpu_freq isn't exact).
static double hrt_nominal_freq = -1.0;

// actual resolution of the time source (may differ from hrt_nominal_freq
// for timers with adjustment > 1 tick).
static double hrt_res = -1.0;

// current ticks per second; average of last few values measured in
// calibrate(). needed to prevent long-term drift, and because
// hrt_nominal_freq isn't necessarily correct. only affects the ticks since
// last calibration - don't want to retroactively change the time.
static double hrt_cur_freq = -1.0;

// ticks at init or last calibration.
// ticks since then are scaled by 1/hrt_cur_freq and added to hrt_cal_time
// to yield the current time.
static i64 hrt_cal_ticks = 0;

// value of hrt_time() at last calibration. needed so that changes to
// hrt_cur_freq don't affect the previous ticks (example: 72 ticks elapsed,
// nominal freq = 8 => time = 9.0. if freq is calculated as 9, time would
// go backwards to 8.0).
static double hrt_cal_time = 0.0;


// possible high resolution timers, in order of preference.
// see below for timer properties + problems.
// used as index into overrides[].
enum HRTImpl
{
	// CPU timestamp counter
	HRT_TSC,

	// Windows QueryPerformanceCounter
	HRT_QPC,

	// Windows GetTickCount
	HRT_GTC,

	// there will always be a valid timer in use.
	// this is only used with hrt_override_impl.
	HRT_NONE,

	HRT_NUM_IMPLS
};

static HRTImpl hrt_impl = HRT_NONE;

// while we do our best to work around timer problems or avoid them if unsafe,
// future requirements and problems may be different:
// allow the user or app to override our decisions (via hrt_override_impl)
enum HRTOverride
{
	// allow use of this implementation if available,
	// and we can work around its problems
	//
	// HACK: give it value 0 for easier static data initialization
	HRT_DEFAULT = 0,

	// override our 'safe to use' recommendation
	// set by hrt_override_impl (via command line arg or console function)
	HRT_DISABLE,
	HRT_FORCE
};

static HRTOverride overrides[HRT_NUM_IMPLS];
	// HRTImpl enums as index
	// HACK: no init needed - static data is zeroed (= HRT_DEFAULT)
cassert((int)HRT_DEFAULT == 0);


// convenience
static const long _1e6 = 1000000;
static const long _1e7 = 10000000;
static const i64  _1e9 = 1000000000;


static inline void lock(void)
{
	win_lock(WTIME_CS);
}
static inline void unlock(void)
{
	win_unlock(WTIME_CS);
}


// decide upon a HRT implementation, checking if we can work around
// each timer's issues on this platform, but allow user override
// in case there are unforeseen problems with one of them.
// order of preference (due to resolution and speed): TSC, QPC, GTC.
// split out of reset_impl so we can just return when impl is chosen.
static int choose_impl()
{
	bool safe;
#define SAFETY_OVERRIDE(impl)\
	if(overrides[impl] == HRT_DISABLE)\
		safe = false;\
	if(overrides[impl] == HRT_FORCE)\
		safe = true;

#if CPU_IA32 && !defined(NO_TSC)
	// CPU Timestamp Counter (incremented every clock)
	// ns resolution, moderate precision (poor clock crystal?)
	//
	// issues:
	// - multiprocessor systems: may be inconsistent across CPUs.
	//   we could discard really bad values, but that's still inaccurate.
	//   having a high-priority thread with set CPU affinity read the TSC
	//   might work, but would be rather slow. could fix the problem by
	//   keeping per-CPU timer state (freq and delta). we'd use the APIC ID
	//   (cpuid, function 1) or GetCurrentProcessorNumber (only available
	//   on Win Server 2003) to determine the CPU. however, this is
	//   too much work for little benefit ATM, so call it unsafe.
	// - deep sleep modes: TSC may not be advanced.
	//   not a problem though, because if the TSC is disabled, the CPU
	//   isn't doing any other work, either.
	// - SpeedStep/'gearshift' CPUs: frequency may change.
	//   this happens on notebooks now, but eventually desktop systems
	//   will do this as well (if not to save power, for heat reasons).
	//   frequency changes are too often and drastic to correct,
	//   and we don't want to mess with the system power settings => unsafe.
	if(cpu_freq > 0.0 && ia32_cap(TSC))
	{
		safe = (cpu_smp == 0 && cpu_speedstep == 0);
		SAFETY_OVERRIDE(HRT_TSC);
		if(safe)
		{
			hrt_impl = HRT_TSC;
			hrt_nominal_freq = cpu_freq;
			hrt_res = (1.0 / hrt_nominal_freq);
			return 0;
		}
	}
#endif	// TSC

#if OS_WIN && !defined(NO_QPC)
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
			hrt_nominal_freq = (double)qpc_freq;
			hrt_res = (1.0 / hrt_nominal_freq);
			return 0;
		}
	}
#endif	// QPC

	//
	// GTC
	//
	safe = true;
	SAFETY_OVERRIDE(HRT_GTC);
	if(safe)
	{
		hrt_impl = HRT_GTC;
		hrt_nominal_freq = 1000.0;	// units returned
		hrt_res = 1e-2;	// guess, in case the following fails

		// get actual resolution
		DWORD adj; BOOL adj_disabled; // unused, but must be passed to GSTA
		DWORD timer_period;	// [hectonanoseconds]
		if(GetSystemTimeAdjustment(&adj, &timer_period, &adj_disabled))
			hrt_res = (timer_period / 1e7);
		return 0;
	}

	debug_warn("hrt_choose_impl: no safe timer found!");
	hrt_impl = HRT_NONE;
	hrt_nominal_freq = -1.0;
	return -1;
}


// return ticks (unspecified start point). lock must be held.
//
// split to allow calling from reset_impl_lk without recursive locking.
// (not a problem, but avoids a BoundsChecker warning)
static i64 ticks_lk()
{
	switch(hrt_impl)
	{
	// TSC
#if CPU_IA32 && !defined(NO_TSC)
	case HRT_TSC:
		return (i64)rdtsc();
#endif

	// QPC
#if OS_WIN && !defined(NO_QPC)
	case HRT_QPC:
		{
		LARGE_INTEGER i;
		BOOL ok = QueryPerformanceCounter(&i);
		debug_assert(ok);	// shouldn't fail if it was chosen above
		return i.QuadPart;
		}
#endif

	// TGT
#if OS_WIN
	case HRT_GTC:
		return (i64)GetTickCount();
#endif

	// add further timers here.

	case HRT_NUM_IMPLS:
	default:
		debug_warn("ticks_lk: invalid impl");
		//-fallthrough

	case HRT_NONE:
		return 0;
	}	// switch(impl)
}


// return seconds since init. lock must be held.
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




// this module is dependent upon detect (supplies system information needed to
// choose a HRT), which in turn uses our timer to detect the CPU clock
// when running on Windows (clock(), the only cross platform HRT available on
// Windows, isn't good enough - only 10..15 ms resolution).
//
// we first use a safe timer, and choose again after client code calls
// hrt_override_impl when system information is available.
// the timer will work without this call, but it won't use certain
// implementations. we do it this way, instead of polling on each timer use,
// because a timer implementation change may cause the timer to jump a bit.


// choose a HRT implementation and prepare it for use. lock must be held.
//
// don't want to saddle timer module with the problem of initializing
// us on first call - it wouldn't otherwise need to be thread-safe.
static int reset_impl_lk()
{
	HRTImpl old_impl = hrt_impl;

	// if changing implementation: get time at which to continue
	// (when switching, we set everything calibrate() would output)
	double old_time;
	// .. first call; hrt_cur_freq not initialized; can't call time_lk.
	//    setting to 0 will start the timer at 0.
	if(hrt_cur_freq <= 0.0)
		old_time = 0.0;
	// .. timer has been initialized; use current reported time.
	else
		old_time = time_lk();		

	CHECK_ERR(choose_impl());
	debug_assert(hrt_impl != HRT_NONE && hrt_nominal_freq > 0.0);

	// impl has changed; reset timer state.
	if(old_impl != hrt_impl)
	{
		hrt_cur_freq = hrt_nominal_freq;
		hrt_cal_time = old_time;
		hrt_cal_ticks = ticks_lk();
	}

	return 0;
}


// return ticks (unspecified start point)
static i64 hrt_ticks()
{
	i64 t;
lock();
	t = ticks_lk();
unlock();
	return t;
}


// return seconds since init.
static double hrt_time()
{
	double t;
lock();
	t = time_lk();
unlock();
	return t;
}


// return seconds between start and end timestamps (returned by hrt_ticks).
// negative if end comes before start. not intended to be called for long
// intervals (start -> end), since the current frequency is used!
static double hrt_delta_s(i64 start, i64 end)
{
	// paranoia: reading double may not be atomic.
lock();
	double freq = hrt_cur_freq;
unlock();

	debug_assert(freq != -1.0 && "hrt_delta_s: hrt_cur_freq not set");
	return (end - start) / freq;
}


// return current timer implementation and its nominal (rated) frequency.
// nominal_freq is never 0.
// implementation only changes after hrt_override_impl.
//
// may be called before first hrt_ticks / hrt_time, so do init here also.
static void hrt_query_impl(HRTImpl& impl, double& nominal_freq, double& res)
{
lock();

	impl = hrt_impl;
	nominal_freq = hrt_nominal_freq;
	res = hrt_res;

unlock();

	debug_assert(nominal_freq > 0.0 && "hrt_query_impl: invalid hrt_nominal_freq");
}


// override our 'safe to use' decision.
// resets (and chooses another, if applicable) implementation;
// the timer may jump after doing so.
// call with HRT_DEFAULT, HRT_NONE to re-evaluate implementation choice
// after system info becomes available.
static int hrt_override_impl(HRTOverride ovr, HRTImpl impl)
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


//////////////////////////////////////////////////////////////////////////////
//
// calibration
//
//////////////////////////////////////////////////////////////////////////////


// 'safe' timer, used to measure HRT freq in calibrate()
static const long safe_timer_freq = 1000;

static long safe_time()
{
#if OS_WIN
	return (long)GetTickCount();
#else
	return (long)(clock() * 1000.0 / CLOCKS_PER_SEC);
#endif
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

	if(fabs(hrt_est_freq / hrt_nominal_freq - 1.0) < 0.10)
		// only add to buffer if within 10% of nominal
		// (don't want to pollute buffer with flukes / incorrect results)
	{
		samples.push_back(hrt_est_freq);

		// average all samples in buffer
		double freq_sum = std::accumulate(samples.begin(), samples.end(), 0.0);
		const int num = (int)samples.size();	// divide-by-0 paranoia
		hrt_cur_freq = (num == 0)? 0.0 : freq_sum / num;
	}
	else
	{
		samples.clear();

		hrt_cur_freq = hrt_nominal_freq;
	}

	debug_assert(hrt_cur_freq > 0.0);
}


// calibration thread
// note: winmm event is better than a thread or just checking elapsed time
// in hrt_ticks, because it's called right after GTC is updated;
// otherwise, we may be in the middle of a tick.
// however, we want to avoid dependency on WinMM to shorten startup time.
// hence, start a thread.

static pthread_t thread;
static sem_t exit_flag;

static void* calibration_thread(void* UNUSED(data))
{
	debug_set_thread_name("wtime");

	for(;;)
	{
		// calculate absolute timeout for sem_timedwait
		struct timespec abs_timeout;
		clock_gettime(CLOCK_REALTIME, &abs_timeout);
		abs_timeout.tv_nsec += _1e9 / CALIBRATION_FREQ;
		// .. handle nanosecond wraparound (must not be > 1000m)
		if(abs_timeout.tv_nsec >= _1e9)
		{
			abs_timeout.tv_nsec -= _1e9;
			abs_timeout.tv_sec++;
		}

		errno = 0;
		// if we acquire the semaphore, exit was requested.
		if(sem_timedwait(&exit_flag, &abs_timeout) == 0)
			break;
		// actual error: warn
		if(errno != ETIMEDOUT)
			debug_warn("wtime calibration_thread: sem_timedwait failed");

		lock();
		calibrate_lk();
		unlock();
	}

	return 0;
}


static inline int init_calibration_thread()
{
	sem_init(&exit_flag, 0, 0);
	pthread_create(&thread, 0, calibration_thread, 0);
	return 0;
}


static inline int shutdown_calibration_thread()
{
	sem_post(&exit_flag);
	pthread_join(thread, 0);
	sem_destroy(&exit_flag);
	return 0;
}




static int hrt_init()
{
	// no lock needed - calibration thread hasn't yet been created
	reset_impl_lk();
	return init_calibration_thread();
}


static int hrt_shutdown()
{
	// don't take a lock here! race condition:
	// 1) calibration_thread is about to call clock_gettime
	// 2) we take the lock and wait for the thread to exit
	// 3) thread's clock_gettime waits on the lock we're holding => deadlock
	//
	// the calibration thread protects itself anyway, so nothing breaks.
	return shutdown_calibration_thread();
}


//////////////////////////////////////////////////////////////////////////////
//
// wtime wrapper: emulates POSIX functions
//
//////////////////////////////////////////////////////////////////////////////

// NT system time and FILETIME are hectonanoseconds since Jan. 1, 1601 UTC.
// SYSTEMTIME is a struct containing month, year, etc.


//
// FILETIME -> time_t routines; used by wposix filetime_to_time_t wrapper.
//

// hectonanoseconds between Windows and POSIX epoch
static const u64 posix_epoch_hns = 0x019DB1DED53E8000;

// this function avoids the pitfall of casting FILETIME* to u64*,
// which is not safe due to differing alignment guarantees!
// on some platforms, that would result in an exception.
static u64 u64_from_FILETIME(const FILETIME* ft)
{
	return u64_from_u32(ft->dwHighDateTime, ft->dwLowDateTime);
}


// convert UTC FILETIME to seconds-since-1970 UTC:
// we just have to subtract POSIX epoch and scale down to units of seconds.
//
// note: RtlTimeToSecondsSince1970 isn't officially documented,
// so don't use that.
time_t utc_filetime_to_time_t(FILETIME* ft)
{
	u64 hns = u64_from_FILETIME(ft);
	u64 s = (hns - posix_epoch_hns) / _1e7;
	return (time_t)(s & 0xffffffff);
}


// convert local FILETIME (includes timezone bias and possibly DST bias)
// to seconds-since-1970 UTC.
//
// note: splitting into month, year etc. is inefficient,
//   but much easier than determining whether ft lies in DST,
//   and ourselves adding the appropriate bias.
//
// called for FAT file times; see wposix filetime_to_time_t.
time_t time_t_from_local_filetime(FILETIME* ft)
{
	SYSTEMTIME st;
	FileTimeToSystemTime(ft, &st);

	struct tm t;
	t.tm_sec   = st.wSecond;
	t.tm_min   = st.wMinute;
	t.tm_hour  = st.wHour;
	t.tm_mday  = st.wDay;
	t.tm_mon   = st.wMonth-1;
	t.tm_year  = st.wYear-1900;
	t.tm_isdst = -1;
		// let the CRT determine whether this local time
		// falls under DST by the US rules.
    return mktime(&t);
}




// return nanoseconds since posix epoch as reported by system time
// only 10 or 15 ms resolution!
static i64 st_time_ns()
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	u64 hns = u64_from_FILETIME(&ft);
	return (hns - posix_epoch_hns) * 100;
}


// return nanoseconds since posix epoch as reported by HRT.
// we get system time at init and add HRT elapsed time.
static i64 time_ns()
{
	// we don't really need to get the HRT start time (it starts at 0,
	// and will be slightly higher when we get here; doesn't matter if the
	// time returned is a few ms off the real system time). do so anyway,
	// because we have to get the starting ST value anyway.
	static double hrt_start_time;
	static i64 st_start;

	if(!st_start)
	{
		hrt_start_time = hrt_time();
		st_start = st_time_ns();
	}

	const double dt = hrt_time() - hrt_start_time;
	const i64 ns = st_start + (i64)(dt * _1e9);
	return ns;
}


static int wtime_init()
{
	hrt_init();

	// first call latches start times
	time_ns();

	return 0;
}


static int wtime_shutdown()
{
	return hrt_shutdown();
}


void wtime_reset_impl()
{
	hrt_override_impl(HRT_DEFAULT, HRT_NONE);
}




static void sleep_ns(i64 ns)
{
	DWORD ms = DWORD(ns / _1e6);
	if(ms != 0)
		Sleep(ms);
	else
	{
		i64 t0 = hrt_ticks(), t1;
		do
			t1 = hrt_ticks();
		while(hrt_delta_s(t0, t1) * _1e9 < ns);
	}
}


int clock_gettime(clockid_t clock, struct timespec* t)
{
	debug_assert(clock == CLOCK_REALTIME);

	const i64 ns = time_ns();
	t->tv_sec  = (time_t)((ns / _1e9) & 0xffffffff);
	t->tv_nsec = (long)  (ns % _1e9);
	return 0;
}


int clock_getres(clockid_t clock, struct timespec* ts)
{
	debug_assert(clock == CLOCK_REALTIME);

	HRTImpl impl;
	double nominal_freq, res;
	hrt_query_impl(impl, nominal_freq, res);

	ts->tv_sec  = 0;
	ts->tv_nsec = (long)(res * 1e9);
	return 0;
}


int nanosleep(const struct timespec* rqtp, struct timespec* /* rmtp */)
{
	i64 ns = rqtp->tv_sec;	// make sure we don't overflow
	ns *= _1e9;
	ns += rqtp->tv_nsec;
	sleep_ns(ns);
	return 0;
}


int gettimeofday(struct timeval* tv, void* UNUSED(tzp))
{
	const long us = (long)(time_ns() / 1000);
	tv->tv_sec  = (time_t)     (us / _1e6);
	tv->tv_usec = (suseconds_t)(us % _1e6);
	return 0;
}


uint sleep(uint sec)
{
	Sleep(sec * 1000);	// don't bother checking for overflow (user's fault)
	return sec;
}


int usleep(useconds_t us)
{
	debug_assert(us < _1e6);
	sleep_ns(us * 1000);	// can't overflow due to <us> limit
	return 0;
}
