/**
 * =========================================================================
 * File        : hpet.h
 * Project     : 0 A.D.
 * Description : Timer implementation using High Precision Event Timer
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_HPET
#define INCLUDED_HPET

#include "counter.h"


class CounterHPET : public ICounter
{
public:
	CounterHPET()
	: m_hpetRegisters(0)
	{
	}

	virtual const char* Name() const
	{
		return "HPET";
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

private:
	struct HpetRegisters;
	volatile HpetRegisters* m_hpetRegisters;
};

#endif	// #ifndef INCLUDED_HPET
