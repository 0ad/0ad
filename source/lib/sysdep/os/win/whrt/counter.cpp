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
 * Interface for counter implementations
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/whrt/counter.h"

#include "lib/alignment.h"
#include "lib/sysdep/cpu.h"	// cpu_CAS

#include "lib/sysdep/os/win/whrt/tsc.h"
#include "lib/sysdep/os/win/whrt/hpet.h"
#include "lib/sysdep/os/win/whrt/pmt.h"
#include "lib/sysdep/os/win/whrt/qpc.h"
#include "lib/sysdep/os/win/whrt/tgt.h"
// to add a new counter type, simply include its header here and
// insert a case in ConstructCounterAt's switch statement.


//-----------------------------------------------------------------------------
// create/destroy counters

/**
 * @param id
 * @param address
 * @param size Maximum allowable size [bytes] of the subclass instance
 * @return pointer to a newly constructed ICounter subclass of type \<id\> at
 *		   the given address, or 0 iff the ID is invalid.
 **/
static ICounter* ConstructCounterAt(size_t id, void* address, size_t size)
{
	// rationale for placement new: see call site.

	// counters are chosen according to the following order. rationale:
	// - TSC must come before QPC and PMT to make sure a bug in the latter on
	//   Pentium systems doesn't come up.
	// - PMT works, but is inexplicably slower than QPC on a PIII Mobile.
	// - TGT really isn't as safe as the others, so it should be last.
	// - low-overhead and high-resolution counters are preferred.
	switch(id)
	{
	case 0:
		return CreateCounterHPET(address, size);
	case 1:
		return CreateCounterTSC(address, size);
	case 2:
		return CreateCounterQPC(address, size);
	case 3:
		return CreateCounterPMT(address, size);
	case 4:
		return CreateCounterTGT(address, size);
	default:
		return 0;
	}
}


static volatile intptr_t isCounterAllocated;

ICounter* CreateCounter(size_t id)
{
	// we placement-new the Counter classes in a static buffer.
	// this is dangerous, but we are careful to ensure alignment. it is
	// unusual and thus bad, but there's also one advantage: we avoid
	// using global operator new before the CRT is initialized (risky).
	//
	// alternatives:
	// - defining as static doesn't work because the ctors (necessary for
	//   vptr initialization) run during _cinit, which comes after our
	//   first use of them.
	// - using static_calloc isn't possible because we don't know the
	//   size until after the alloc / placement new.

	if(!cpu_CAS(&isCounterAllocated, 0, 1))
		DEBUG_WARN_ERR(ERR::LOGIC);	// static counter memory is already in use!

	static const size_t memSize = 200;
	static u8 mem[memSize];
	u8* alignedMem = (u8*)Align<16>((uintptr_t)mem);
	const size_t bytesLeft = mem+memSize - alignedMem;
	ICounter* counter = ConstructCounterAt(id, alignedMem, bytesLeft);

	return counter;
}


void DestroyCounter(ICounter*& counter)
{
	ENSURE(counter);
	counter->Shutdown();
	counter->~ICounter();	// must be called due to placement new
	counter = 0;

	isCounterAllocated = 0;
}
