/**
 * =========================================================================
 * File        : timer.cpp
 * Project     : 0 A.D.
 * Description : platform-independent high resolution timer and
 *             : FPS measuring code.
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
#include "adts.h"
#include "module_init.h"
#include "lib/sysdep/cpu.h"
#if OS_WIN
#include "lib/sysdep/win/whrt/whrt.h"
#endif
#if OS_UNIX
# include <unistd.h>
#endif
#if ARCH_IA32 && CONFIG_TIMER_ALLOW_RDTSC
# include "lib/sysdep/ia32/ia32.h"	// ia32_rdtsc
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

static void LatchStartTime()
{
#if HAVE_CLOCK_GETTIME
	(void)clock_gettime(CLOCK_REALTIME, &start);
#elif HAVE_GETTIMEOFDAY
	gettimeofday(&start, 0);
#endif
}

double get_time()
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
# error "get_time: add timer implementation for this platform!"
#endif

	// make sure time is monotonic (never goes backwards)
	static double t_last = 0.0;
	if(t < t_last)
		t = t_last+DBL_EPSILON;
	t_last = t;

	return t;
}


// return resolution (expressed in [s]) of the time source underlying
// get_time.
double timer_res()
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
	const double t0 = get_time();
	double t1, t2;
	do t1 = get_time();	while(t1 == t0);
	do t2 = get_time();	while(t2 == t1);
	res = t2-t1;
#endif

	cached_res = res;
	return res;
}


//-----------------------------------------------------------------------------

// cumulative timer API, useful for profiling.
// this supplements in-game profiling by providing low-overhead,
// high resolution time accounting.

// intrusive linked-list of all clients. a fixed-size limit would be
// acceptable (since timers are added manually), but the list is easy
// to implement and only has the drawback of exposing TimerClient to users.
//
// do not use std::list et al. for this! we must be callable at any time,
// especially before NLSO ctors run or before heap init.
static uint num_clients;
static TimerClient* clients;


// make the given TimerClient (usually instantiated as static data)
// ready for use. returns its address for TIMER_ADD_CLIENT's convenience.
// this client's total (added to by timer_bill_client) will be
// displayed by timer_display_client_totals.
// notes:
// - may be called at any time;
// - always succeeds (there's no fixed limit);
// - free() is not needed nor possible.
// - description must remain valid until exit; a string literal is safest.
TimerClient* timer_add_client(TimerClient* tc, const char* description)
{
	tc->sum = 0.0;
	tc->description = description;

	// insert at front of list
	tc->next = clients;
	clients = tc;
	num_clients++;

	return tc;
}


// add <dt> to the client's total.
void timer_bill_client(TimerClient* tc, TimerUnit dt)
{
	tc->sum += dt;
	tc->num_calls++;
}


// display all clients' totals; does not reset them.
// typically called at exit.
void timer_display_client_totals()
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

		const double sum = Timer::ToSeconds(tc->sum);

		// determine scale factor for pretty display
		double scale = 1e6;
		const char* unit = "us";
		if(sum > 1.0)
			scale = 1, unit = "s";
		else if(sum > 1e-3)
			scale = 1e3, unit = "ms";

		debug_printf("  %s: %g %s (%dx)\n", tc->description, sum*scale, unit, tc->num_calls);
	}

	debug_printf("-----------------------------------------------------\n");
}


#if ARCH_IA32 && CONFIG_TIMER_ALLOW_RDTSC

TimerRdtsc::unit TimerRdtsc::get_timestamp() const
{
	return ia32_rdtsc();
}

#endif


//-----------------------------------------------------------------------------

static ModuleInitState initState;

void timer_Init()
{
	if(!ModuleShouldInitialize(&initState))
		return;

	LatchStartTime();
}

void timer_Shutdown()
{
	if(!ModuleShouldShutdown(&initState))
		return;

	// nothing to do
}
