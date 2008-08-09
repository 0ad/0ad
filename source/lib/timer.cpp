/**
 * =========================================================================
 * File        : timer.cpp
 * Project     : 0 A.D.
 * Description : platform-independent high resolution timer
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "timer.h"

#include <numeric>
#include <math.h>
#include <float.h>
#include <stdarg.h>

#include "lib/posix/posix_time.h"
#include "lib/sysdep/os_cpu.h"
#if OS_WIN
#include "lib/sysdep/os/win/whrt/whrt.h"
#endif
#if OS_UNIX
# include <unistd.h>
#endif
#include "lib/config2.h"	// CONFIG2_TIMER_ALLOW_RDTSC
#if (ARCH_IA32 || ARCH_AMD64) && CONFIG2_TIMER_ALLOW_RDTSC
# include "lib/sysdep/arch/x86_x64/x86_x64.h"	// x86_x64_rdtsc
#endif

#if OS_UNIX || OS_WIN
# define HAVE_GETTIMEOFDAY 1
#else
# define HAVE_GETTIMEOFDAY 0
#endif

#if (defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0) || OS_WIN
# define HAVE_CLOCK_GETTIME 1
#else
# define HAVE_CLOCK_GETTIME 0
#endif

// rationale for wrapping gettimeofday and clock_gettime, instead of just
// emulating them where not available: allows returning higher-resolution
// timer values than their us / ns interface, via double [seconds].
// they're also not guaranteed to be monotonic.

#if HAVE_GETTIMEOFDAY
static struct timespec start;
#elif HAVE_CLOCK_GETTIME
static struct timeval start;
#endif


//-----------------------------------------------------------------------------
// timer API

void timer_LatchStartTime()
{
#if HAVE_CLOCK_GETTIME
	(void)clock_gettime(CLOCK_REALTIME, &start);
#elif HAVE_GETTIMEOFDAY
	gettimeofday(&start, 0);
#endif
}


double timer_Time()
{
	double t;

#if OS_WIN
	t = whrt_Time();
#elif HAVE_CLOCK_GETTIME
	struct timespec cur;
	(void)clock_gettime(CLOCK_REALTIME, &cur);
	t = (cur.tv_sec - start.tv_sec) + (cur.tv_nsec - start.tv_nsec)*1e-9;
#elif HAVE_GETTIMEOFDAY
	struct timeval cur;
	gettimeofday(&cur, 0);
	t = (cur.tv_sec - start.tv_sec) + (cur.tv_usec - start.tv_usec)*1e-6;
#else
# error "timer_Time: add timer implementation for this platform!"
#endif

	// make sure time is monotonic (never goes backwards)
	static double t_last = 0.0;
	if(t < t_last)
		t = t_last+DBL_EPSILON;
	t_last = t;

	return t;
}


double timer_Resolution()
{
	// may take a while to determine, so cache it
	static double cached_res = 0.0;
	if(cached_res != 0.0)
		return cached_res;

	double res = 0.0;

#if HAVE_CLOCK_GETTIME
	struct timespec ts;
	if(clock_getres(CLOCK_REALTIME, &ts) == 0)
		res = ts.tv_nsec * 1e-9;
#elif OS_WIN
	res = whrt_Resolution();
#else
	const double t0 = timer_Time();
	double t1, t2;
	do t1 = timer_Time();	while(t1 == t0);
	do t2 = timer_Time();	while(t2 == t1);
	res = t2-t1;
#endif

	cached_res = res;
	return res;
}


//-----------------------------------------------------------------------------

ScopeTimer::ScopeTimer(const char* description)
	: m_t0(timer_Time()), m_description(description)
{
}


ScopeTimer::~ScopeTimer()
{
	double t1 = timer_Time();
	double dt = t1-m_t0;

	// determine scale factor for pretty display
	double scale = 1e6;
	const char* unit = "us";
	if(dt > 1.0)
		scale = 1, unit = "s";
	else if(dt > 1e-3)
		scale = 1e3, unit = "ms";

	debug_printf("TIMER| %s: %g %s\n", m_description, dt*scale, unit);
}


//-----------------------------------------------------------------------------
// TimerUnit

// since TIMER_ACCRUE et al. are called so often, we try to keep
// overhead to an absolute minimum. storing raw tick counts (e.g. CPU cycles
// returned by ia32_rdtsc) instead of absolute time has two benefits:
// - no need to convert from raw->time on every call
//   (instead, it's only done once when displaying the totals)
// - possibly less overhead to querying the time itself
//   (timer_Time may be using slower time sources with ~3us overhead)
//
// however, the cycle count is not necessarily a measure of wall-clock time
// (see http://www.gamedev.net/reference/programming/features/timing).
// therefore, on systems with SpeedStep active, measurements of I/O or other
// non-CPU bound activity may be skewed. this is ok because the timer is
// only used for profiling; just be aware of the issue.
// if this is a problem, disable CONFIG2_TIMER_ALLOW_RDTSC.
// 
// note that overflow isn't an issue either way (63 bit cycle counts
// at 10 GHz cover intervals of 29 years).

#if ARCH_IA32 && CONFIG2_TIMER_ALLOW_RDTSC

void TimerUnit::SetToZero()
{
	m_ticks = 0;
}

void TimerUnit::SetFromTimer()
{
	m_ticks = x86_x64_rdtsc();
}

void TimerUnit::AddDifference(TimerUnit t0, TimerUnit t1)
{
	m_ticks += t1.m_ticks - t0.m_ticks;
}

void TimerUnit::Subtract(TimerUnit t)
{
	m_ticks -= t.m_ticks;
}

std::string TimerUnit::ToString() const
{
	debug_assert(m_ticks >= 0.0);

	// determine scale factor for pretty display
	double scale = 1.0;
	const char* unit = " c";
	if(m_ticks > 10000000000LL)	// 10 Gc
		scale = 1e-9, unit = " Gc";
	else if(m_ticks > 10000000)	// 10 Mc
		scale = 1e-6, unit = " Mc";
	else if(m_ticks > 10000)	// 10 kc
		scale = 1e-3, unit = " kc";

	std::stringstream ss;
	ss << m_ticks*scale;
	ss << unit;
	return ss.str();
}

double TimerUnit::ToSeconds() const
{
	return m_ticks / os_cpu_ClockFrequency();
}

#else

void TimerUnit::SetToZero()
{
	m_seconds = 0.0;
}

void TimerUnit::SetFromTimer()
{
	m_seconds = timer_Time();
}

void TimerUnit::AddDifference(TimerUnit t0, TimerUnit t1)
{
	m_seconds += t1.m_seconds - t0.m_seconds;
}

void TimerUnit::Subtract(TimerUnit t)
{
	m_seconds -= t.m_seconds;
}

std::string TimerUnit::ToString() const
{
	debug_assert(m_seconds >= 0.0);

	// determine scale factor for pretty display
	double scale = 1e6;
	const char* unit = " us";
	if(m_seconds > 1.0)
		scale = 1, unit = " s";
	else if(m_seconds > 1e-3)
		scale = 1e3, unit = " ms";

	std::stringstream ss;
	ss << m_seconds*scale;
	ss << unit;
	return ss.str();
}

double TimerUnit::ToSeconds() const
{
	return m_seconds;
}

#endif


//-----------------------------------------------------------------------------
// client API

// intrusive linked-list of all clients. a fixed-size limit would be
// acceptable (since timers are added manually), but the list is easy
// to implement and only has the drawback of exposing TimerClient to users.
//
// do not use std::list et al. for this! we must be callable at any time,
// especially before NLSO ctors run or before heap init.
static size_t num_clients;
static TimerClient* clients;


TimerClient* timer_AddClient(TimerClient* tc, const char* description)
{
	tc->sum.SetToZero();

	tc->description = description;

	// insert at front of list
	tc->next = clients;
	clients = tc;
	num_clients++;

	return tc;
}


void timer_BillClient(TimerClient* tc, TimerUnit t0, TimerUnit t1)
{
	tc->sum.AddDifference(t0, t1);
	tc->num_calls++;
}


void timer_DisplayClientTotals()
{
	debug_printf("TIMER TOTALS (%d clients)\n", num_clients);
	debug_printf("-----------------------------------------------------\n");

	while(clients)
	{
		// (make sure list and count are consistent)
		debug_assert(num_clients != 0);
		TimerClient* tc = clients;
		clients = tc->next;
		num_clients--;

		const std::string duration = tc->sum.ToString();
		debug_printf("  %s: %s (%dx)\n", tc->description, duration.c_str(), tc->num_calls);
	}

	debug_printf("-----------------------------------------------------\n");
}


ScopeTimerAccrue::ScopeTimerAccrue(TimerClient* tc)
	: m_tc(tc)
{
	m_t0.SetFromTimer();
}


ScopeTimerAccrue::~ScopeTimerAccrue()
{
	TimerUnit t1;
	t1.SetFromTimer();
	timer_BillClient(m_tc, m_t0, t1);
}
