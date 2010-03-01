/* Copyright (c) 2010 Wildfire Games
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

#include <numeric>
#include <math.h>
#include <float.h>
#include <stdarg.h>

#include "lib/posix/posix_time.h"
#if OS_WIN
#include "lib/sysdep/os/win/whrt/whrt.h"
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

#if OS_WIN
	res = whrt_Resolution();
#elif HAVE_CLOCK_GETTIME
	struct timespec ts;
	if(clock_getres(CLOCK_REALTIME, &ts) == 0)
		res = ts.tv_nsec * 1e-9;
#else
	const double t0 = timer_Time();
	double t1, t2;
	do t1 = timer_Time(); while(t1 == t0);
	do t2 = timer_Time(); while(t2 == t1);
	res = t2-t1;
#endif

	cached_res = res;
	return res;
}


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


TimerClient* timer_AddClient(TimerClient* tc, const wchar_t* description)
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
	debug_printf(L"TIMER TOTALS (%lu clients)\n", (unsigned long)num_clients);
	debug_printf(L"-----------------------------------------------------\n");

	while(clients)
	{
		// (make sure list and count are consistent)
		debug_assert(num_clients != 0);
		TimerClient* tc = clients;
		clients = tc->next;
		num_clients--;

		const std::wstring duration = tc->sum.ToString();
		debug_printf(L"  %ls: %ls (%lux)\n", tc->description, duration.c_str(), (unsigned long)tc->num_calls);
	}

	debug_printf(L"-----------------------------------------------------\n");
}
