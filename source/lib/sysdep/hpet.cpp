/**
 * =========================================================================
 * File        : hpet.cpp
 * Project     : 0 A.D.
 * Description : HPET timer backend
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"

#include "hpet.h"
#include "acpi.h"
#include "win/mahaf.h"
#include "lib/bits.h"

#pragma pack(1)

struct HpetDescriptionTable
{
	AcpiTable header;
	u32 eventTimerBlockId;
	AcpiGenericAddress baseAddress;
	u8 sequenceNumber;
	u16 minimumPeriodicTicks;
	u8 attributes;
};

struct HpetRegisters
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

static volatile HpetRegisters* hpetRegisters;

static const u64 CONFIG_ENABLE = BIT64(0);



bool hpetInit()
{
	if(!acpiInit())
		return false;

	const HpetDescriptionTable* desc = (const HpetDescriptionTable*)acpiGetTable("HPET");
	debug_assert(desc->baseAddress.addressSpaceId == ACPI_AS_MEMORY);
	hpetRegisters = (volatile HpetRegisters*)MapPhysicalMemory(desc->baseAddress.address, sizeof(HpetRegisters));
	if(!hpetRegisters)
		return false;

	const u32 timerPeriod_fs = bits64(hpetRegisters->capabilities, 32, 63);
	const double freq = 1e15 / timerPeriod_fs;

	hpetRegisters->config &= ~CONFIG_ENABLE;
	hpetRegisters->counterValue = 0ull;
	hpetRegisters->config |= CONFIG_ENABLE;

	debug_printf("HPET freq=%f counter=%I64d\n", freq, hpetRegisters->counterValue);
}


void hpetShutdown()
{
	UnmapPhysicalMemory(hpetRegisters);
}
