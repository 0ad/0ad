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

#include "timer.h"
#include "types.h"
#include "misc.h"
#include "lib.h"

#ifdef _WIN32
#include "sysdep/win/hrt.h"
#endif


// wrapper over gettimeofday, instead of emulating it for Windows,
// because double return type allows higher resolution (e.g. if using TSC),
// and gettimeofday is not guaranteed to be monotonic.

double get_time()
{
	double t;

#ifdef _WIN32

	t = hrt_time();

#elif defined(HAVE_CLOCK_GETTIME)

	static struct timespec start;
	struct timespec ts;

	if(!start.tv_sec)
		clock_gettime(CLOCK_REALTIME, &start);

	clock_gettime(CLOCK_REALTIME, &ts);
	t = (ts.tv_sec - start.tv_sec) + (ts.tv_nsec - start.tv_nsec)*1e-9;

#elif defined(HAVE_GETTIMEOFDAY)

	static struct timeval start;
	struct timeval cur;

	if(!start.tv_sec)
		gettimeofday(&start, 0);

	gettimeofday(&cur, 0);
	t = (cur.tv_sec - start.tv_sec) + (cur.tv_nsec - start.tv_nsec)*1e-6;

#else

#error "get_time: add timer implementation for this platform!"

#endif

	// make sure time is monotonic (never goes backwards)
	static double t_last;
	if(t_last != 0.0 && t < t_last)
		t = t_last;

	return t;
}


double timer_res()
{
#ifdef _WIN32

	HRTImpl impl;
	i64 nominal_freq;
	hrt_query_impl(impl, nominal_freq);
	return 1.0 / nominal_freq;

#elif defined(HAVE_CLOCK_GETTIME)

	struct timespec res;
	clock_getres(CLOCK_REALTIME, &res);
	return res.tv_nsec * 1e-9;

#else

	// guess millisecond-class
	return 1e-3;

#endif
}


// calculate fps (call once per frame)
// several smooth filters:
// - throw out single spikes / dips
// - average via small history buffer
// - update final value iff the difference (% or absolute) is too great,
//   or if the change is consistent with the trend over the last few frames.
//
// => less fluctuation, but rapid tracking.
// filter values are tuned for 100 FPS.

int fps = 0;

void calc_fps()
{
	// history buffer - smooth out slight variations
#define H 10					// # buffer entries
	static float fps_sum = 0;	// sum of last H frames' cur_fps
	static float fps_hist[H];	// last H frames' cur_fps
								// => don't need to re-average every time
	static uint head = 0;		// oldest entry in fps_hist
								// must be unsigned, b/c we do (head-1)%H

	// get elapsed time [s] since last frame; approximate current fps
	static double last_t;
	double t = get_time();
	float cur_fps = 30.0f;	// start value => history converges faster
	if(last_t != 0.0)
		cur_fps = 1.0f / (float)(t-last_t);	// = 1 / elapsed time
	last_t = t;

	// calculate fps activity over 3 frames (used below to prevent fluctuation)
	// -1: decreasing, +1: increasing, 0: neither or fluctuating
	float h1 = fps_hist[(head-1)%H];	// last frame's cur_fps
	float h2 = fps_hist[(head-2)%H];	// 2nd most recent frame's cur_fps

	int trend = 0;
	if(h2 > h1 && h1 > cur_fps)			// decreasing
		trend = -1;
	else if(cur_fps < h1 && h1 < h2)	// increasing
		trend = 1;

	// ignore onetime skips in fps (probably page faults or similar)
	static int ignored;
	if(fabs(h1-cur_fps) > .05f*h1 &&	// > 5% difference
	   !ignored++)			// was it first value we're discarding?
		return;				// yes: don't update fps_hist/fps
	ignored = 0;	// either value ok, or it wasn't a fluke - reset counter

	// remove oldest cur_fps value in fps_hist from the sum
	// and add cur_fps; also insert cur_fps in fps_hist
	fps_sum -=  fps_hist[head];
	fps_sum += (fps_hist[head] = cur_fps);
	head = (head+1)%H;

	// update fps counter if update threshold is exceeded
	const float avg_fps = fps_sum / H;
	const float d_avg = avg_fps-fps;
	const float max_diff = fminf(5.f, 0.05f*fps);

	if((trend > 0 && (avg_fps > fps || d_avg < -4.f)) ||	// going up, or large drop
	   (trend < 0 && (avg_fps < fps || d_avg >  4.f)) ||	// going down, or large raise
	   (fabs(d_avg) > max_diff))							// significant difference
		fps = (int)avg_fps;
}
