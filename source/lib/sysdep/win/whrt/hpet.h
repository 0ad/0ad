/**
 * =========================================================================
 * File        : hpet.h
 * Project     : 0 A.D.
 * Description : Timer implementation using timeGetTime
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_HPET
#define INCLUDED_HPET

#include "tick_source.h"


class TickSourceHpet : public TickSource
{
public:
	TickSourceHpet();
	virtual ~TickSourceHpet();

	virtual const char* Name() const
	{
		return "HPET";
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
	double m_frequency;
	struct HpetRegisters;
	volatile HpetRegisters* m_hpetRegisters;
	uint m_counterBits;
};

#endif	// #ifndef INCLUDED_HPET
