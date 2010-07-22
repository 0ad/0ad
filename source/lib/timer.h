/* Copyright (c) 2010 Wildfire Games
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
# include "lib/sysdep/arch/x86_x64/x86_x64.h"	// x86_x64_rdtsc
# include "lib/sysdep/os_cpu.h"	// os_cpu_ClockFrequency
#endif

#include <sstream>	// std::stringstream

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
		double t1 = timer_Time();
		double dt = t1-m_t0;

		// determine scale factor for pretty display
		double scale = 1e6;
		const wchar_t* unit = L"us";
		if(dt > 1.0)
			scale = 1, unit = L"s";
		else if(dt > 1e-3)
			scale = 1e3, unit = L"ms";

		debug_printf(L"TIMER| %ls: %g %ls\n", m_description, dt*scale, unit);
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
// returned by ia32_rdtsc) instead of absolute time has two benefits:
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
		m_ticks = 0;
	}

	void SetFromTimer()
	{
		m_ticks = x86_x64_rdtsc();
	}

	void AddDifference(TimerUnit t0, TimerUnit t1)
	{
		m_ticks += t1.m_ticks - t0.m_ticks;
	}

	void AddDifferenceAtomic(TimerUnit t0, TimerUnit t1)
	{
		const i64 delta = t1.m_ticks - t0.m_ticks;
#if ARCH_AMD64
		cpu_AtomicAdd((volatile intptr_t*)&m_ticks, (intptr_t)delta);
#else
retry:
		if(!cpu_CAS64(&m_ticks, m_ticks, m_ticks+delta))
			goto retry;
#endif
	}

	void Subtract(TimerUnit t)
	{
		m_ticks -= t.m_ticks;
	}

	std::wstring ToString() const
	{
		debug_assert(m_ticks >= 0.0);

		// determine scale factor for pretty display
		double scale = 1.0;
		const wchar_t* unit = L" c";
		if(m_ticks > 10000000000LL)	// 10 Gc
			scale = 1e-9, unit = L" Gc";
		else if(m_ticks > 10000000)	// 10 Mc
			scale = 1e-6, unit = L" Mc";
		else if(m_ticks > 10000)	// 10 kc
			scale = 1e-3, unit = L" kc";

		std::wstringstream ss;
		ss << m_ticks*scale;
		ss << unit;
		return ss.str();
	}

	double ToSeconds() const
	{
		return m_ticks / os_cpu_ClockFrequency();
	}

private:
	i64 m_ticks;
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

	std::wstring ToString() const
	{
		debug_assert(m_seconds >= 0.0);

		// determine scale factor for pretty display
		double scale = 1e6;
		const wchar_t* unit = L" us";
		if(m_seconds > 1.0)
			scale = 1, unit = L" s";
		else if(m_seconds > 1e-3)
			scale = 1e3, unit = L" ms";

		std::wstringstream ss;
		ss << m_seconds*scale;
		ss << unit;
		return ss.str();
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

	// how often timer_BillClient was called (helps measure relative
	// performance of something that is done indeterminately often).
	intptr_t num_calls;
};

/**
 * make the given TimerClient (usually instantiated as static data)
 * ready for use. returns its address for TIMER_ADD_CLIENT's convenience.
 * this client's total (added to by timer_BillClient) will be
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
 * name <id> will be defined, which should be passed to TIMER_ACCRUE.
 **/
#define TIMER_ADD_CLIENT(id)\
	static TimerClient UID__;\
	static TimerClient* id = timer_AddClient(&UID__, WIDEN(#id))

/**
 * bill the difference between t0 and t1 to the client's total.
 **/
inline void timer_BillClient(TimerClient* tc, TimerUnit t0, TimerUnit t1)
{
	tc->sum.AddDifference(t0, t1);
	tc->num_calls++;
}

/**
 * thread-safe version of timer_BillClient
 * (not used by default due to its higher overhead)
 **/
inline void timer_BillClientAtomic(TimerClient* tc, TimerUnit t0, TimerUnit t1)
{
	tc->sum.AddDifferenceAtomic(t0, t1);
	cpu_AtomicAdd(&tc->num_calls, +1);
}

/**
 * display all clients' totals; does not reset them.
 * typically called at exit.
 **/
LIB_API void timer_DisplayClientTotals();

/// used by TIMER_ACCRUE
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
		timer_BillClient(m_tc, m_t0, t1);
	}

private:
	TimerUnit m_t0;
	TimerClient* m_tc;
};

class ScopeTimerAccrueAtomic
{
	NONCOPYABLE(ScopeTimerAccrueAtomic);
public:
	ScopeTimerAccrueAtomic(TimerClient* tc)
		: m_tc(tc)
	{
		m_t0.SetFromTimer();
	}

	~ScopeTimerAccrueAtomic()
	{
		TimerUnit t1;
		t1.SetFromTimer();
		timer_BillClientAtomic(m_tc, m_t0, t1);
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
 * <description> must remain valid over the lifetime of this object;
 * a string literal is safest.
 * 
 * Example usage:
 * 	TIMER_ADD_CLIENT(identifier);
 * 
 * 	void func()
 * 	{
 * 		TIMER_ACCRUE(name_of_pointer_to_client);
 * 		// code to be measured
 * 	}
 * 
 * 	[at exit]
 * 	timer_DisplayClientTotals();
 **/
#define TIMER_ACCRUE(client) ScopeTimerAccrue UID__(client)
#define TIMER_ACCRUE_ATOMIC(client) ScopeTimerAccrueAtomic UID__(client)

#endif	// #ifndef INCLUDED_TIMER
