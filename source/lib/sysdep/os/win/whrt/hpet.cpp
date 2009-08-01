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

#include <emmintrin.h>	// for atomic 64-bit read/write

#include "counter.h"

#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/mahaf.h"
#include "lib/sysdep/acpi.h"
#include "lib/bits.h"


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
		RETURN_ERR(MapRegisters(m_hpetRegisters));

		// retrieve capabilities and ID
		{
			const u64 caps_and_id = Read64(CAPS_AND_ID);
			const u8 revision = bits(caps_and_id, 0, 7);
			debug_assert(revision != 0);	// "the value must NOT be 00h"
			m_counterBits = (caps_and_id & Bit<u64>(13))? 64 : 32;
			const u16 vendorID = bits(caps_and_id, 16, 31);
			const u32 period_fs = (u32)bits(caps_and_id, 32, 63);
			debug_assert(period_fs != 0);	// "a value of 0 in this field is not permitted"
			debug_assert(period_fs <= 0x05F5E100);	// 100 ns (min freq is 10 MHz)
			m_frequency = 1e15 / period_fs;
			debug_printf("HPET: rev=%X vendor=%X bits=%d period=%X freq=%g\n", revision, vendorID, m_counterBits, period_fs, m_frequency);
		}

		// start the counter (if not already running)
		Write64(CONFIG, Read64(CONFIG)|1);
		// note: to avoid interfering with any other users of the timer
		// (e.g. Vista QPC), we don't reset the counter value to 0.

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
		// notes:
		// - Read64 is atomic and avoids race conditions.
		// - 32-bit counters (m_counterBits == 32) still allow
		//   reading the whole register (the upper bits are zero).
		return Read64(COUNTER_VALUE);
	}

	size_t CounterBits() const
	{
		return m_counterBits;
	}

	double NominalFrequency() const
	{
		return m_frequency;
	}

	double Resolution() const
	{
		return 1.0 / m_frequency;
	}

private:
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

#pragma pack(pop)

	enum RegisterOffsets
	{
		CAPS_AND_ID   = 0x00,
		CONFIG        = 0x10,
		COUNTER_VALUE = 0xF0,
		MAX_OFFSET    = 0x3FF
	};

	static LibError MapRegisters(volatile void*& registers)
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

		if(hpet->baseAddress.addressSpaceId != ACPI_AS_MEMORY)
			return ERR::NOT_SUPPORTED;	// NOWARN (happens on some BIOSes)
		// hpet->baseAddress.accessSize is reserved
		const uintptr_t address = uintptr_t(hpet->baseAddress.address);
		debug_assert(address % 8 == 0);	// "registers are generally aligned on 64-bit boundaries"

		registers = mahaf_MapPhysicalMemory(address, MAX_OFFSET+1);
		if(!registers)
			WARN_RETURN(ERR::NO_MEM);

		return INFO::OK;
	}

	// note: this is atomic even on 32-bit CPUs (Pentium MMX and
	// above have a 64-bit data bus and MOVQ instruction)
	u64 Read64(size_t offset) const
	{
		debug_assert(offset <= MAX_OFFSET);
		debug_assert(offset % 8 == 0);
		const uintptr_t address = uintptr_t(m_hpetRegisters)+offset;
		const __m128i value128 = _mm_loadl_epi64((__m128i*)address);
#if ARCH_AMD64
		return _mm_cvtsi128_si64x(value128);
#else
		return u64_from_u32(value128.m128i_u32[1], value128.m128i_u32[0]);
#endif
	}

	void Write64(size_t offset, u64 value) const
	{
		debug_assert(offset <= MAX_OFFSET);
		debug_assert(offset % 8 == 0);
		debug_assert(offset != CAPS_AND_ID);	// can't write to read-only registers
		const uintptr_t address = uintptr_t(m_hpetRegisters)+offset;
#if ARCH_AMD64
		const __m128i value128 = _mm_cvtsi64x_si128(value);
#else
		const __m128i value128 = _mm_set_epi32(0, 0, int(value >> 32), int(value & 0xFFFFFFFF));
#endif
		_mm_storel_epi64((__m128i*)address, value128);
	}

	volatile void* m_hpetRegisters;
	double m_frequency;
	u32 m_counterBits;
};

ICounter* CreateCounterHPET(void* address, size_t size)
{
	debug_assert(sizeof(CounterHPET) <= size);
	return new(address) CounterHPET();
}
