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

/*
 * lock-free synchronized data structures.
 */

#include "precompiled.h"
#if 0	// JW: disabled, not used
#include "lockfree.h"

#include <set>
#include <algorithm>

#include "lib/posix/posix_pthread.h"
#include "lib/bits.h"
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/sysdep.h"
#include "timer.h"
#include "module_init.h"


/*
liberties taken:
- R(H) will remain constant
  (since TLS rlist is fixed-size, and we don't care about O(1)
  amortization proofs)


lacking from pseudocode:
- mark HPRec as active when allocated


questions:
- does hp0 ("private, static") need to be in TLS? or is per-"find()" ok?
- memory barriers where?


todo:
-make sure retired node array doesn't overflow. add padding (i.e.  "Scan" if half-full?)
-see why SMR had algo extension of HelpScan
-simple iteration is safe?
*/

// total number of hazard pointers needed by each thread.
// determined by the algorithms using SMR; the LF list requires 2.
static const size_t NUM_HPS = 2;

// number of slots for the per-thread node freelist.
// this is a reasonable size and pads struct TLS to 64 bytes.
static const size_t MAX_RETIRED = 11;


// used to allocate a flat array of all hazard pointers.
// changed via cpu_AtomicAdd by TLS when a thread first calls us / exits.
static intptr_t active_threads;

// basically module refcount; we can't shut down before it's 0.
// changed via cpu_AtomicAdd by each data structure's init/free.
static intptr_t active_data_structures;


// Nodes are internal to this module. having callers pass them in would
// be more convenient but risky, since they might change <next> and <key>,
// or not allocate via malloc (necessary since Nodes are garbage-collected
// and allowing user-specified destructors would be more work).
//
// to still allow storing arbitrary user data without requiring an
// additional memory alloc per node, we append <user_size> bytes to the
// end of the Node structure; this is what is returned by find.
struct Node
{
	Node* next;
	uintptr_t key;

	// <additional_bytes> are allocated here at the caller's discretion.
};

static inline Node* node_alloc(size_t additional_bytes)
{
	return (Node*)calloc(1, sizeof(Node) + additional_bytes);
}

static inline void node_free(Node* n)
{
	free(n);
}

static inline void* node_user_data(Node* n)
{
	return (u8*)n + sizeof(Node);
}


//////////////////////////////////////////////////////////////////////////////
//
// thread-local storage for SMR
//
//////////////////////////////////////////////////////////////////////////////

static pthread_key_t tls_key;

struct TLS
{
	TLS* next;

	void* hp[NUM_HPS];
	uintptr_t active;	// used as bool, but set by cpu_CAS

	Node* retired_nodes[MAX_RETIRED];
	size_t num_retired_nodes;
};

static TLS* tls_list = 0;


// mark a participating thread's slot as unused; clear its hazard pointers.
// called during smr_shutdown and when a thread exits
// (by pthread dtor, which is registered in tls_init).
static void tls_retire(void* tls_)
{
	TLS* tls = (TLS*)tls_;

	// our hazard pointers are no longer in use
	for(size_t i = 0; i < NUM_HPS; i++)
		tls->hp[i] = 0;

	// successfully marked as unused (must only decrement once)
	if(cpu_CAS(&tls->active, 1, 0))
	{
		cpu_AtomicAdd(&active_threads, -1);
		debug_assert(active_threads >= 0);
	}
}


static void tls_init()
{
	WARN_ERR(pthread_key_create(&tls_key, tls_retire));
}


// free all TLS info. called by smr_shutdown.
static void tls_shutdown()
{
	WARN_ERR(pthread_key_delete(tls_key));
	memset(&tls_key, 0, sizeof(tls_key));

	while(tls_list)
	{
		TLS* tls = tls_list;
		tls_list = tls->next;
		free(tls);
	}
}


// return a new TLS struct ready for use; either a previously
// retired slot, or if none are available, a newly allocated one.
// if out of memory, return (TLS*)-1; see fail path.
// called from tls_get after tls_init.
static TLS* tls_alloc()
{
	TLS* tls;

	// try to reuse a retired TLS slot
	for(tls = tls_list; tls; tls = tls->next)
		// .. succeeded in reactivating one.
		if(cpu_CAS(&tls->active, 0, 1))
			goto have_tls;

	// no unused slots available - allocate another
	{
	tls = (TLS*)calloc(1, sizeof(TLS));
	// .. not enough memory. poison the thread's TLS value to
	//    prevent a later tls_get from succeeding, because that
	//    would potentially break the user's LF data structure.
	if(!tls)
	{
		tls = (TLS*)-1;
		WARN_ERR(pthread_setspecific(tls_key, tls));
		return tls;
	}
	tls->active = 1;
	// insert at front of list (wait free since # threads is finite).
	TLS* old_tls_list;
	do
	{
		old_tls_list = tls_list;
		tls->next = old_tls_list;
	}
	while(!cpu_CAS(&tls_list, old_tls_list, tls));
	}


have_tls:
	cpu_AtomicAdd(&active_threads, 1);

	WARN_ERR(pthread_setspecific(tls_key, tls));
	return tls;
}


// return this thread's struct TLS, or (TLS*)-1 if tls_alloc failed.
// called from each lfl_* function, so don't waste any time.
static TLS* tls_get()
{
	// already allocated or tls_alloc failed.
	TLS* tls = (TLS*)pthread_getspecific(tls_key);
	if(tls)
		return tls;

	// first call: return a newly allocated slot.
	return tls_alloc();
}


//////////////////////////////////////////////////////////////////////////////
//
// "Safe Memory Reclamation for Lock-Free Objects" via hazard pointers
//
//////////////////////////////////////////////////////////////////////////////

// is one of the hazard pointers in <hps> pointing at <node>?
static bool is_node_referenced(Node* node, void** hps, size_t num_hps)
{
	for(size_t i = 0; i < num_hps; i++)
		if(hps[i] == node)
			return true;

	return false;
}


// "Scan"
// run through all retired nodes in this thread's freelist; any of them
// not currently referenced are released (their memory freed).
static void smr_release_unreferenced_nodes(TLS* tls)
{
	// nothing to do, and taking address of array[-1] isn't portable.
	if(tls->num_retired_nodes == 0)
		return;

	// required for head/tail below; guaranteed by callers.
	debug_assert(0 < tls->num_retired_nodes && tls->num_retired_nodes <= MAX_RETIRED);

	//
	// build array of all active (non-NULL) hazard pointers (more efficient
	// than walking through tls_list on every is_node_referenced call)
	//
retry:
	const size_t max_hps = (active_threads+3) * NUM_HPS;
		// allow for creating a few additional threads during the loop
	void** hps = (void**)alloca(max_hps * sizeof(void*));
	size_t num_hps = 0;
	// for each participating thread:
	for(TLS* t = tls_list; t; t = t->next)
		// for each of its non-NULL hazard pointers:
		for(size_t i = 0; i < NUM_HPS; i++)
		{
			void* hp = t->hp[i];
			if(!hp)
				continue;

			// many threads were created after choosing max_hps =>
			// start over. this won't realistically happen, though.
			if(num_hps >= max_hps)
			{
				debug_assert(0);	// max_hps overrun - why?
				goto retry;
			}

			hps[num_hps++] = hp;
		}

	//
	// free all discarded nodes that are no longer referenced
	// (i.e. no element in hps[] points to them). no need to lock or
	// clone the retired_nodes list since it's in TLS.
	//
	Node** head = tls->retired_nodes;
	Node** tail = head + tls->num_retired_nodes-1;
	while(head <= tail)
	{
		Node* node = *head;
		// still in use - just skip to the next
		if(is_node_referenced(node, hps, num_hps))
			head++;
		else
		{
			node_free(node);

			// to avoid holes in the freelist, replace with last entry.
			// this is easier than building a new list.
			*head = *tail;	// if last element, no-op
			tail--;
			tls->num_retired_nodes--;
		}
	}
}


// note: we don't implement "HelpScan" - it is sufficient for the
// freelists in retired but never-reused TLS slots to be emptied at exit,
// since huge spikes of active threads are unrealistic.
static void smr_retire_node(Node* node)
{
	TLS* tls = tls_get();
	debug_assert(tls != (void*)-1);
		// if this triggers, tls_alloc called from lfl_init failed due to
		// lack of memory and the caller didn't check its return value.

	debug_assert(tls->num_retired_nodes < MAX_RETIRED);
	tls->retired_nodes[tls->num_retired_nodes++] = node;
	if(tls->num_retired_nodes >= MAX_RETIRED/2)
		smr_release_unreferenced_nodes(tls);
}


// although not strictly necessary (the OS will free resources at exit),
// we free all nodes and TLS to avoid spurious leak reports.
static void smr_shutdown()
{
	// there better not be any data structures still in use, else we're
	// going to pull the rug out from under them.
	debug_assert(active_data_structures == 0);

	for(TLS* t = tls_list; t; t = t->next)
	{
		// wipe out hazard pointers so that everything can be freed.
		tls_retire(t);

		smr_release_unreferenced_nodes(t);
	}

	tls_shutdown();
}


//////////////////////////////////////////////////////////////////////////////
//
// lock-free singly linked list
//
//////////////////////////////////////////////////////////////////////////////

// output of lfl_lookup
struct ListPos
{
	Node** pprev;
	Node* cur;
	Node* next;
};


// we 'mark' the next pointer of a retired node to prevent linking
// to it in concurrent inserts. since all pointers returned by malloc are
// at least 2-byte aligned, we can use the least significant bit.
static inline bool is_marked_as_deleted(Node* p)
{
	const uintptr_t u = (uintptr_t)p;
	return (u & Bit<uintptr_t>(0)) != 0;
}

static inline Node* with_mark(Node* p)
{
	debug_assert(!is_marked_as_deleted(p));	// paranoia
	return p+1;
}

static inline Node* without_mark(Node* p)
{
	debug_assert(is_marked_as_deleted(p));	// paranoia
	return p-1;
}



// make ready a previously unused(!) list object. if a negative error
// code (currently only ERR::NO_MEM) is returned, the list can't be used.
LibError lfl_init(LFList* list)
{
	// make sure a TLS slot has been allocated for this thread.
	// if not (out of memory), the list object must not be used -
	// other calls don't have a "tls=0" failure path.
	// (it doesn't make sense to allow some calls to fail until more
	// memory is available, since that might leave the list in an
	// invalid state or leak memory)
	TLS* tls = tls_get();
	if(!tls)
	{
		list->head = (void*)-1;	// 'poison' prevents further use
		return ERR::NO_MEM;
	}

	list->head = 0;
	cpu_AtomicAdd(&active_data_structures, 1);
	return INFO::OK;
}


// call when list is no longer needed; should no longer hold any references.
void lfl_free(LFList* list)
{
	// TODO: is this iteration safe?
	Node* cur = (Node*)list->head;
	while(cur)
	{
		Node* next = cur->next;
			// must latch before smr_retire_node, since that may
			// actually free the memory.
		smr_retire_node(cur);
		cur = next;
	}

	cpu_AtomicAdd(&active_data_structures, -1);
	debug_assert(active_data_structures >= 0);
}


// "Find"
// look for a given key in the list; return true iff found.
// pos points to the last inspected node and its successor and predecessor.
static bool list_lookup(LFList* list, uintptr_t key, ListPos* pos)
{
	TLS* tls = tls_get();
	debug_assert(tls != (void*)-1);
		// if this triggers, tls_alloc called from lfl_init failed due to
		// lack of memory and the caller didn't check its return value.

	void** hp0 = &tls->hp[0];	// protects cur
	void** hp1 = &tls->hp[1];	// protects *pprev

retry:
	pos->pprev = (Node**)&list->head;
		// linearization point of erase and find if list is empty.
		// already protected by virtue of being the root node.
	pos->cur = *pos->pprev;

	// until end of list:
	while(pos->cur)
	{
		*hp0 = pos->cur;

		// pprev changed (<==> *pprev or cur was removed) => start over.
		// lock-free, since other threads thereby make progress.
		if(*pos->pprev != pos->cur)
			goto retry;

		pos->next = pos->cur->next;
			// linearization point of the following if list is not empty:
			// unsuccessful insert or erase; find.

		// this node has been removed from the list; retire it before
		// continuing (we don't want to add references to it).
		if(is_marked_as_deleted(pos->next))
		{
			Node* next = without_mark(pos->next);
			if(!cpu_CAS(pos->pprev, pos->cur, next))
				goto retry;

			smr_retire_node(pos->cur);
			pos->cur = next;
		}
		else
		{
			// (see above goto)
			if(*pos->pprev != pos->cur)
				goto retry;

			// the nodes are sorted in ascending key order, so we've either
			// found <key>, or it's not in the list.
			const uintptr_t cur_key = pos->cur->key;
			if(cur_key >= key)
				return (cur_key == key);

			pos->pprev = &pos->cur->next;
			pos->cur   = pos->next;

			// protect pprev in the subsequent iteration; it has assumed an
			// arithmetic variation of cur (adding offsetof(Node, next)).
			// note that we don't need to validate *pprev, since *hp0 is
			// already protecting cur.
			std::swap(hp0, hp1);
		}
	}

	// hit end of list => not found.
	return false;
}


// return pointer to "user data" attached to <key>,
// or 0 if not found in the list.
void* lfl_find(LFList* list, uintptr_t key)
{
	ListPos* pos = (ListPos*)alloca(sizeof(ListPos));
	if(!list_lookup(list, key, pos))
		return 0;
	return node_user_data(pos->cur);
}


// insert into list in order of increasing key. ensures items are unique
// by first checking if already in the list. returns 0 if out of memory,
// otherwise a pointer to "user data" attached to <key>. the optional
// <was_inserted> return variable indicates whether <key> was added.
void* lfl_insert(LFList* list, uintptr_t key, size_t additional_bytes, int* was_inserted)
{
	TLS* tls = tls_get();
	debug_assert(tls != (void*)-1);
		// if this triggers, tls_alloc called from lfl_init failed due to
		// lack of memory and the caller didn't check its return value.

	ListPos* pos = (ListPos*)alloca(sizeof(ListPos));

	Node* node = 0;
	if(was_inserted)
		*was_inserted = 0;

retry:
	// already in list - return it and leave <was_inserted> 'false'
	if(list_lookup(list, key, pos))
	{
		// free in case we allocated below, but cpu_CAS failed;
		// no-op if node == 0, i.e. it wasn't allocated.
		node_free(node);

		node = pos->cur;
		goto have_node;
	}
	// else: not yet in list, so allocate a new Node if we haven't already.
	// doing that after list_lookup avoids needless alloc/free.
	if(!node)
	{
		node = node_alloc(additional_bytes);
		// .. out of memory
		if(!node)
			return 0;
	}
	node->key  = key;
	node->next = pos->cur;

	// atomic insert immediately before pos->cur. failure implies
	// at least of the following happened after list_lookup; we try again.
	// - *pprev was removed (i.e. it's 'marked')
	// - cur was retired (i.e. no longer reachable from *phead)
	// - a new node was inserted immediately before cur
	if(!cpu_CAS(pos->pprev, pos->cur, node))
		goto retry;
	// else: successfully inserted; linearization point
	if(was_inserted)
		*was_inserted = 1;

have_node:
	return node_user_data(node);
}


// remove from list; return -1 if not found, or 0 on success.
LibError lfl_erase(LFList* list, uintptr_t key)
{
	TLS* tls = tls_get();
	debug_assert(tls != (void*)-1);
		// if this triggers, tls_alloc called from lfl_init failed due to
		// lack of memory and the caller didn't check its return value.

	ListPos* pos = (ListPos*)alloca(sizeof(ListPos));

retry:
	// not found in list - abort.
	if(!list_lookup(list, key, pos))
		return ERR::FAIL;
	// mark as removed (avoids subsequent linking to it). failure implies
	// at least of the following happened after list_lookup; we try again.
	// - next was removed
	// - cur was retired (i.e. no longer reachable from *phead)
	// - a new node was inserted immediately after cur
	if(!cpu_CAS(&pos->cur->next, pos->next, with_mark(pos->next)))
		goto retry;
	// remove from list; if successful, this is the
	// linearization point and *pprev isn't marked.
	if(cpu_CAS(pos->pprev, pos->cur, pos->next))
		smr_retire_node(pos->cur);
	// failed: another thread removed cur after it was marked above.
	// call list_lookup to ensure # non-released nodes < # threads.
	else
		list_lookup(list, key, pos);
	return INFO::OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// lock-free hash table
//
//////////////////////////////////////////////////////////////////////////////

// note: implemented via lfl, so we don't need to update
// active_data_structures.

static void validate(LFHash* hash)
{
	debug_assert(hash->tbl);
	debug_assert(is_pow2(hash->mask+1));
}

// return hash "chain" (i.e. linked list) that is assigned to <key>.
static LFList* chain(LFHash* hash, uintptr_t key)
{
	validate(hash);
	return &hash->tbl[key & hash->mask];
}


// make ready a previously unused(!) hash object. table size will be
// <num_entries>; this cannot currently be expanded. if a negative error
// code (currently only ERR::NO_MEM) is returned, the hash can't be used.
LibError lfh_init(LFHash* hash, size_t num_entries)
{
	hash->tbl  = 0;
	hash->mask = ~0u;

	if(!is_pow2((long)num_entries))
	{
		debug_assert(0);	// lfh_init: size must be power of 2
		return ERR::INVALID_PARAM;
	}

	hash->tbl = (LFList*)malloc(sizeof(LFList) * num_entries);
	if(!hash->tbl)
		return ERR::NO_MEM;
	hash->mask = (size_t)num_entries-1;

	for(int i = 0; i < (int)num_entries; i++)
	{
		int err = lfl_init(&hash->tbl[i]);
		if(err < 0)
		{
			// failed - free all and bail
			for(int j = 0; j < i; j++)
				lfl_free(&hash->tbl[j]);
			return ERR::NO_MEM;
		}
	}

	return INFO::OK;
}


// call when hash is no longer needed; should no longer hold any references.
void lfh_free(LFHash* hash)
{
	validate(hash);

	// free all chains
	for(size_t i = 0; i < hash->mask+1; i++)
		lfl_free(&hash->tbl[i]);

	free(hash->tbl);
	hash->tbl  = 0;
	hash->mask = 0;
}


// return pointer to "user data" attached to <key>,
// or 0 if not found in the hash.
void* lfh_find(LFHash* hash, uintptr_t key)
{
	return lfl_find(chain(hash,key), key);
}


// insert into hash if not already present. returns 0 if out of memory,
// otherwise a pointer to "user data" attached to <key>. the optional
// <was_inserted> return variable indicates whether <key> was added.
void* lfh_insert(LFHash* hash, uintptr_t key, size_t additional_bytes, int* was_inserted)
{
	return lfl_insert(chain(hash,key), key, additional_bytes, was_inserted);
}


// remove from hash; return -1 if not found, or 0 on success.
LibError lfh_erase(LFHash* hash, uintptr_t key)
{
	return lfl_erase(chain(hash,key), key);
}


//-----------------------------------------------------------------------------

static ModuleInitState initState;

void lockfree_Init()
{
	if(!ModuleShouldInitialize(&initState))
		return;

	tls_init();
}

void lockfree_Shutdown()
{
	if(!ModuleShouldShutdown(&initState))
		return;

	smr_shutdown();
}

#endif
