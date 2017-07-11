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

#include "precompiled.h"
#include "lib/sysdep/arch/x86_x64/msr.h"

#include "lib/sysdep/os/win/mahaf.h"
#include "lib/sysdep/arch/x86_x64/x86_x64.h"


namespace MSR {

bool IsAccessible()
{
	if(!x86_x64::Cap(x86_x64::CAP_MSR))
		return false;

	// only read/writable from ring 0, so we need the driver.
	if(mahaf_Init() < 0)
		return false;

	return true;
}


bool HasEnergyPerfBias()
{
#if 1
	// the documentation is unclear. until it improves, disable
	// this, lest we provoke a GPF.
	return false;
#else
	if(x86_x64::Vendor() != x86_x64::VENDOR_INTEL)
		return false;

	if(x86_x64::Family() < 6)
		return false;

	if(x86_x64::Model() < 0xE)
		return false;

	return true;
#endif
}


bool HasPlatformInfo()
{
	if(x86_x64::Vendor() != x86_x64::VENDOR_INTEL)
		return false;

	if(x86_x64::Family() != 6)
		return false;

	switch(x86_x64::Model())
	{
	// section 34.4 in 253665-041US
	case x86_x64::MODEL_NEHALEM_EP:
	case x86_x64::MODEL_NEHALEM_EP_2:
	case x86_x64::MODEL_NEHALEM_EX:
	case x86_x64::MODEL_I7_I5:
		return true;

	// section 34.5
	case x86_x64::MODEL_CLARKDALE:
	case x86_x64::MODEL_WESTMERE_EP:
		return true;

	// section 34.6
	case x86_x64::MODEL_WESTMERE_EX:
		return true;

	// section 34.7
	case x86_x64::MODEL_SANDY_BRIDGE:
	case x86_x64::MODEL_SANDY_BRIDGE_2:
		return true;

	default:
		return false;
	}
}


bool HasUncore()
{
	if(x86_x64::Vendor() != x86_x64::VENDOR_INTEL)
		return false;

	if(x86_x64::Family() != 6)
		return false;

	switch(x86_x64::Model())
	{
	// Xeon 5500 / i7 (section B.4.1 in 253669-037US)
	case 0x1A:	// Bloomfield, Gainstown
	case 0x1E:	// Clarksfield, Lynnfield, Jasper Forest
	case 0x1F:
		return true;

	// Xeon 5600 / Westmere (section B.5)
	case 0x25:	// Clarkdale, Arrandale
	case 0x2C:	// Gulftown
		return true;

	default:
		return false;
	}
}


u64 Read(u64 reg)
{
	return mahaf_ReadModelSpecificRegister(reg);
}


void Write(u64 reg, u64 value)
{
	mahaf_WriteModelSpecificRegister(reg, value);
}

}	// namespace MSR
