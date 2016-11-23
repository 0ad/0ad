/* Copyright (c) 2011 Wildfire Games
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
#include "lib/sysdep/arch/x86_x64/apic.h"

#include "lib/bits.h"
#include "lib/module_init.h"
#include "lib/sysdep/cpu.h"	// ERR::CPU_FEATURE_MISSING
#include "lib/sysdep/os_cpu.h"
#include "lib/sysdep/arch/x86_x64/x86_x64.h"


ApicId GetApicId()
{
	x86_x64::CpuidRegs regs = { 0 };
	regs.eax = 1;
	// note: CPUID function 1 is always supported, but only processors with
	// an xAPIC (e.g. P4/Athlon XP) will return a nonzero ID.
	bool ok = x86_x64::cpuid(&regs);
	ASSERT(ok); UNUSED2(ok);
	const u8 apicId = (u8)bits(regs.ebx, 24, 31);
	return apicId;
}


static size_t numIds;
static ApicId processorApicIds[os_cpu_MaxProcessors];
static ApicId sortedApicIds[os_cpu_MaxProcessors];

static Status GetAndValidateApicIds()
{
	numIds = os_cpu_NumProcessors();
	struct StoreEachProcessorsApicId
	{
		static void Callback(size_t processor, uintptr_t UNUSED(data))
		{
			processorApicIds[processor] = GetApicId();
		}
	};
	// (can fail due to restrictions on our process affinity or lack of
	// support for affinity masks in OS X.)
	RETURN_STATUS_IF_ERR(os_cpu_CallByEachCPU(StoreEachProcessorsApicId::Callback, 0));

	std::copy(processorApicIds, processorApicIds+numIds, sortedApicIds);
	std::sort(sortedApicIds, sortedApicIds+numIds);
	ApicId* const end = std::unique(sortedApicIds, sortedApicIds+numIds);
	const size_t numUnique = end-sortedApicIds;

	// all IDs are zero - system lacks an xAPIC.
	// (NB: we exclude single-processor systems in this test -
	// having one zero-valued ID is legitimate)
	if(numUnique == 1 && sortedApicIds[0] == 0 && numIds != 1)
	{
		debug_printf("APIC: all zero\n");
		return ERR::CPU_FEATURE_MISSING;	// NOWARN
	}

	// not all unique - probably running in a VM whose emulation is
	// imperfect or doesn't allow access to all processors.
	if(numUnique != numIds)
	{
		debug_printf("APIC: not unique\n");
		return ERR::FAIL;	// NOWARN
	}

	return INFO::OK;
}

static Status InitApicIds()
{
	const Status status = GetAndValidateApicIds();
	if(status < 0)	// failed
	{
		// generate fake but legitimate APIC IDs
		for(size_t processor = 0; processor < numIds; processor++)
			processorApicIds[processor] = sortedApicIds[processor] = (ApicId)processor;
	}

	return status;
}

static ModuleInitState apicInitState;


bool AreApicIdsReliable()
{
	ModuleInit(&apicInitState, InitApicIds);
	if(apicInitState < 0)
		return false;
	return true;
}


static size_t IndexFromApicId(const ApicId* apicIds, ApicId apicId)
{
	ModuleInit(&apicInitState, InitApicIds);

	const ApicId* pos = std::find(apicIds, apicIds+numIds, apicId);
	if(pos == apicIds+numIds)
	{
		DEBUG_WARN_ERR(ERR::LOGIC);
		return 0;
	}

	const size_t index = pos - apicIds;
	return index;
}

size_t ProcessorFromApicId(ApicId apicId)
{
	return IndexFromApicId(processorApicIds, apicId);
}

size_t ContiguousIdFromApicId(ApicId apicId)
{
	return IndexFromApicId(sortedApicIds, apicId);
}


static ApicId ApicIdFromIndex(const ApicId* apicIds, size_t index)
{
	ModuleInit(&apicInitState, InitApicIds);
	ASSERT(index < numIds);
	return apicIds[index];
}

ApicId ApicIdFromProcessor(size_t processor)
{
	return ApicIdFromIndex(processorApicIds, processor);
}

ApicId ApicIdFromContiguousId(size_t contiguousId)
{
	return ApicIdFromIndex(sortedApicIds, contiguousId);
}
