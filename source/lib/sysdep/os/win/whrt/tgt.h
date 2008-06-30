/**
 * =========================================================================
 * File        : tgt.h
 * Project     : 0 A.D.
 * Description : Timer implementation using timeGetTime
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_TGT
#define INCLUDED_TGT

#include "counter.h"

class CounterTGT : public ICounter
{
public:
	virtual const char* Name() const
	{
		return "TGT";
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
	 * (e.g. when using TSC: os_cpu_ClockFrequency isn't exact).
	 **/
	virtual double NominalFrequency() const;

	/**
	 * actual resolution [s]
	 **/
	virtual double Resolution() const;
};

#endif	// #ifndef INCLUDED_TGT
