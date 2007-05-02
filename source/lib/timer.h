/**
 * =========================================================================
 * File        : timer.h
 * Project     : 0 A.D.
 * Description : platform-independent high resolution timer and
 *             : FPS measuring code.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef TIMER_H
#define TIMER_H

#include <string>

#include "debug.h"	// debug_printf


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


//-----------------------------------------------------------------------------
// timestamp sources

// since TIMER_ACCRUE et al. are called so often, we try to keep
// overhead to an absolute minimum. storing raw tick counts (e.g. CPU cycles
// returned by ia32_rdtsc) instead of absolute time has two benefits:
// - no need to convert from raw->time on every call
//   (instead, it's only done once when displaying the totals)
// - possibly less overhead to querying the time itself
//   (get_time may be using slower time sources with ~3us overhead)
//
// however, the cycle count is not necessarily a measure of wall-clock time.
// therefore, on systems with SpeedStep active, measurements of I/O or other
// non-CPU bound activity may be skewed. this is ok because the timer is
// only used for profiling; just be aware of the issue.
// if this is a problem, disable CONFIG_TIMER_ALLOW_RDTSC.
// 
// note that overflow isn't an issue either way (63 bit cycle counts
// at 10 GHz cover intervals of 29 years).

#if CPU_IA32

// fast, not usable as wall-clock (http://www.gamedev.net/reference/programming/features/timing)
class TimerRdtsc
{
public:
	typedef i64 unit;
	unit get_timestamp() const;
};

#endif

class TimerSafe
{
public:
	typedef double unit;
	unit get_timestamp() const
	{
		return get_time();
	}
};

#if CPU_IA32 && TIMER_ALLOW_RDTSC
typedef TimerRdtsc Timer;
#else
typedef TimerSafe Timer;
#endif

typedef Timer::unit TimerUnit;	// convenience


//-----------------------------------------------------------------------------
// cumulative timer API

// this supplements in-game profiling by providing low-overhead,
// high resolution time accounting.

// opaque - do not access its fields!
// note: must be defined here because clients instantiate them;
// fields cannot be made private due to C compatibility requirement.
struct TimerClient
{
	TimerUnit sum;	// total bill

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

// add <dt> to the client's total.
extern void timer_bill_client(TimerClient* tc, TimerUnit dt);

// display all clients' totals; does not reset them.
// typically called at exit.
extern void timer_display_client_totals();


//-----------------------------------------------------------------------------
// scoped-based timers

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

		debug_printf("TIMER| %s: %g %s\n", description, dt*scale, unit);
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
template<class TimerImpl = Timer> class ScopeTimerAccrue
{
	TimerImpl impl;
	typename TimerImpl::unit t0;
	TimerClient* tc;

public:
	ScopeTimerAccrue<TimerImpl>(TimerClient* tc_)
	{
		t0 = impl.get_timestamp();
		tc = tc_;
	}
	~ScopeTimerAccrue<TimerImpl>()
	{
		typename TimerImpl::unit dt = impl.get_timestamp() - t0;
		timer_bill_client(tc, dt);
	}

	// disallow copying (makes no sense)
private:
	ScopeTimerAccrue<TimerImpl>& operator=(const ScopeTimerAccrue<TimerImpl>&);
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
#define TIMER_ACCRUE(client) ScopeTimerAccrue<> UID__(client)

#endif	// #ifndef TIMER_H
