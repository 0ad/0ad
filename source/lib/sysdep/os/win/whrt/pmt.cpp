/* Copyright (C) 2010 Wildfire Games.
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
 * Timer implementation using ACPI PM timer
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/whrt/pmt.h"

#include "lib/sysdep/os/win/whrt/counter.h"

#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/acpi.h"
#include "lib/sysdep/os/win/mahaf.h"
#include "lib/bits.h"

static const u32 TMR_VAL_EXT = Bit<u32>(8);	// FADT flags

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

	Status Activate()
	{
		// mahaf is needed for port I/O.
		RETURN_STATUS_IF_ERR(mahaf_Init());	// (fails without Administrator privileges)
		// (note: it's called FADT, but the signature is "FACP")
		const FADT* fadt = (const FADT*)acpi_GetTable("FACP");
		if(!fadt)
			return ERR::NOT_SUPPORTED;	// NOWARN (ACPI tables might not be available)
		m_portAddress = u16_from_larger(fadt->pmTimerPortAddress);

		return INFO::OK;
	}

	void Shutdown()
	{
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
		ENSURE(fadt);	// Activate made sure FADT is available
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
	ENSURE(sizeof(CounterPMT) <= size);
	return new(address) CounterPMT();
}
