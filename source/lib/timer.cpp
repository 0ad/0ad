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
#include "timer.h"
#include "adts.h"

#include <numeric>
#include <math.h>

// wrapper over gettimeofday, instead of emulating it for Windows,
// because double return type allows higher resolution (e.g. if using TSC),
// and gettimeofday is not guaranteed to be monotonic.

double get_time()
{
	double t;

#ifdef HAVE_CLOCK_GETTIME

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
	t = (cur.tv_sec - start.tv_sec) + (cur.tv_usec - start.tv_usec)*1e-6;

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
#ifdef HAVE_CLOCK_GETTIME

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
	RingBuf<float, 16> samples;

	// get elapsed time [s] since last update
	static double last_t;
	const double t = get_time();
	const double dt = t - last_t;

	// (in case timer resolution is low): count frames until
	// timer value has changed "enough".
	static uint num_frames = 1;
	if(dt < 1e-3)
		// bonus: if FPS > 1000, updates are slowed down a few frames.
	{
		num_frames++;
		return;
	}

	// dt is big enough => we will update.
	// calculate approximate current FPS (= 1 / elapsed time per frame).
	float cur_fps = 30.0f;	// start value => history converges faster
	if(last_t != 0.0)
		cur_fps = 1.0f / (float)dt * num_frames;
	num_frames = 1;	// reset for next time
	last_t = t;

	// calculate fps activity over 3 frames (used below to prevent fluctuation)
	// -1: decreasing, +1: increasing, 0: neither or fluctuating
	const float h1 = samples[-1];	// last frame's cur_fps
	const float h2 = samples[-2];	// 2nd most recent frame's cur_fps

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

	// add new sample and average
	samples.push_back(cur_fps);
	const double sum_fps = std::accumulate(samples.begin(), samples.end(), 0.0);
	const double avg_fps = sum_fps / (int)samples.size();

	// update fps counter if update threshold is exceeded
	const float d_avg = (float)(avg_fps-fps);
	const float max_diff = fminf(5.f, 0.05f*fps);
	if((trend > 0 && (avg_fps > fps || d_avg < -4.f)) ||	// going up, or large drop
	   (trend < 0 && (avg_fps < fps || d_avg >  4.f)) ||	// going down, or large raise
	   (fabs(d_avg) > max_diff))							// significant difference
		fps = (int)avg_fps;
}
