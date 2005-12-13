// platform-independent high resolution timer and FPS measuring code
//
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

#include "lib.h"
#include "posix.h"
#include "timer.h"
#include "adts.h"

#include <numeric>
#include <math.h>
#include <stdarg.h>

// rationale for wrapping gettimeofday and clock_gettime, instead of emulating
// them where not available: allows returning higher-resolution timer values
// than their us / ns interface, via double [seconds]. they're also not
// guaranteed to be monotonic.

double get_time()
{
	double t;

#if HAVE_CLOCK_GETTIME
	static struct timespec start = {0};
	struct timespec ts;

	if(!start.tv_sec)
		(void)clock_gettime(CLOCK_REALTIME, &start);

	(void)clock_gettime(CLOCK_REALTIME, &ts);
	t = (ts.tv_sec - start.tv_sec) + (ts.tv_nsec - start.tv_nsec)*1e-9;
#elif HAVE_GETTIMEOFDAY
	static struct timeval start;
	struct timeval cur;

	if(!start.tv_sec)
		gettimeofday(&start, 0);

	gettimeofday(&cur, 0);
	t = (cur.tv_sec - start.tv_sec) + (cur.tv_usec - start.tv_usec)*1e-6;
#else
#error "get_time: add timer implementation for this platform!"
#endif

	// make sure time is monotonic (never goes backwards)
	static double t_last = 0.0;
	if(t < t_last)
		t = t_last;
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


// calculate fps (call once per frame).
// algorithm: variable-gain IIR filter.
// less fluctuation, but rapid tracking.
// filter values are tuned for 100 FPS.

int fps;
float spf;

void calc_fps()
{
	static double avg_fps = 60.0;
	double cur_fps = avg_fps;

	// get elapsed time [s] since last update
	static double last_t;
	const double t = get_time();
	ONCE(last_t = t - 1.0/60.0);	// first call: 60 FPS
	const double dt = t - last_t;

	// (in case timer resolution is low): count frames until
	// timer value has changed "enough".
	static double min_dt;
	ONCE(min_dt = timer_res() * 4.0);
		// chosen to reduce error but still yield rapid updates.
	static uint num_frames = 1;
	if(dt < min_dt)
	{
		num_frames++;
		return;
	}

	// dt is big enough => we will update.
	// calculate approximate current FPS (= 1 / elapsed time per frame).
	last_t = t;
	cur_fps = (1.0 / dt) * num_frames;
	num_frames = 1;	// reset for next time


	// average and smooth cur_fps.
	//
	// filter design goals: steady output, but rapid signal tracking.
	//
	// implemented as a variable-gain IIR filter with knowledge of typical
	// function characteristics. this is easier to stabilize than a PID
	// scheme, since it is based on averaging actual function values,
	// instead of trying to minimize output-vs-input error.
	// there are some similarities, though: same_side ~= I, and
	// bounced ~= D.

	//
	// check cur_fps function for several characteristics that
	// help decide if it's actually changing or just jittering.
	//

#define REL_ERR(correct, measured) (fabs((correct) - (measured)) / (correct))
#define SIGN_EQ(x0, x1, x2) ( ((x0) * (x1)) > 0.0 && ((x1) * (x2)) > 0.0 )
#define ONE_SIDE(x, x0, x1, x2) SIGN_EQ(x-x0, x-x1, x-x2)

	// cur_fps history and changes over past few frames
	static double h2, h1 = 30.0, h0 = 30.0;
	h2 = h1; h1 = h0; h0 = cur_fps;
	const double d21 = h1 - h2, d10 = h0 - h1;
	const double e20 = REL_ERR(h2, h0), e10 = REL_ERR(h1, h0);
	const double e0 = REL_ERR(avg_fps, h0);

	// indicators that the function is jittering
	const bool bounced = d21 * d10 < 0.0 && e20 < 0.05 && e10 > 0.10;
		// /\ or \/
	const bool jumped = e10 > 0.30;
		// large change (have seen semi-legitimate changes of 25%)
	const bool is_close = e0 < 0.02;
		// cur_fps - avg_fps is "small"

	// "same-side" check for rapid tracking of the function.
	// if the past few samples have been consistently above/below the average,
	// the function is moving up/down and we need to catch up.
	static int same_side;
		// consecutive times the last 3 samples have been on the same side.
	if(!ONE_SIDE(avg_fps, h0, h1, h2))	// not all on same side:
		same_side = 0;					// reset counter
	// (only increase if not too close to average,
	// so that this isn't triggered by jitter alone)
	if(!is_close)
		same_side++;


	//
	// determine filter gain, based on above characteristics.
	//

	static double gain;	// sensitivity to changes in cur_fps ([0,1])
	double bias = 0.0;	// (unlimited) exponential change to gain

	// ignore (gain -> 0) large jumps.
	if(jumped)
		bias -= 4.0;
	// don't let a "bounce" affect things too much.
	else if(bounced)
		bias -= 1.0;
	// otherwise, function is normal here.
	else
	{
		// function is changing, we need to track it rapidly.
		// note: check close again so we aren't too loose if the function
		// comes closer to the average again (meaning it probably
		// wasn't really changing).
		if(same_side >= 2 && !is_close)
			bias += MIN(same_side, 4);
	}

	// bias = 0: no change. > 0: increase (n-th root). < 0: decrease (^n)
	double e = (bias > 0)? 1.0 / bias : -bias;
	if(e == 0.0) e = 1.0;
	gain = pow(0.08, e);
		// default: fairly insensitive to changes (~= 16 sample average)


	// IIR filter
	static double old = 30.0;
	old = cur_fps*gain + old*(1.0-gain);
	avg_fps = old;

	spf = 1.0 / avg_fps;

	// update fps counter if it differs "enough"
	// currently, that means off by more than 5 FPS or 5%.
	const double difference = fabs(avg_fps-fps);
	const double threshold = fminf(5.f, 0.05f*fps);
	if(difference > threshold)
		fps = (int)(avg_fps + 0.99);
			// C float -> int rounds down; we want to round up to
			// hit vsync-locked framerates exactly.
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

		// convert raw ticks into seconds, if necessary
		double sum;
#if TIMER_USE_RAW_TICKS
# if CPU_IA32
		sum = tc->sum / cpu_freq;
# else
#  error "port"
# endif
#else
		sum = tc->sum;
#endif

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
