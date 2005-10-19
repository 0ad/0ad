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

// return resolution (expressed in [s]) of the time source underlying
// get_time.
extern double timer_res(void);

// calculate fps (call once per frame)
// several smooth filters (tuned for ~100 FPS)
// => less fluctuation, but rapid tracking

extern int fps;	// for user display
extern float spf;	// for time-since-last-frame use

extern void calc_fps(void);


//
// cumulative timer API
//

// this supplements in-game profiling by providing low-overhead,
// high resolution time accounting.

// opaque - do not access its fields!
// note: must be defined here because clients instantiate them;
// fields cannot be made private due to C compatibility requirement.
struct TimerClient
{
	double sum;	// total bill [s]

	// only store a pointer for efficiency.
	const char* description;

	TimerClient* next;

	// how often timer_bill_client was called (helps measure relative
	// performance of something that is done indeterminately often).
	uint num_calls;
};


// make the given TimerClient (usually instantiated as static data)
// ready for use. returns its address for TIMER_ADD_CLIENT's convenience.
// this client's total (added to by timer_bill_client) will be
// displayed by timer_display_client_totals.
// notes:
// - may be called at any time;
// - always succeeds (there's no fixed limit);
// - free() is not needed nor possible.
// - description must remain valid until exit; a string literal is safest.
extern TimerClient* timer_add_client(TimerClient* tc, const char* description);

// add <dt> [s] to the client's total.
extern void timer_bill_client(TimerClient* tc, double dt);

// display all clients' totals; does not reset them.
// typically called at exit.
extern void timer_display_client_totals();

#ifdef __cplusplus
}
#endif




// used via TIMER* macros below.
class ScopeTimer
{
	double t0;
	const char* description;

public:
	ScopeTimer(const char* _description)
	{
		t0 = get_time();
		description = _description;
	}

	~ScopeTimer()
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

		debug_printf("TIMER %s: %g %s\n", description, dt*scale, unit);
	}

	// disallow copying (makes no sense)
private:
	ScopeTimer& operator=(const ScopeTimer&);
};

/*
Measure the time taken to execute code up until end of the current scope; 
display it via debug_printf. Can safely be nested.
Useful for measuring time spent in a function or basic block.
<description> must remain valid over the lifetime of this object;
a string literal is safest.

Example usage:
	void func()
	{
		TIMER("description");
		// code to be measured
	}
*/
#define TIMER(description) ScopeTimer UID__(description)

/*
Measure the time taken to execute code between BEGIN and END markers;
display it via debug_printf. Can safely be nested.
Useful for measuring several pieces of code within the same function/block.
<description> must remain valid over the lifetime of this object;
a string literal is safest.

Caveats:
- this wraps the code to be measured in a basic block, so any
  variables defined there are invisible to surrounding code.
- the description passed to END isn't inspected; you are responsible for
  ensuring correct nesting!

Example usage:
	void func2()
	{
		// uninteresting code
		TIMER_BEGIN("description2");
		// code to be measured
		TIMER_END("description2");
		// uninteresting code
	}
*/
#define TIMER_BEGIN(description) { ScopeTimer UID__(description)
#define TIMER_END(description) }


// used via TIMER_ACCRUE
class ScopeTimerAccrue
{
	double t0;
	TimerClient* tc;

public:
	ScopeTimerAccrue(TimerClient* tc_)
	{
		t0 = get_time();
		tc = tc_;
	}
	~ScopeTimerAccrue()
	{
		double t1 = get_time();
		double dt = t1-t0;
		timer_bill_client(tc, dt);
	}

	// disallow copying (makes no sense)
private:
	ScopeTimerAccrue& operator=(const ScopeTimerAccrue&);
};


// "allocate" a new TimerClient that will keep track of the total time
// billed to it, along with a description string. These are displayed when
// timer_display_client_totals is called.
// Invoke this at file or function scope; a (static) TimerClient pointer of
// name <id> will be defined, which should be passed to TIMER_ACCRUE.
#define TIMER_ADD_CLIENT(id)\
	static TimerClient UID__;\
	static TimerClient* id = timer_add_client(&UID__, #id);

/*
Measure the time taken to execute code up until end of the current scope; 
bill it to the given TimerClient object. Can safely be nested.
Useful for measuring total time spent in a function or basic block over the
entire program.
<description> must remain valid over the lifetime of this object;
a string literal is safest.

Example usage:
	TIMER_ADD_CLIENT(identifier)

	void func()
	{
		TIMER_ACCRUE(name_of_pointer_to_client);
		// code to be measured
	}

	[at exit]
	timer_display_client_totals();
*/
#define TIMER_ACCRUE(client) ScopeTimerAccrue UID__(client)

#endif	// #ifndef TIMER_H
