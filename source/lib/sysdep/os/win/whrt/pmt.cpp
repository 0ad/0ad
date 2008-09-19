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

#include "counter.h"

#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/acpi.h"
#include "lib/sysdep/os/win/mahaf.h"
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

static const u32 TMR_VAL_EXT = Bit<u32>(8);

//-----------------------------------------------------------------------------


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

	LibError Activate()
	{
		// mahaf is needed for port I/O.
		if(!mahaf_Init())
			return ERR::FAIL;	// NOWARN (no Administrator privileges)
		if(!acpi_Init())
			return ERR::FAIL;	// NOWARN (happens on Win2k; see mahaf_IsPhysicalMappingDangerous)
		// (note: it's called FADT, but the signature is "FACP")
		const FADT* fadt = (const FADT*)acpi_GetTable("FACP");
		if(!fadt)
			WARN_RETURN(ERR::NO_SYS);
		m_portAddress = u16_from_larger(fadt->pmTimerPortAddress);

		return INFO::OK;
	}

	void Shutdown()
	{
		acpi_Shutdown();
		mahaf_Shutdown();
	}

	bool IsSafe() const
	{
		// the PMT has one issue: "Performance counter value may unexpectedly
		// leap forward" (Q274323). This happens on some buggy Pentium-era
		// systems under heavy PCI bus load. We are clever and observe that
		// the TSC implementation would be used on such systems (because it
		// has higher precedence and is safe on P5 CPUs), so the PMT is fine
		// in general.
		return true;
	}

	u64 Counter() const
	{
		return mahaf_ReadPort32(m_portAddress);
	}

	size_t CounterBits() const
	{
		// (see previous acpi_GetTable call)
		const FADT* fadt = (const FADT*)acpi_GetTable("FACP");
		debug_assert(fadt);	// Activate made sure FADT is available
		const size_t counterBits = (fadt->flags & TMR_VAL_EXT)? 32 : 24;
		return counterBits;
	}

	double NominalFrequency() const
	{
		return (double)PMT_FREQ;
	}

	double Resolution() const
	{
		return 1.0 / PMT_FREQ;
	}

private:
	u16 m_portAddress;
};

ICounter* CreateCounterPMT(void* address, size_t size)
{
	debug_assert(sizeof(CounterPMT) <= size);
	return new(address) CounterPMT();
}
