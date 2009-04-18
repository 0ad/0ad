/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Timer implementation using High Precision Event Timer
 */

#include "precompiled.h"
#include "hpet.h"

#include "counter.h"

#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/mahaf.h"
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

#pragma pack(pop)

static const u64 CAP_SIZE64 = Bit<u64>(13);

static const u64 CONFIG_ENABLE = Bit<u64>(0);


//-----------------------------------------------------------------------------

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

	LibError Activate()
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

	void Shutdown()
	{
		if(m_hpetRegisters)
		{
			mahaf_UnmapPhysicalMemory((void*)m_hpetRegisters);
			m_hpetRegisters = 0;
		}

		acpi_Shutdown();
		mahaf_Shutdown();
	}

	bool IsSafe() const
	{
		// the HPET having been created to address other timers' problems,
		// it has no issues of its own.
		return true;
	}

	u64 Counter() const
	{
		// note: we assume the data bus can do atomic 64-bit transfers,
		// which has been the case since the original Pentium.
		// (note: see implementation of GetTickCount for an algorithm to
		// cope with non-atomic reads)
		return m_hpetRegisters->counterValue;
	}

	size_t CounterBits() const
	{
		const u64 caps = m_hpetRegisters->capabilities;
		const size_t counterBits = (caps & CAP_SIZE64)? 64 : 32;
		return counterBits;
	}

	double NominalFrequency() const
	{
		const u64 caps = m_hpetRegisters->capabilities;
		const u32 timerPeriod_fs = (u32)bits(caps, 32, 63);
		debug_assert(timerPeriod_fs != 0);	// guaranteed by HPET spec
		const double frequency = 1e15 / timerPeriod_fs;
		return frequency;
	}

	double Resolution() const
	{
		return 1.0 / NominalFrequency();
	}

private:
	volatile HpetRegisters* m_hpetRegisters;
};

ICounter* CreateCounterHPET(void* address, size_t size)
{
	debug_assert(sizeof(CounterHPET) <= size);
	return new(address) CounterHPET();
}
