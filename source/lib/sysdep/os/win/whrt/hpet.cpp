/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Timer implementation using High Precision Event Timer
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/whrt/hpet.h"

// for atomic 64-bit read/write:
#define HAVE_X64_MOVD ARCH_AMD64 && (ICC_VERSION || MSC_VERSION >= 1500)
#if HAVE_X64_MOVD
# include <intrin.h>
#else
# include <emmintrin.h>
#endif

#include "lib/sysdep/os/win/whrt/counter.h"

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

	Status Activate()
	{
		RETURN_STATUS_IF_ERR(MapRegisters(m_hpetRegisters));

		RETURN_STATUS_IF_ERR(VerifyCapabilities(m_frequency, m_counterBits));

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

	static Status MapRegisters(volatile void*& registers)
	{
		if(mahaf_IsPhysicalMappingDangerous())
			return ERR::FAIL;	// NOWARN (happens on Win2k)
		RETURN_STATUS_IF_ERR(mahaf_Init());	// (fails without Administrator privileges)

		const HpetDescriptionTable* hpet = (const HpetDescriptionTable*)acpi_GetTable("HPET");
		if(!hpet)
			return ERR::NOT_SUPPORTED;	// NOWARN (HPET not reported by BIOS)

		if(hpet->baseAddress.addressSpaceId != ACPI_AS_MEMORY)
			return ERR::NOT_SUPPORTED;	// NOWARN (happens on some BIOSes)
		// hpet->baseAddress.accessSize is reserved
		const uintptr_t address = uintptr_t(hpet->baseAddress.address);
		ENSURE(address % 8 == 0);	// "registers are generally aligned on 64-bit boundaries"

		registers = mahaf_MapPhysicalMemory(address, MAX_OFFSET+1);
		if(!registers)
			WARN_RETURN(ERR::NO_MEM);

		return INFO::OK;
	}

	// note: this is atomic even on 32-bit CPUs (Pentium MMX and
	// above have a 64-bit data bus and MOVQ instruction)
	u64 Read64(size_t offset) const
	{
		ENSURE(offset <= MAX_OFFSET);
		ENSURE(offset % 8 == 0);
		const uintptr_t address = uintptr_t(m_hpetRegisters)+offset;
		const __m128i value128 = _mm_loadl_epi64((__m128i*)address);
#if HAVE_X64_MOVD
		return _mm_cvtsi128_si64x(value128);
#else
		__declspec(align(16)) u32 values[4];
		_mm_store_si128((__m128i*)values, value128);
		return u64_from_u32(values[1], values[0]);
#endif
	}

	void Write64(size_t offset, u64 value) const
	{
		ENSURE(offset <= MAX_OFFSET);
		ENSURE(offset % 8 == 0);
		ENSURE(offset != CAPS_AND_ID);	// can't write to read-only registers
		const uintptr_t address = uintptr_t(m_hpetRegisters)+offset;
#if HAVE_X64_MOVD
		const __m128i value128 = _mm_cvtsi64x_si128(value);
#else
		const __m128i value128 = _mm_set_epi32(0, 0, int(value >> 32), int(value & 0xFFFFFFFF));
#endif
		_mm_storel_epi64((__m128i*)address, value128);
	}

	Status VerifyCapabilities(double& frequency, u32& counterBits) const
	{
		// AMD document 43366 indicates the clock generator that drives the
		// HPET is "spread-capable". Wikipedia's frequency hopping article
		// explains that this reduces electromagnetic interference.
		// The AMD document recommends BIOS writers add SMM hooks for
		// reporting the resulting slightly different frequency.
		// This apparently requires calibration triggered when the HPET is
		// accessed, during which the config register is -1. We'll wait
		// about 1 ms (MMIO is expected to take at least 1 us) and
		// then ensure the HPET timer period is within reasonable bounds.
		u64 caps_and_id = Read64(CAPS_AND_ID);
		for(size_t reps = 0; reps < 1000; reps++)
		{
			if(caps_and_id != ~u64(0))	// register seems valid
				break;
			caps_and_id = Read64(CAPS_AND_ID);
		}

		const u8 revision = (u8)bits(caps_and_id, 0, 7);
		ENSURE(revision != 0);	// "the value must NOT be 00h"
		counterBits = (caps_and_id & Bit<u64>(13))? 64 : 32;
		const u16 vendorID = (u16)bits(caps_and_id, 16, 31);
		const u32 period_fs = (u32)bits(caps_and_id, 32, 63);
		ENSURE(period_fs != 0);	// "a value of 0 in this field is not permitted"
		frequency = 1e15 / period_fs;
		debug_printf("HPET: rev=%X vendor=%X bits=%d period=%08X freq=%g\n", revision, vendorID, counterBits, period_fs, frequency);

		if(period_fs > 0x05F5E100)	// 100 ns (spec guarantees >= 10 MHz)
			return ERR::CORRUPTED;	// avoid using HPET (e.g. if calibration was still in progress)

		return INFO::OK;
	}

	volatile void* m_hpetRegisters;
	double m_frequency;
	u32 m_counterBits;
};

ICounter* CreateCounterHPET(void* address, size_t size)
{
	ENSURE(sizeof(CounterHPET) <= size);
	return new(address) CounterHPET();
}
