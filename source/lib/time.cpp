// platform indepentend high resolution timer
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

#include <time.h>
#include <cmath>

#include "win.h"

#include "ia32.h"
#include "posix.h"
#include "detect.h"
#include "time.h"
#include "types.h"
#include "misc.h"



// high resolution (> 1 µs) timestamp [s], starting at or near 0 s.
//
// uses TSC on single processor x86 desktop systems unless NO_TSC is defined,
// otherwise platform specific timers (QueryPerformanceCounter, gettimeofday).
double get_time()
{
	static double to_s;
	double t;

#if defined(_M_IX86) && !defined(NO_TSC)
	static int use_tsc = -1;
	static u64 tsc_start;

	// spaghetti code for minimum timing overhead
first_tsc:
	if(use_tsc == 1)
		return (__int64)(rdtsc() - tsc_start) * to_s;
			// VC6 can't convert u64 -> double; we don't need full range anyway
	// don't know yet
	if(use_tsc == -1)
		// don't use yet - need a time reference for CPU freq calculation.
		if(cpu_freq != 0.0f)
			// use only on single processor desktop systems
			// (otherwise CPU freq may change, clocks may get out of sync)
			if(cpus == 1 && !is_notebook && (cpu_caps & TSC))
			{
				use_tsc = 1;
				to_s = 1.0 / cpu_freq;
				tsc_start = rdtsc();
				goto first_tsc;	// using the other timers now would trash to_s
			}
			else
				use_tsc = 0;
#endif

#ifdef _WIN32

	static LARGE_INTEGER start;
	LARGE_INTEGER i;

	if(!to_s)
	{
		QueryPerformanceFrequency(&i);
		to_s = 1.0 / i.QuadPart;

		QueryPerformanceCounter(&start);
	}

	QueryPerformanceCounter(&i);
	t = (i.QuadPart - start.QuadPart) * to_s;

#else

	static struct timeval start;
	struct timeval tv;

	if(!start.tv_sec)
		gettimeofday(&start, 0);

	gettimeofday(&tv, 0);
	t = (tv.tv_sec - start.tv_sec) + (tv.tv_usec - start.tv_usec)*1e-6;

#endif

	return t;
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
	static int bad = 0;	// bad > 0 <==> last value was skipped
	if(fabs(h1-cur_fps) > .05f*h1)	// > 5% difference
	{
		// first 'bad' value: don't update fps_hist/fps; otherwise, reset bad
		if(!bad++)
			return;
	}
	else
		bad = 0;

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
