/**
 * =========================================================================
 * File        : timer.h
 * Project     : 0 A.D.
 * Description : platform-independent high resolution timer
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_TIMER
#define INCLUDED_TIMER

/**
 * timer_Time will subsequently return values relative to the current time.
 **/
LIB_API void timer_LatchStartTime();

/**
 * @return high resolution (> 1 us) timestamp [s].
 **/
LIB_API double timer_Time(void);

/**
 * @return resolution [s] of the timer.
 **/
LIB_API double timer_Resolution(void);


//-----------------------------------------------------------------------------
// scope timing

/// used by TIMER
class LIB_API ScopeTimer : noncopyable
{
public:
	ScopeTimer(const char* description);
	~ScopeTimer();

private:
	double m_t0;
	const char* m_description;
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
 * 		TIMER("description");
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
 * 		TIMER_BEGIN("description2");
 * 		// code to be measured
 * 		TIMER_END("description2");
 * 		// uninteresting code
 * 	}
 **/
#define TIMER_BEGIN(description) { ScopeTimer UID__(description)
#define TIMER_END(description) }


//-----------------------------------------------------------------------------
// cumulative timer API

// this supplements in-game profiling by providing low-overhead,
// high resolution time accounting of specific areas.

union LIB_API TimerUnit
{
public:
	void SetToZero();
	void SetFromTimer();
	void AddDifference(TimerUnit t0, TimerUnit t1);
	void Subtract(TimerUnit t);
	std::string ToString() const;
	double ToSeconds() const;

private:
	u64 m_ticks;
	double m_seconds;
};

// opaque - do not access its fields!
// note: must be defined here because clients instantiate them;
// fields cannot be made private due to POD requirement.
struct TimerClient
{
	TimerUnit sum;	// total bill

	// only store a pointer for efficiency.
	const char* description;

	TimerClient* next;

	// how often timer_BillClient was called (helps measure relative
	// performance of something that is done indeterminately often).
	size_t num_calls;
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
LIB_API TimerClient* timer_AddClient(TimerClient* tc, const char* description);

/**
 * "allocate" a new TimerClient that will keep track of the total time
 * billed to it, along with a description string. These are displayed when
 * timer_DisplayClientTotals is called.
 * Invoke this at file or function scope; a (static) TimerClient pointer of
 * name <id> will be defined, which should be passed to TIMER_ACCRUE.
 **/
#define TIMER_ADD_CLIENT(id)\
	static TimerClient UID__;\
	static TimerClient* id = timer_AddClient(&UID__, #id);

/**
 * bill the difference between t0 and t1 to the client's total.
 **/
LIB_API void timer_BillClient(TimerClient* tc, TimerUnit t0, TimerUnit t1);

/**
 * display all clients' totals; does not reset them.
 * typically called at exit.
 **/
LIB_API void timer_DisplayClientTotals();

/// used by TIMER_ACCRUE
class LIB_API ScopeTimerAccrue
{
public:
	ScopeTimerAccrue(TimerClient* tc);
	~ScopeTimerAccrue();

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
 * 	TIMER_ADD_CLIENT(identifier)
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

#endif	// #ifndef INCLUDED_TIMER
