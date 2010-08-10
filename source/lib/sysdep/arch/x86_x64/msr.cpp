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

#include "precompiled.h"
#include "lib/sysdep/arch/x86_x64/msr.h"

#include "lib/sysdep/os/win/mahaf.h"
#include "lib/sysdep/arch/x86_x64/x86_x64.h"


namespace MSR {

bool IsAccessible()
{
	if(!x86_x64_Cap(X86_X64_CAP_MSR))
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
	if(x86_x64_Vendor() != X86_X64_VENDOR_INTEL)
		return false;

	if(x86_x64_Family() < 6)
		return false;

	if(x86_x64_Model() < 0xE)
		return false;

	return true;
#endif
}


bool HasNehalem()
{
	if(x86_x64_Vendor() != X86_X64_VENDOR_INTEL)
		return false;

	if(x86_x64_Family() != 6)
		return false;

	switch(x86_x64_Model())
	{
	// Nehalem (documented in 253669-035US B.4.1)
	case 0x1A:	// Bloomfield, Gainstown
	case 0x1E:	// Clarksfield, Lynnfield, Jasper Forest
	case 0x1F:
		return true;

	// Westmere (documented in 253669-035US B.5)
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
