// Windows-specific high resolution timer
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

#include "types.h"

// possible high resolution timers, in order of preference.
// see source for timer properties + problems.
// used as index into hrt_overrides.
enum HRTImpl
{
	// CPU timestamp counter
	HRT_TSC,

	// Windows QueryPerformanceCounter
	HRT_QPC,

	// Windows timeGetTime
	HRT_TGT,

	// there will always be a valid timer in use.
	// this is only used with hrt_override_impl.
	HRT_NONE,

	HRT_NUM_IMPLS
};


// while we do our best to work around timer problems or avoid them if unsafe,
// future requirements and problems may be different:
// allow the user or app to override our decisions (via hrt_override_impl)
enum HRTOverride
{
	// allow use of this implementation if available,
	// and we can work around its problems
	HRT_DEFAULT,

	// override our 'safe to use' recommendation
	// set by hrt_override_impl (via command line arg or console function)
	HRT_DISABLE,
	HRT_FORCE
};


// return ticks since first call.
extern i64 hrt_ticks();

// return seconds since first call.
extern double hrt_time();

// return seconds between start and end timestamps (returned by hrt_ticks).
// negative if end comes before start.
extern double hrt_delta_s(i64 start, i64 end);

// return current timer implementation and its nominal (rated) frequency.
// nominal_freq is never 0.
// implementation only changes after hrt_override_impl.
extern void hrt_query_impl(HRTImpl& impl, i64& nominal_freq);

// override our 'safe to use' decision.
// resets (and chooses another, if applicable) implementation;
// the timer may jump after doing so.
// call with HRT_DEFAULT, HRT_NONE to re-evaluate implementation choice
// after system info becomes available.
extern int hrt_override_impl(HRTOverride ovr, HRTImpl impl);
