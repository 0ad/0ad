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

#include "counter.h"

class CounterQPC : public ICounter
{
public:
	CounterQPC()
	: m_frequency(-1)
	{
	}

	virtual const char* Name() const
	{
		return "QPC";
	}

	virtual LibError Activate();
	virtual void Shutdown();

	virtual bool IsSafe() const;

	virtual u64 Counter() const;

	/**
	 * WHRT uses this to ensure the counter (running at nominal frequency)
	 * doesn't overflow more than once during CALIBRATION_INTERVAL_MS.
	 **/
	virtual size_t CounterBits() const;

	/**
	 * initial measurement of the tick rate. not necessarily correct
	 * (e.g. when using TSC: cpu_ClockFrequency isn't exact).
	 **/
	virtual double NominalFrequency() const;

private:
	// used in several places and QPF is a bit slow+cumbersome.
	// (i64 allows easier conversion to double)
	i64 m_frequency;
};

#endif	// #ifndef INCLUDED_QPC
