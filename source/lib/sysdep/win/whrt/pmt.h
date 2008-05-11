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

#include "counter.h"

static const i64 PMT_FREQ = 3579545;	// (= master oscillator frequency/4)

class CounterPMT : public ICounter
{
public:
	CounterPMT()
	: m_portAddress(0xFFFF)
	{
	}

	virtual const char* Name() const
	{
		return "PMT";
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
	u16 m_portAddress;
};

#endif	// #ifndef INCLUDED_PMT
