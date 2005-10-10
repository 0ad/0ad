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

#ifndef TIMER_H
#define TIMER_H

#include <string>

#include "debug.h"	// debug_printf

#ifdef __cplusplus
extern "C" {
#endif

// high resolution (> 1 us) timestamp [s], starting at or near 0 s.
extern double get_time(void);

extern double timer_res(void);

// calculate fps (call once per frame)
// several smooth filters (tuned for ~100 FPS)
// => less fluctuation, but rapid tracking

extern int fps;

extern void calc_fps(void);


//
// cumulative timer API
//

struct TimerClient;	// opaque

// allocate a new TimerClient whose total (added to by timer_bill_client)
// will be displayed by timer_display_client_totals.
// notes:
// - uses static data; there is a fixed limit. rationale: see clients[].
// - may be called at any time;
// - free() is not needed nor possible.
// - name must remain valid until exit; passing a string literal is safest.
extern TimerClient* timer_add_client(const char* name);

// add <dt> [s] to the client's total.
extern void timer_bill_client(TimerClient* tc, double dt);

// display all clients' totals; does not reset them.
// typically called at exit.
extern void timer_display_client_totals();

#ifdef __cplusplus
}
#endif



class ScopedTimer
{
	double t0;
	const char* name;

public:
	ScopedTimer(const char* _name)
	{
		t0 = get_time();
		name = _name;
	}
	~ScopedTimer()
	{
		double t1 = get_time();
		double dt = t1-t0;

		// determine scale factor for pretty display
		double scale = 1e6;
		const char* unit = "us";
		if(dt > 1.0)
			scale = 1, unit = "s";
		else if(dt > 1e-3)
			scale = 1e3, unit = "ms";

		debug_printf("TIMER %s: %g %s\n", name, dt*scale, unit);
	}

	// disallow copying (makes no sense)
private:
	ScopedTimer& operator=(const ScopedTimer&);
};

#define TIMER(name) ScopedTimer ST##name(#name)
// Cheat a bit to make things slightly easier on the user
#define TIMER_START(name) { ScopedTimer __timer( name )
#define TIMER_END(name) }




/*
Example usage:

	static TimerClient* client = timer_add_client("description");

	void func()
	{
		SUM_TIMER(client);
		(code to be measured)
	}

	[at exit]
	timer_display_client_totals();

*/

class ScopedSumTimer
{
	double t0;
	TimerClient* tc;

public:
	ScopedSumTimer(TimerClient* tc_)
	{
		t0 = get_time();
		tc = tc_;
	}
	~ScopedSumTimer()
	{
		double t1 = get_time();
		double dt = t1-t0;
		timer_bill_client(tc, dt);
	}

	// disallow copying (makes no sense)
private:
	ScopedSumTimer& operator=(const ScopedSumTimer&);
};

// bills to <client> the time elapsed between execution reaching the
// point of macro invocation and end of the current basic block.
// total times for all clients are displayed by timer_display_client_totals.
#define SUM_TIMER(client) ScopedSumTimer UID__(client)

#endif	// #ifndef TIMER_H
