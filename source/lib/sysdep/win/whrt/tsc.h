/**
 * =========================================================================
 * File        : tsc.h
 * Project     : 0 A.D.
 * Description : Timer implementation using RDTSC
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_TSC
#define INCLUDED_TSC

#include "tick_source.h"

class TickSourceTsc : public TickSource
{
public:
	TickSourceTsc();
	~TickSourceTsc();

	virtual const char* Name() const
	{
		return "TSC";
	}

	virtual bool IsSafe() const;

	virtual u64 Ticks() const;

	/**
	 * WHRT uses this to ensure the counter (running at nominal frequency)
	 * doesn't overflow more than once during CALIBRATION_INTERVAL_MS.
	 **/
	virtual uint CounterBits() const;

	/**
	 * initial measurement of the tick rate. not necessarily correct
	 * (e.g. when using TSC: cpu_ClockFrequency isn't exact).
	 **/
	virtual double NominalFrequency() const;

private:
};

#endif	// #ifndef INCLUDED_TSC
