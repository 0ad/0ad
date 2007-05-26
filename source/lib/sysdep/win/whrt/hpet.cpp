/**
 * =========================================================================
 * File        : hpet.cpp
 * Project     : 0 A.D.
 * Description : Timer implementation using timeGetTime
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "hpet.h"

#include "lib/sysdep/win/win.h"
#include "lib/sysdep/win/mahaf.h"
#include "lib/sysdep/acpi.h"
#include "lib/bits.h"

#pragma pack(push, 1)

struct HpetDescriptionTable
{
	AcpiTable header;
	u32 eventTimerBlockId;
	AcpiGenericAddress baseAddress;
	u8 sequenceNumber;
	u16 minimumPeriodicTicks;
	u8 attributes;
};

struct TickSourceHpet::HpetRegisters
{
	u64 capabilities;
	u64 reserved1;
	u64 config;
	u64 reserved2;
	u64 interruptStatus;
	u64 reserved3[25];
	u64 counterValue;
	u64 reserved4;

	// .. followed by blocks for timers 0..31
};

#pragma pack(pop)

static const u64 CAP_SIZE64 = BIT64(13);

static const u64 CONFIG_ENABLE = BIT64(0);


//-----------------------------------------------------------------------------

TickSourceHpet::TickSourceHpet()
{
	// (no need to check return value - valid hpet implies success)
	(void)mahaf_Init();
	(void)acpi_Init();
	const HpetDescriptionTable* hpet = (const HpetDescriptionTable*)acpi_GetTable("HPET");
	if(!hpet)
		throw TickSourceUnavailable("HPET: no ACPI table");
	debug_assert(hpet->baseAddress.addressSpaceId == ACPI_AS_MEMORY);
	m_hpetRegisters = (volatile HpetRegisters*)mahaf_MapPhysicalMemory(hpet->baseAddress.address, sizeof(HpetRegisters));
	if(!m_hpetRegisters)
		throw TickSourceUnavailable("HPET: map failed");

	// get counter parameters
	const u64 caps = m_hpetRegisters->capabilities;
	const u32 timerPeriod_fs = bits64(caps, 32, 63);
	m_frequency = 1e15 / timerPeriod_fs;
	m_counterBits = (caps & CAP_SIZE64)? 64 : 32;

	// start the counter (if not already running)
	// note: do not reset value to 0 to avoid interfering with any
	// other users of the timer (e.g. Vista QPC)
	m_hpetRegisters->config |= CONFIG_ENABLE;
}

TickSourceHpet::~TickSourceHpet()
{
	mahaf_UnmapPhysicalMemory((void*)m_hpetRegisters);

	mahaf_Shutdown();

	acpi_Shutdown();
}

bool TickSourceHpet::IsSafe() const
{
return false;

	// the HPET being created to address other timers' problems, it has
	// no issues of its own.
	return true;
}

u64 TickSourceHpet::Ticks() const
{
	u64 ticks = m_hpetRegisters->counterValue;
	// note: the spec allows 32 or 64 bit counters. given the typical
	// frequency of 14.318 MHz, worst case is rollover within 300 s. this is
	// obviously not long enough to never happen more than once, so it must
	// be handled; there is no benefit in using all 64 bits where available.
	//
	// note that limiting ourselves to 32 bits also avoids the potential
	// headache of non-atomic bus reads.
	ticks &= 0xFFFFFFFFu;
	return ticks;
}

/**
 * WHRT uses this to ensure the counter (running at nominal frequency)
 * doesn't overflow more than once during CALIBRATION_INTERVAL_MS.
 **/
uint TickSourceHpet::CounterBits() const
{
	return 32;
}

/**
 * initial measurement of the tick rate. not necessarily correct
 * (e.g. when using TSC: cpu_ClockFrequency isn't exact).
 **/
double TickSourceHpet::NominalFrequency() const
{
	return m_frequency;
}
