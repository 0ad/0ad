/**
 * =========================================================================
 * File        : pmt.cpp
 * Project     : 0 A.D.
 * Description : Timer implementation using ACPI PM timer
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "pmt.h"

#include "lib/sysdep/win/win.h"
#include "lib/sysdep/acpi.h"
#include "lib/sysdep/win/mahaf.h"
#include "lib/bits.h"

#pragma pack(push,1)
struct FADT
{
	AcpiTable header;
	u8 unused[40];
	u32 pmTimerPortAddress;
	u8 unused2[32];
	u32 flags;
};
#pragma pack(pop)

static const u32 TMR_VAL_EXT = BIT(8);

TickSourcePmt::TickSourcePmt()
{
	// (no need to check return value - valid fadt implies success)
	(void)mahaf_Init();
	(void)acpi_Init();
	const FADT* fadt = (const FADT*)acpi_GetTable("FADT");
	if(!fadt)
		throw TickSourceUnavailable("PMT: no FADT");
	m_portAddress = u16_from_larger(fadt->pmTimerPortAddress);
	m_counterBits = (fadt->flags & TMR_VAL_EXT)? 32 : 24;
}

TickSourcePmt::~TickSourcePmt()
{
	acpi_Shutdown();
	mahaf_Shutdown();
}

bool TickSourcePmt::IsSafe() const
{
return false;

	// the PMT has one issue: "Performance counter value may unexpectedly
	// leap forward" (Q274323). This happens on some buggy Pentium-era
	// systems under heavy PCI bus load. We are clever and observe that
	// the TSC implementation would be used on such systems (because it
	// has higher precedence and is safe on P5 CPUs), so the PMT is fine
	// in general.
	return true;
}

u64 TickSourcePmt::Ticks() const
{
	u32 ticks = mahaf_ReadPort32(m_portAddress);
	// note: the spec allows 24 or 32 bit counters. given the fixed
	// frequency of 3.57 MHz, worst case is rollover within 4.6 s. this is
	// obviously not long enough to never happen more than once, so it must
	// be handled; there is no benefit in using all 32 bits where available.
	ticks &= 0xFFFFFFu;
	return (u64)ticks;
}

/**
 * WHRT uses this to ensure the counter (running at nominal frequency)
 * doesn't overflow more than once during CALIBRATION_INTERVAL_MS.
 **/
uint TickSourcePmt::CounterBits() const
{
	return m_counterBits;
}

/**
 * initial measurement of the tick rate. not necessarily correct
 * (e.g. when using TSC: cpu_ClockFrequency isn't exact).
 **/
double TickSourcePmt::NominalFrequency() const
{
	return (double)PMT_FREQ;
}
