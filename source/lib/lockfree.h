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

/**
 * =========================================================================
 * File        : lockfree.h
 * Project     : 0 A.D.
 * Description : lock-free synchronized data structures.
 * =========================================================================
 */

#ifndef INCLUDED_LOCKFREE
#define INCLUDED_LOCKFREE

#include "lib/sysdep/cpu.h"		// cpu_CAS

/*

[KEEP IN SYNC WITH WIKI]

overview
--------

this module provides several implicitly thread-safe data structures.
rather than allowing only one thread to access them at a time, their
operations are carefully implemented such that they take effect in
one atomic step. data consistency problems are thus avoided.
this novel approach to synchronization has several advantages:
- deadlocks are impossible;
- overhead due to OS kernel entry is avoided;
- graceful scaling to multiple processors is ensured.


mechanism
---------

the basic primitive that makes this possible is "compare and swap",
a CPU instruction that performs both steps atomically. it compares a
machine word against the expected value; if equal, the new value is
written and an indication returned. otherwise, another thread must have
been writing to the same location; the operation is typically retried.

this instruction is available on all modern architectures; in some cases,
emulation in terms of an alternate primitive (LL/SC) is necessary.


memory management
-----------------

one major remaining problem is how to free no longer needed nodes in the
data structure. in general, we want to reclaim their memory for arbitrary use;
this isn't safe as long as other threads are still accessing them.

the RCU algorithm recognizes that all CPUs having entered a quiescent
state means that no threads are still referencing data.
lacking such kernel support, we use a similar mechanism - "hazard pointers"
are set before accessing data; only if none are pointing to a node can it
be freed. until then, they are stored in a per-thread 'waiting list'.

this approach has several advantages over previous algorithms
(typically involving reference count): the CAS primitive need only
operate on single machine words, and space/time overhead is much reduced.


usage notes
-----------

useful "payload" in the data structures is allocated when inserting each
item: additional_bytes are appended. rationale: see struct Node definition.

since lock-free algorithms are subtle and easy to get wrong, an extensive
self-test is included.


terminology
-----------

"atomic" means indivisible; in this case, other CPUs cannot
  interfere with such an operation.
"race conditions" are potential data consistency
  problems resulting from lack of thread synchronization.
"deadlock" is a state where several threads are waiting on
  one another and no progress is possible.
"thread-safety" is understood to mean the
  preceding two problems do not occur.
"scalability" is a measure of how efficient synchronization is;
  overhead should not increase significantly with more processors.
"linearization point" denotes the time at which an external
  observer believes a lock-free operation to have taken effect.

*/


extern void lockfree_Init();
extern void lockfree_Shutdown();


//
// lock-free singly linked list
//

struct LFList
{
	void* head;
};

// make ready a previously unused(!) list object. if a negative error
// code (currently only ERR::NO_MEM) is returned, the list can't be used.
extern LibError lfl_init(LFList* list);

// call when list is no longer needed; should no longer hold any references.
extern void lfl_free(LFList* list);

// return pointer to "user data" attached to <key>,
// or 0 if not found in the list.
extern void* lfl_find(LFList* list, uintptr_t key);

// insert into list in order of increasing key. ensures items are unique
// by first checking if already in the list. returns 0 if out of memory,
// otherwise a pointer to "user data" attached to <key>. the optional
// <was_inserted> return variable indicates whether <key> was added.
extern void* lfl_insert(LFList* list, uintptr_t key, size_t additional_bytes, int* was_inserted);

// remove from list; return -1 if not found, or 0 on success.
extern LibError lfl_erase(LFList* list, uintptr_t key);


//
// lock-free hash table (chained, fixed size)
//

struct LFHash
{
	LFList* tbl;
	size_t mask;
};

// make ready a previously unused(!) hash object. table size will be
// <num_entries>; this cannot currently be expanded. if a negative error
// code (currently only ERR::NO_MEM) is returned, the hash can't be used.
extern LibError lfh_init(LFHash* hash, size_t num_entries);

// call when hash is no longer needed; should no longer hold any references.
extern void lfh_free(LFHash* hash);

// return pointer to "user data" attached to <key>,
// or 0 if not found in the hash.
extern void* lfh_find(LFHash* hash, uintptr_t key);

// insert into hash if not already present. returns 0 if out of memory,
// otherwise a pointer to "user data" attached to <key>. the optional
// <was_inserted> return variable indicates whether <key> was added.
extern void* lfh_insert(LFHash* hash, uintptr_t key, size_t additional_bytes, int* was_inserted);

// remove from hash; return -1 if not found, or 0 on success.
extern LibError lfh_erase(LFHash* hash, uintptr_t key);



/**
* thread-safe (lock-free) reference counter with an extra 'exclusive' state.
**/
class LF_ReferenceCounter
{
public:
	LF_ReferenceCounter()
		: m_status(0)
	{
	}

	/**
	* @return true if successful or false if exclusive access has already
	* been granted or reference count is non-zero.
	**/
	bool AcquireExclusiveAccess()
	{
		return cpu_CAS(&m_status, 0, S_EXCLUSIVE);
	}

	/**
	* re-enables adding references.
	**/
	void RelinquishExclusiveAccess()
	{
		const bool ok = cpu_CAS(&m_status, S_EXCLUSIVE, 0);
		debug_assert(ok);
	}

	/**
	* increase the reference count (bounds-checked).
	*
	* @return true if successful or false if the item is currently locked.
	**/
	bool AddReference()
	{
		const uintptr_t oldRefCnt = ReferenceCount();
		debug_assert(oldRefCnt < S_REFCNT);
		// (returns false if S_EXCLUSIVE is set)
		return cpu_CAS(&m_status, oldRefCnt, oldRefCnt+1);
	}

	/**
	* decrease the reference count (bounds-checked).
	**/
	void Release()
	{
		const uintptr_t oldRefCnt = ReferenceCount();
		debug_assert(oldRefCnt != 0);
		// (fails if S_EXCLUSIVE is set)
		const bool ok = cpu_CAS(&m_status, oldRefCnt, oldRefCnt+1);
		debug_assert(ok);
	}

	uintptr_t ReferenceCount() const
	{
		return m_status & S_REFCNT;
	}

private:
	static const uintptr_t S_REFCNT = (~0u) >> 1;		// 0x7F..F
	static const uintptr_t S_EXCLUSIVE = S_REFCNT+1u;	// 0x80..0

	volatile uintptr_t m_status;
};

#endif	// #ifndef INCLUDED_LOCKFREE
