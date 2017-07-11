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
 * platform-independent high resolution timer
 */

#include "precompiled.h"
#include "lib/timer.h"

#include <sstream>	// std::stringstream
#include <numeric>
#include <cmath>
#include <cfloat>
#include <cstdarg>

#include "lib/module_init.h"
#include "lib/posix/posix_pthread.h"
#include "lib/posix/posix_time.h"
# include "lib/sysdep/cpu.h"
#if OS_WIN
# include "lib/sysdep/os/win/whrt/whrt.h"
#endif
#if OS_UNIX
# include <unistd.h>
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

#if HAVE_CLOCK_GETTIME
static struct timespec start;
#elif HAVE_GETTIMEOFDAY
static struct timeval start;
#endif


//-----------------------------------------------------------------------------
// timer API

void timer_LatchStartTime()
{
#if OS_WIN
	// whrt_Time starts at zero, nothing needs to be done.
#elif HAVE_CLOCK_GETTIME
	(void)clock_gettime(CLOCK_REALTIME, &start);
#elif HAVE_GETTIMEOFDAY
	gettimeofday(&start, 0);
#endif
}

static pthread_mutex_t ensure_monotonic_mutex = PTHREAD_MUTEX_INITIALIZER;
// NB: does not guarantee strict monotonicity - callers must avoid
// dividing by the difference of two equal times.
static void EnsureMonotonic(double& newTime)
{
	pthread_mutex_lock(&ensure_monotonic_mutex);
	static double maxTime;
	maxTime = std::max(maxTime, newTime);
	newTime = maxTime;
	pthread_mutex_unlock(&ensure_monotonic_mutex);
}


double timer_Time()
{
	double t;

#if OS_WIN
	t = whrt_Time();
#elif HAVE_CLOCK_GETTIME
	ENSURE(start.tv_sec || start.tv_nsec);	// must have called timer_LatchStartTime first
	struct timespec cur;
	(void)clock_gettime(CLOCK_REALTIME, &cur);
	t = (cur.tv_sec - start.tv_sec) + (cur.tv_nsec - start.tv_nsec)*1e-9;
#elif HAVE_GETTIMEOFDAY
	ENSURE(start.tv_sec || start.tv_usec);	// must have called timer_LatchStartTime first
	struct timeval cur;
	gettimeofday(&cur, 0);
	t = (cur.tv_sec - start.tv_sec) + (cur.tv_usec - start.tv_usec)*1e-6;
#else
# error "timer_Time: add timer implementation for this platform!"
#endif

	EnsureMonotonic(t);
	return t;
}


// cached because the default implementation may take several milliseconds
static double resolution;

static Status InitResolution()
{
#if OS_WIN
	resolution = whrt_Resolution();
#elif HAVE_CLOCK_GETTIME
	struct timespec ts;
	if(clock_getres(CLOCK_REALTIME, &ts) == 0)
		resolution = ts.tv_nsec * 1e-9;
#else
	const double t0 = timer_Time();
	double t1, t2;
	do t1 = timer_Time(); while(t1 == t0);
	do t2 = timer_Time(); while(t2 == t1);
	resolution = t2-t1;
#endif

	return INFO::OK;
}

double timer_Resolution()
{
	static ModuleInitState initState;
	ModuleInit(&initState, InitResolution);
	return resolution;
}


//-----------------------------------------------------------------------------
// client API

// intrusive linked-list of all clients. a fixed-size limit would be
// acceptable (since timers are added manually), but the list is easy
// to implement and only has the drawback of exposing TimerClient to users.
//
// do not use std::list et al. for this! we must be callable at any time,
// especially before NLSO ctors run or before heap init.
static size_t numClients;
static TimerClient* clients;


TimerClient* timer_AddClient(TimerClient* tc, const wchar_t* description)
{
	tc->sum.SetToZero();

	tc->description = description;

	// insert at front of list
	tc->next = clients;
	clients = tc;
	numClients++;

	return tc;
}


void timer_DisplayClientTotals()
{
	debug_printf("TIMER TOTALS (%lu clients)\n", (unsigned long)numClients);
	debug_printf("-----------------------------------------------------\n");

	while(clients)
	{
		// (make sure list and count are consistent)
		ENSURE(numClients != 0);
		TimerClient* tc = clients;
		clients = tc->next;
		numClients--;

		const std::string duration = tc->sum.ToString();
		debug_printf("  %s: %s (%lux)\n", utf8_from_wstring(tc->description).c_str(), duration.c_str(), (unsigned long)tc->num_calls);
	}

	debug_printf("-----------------------------------------------------\n");
}


//-----------------------------------------------------------------------------

std::string StringForSeconds(double seconds)
{
	double scale = 1e6;
	const char* unit = " us";
	if(seconds > 1.0)
		scale = 1, unit = " s";
	else if(seconds > 1e-3)
		scale = 1e3, unit = " ms";

	std::stringstream ss;
	ss << seconds*scale;
	ss << unit;
	return ss.str();
}


std::string StringForCycles(Cycles cycles)
{
	double scale = 1.0;
	const char* unit = " c";
	if(cycles > 10000000000LL)	// 10 Gc
		scale = 1e-9, unit = " Gc";
	else if(cycles > 10000000)	// 10 Mc
		scale = 1e-6, unit = " Mc";
	else if(cycles > 10000)	// 10 kc
		scale = 1e-3, unit = " kc";

	std::stringstream ss;
	ss << cycles*scale;
	ss << unit;
	return ss.str();
}
