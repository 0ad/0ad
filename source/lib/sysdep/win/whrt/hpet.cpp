/**
 * =========================================================================
 * File        : hpet.cpp
 * Project     : 0 A.D.
 * Description : Timer implementation using High Precision Event Timer
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

struct CounterHPET::HpetRegisters
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

LibError CounterHPET::Activate()
{
	if(mahaf_IsPhysicalMappingDangerous())
		return ERR::FAIL;	// NOWARN (happens on Win2k)
	if(!mahaf_Init())
		return ERR::FAIL;	// NOWARN (no Administrator privileges)
	if(!acpi_Init())
		WARN_RETURN(ERR::FAIL);	// shouldn't fail, since we've checked mahaf_IsPhysicalMappingDangerous
	const HpetDescriptionTable* hpet = (const HpetDescriptionTable*)acpi_GetTable("HPET");
	if(!hpet)
		return ERR::NO_SYS;	// NOWARN (HPET not reported by BIOS)
	debug_assert(hpet->baseAddress.addressSpaceId == ACPI_AS_MEMORY);
	m_hpetRegisters = (volatile HpetRegisters*)mahaf_MapPhysicalMemory(hpet->baseAddress.address, sizeof(HpetRegisters));
	if(!m_hpetRegisters)
		WARN_RETURN(ERR::NO_MEM);

	// start the counter (if not already running)
	// note: do not reset value to 0 to avoid interfering with any
	// other users of the timer (e.g. Vista QPC)
	m_hpetRegisters->config |= CONFIG_ENABLE;

	return INFO::OK;
}

void CounterHPET::Shutdown()
{
	if(m_hpetRegisters)
		mahaf_UnmapPhysicalMemory((void*)m_hpetRegisters);

	acpi_Shutdown();
	mahaf_Shutdown();
}

bool CounterHPET::IsSafe() const
{
	// the HPET having been created to address other timers' problems,
	// it has no issues of its own.
	return true;
}

u64 CounterHPET::Counter() const
{
	// note: we assume the data bus can do atomic 64-bit transfers,
	// which has been the case since the original Pentium.
	// (note: see implementation of GetTickCount for an algorithm to
	// cope with non-atomic reads)
	return m_hpetRegisters->counterValue;
}

/**
 * WHRT uses this to ensure the counter (running at nominal frequency)
 * doesn't overflow more than once during CALIBRATION_INTERVAL_MS.
 **/
size_t CounterHPET::CounterBits() const
{
	const u64 caps = m_hpetRegisters->capabilities;
	const size_t counterBits = (caps & CAP_SIZE64)? 64 : 32;
	return counterBits;
}

/**
 * initial measurement of the tick rate. not necessarily correct
 * (e.g. when using TSC: cpu_ClockFrequency isn't exact).
 **/
double CounterHPET::NominalFrequency() const
{
	const u64 caps = m_hpetRegisters->capabilities;
	const u32 timerPeriod_fs = (u32)bits(caps, 32, 63);
	const double frequency = 1e15 / timerPeriod_fs;
	return frequency;
}
