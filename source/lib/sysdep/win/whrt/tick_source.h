/**
 * =========================================================================
 * File        : tick_source.h
 * Project     : 0 A.D.
 * Description : Interface for timer implementations
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_TICK_SOURCE
#define INCLUDED_TICK_SOURCE

class TickSourceUnavailable : public std::runtime_error
{
public:
	TickSourceUnavailable(const std::string& msg)
	: std::runtime_error(msg)
	{
	}
};

class TickSource
{
public:
	TickSource() {}
	virtual ~TickSource() {}

	virtual bool IsSafe() const = 0;

	virtual const char* Name() const = 0;

	virtual u64 Ticks() const = 0;

	/**
	 * WHRT uses this to ensure the counter (running at nominal frequency)
	 * doesn't overflow more than once during CALIBRATION_INTERVAL_MS.
	 **/
	virtual uint CounterBits() const = 0;

	/**
	 * initial measurement of the tick rate. not necessarily correct
	 * (e.g. when using TSC: cpu_ClockFrequency isn't exact).
	 **/
	virtual double NominalFrequency() const = 0;

	/**
	 * actual resolution [s]
	 * (override if the timer adjustment is greater than 1 tick).
	 **/
	virtual double Resolution() const
	{
		return 1.0 / NominalFrequency();
	}
};

#endif	// #ifndef INCLUDED_TICK_SOURCE
