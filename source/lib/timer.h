/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_TIMER
#define INCLUDED_TIMER

#include "lib/config2.h"	// CONFIG2_TIMER_ALLOW_RDTSC
#include "lib/sysdep/cpu.h"	// cpu_AtomicAdd
#if ARCH_X86_X64 && CONFIG2_TIMER_ALLOW_RDTSC
# include "lib/sysdep/os_cpu.h"	// os_cpu_ClockFrequency
# include "lib/sysdep/arch/x86_x64/x86_x64.h"	// x86_x64::rdtsc
#endif

#include "lib/utf8.h"

/**
 * timer_Time will subsequently return values relative to the current time.
 **/
LIB_API void timer_LatchStartTime();

/**
 * @return high resolution (> 1 us) timestamp [s].
 **/
LIB_API double timer_Time();

/**
 * @return resolution [s] of the timer.
 **/
LIB_API double timer_Resolution();


// (allow using XADD (faster than CMPXCHG) in 64-bit builds without casting)
#if ARCH_AMD64
typedef intptr_t Cycles;
#else
typedef i64 Cycles;
#endif

/**
 * internal helper functions for returning an easily readable
 * string (i.e. re-scaled to appropriate units)
 **/
LIB_API std::string StringForSeconds(double seconds);
LIB_API std::string StringForCycles(Cycles cycles);


//-----------------------------------------------------------------------------
// scope timing

/// used by TIMER
class ScopeTimer
{
	NONCOPYABLE(ScopeTimer);
public:
	ScopeTimer(const wchar_t* description)
		: m_t0(timer_Time()), m_description(description)
	{
	}

	~ScopeTimer()
	{
		const double t1 = timer_Time();
		const std::string elapsedTimeString = StringForSeconds(t1-m_t0);
		debug_printf("TIMER| %s: %s\n", utf8_from_wstring(m_description).c_str(), elapsedTimeString.c_str());
	}

private:
	double m_t0;
	const wchar_t* m_description;
};

/**
 * Measures the time taken to execute code up until end of the current scope;
 * displays it via debug_printf. Can safely be nested.
 * Useful for measuring time spent in a function or basic block.
 * <description> must remain valid over the lifetime of this object;
 * a string literal is safest.
 *
 * Example usage:
 * 	void func()
 * 	{
 * 		TIMER(L"description");
 * 		// code to be measured
 * 	}
 **/
#define TIMER(description) ScopeTimer UID__(description)

/**
 * Measures the time taken to execute code between BEGIN and END markers;
 * displays it via debug_printf. Can safely be nested.
 * Useful for measuring several pieces of code within the same function/block.
 * <description> must remain valid over the lifetime of this object;
 * a string literal is safest.
 *
 * Caveats:
 * - this wraps the code to be measured in a basic block, so any
 *   variables defined there are invisible to surrounding code.
 * - the description passed to END isn't inspected; you are responsible for
 *   ensuring correct nesting!
 *
 * Example usage:
 * 	void func2()
 * 	{
 * 		// uninteresting code
 * 		TIMER_BEGIN(L"description2");
 * 		// code to be measured
 * 		TIMER_END(L"description2");
 * 		// uninteresting code
 * 	}
 **/
#define TIMER_BEGIN(description) { ScopeTimer UID__(description)
#define TIMER_END(description) }


//-----------------------------------------------------------------------------
// cumulative timer API

// this supplements in-game profiling by providing low-overhead,
// high resolution time accounting of specific areas.

// since TIMER_ACCRUE et al. are called so often, we try to keep
// overhead to an absolute minimum. storing raw tick counts (e.g. CPU cycles
// returned by x86_x64::rdtsc) instead of absolute time has two benefits:
// - no need to convert from raw->time on every call
//   (instead, it's only done once when displaying the totals)
// - possibly less overhead to querying the time itself
//   (timer_Time may be using slower time sources with ~3us overhead)
//
// however, the cycle count is not necessarily a measure of wall-clock time
// (see http://www.gamedev.net/reference/programming/features/timing).
// therefore, on systems with SpeedStep active, measurements of I/O or other
// non-CPU bound activity may be skewed. this is ok because the timer is
// only used for profiling; just be aware of the issue.
// if this is a problem, disable CONFIG2_TIMER_ALLOW_RDTSC.
//
// note that overflow isn't an issue either way (63 bit cycle counts
// at 10 GHz cover intervals of 29 years).

#if ARCH_X86_X64 && CONFIG2_TIMER_ALLOW_RDTSC

class TimerUnit
{
public:
	void SetToZero()
	{
		m_cycles = 0;
	}

	void SetFromTimer()
	{
		m_cycles = x86_x64::rdtsc();
	}

	void AddDifference(TimerUnit t0, TimerUnit t1)
	{
		m_cycles += t1.m_cycles - t0.m_cycles;
	}

	void AddDifferenceAtomic(TimerUnit t0, TimerUnit t1)
	{
		const Cycles delta = t1.m_cycles - t0.m_cycles;
#if ARCH_AMD64
		cpu_AtomicAdd(&m_cycles, delta);
#elif ARCH_IA32
retry:
		if(!cpu_CAS64(&m_cycles, m_cycles, m_cycles+delta))
			goto retry;
#else
# error "port"
#endif
	}

	void Subtract(TimerUnit t)
	{
		m_cycles -= t.m_cycles;
	}

	std::string ToString() const
	{
		ENSURE(m_cycles >= 0);
		return StringForCycles(m_cycles);
	}

	double ToSeconds() const
	{
		return (double)m_cycles / os_cpu_ClockFrequency();
	}

private:
	Cycles m_cycles;
};

#else

class TimerUnit
{
public:
	void SetToZero()
	{
		m_seconds = 0.0;
	}

	void SetFromTimer()
	{
		m_seconds = timer_Time();
	}

	void AddDifference(TimerUnit t0, TimerUnit t1)
	{
		m_seconds += t1.m_seconds - t0.m_seconds;
	}

	void AddDifferenceAtomic(TimerUnit t0, TimerUnit t1)
	{
retry:
		i64 oldRepresentation;
		memcpy(&oldRepresentation, &m_seconds, sizeof(oldRepresentation));

		const double seconds = m_seconds + t1.m_seconds - t0.m_seconds;
		i64 newRepresentation;
		memcpy(&newRepresentation, &seconds, sizeof(newRepresentation));

		if(!cpu_CAS64((volatile i64*)&m_seconds, oldRepresentation, newRepresentation))
			goto retry;
	}

	void Subtract(TimerUnit t)
	{
		m_seconds -= t.m_seconds;
	}

	std::string ToString() const
	{
		ENSURE(m_seconds >= 0.0);
		return StringForSeconds(m_seconds);
	}

	double ToSeconds() const
	{
		return m_seconds;
	}

private:
	double m_seconds;
};

#endif

// opaque - do not access its fields!
// note: must be defined here because clients instantiate them;
// fields cannot be made private due to POD requirement.
struct TimerClient
{
	TimerUnit sum;	// total bill

	// only store a pointer for efficiency.
	const wchar_t* description;

	TimerClient* next;

	// how often the timer was billed (helps measure relative
	// performance of something that is done indeterminately often).
	intptr_t num_calls;
};

/**
 * make the given TimerClient (usually instantiated as static data)
 * ready for use. returns its address for TIMER_ADD_CLIENT's convenience.
 * this client's total (which is increased by a BillingPolicy) will be
 * displayed by timer_DisplayClientTotals.
 * notes:
 * - may be called at any time;
 * - always succeeds (there's no fixed limit);
 * - free() is not needed nor possible.
 * - description must remain valid until exit; a string literal is safest.
 **/
LIB_API TimerClient* timer_AddClient(TimerClient* tc, const wchar_t* description);

/**
 * "allocate" a new TimerClient that will keep track of the total time
 * billed to it, along with a description string. These are displayed when
 * timer_DisplayClientTotals is called.
 * Invoke this at file or function scope; a (static) TimerClient pointer of
 * name \<id\> will be defined, which should be passed to TIMER_ACCRUE.
 **/
#define TIMER_ADD_CLIENT(id)\
	static TimerClient UID__;\
	static TimerClient* id = timer_AddClient(&UID__, WIDEN(#id))

/**
 * bill the difference between t0 and t1 to the client's total.
 **/
struct BillingPolicy_Default
{
	void operator()(TimerClient* tc, TimerUnit t0, TimerUnit t1) const
	{
		tc->sum.AddDifference(t0, t1);
		tc->num_calls++;
	}
};

/**
 * thread-safe (not used by default due to its higher overhead)
 * note: we can't just use thread-local variables to avoid
 * synchronization overhead because we don't have control over all
 * threads (for accumulating their separate timer copies).
 **/
struct BillingPolicy_Atomic
{
	void operator()(TimerClient* tc, TimerUnit t0, TimerUnit t1) const
	{
		tc->sum.AddDifferenceAtomic(t0, t1);
		cpu_AtomicAdd(&tc->num_calls, +1);
	}
};

/**
 * display all clients' totals; does not reset them.
 * typically called at exit.
 **/
LIB_API void timer_DisplayClientTotals();


/// used by TIMER_ACCRUE
template<class BillingPolicy = BillingPolicy_Default>
class ScopeTimerAccrue
{
	NONCOPYABLE(ScopeTimerAccrue);
public:
	ScopeTimerAccrue(TimerClient* tc)
		: m_tc(tc)
	{
		m_t0.SetFromTimer();
	}

	~ScopeTimerAccrue()
	{
		TimerUnit t1;
		t1.SetFromTimer();
		BillingPolicy()(m_tc, m_t0, t1);
	}

private:
	TimerUnit m_t0;
	TimerClient* m_tc;
};

/**
 * Measure the time taken to execute code up until end of the current scope;
 * bill it to the given TimerClient object. Can safely be nested.
 * Useful for measuring total time spent in a function or basic block over the
 * entire program.
 * `client' is an identifier registered via TIMER_ADD_CLIENT.
 *
 * Example usage:
 * 	TIMER_ADD_CLIENT(client);
 *
 * 	void func()
 * 	{
 * 		TIMER_ACCRUE(client);
 * 		// code to be measured
 * 	}
 *
 * 	[later or at exit]
 * 	timer_DisplayClientTotals();
 **/
#define TIMER_ACCRUE(client) ScopeTimerAccrue<> UID__(client)
#define TIMER_ACCRUE_ATOMIC(client) ScopeTimerAccrue<BillingPolicy_Atomic> UID__(client)

#endif	// #ifndef INCLUDED_TIMER
