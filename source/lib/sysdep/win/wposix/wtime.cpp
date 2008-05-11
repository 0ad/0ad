/**
 * =========================================================================
 * File        : wtime.cpp
 * Project     : 0 A.D.
 * Description : emulate POSIX time functionality on Windows.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

// note: clock_gettime et al. have been removed. callers should use the
// WHRT directly, rather than needlessly translating s -> ns -> s,
// which costs time and accuracy.

#include "precompiled.h"
#include "wtime.h"

#include "wposix_internal.h"
#include "lib/sysdep/cpu.h"				// cpu_i64FromDouble
#include "lib/sysdep/win/whrt/whrt.h"

WINIT_REGISTER_MAIN_INIT(wtime_Init);	// whrt -> wtime

// NT system time and FILETIME are hectonanoseconds since Jan. 1, 1601 UTC.
// SYSTEMTIME is a struct containing month, year, etc.

static const long _1e3 = 1000;
static const long _1e6 = 1000000;
static const long _1e7 = 10000000;
static const i64  _1e9 = 1000000000;


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
// used by wfilesystem.
//
// note: RtlTimeToSecondsSince1970 isn't officially documented,
// so don't use that.
time_t wtime_utc_filetime_to_time_t(FILETIME* ft)
{
	u64 hns = u64_from_FILETIME(ft);
	u64 s = (hns - posix_epoch_hns) / _1e7;
	return (time_t)(s & 0xFFFFFFFF);
}


//-----------------------------------------------------------------------------

// system clock at startup [nanoseconds since POSIX epoch]
// note: the HRT starts at 0; any increase by the time we get here
// just makes our notion of the start time more accurate)
static i64 stInitial_ns;

static void LatchInitialSystemTime()
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	const u64 hns = u64_from_FILETIME(&ft);
	stInitial_ns = (hns - posix_epoch_hns) * 100;
}

// return nanoseconds since POSIX epoch.
// algorithm: add current HRT value to the startup system time
static i64 CurrentSystemTime_ns()
{
	const i64 ns = stInitial_ns + cpu_i64FromDouble(whrt_Time() * _1e9);
	return ns;
}

static timespec TimespecFromNs(i64 ns)
{
	timespec ts;
	ts.tv_sec = (time_t)((ns / _1e9) & 0xFFFFFFFF);
	ts.tv_nsec = (long)(ns % _1e9);
	return ts;
}

static size_t MsFromTimespec(const timespec& ts)
{
	i64 ms = ts.tv_sec;	// avoid overflow
	ms *= _1e3;
	ms += ts.tv_nsec / _1e6;
	return ms;
}


//-----------------------------------------------------------------------------

int clock_gettime(clockid_t clock, struct timespec* ts)
{
	debug_assert(clock == CLOCK_REALTIME || clock == CLOCK_MONOTONIC);

	const i64 ns = CurrentSystemTime_ns();
	*ts = TimespecFromNs(ns);
	return 0;
}


int clock_getres(clockid_t clock, struct timespec* ts)
{
	debug_assert(clock == CLOCK_REALTIME || clock == CLOCK_MONOTONIC);

	const i64 ns = cpu_i64FromDouble(whrt_Resolution() * 1e9);
	*ts = TimespecFromNs(ns);
	return 0;
}


int nanosleep(const struct timespec* rqtp, struct timespec* /* rmtp */)
{
	const DWORD ms = (DWORD)MsFromTimespec(*rqtp);
	if(ms)
		Sleep(ms);
	return 0;
}


unsigned sleep(unsigned sec)
{
	// warn if overflow would result (it would be insane to ask for
	// such lengthy sleep timeouts, but still)
	debug_assert(sec < std::numeric_limits<size_t>::max()/1000);

	const DWORD ms = sec * 1000;
	if(ms)
		Sleep(ms);
	return sec;
}


int usleep(useconds_t us)
{
	debug_assert(us < _1e6);

	const DWORD ms = us/1000;
	if(ms)
		Sleep(ms);
	return 0;
}


//-----------------------------------------------------------------------------

static LibError wtime_Init()
{
	LatchInitialSystemTime();
	return INFO::OK;
}
