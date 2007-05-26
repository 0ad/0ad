/**
 * =========================================================================
 * File        : pmt.h
 * Project     : 0 A.D.
 * Description : Timer implementation using ACPI PM timer
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_PMT
#define INCLUDED_PMT

#include "tick_source.h"

static const i64 PMT_FREQ = 3579545;	// (= master oscillator frequency/4)

class TickSourcePmt : public TickSource
{
public:
	TickSourcePmt();
	virtual ~TickSourcePmt();

	virtual const char* Name() const
	{
		return "PMT";
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
	u16 m_portAddress;
	uint m_counterBits;
};

#endif	// #ifndef INCLUDED_PMT
