/**
 * =========================================================================
 * File        : counter.cpp
 * Project     : 0 A.D.
 * Description : Interface for counter implementations
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "counter.h"

#include "lib/bits.h"
#include "lib/sysdep/cpu.h"	// cpu_CAS

#include "tsc.h"
#include "hpet.h"
#include "pmt.h"
#include "qpc.h"
#include "tgt.h"
// to add a new counter type, simply include its header here and
// insert a case in ConstructCounterAt's switch statement.


//-----------------------------------------------------------------------------
// create/destroy counters

/**
 * @return pointer to a newly constructed ICounter subclass of type <id> at
 * the given address, or 0 iff the ID is invalid.
 * @param size maximum allowable size [bytes] of the subclass instance
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


static volatile uintptr_t isCounterAllocated;

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
		debug_assert(0);	// static counter memory is already in use!

	static const size_t memSize = 200;
	static u8 mem[memSize];
	u8* alignedMem = (u8*)round_up((uintptr_t)mem, (uintptr_t)16u);
	const size_t bytesLeft = mem+memSize - alignedMem;
	ICounter* counter = ConstructCounterAt(id, alignedMem, bytesLeft);

	return counter;
}


void DestroyCounter(ICounter*& counter)
{
	debug_assert(counter);
	counter->Shutdown();
	counter->~ICounter();	// must be called due to placement new
	counter = 0;

	isCounterAllocated = 0;
}
