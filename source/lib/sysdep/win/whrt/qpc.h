/**
 * =========================================================================
 * File        : qpc.h
 * Project     : 0 A.D.
 * Description : Timer implementation using QueryPerformanceCounter
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_QPC
#define INCLUDED_QPC

#include "tick_source.h"

class TickSourceQpc : public TickSource
{
public:
	TickSourceQpc();
	virtual ~TickSourceQpc();

	virtual const char* Name() const
	{
		return "QPC";
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
	// cached because QPF is a bit slow.
	// (i64 allows easier conversion to double)
	i64 m_frequency;
};

#endif	// #ifndef INCLUDED_QPC
