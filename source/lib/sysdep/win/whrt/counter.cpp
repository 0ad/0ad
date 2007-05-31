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
 * @param size receives the size [bytes] of the created instance.
 **/
static ICounter* ConstructCounterAt(uint id, void* address, size_t& size)
{
	// rationale for placement new: see call site.
#define CREATE(impl)\
	size = sizeof(Counter##impl);\
	return new(address) Counter##impl();

#include "lib/nommgr.h"	// MMGR interferes with placement new

	// counters are chosen according to the following order. rationale:
	// - TSC must come before QPC and PMT to make sure a bug in the latter on
	//   Pentium systems doesn't come up.
	// - PMT works, but is inexplicably slower than QPC on a PIII Mobile.
	// - TGT really isn't as safe as the others, so it should be last.
	// - low-overhead and high-resolution counters are preferred.
	switch(id)
	{
	case 0:
		CREATE(HPET)
	case 1:
		CREATE(TSC)
	case 2:
		CREATE(QPC)
	case 3:
		CREATE(PMT)
	case 4:
		CREATE(TGT)
	default:
		size = 0;
		return 0;
	}

#include "lib/mmgr.h"

#undef CREATE
}

ICounter* CreateCounter(uint id)
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
	static const size_t MEM_SIZE = 200;	// checked below
	static u8 mem[MEM_SIZE];
	static u8* nextMem = mem;

	u8* addr = (u8*)round_up((uintptr_t)nextMem, 16);
	size_t size;
	ICounter* counter = ConstructCounterAt(id, addr, size);

	nextMem = addr+size;
	debug_assert(nextMem < mem+MEM_SIZE);	// had enough room?

	return counter;
}


void DestroyCounter(ICounter*& counter)
{
	if(!counter)
		return;

	counter->Shutdown();
	counter->~ICounter();	// must be called due to placement new
	counter = 0;
}
