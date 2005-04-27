// lock-free primitives and algorithms
//
// Copyright (c) 2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include <algorithm>

#include "lib.h"
#include "posix.h"
#include "sysdep/cpu.h"
#include "lockfree.h"
#include "timer.h"

#ifndef PERFORM_SELF_TEST
#define PERFORM_SELF_TEST 0
#endif


// note: a 486 or later processor is required since we use CMPXCHG.
// there's no feature flag we can check, and the ia32 code doesn't
// bother detecting anything < Pentium, so this'll crash and burn if
// run on 386. we could replace cmpxchg with a simple mov (since 386
// CPUs aren't MP-capable), but it's not worth the trouble.

__declspec(naked) bool __cdecl CAS_(uintptr_t* location, uintptr_t expected, uintptr_t new_value)
{
	// try to see if caller isn't passing in an address
	// (CAS's arguments are silently casted)
	assert2(location >= (uintptr_t*)0x10000);

__asm
{
	cmp		byte ptr [cpus], 1
	mov		eax, [esp+8]	// expected
	mov		edx, [esp+4]	// location
	mov		ecx, [esp+12]	// new_value
	je		$no_lock
_emit 0xf0	// LOCK prefix
$no_lock:
	cmpxchg	[edx], ecx
	mov		eax, 0
	sete	al
	ret
}
}


__declspec(naked) void __cdecl atomic_add(intptr_t* location, intptr_t increment)
{
__asm
{
	cmp		byte ptr [cpus], 1
	mov		edx, [esp+4]	// location
	mov		eax, [esp+8]	// increment
	je		$no_lock
_emit 0xf0	// LOCK prefix
$no_lock:
	add		[edx], eax
	ret
}
}




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
make sure retired node array doesn't overflow. add padding (i.e.  "Scan" if half-full?)
see why SMR had algo extension of HelpScan
*/

// total number of hazard pointers needed by each thread.
// determined by the algorithms using SMR; the LF list requires 2.
static const uint NUM_HPS = 2;

// number of slots for the per-thread node freelist.
// this is a reasonable size and pads struct TLS to 64 bytes.
static const size_t MAX_RETIRED = 11;


// used to allocate a flat array of all hazard pointers.
// changed via atomic_add by TLS when a thread first calls us / exits.
static intptr_t active_threads;

// basically module refcount; we can't shut down before it's 0.
// changed via atomic_add by each data structure's init/free.
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
static pthread_once_t tls_once = PTHREAD_ONCE_INIT;

struct TLS
{
	TLS* next;

	void* hp[NUM_HPS];
	uintptr_t active;	// used as bool, but set by CAS

	Node* retired_nodes[MAX_RETIRED];
	size_t num_retired_nodes;
};

static TLS* tls_list = 0;


// mark a participating thread's slot as unused; clear its hazard pointers.
// called during smr_try_shutdown and when a thread exits
// (by pthread dtor, which is registered in tls_init).
static void tls_retire(void* tls_)
{
	TLS* tls = (TLS*)tls_;

	// our hazard pointers are no longer in use
	for(size_t i = 0; i < NUM_HPS; i++)
		tls->hp[i] = 0;

	// successfully marked as unused (must only decrement once)
	if(CAS(&tls->active, 1, 0))
	{
		atomic_add(&active_threads, -1);
		assert2(active_threads >= 0);
	}
}


// (called via pthread_once from tls_get)
static void tls_init()
{
	int ret = pthread_key_create(&tls_key, tls_retire);
	assert2(ret == 0);
}


// free all TLS info. called by smr_try_shutdown.
static void tls_shutdown()
{
	int ret = pthread_key_delete(tls_key);
	assert2(ret == 0);
	tls_key = 0;

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
	// make sure we weren't shut down in the meantime - re-init isn't
	// possible since pthread_once (which can't be reset) calls tls_init.
	assert2(tls_key != 0);

	TLS* tls;

	// try to reuse a retired TLS slot
	for(tls = tls_list; tls; tls = tls->next)
		// .. succeeded in reactivating one.
		if(CAS(&tls->active, 0, 1))
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
		int ret = pthread_setspecific(tls_key, tls);
		assert2(ret == 0);
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
	while(!CAS(&tls_list, old_tls_list, tls));
	}


have_tls:
	atomic_add(&active_threads, 1);

	int ret = pthread_setspecific(tls_key, tls);
	assert2(ret == 0);
	return tls;
}


// return this thread's struct TLS, or (TLS*)-1 if tls_alloc failed.
// called from each lfl_* function, so don't waste any time.
static TLS* tls_get()
{
	int ret = pthread_once(&tls_once, tls_init);
	assert2(ret == 0);

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
	// we're called from smr_try_shutdown,
	if(tls->num_retired_nodes == 0)
		return;

	// required for head/tail below; guaranteed by callers.
	assert2(tls->num_retired_nodes != 0);

	//
	// build array of all active (non-NULL) hazard pointers (more efficient
	// than walking through tls_list on every is_node_referenced call)
	//
try_again:
	const size_t max_hps = (active_threads+3) * NUM_HPS;
		// allow for creating a few additional threads during the loop
	void** hps = (void**)alloca(max_hps * sizeof(void*));
	size_t num_hps = 0;
	// for each participating thread:
	for(TLS* t = tls_list; t; t = t->next)
		// for each of its non-NULL hazard pointers:
		for(int i = 0; i < NUM_HPS-1; i++)
		{
			void* hp = t->hp[i];
			if(!hp)
				continue;

			// many threads were created after choosing max_hps =>
			// start over. this won't realistically happen, though.
			if(num_hps >= max_hps)
			{
				debug_warn("max_hps overrun - why?");
				goto try_again;
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
	assert2(tls != (void*)-1);
		// if this triggers, tls_alloc called from lfl_init failed due to
		// lack of memory and the caller didn't check its return value.

	tls->retired_nodes[tls->num_retired_nodes++] = node;
	if(tls->num_retired_nodes >= MAX_RETIRED)
		smr_release_unreferenced_nodes(tls);
}


//
// shutdown
//

// although not strictly necessary (the OS will free resources at exit),
// we want to free all nodes and TLS to avoid spamming leak detectors.
// that can only happen after our users indicate all data structures are
// no longer in use (i.e. active_data_structures == 0).
//
// problem: if the first user of a data structure is finished before
// program termination, we'd shut down and not be able to reinitialize
// (see tls_alloc). therefore, we don't shut down before
// static destructors are called, i.e. end of program is at hand.

static bool is_static_dtor_time = false;

// call when a data structure is freed (i.e. no longer in use);
// we shut down if it is time to do so.
static void smr_try_shutdown()
{
	// shouldn't or can't shut down yet.
	if(!is_static_dtor_time || active_data_structures != 0)
		return;

	for(TLS* t = tls_list; t; t = t->next)
	{
		tls_retire(t);
			// wipe out hazard pointers so that everything can be freed.
		smr_release_unreferenced_nodes(t);
	}

	tls_shutdown();
}

// non-local static object - its destructor being called indicates
// program end is at hand. could use atexit for this, but registering
// that would be a bit more work.
static struct NLSO
{
	NLSO()
	{
	}

	~NLSO()
	{
		is_static_dtor_time = true;

		// trigger shutdown in case all data structures have
		// already been freed.
		smr_try_shutdown();
	}
}
nlso;


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
	return (u & BIT(0)) != 0;
}

static inline Node* with_mark(Node* p)
{
	assert2(!is_marked_as_deleted(p));	// paranoia
	return p+1;
}

static inline Node* without_mark(Node* p)
{
	assert2(is_marked_as_deleted(p));	// paranoia
	return p-1;
}



// make ready a previously unused(!) list object. if a negative error
// code (currently only ERR_NO_MEM) is returned, the list can't be used.
int lfl_init(LFList* list)
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
		return ERR_NO_MEM;
	}

	list->head = 0;
	atomic_add(&active_data_structures, 1);
	return 0;
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

	atomic_add(&active_data_structures, -1);
	assert2(active_data_structures >= 0);
	smr_try_shutdown();
}


// "Find"
// look for a given key in the list; return true iff found.
// pos points to the last inspected node and its successor and predecessor.
static bool list_lookup(LFList* list, uintptr_t key, ListPos* pos)
{
	TLS* tls = tls_get();
	assert2(tls != (void*)-1);
		// if this triggers, tls_alloc called from lfl_init failed due to
		// lack of memory and the caller didn't check its return value.

	void** hp0 = &tls->hp[0];	// protects cur
	void** hp1 = &tls->hp[1];	// protects *pprev

try_again:
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
			goto try_again;

		pos->next = pos->cur->next;
			// linearization point of the following if list is not empty:
			// unsuccessful insert or erase; find.

		// this node has been removed from the list; retire it before
		// continuing (we don't want to add references to it).
		if(is_marked_as_deleted(pos->next))
		{
			Node* next = without_mark(pos->next);
			if(!CAS(pos->pprev, pos->cur, next))
				goto try_again;

			smr_retire_node(pos->cur);
			pos->cur = next;
		}
		else
		{
			// (see above goto)
			if(*pos->pprev != pos->cur)
				goto try_again;

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
	assert2(tls != (void*)-1);
		// if this triggers, tls_alloc called from lfl_init failed due to
		// lack of memory and the caller didn't check its return value.

	ListPos* pos = (ListPos*)alloca(sizeof(ListPos));

	Node* node = 0;
	if(was_inserted)
		*was_inserted = 0;

try_again:
	// already in list - return it and leave <was_inserted> 'false'
	if(list_lookup(list, key, pos))
	{
		// free in case we allocated below, but CAS failed;
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
	if(!CAS(pos->pprev, pos->cur, node))
		goto try_again;
	// else: successfully inserted; linearization point
	if(was_inserted)
		*was_inserted = 1;

have_node:
	return node_user_data(node);
}


// remove from list; return -1 if not found, or 0 on success.
int lfl_erase(LFList* list, uintptr_t key)
{
	TLS* tls = tls_get();
	assert2(tls != (void*)-1);
		// if this triggers, tls_alloc called from lfl_init failed due to
		// lack of memory and the caller didn't check its return value.

	ListPos* pos = (ListPos*)alloca(sizeof(ListPos));

try_again:
	// not found in list - abort.
	if(!list_lookup(list, key, pos))
		return -1;
	// mark as removed (avoids subsequent linking to it). failure implies
	// at least of the following happened after list_lookup; we try again.
	// - next was removed
	// - cur was retired (i.e. no longer reachable from *phead)
	// - a new node was inserted immediately after cur
	if(!CAS(&pos->cur->next, pos->next, with_mark(pos->next)))
		goto try_again;
	// remove from list; if successful, this is the
	// linearization point and *pprev isn't marked.
	if(CAS(pos->pprev, pos->cur, pos->next))
		smr_retire_node(pos->cur);
	// failed: another thread removed cur after it was marked above.
	// call list_lookup to ensure # non-released nodes < # threads.
	else
		list_lookup(list, key, pos);
	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
// lock-free hash table
//
//////////////////////////////////////////////////////////////////////////////

// note: implemented via lfl, so we don't need to track
// active_data_structures or call smr_try_shutdown here.

static void validate(LFHash* hash)
{
	assert2(hash->tbl);
	assert2(is_pow2(hash->mask+1));
}

// return hash "chain" (i.e. linked list) that is assigned to <key>.
static LFList* chain(LFHash* hash, uintptr_t key)
{
	validate(hash);
	return &hash->tbl[key & hash->mask];
}


// make ready a previously unused(!) hash object. table size will be
// <num_entries>; this cannot currently be expanded. if a negative error
// code (currently only ERR_NO_MEM) is returned, the hash can't be used.
int lfh_init(LFHash* hash, size_t num_entries)
{
	hash->tbl  = 0;
	hash->mask = ~0;

	if(!is_pow2((long)num_entries))
	{
		debug_warn("lfh_init: size must be power of 2");
		return ERR_INVALID_PARAM;
	}

	hash->tbl = (LFList*)malloc(sizeof(LFList) * num_entries);
	if(!hash->tbl)
		return ERR_NO_MEM;
	hash->mask = (uint)num_entries-1;

	for(int i = 0; i < (int)num_entries; i++)
	{
		int err = lfl_init(&hash->tbl[i]);
		if(err < 0)
		{
			// failed - free all and bail
			for(int j = 0; j < i; j++)
				lfl_free(&hash->tbl[j]);
			return ERR_NO_MEM;
		}
	}

	return 0;
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
int lfh_erase(LFHash* hash, uintptr_t key)
{
	return lfl_erase(chain(hash,key), key);
}


//////////////////////////////////////////////////////////////////////////////
//
// built-in self test
//
//////////////////////////////////////////////////////////////////////////////

namespace test {

#if PERFORM_SELF_TEST

// make sure the data structures work at all; doesn't test thread-safety.
static void basic_single_threaded_test()
{
	int err;
	void* user_data;

	const uint ENTRIES = 50;
	// should be more than max # retired nodes to test release..() code
	uintptr_t key = 0x1000;
	int sig = 10;

	LFList list;
	err = lfl_init(&list);
	assert2(err == 0);

	LFHash hash;
	err = lfh_init(&hash, 8);
	assert2(err == 0);

	// add some entries; store "signatures" (ascending int values)
	for(uint i = 0; i < ENTRIES; i++)
	{
		int was_inserted;

		user_data = lfl_insert(&list, key+i, sizeof(int), &was_inserted);
		assert2(user_data != 0 && was_inserted);
		*(int*)user_data = sig+i;

		user_data = lfh_insert(&hash, key+i, sizeof(int), &was_inserted);
		assert2(user_data != 0 && was_inserted);
		*(int*)user_data = sig+i;
	}

	// make sure all "signatures" are present in list
	for(uint i = 0; i < ENTRIES; i++)
	{
		user_data = lfl_find(&list, key+i);
		assert2(user_data != 0);
		assert2(*(int*)user_data == sig+i);

		user_data = lfh_find(&hash, key+i);
		assert2(user_data != 0);
		assert2(*(int*)user_data == sig+i);

	}

	lfl_free(&list);
	lfh_free(&hash);
}


//
// multithreaded torture test
//

// poor man's synchronization "barrier"
static bool is_complete;
static intptr_t num_active_threads;

static LFList list;
static LFHash hash;

typedef std::set<uintptr_t> KeySet; 
typedef KeySet::const_iterator KeySetIt;
static KeySet keys;
static pthread_mutex_t mutex;	// protects <keys>


static void* thread_func(void* arg)
{
	const uintptr_t thread_number = (uintptr_t)arg;

	atomic_add(&num_active_threads, 1);

	// chosen randomly every iteration (int_value % 4)
	enum TestAction
	{
		TA_FIND   = 0,
		TA_INSERT = 1,
		TA_ERASE  = 2,
		TA_SLEEP  = 3
	};
	static const char* const action_strings[] =
	{
		"find", "insert", "erase", "sleep"
	};

	while(!is_complete)
	{
		void* user_data;

		const int action            = rand_up_to(4);
		const uintptr_t key         = rand_up_to(100);
		const int sleep_duration_ms = rand_up_to(100);
		debug_out("thread %d: %s\n", thread_number, action_strings[action]);

		//
		pthread_mutex_lock(&mutex);
		const bool was_in_set = keys.find(key) != keys.end();
		if(action == TA_INSERT)
			keys.insert(key);
		else if(action == TA_ERASE)
			keys.erase(key);
		pthread_mutex_unlock(&mutex);

		switch(action)
		{
		case TA_FIND:
			{
			user_data = lfl_find(&list, key);
			assert2(was_in_set == (user_data != 0));
			if(user_data)
				assert2(*(uintptr_t*)user_data == ~key);

			user_data = lfh_find(&hash, key);
			assert2(was_in_set == (user_data != 0));
			if(user_data)
				assert2(*(uintptr_t*)user_data == ~key);
			}
			break;

		case TA_INSERT:
			{
			int was_inserted;

			user_data = lfl_insert(&list, key, sizeof(uintptr_t), &was_inserted);
			assert2(user_data != 0);	// only triggers if out of memory
			*(uintptr_t*)user_data = ~key;	// checked above
			assert2(was_in_set == !was_inserted);

			user_data = lfh_insert(&hash, key, sizeof(uintptr_t), &was_inserted);
			assert2(user_data != 0);	// only triggers if out of memory
			*(uintptr_t*)user_data = ~key;	// checked above
			assert2(was_in_set == !was_inserted);
			}
			break;

		case TA_ERASE:
			{
			int err;

			err = lfl_erase(&list, key);
			assert2(was_in_set == (err == 0));

			err = lfh_erase(&hash, key);
			assert2(was_in_set == (err == 0));
			}
			break;

		case TA_SLEEP:
			usleep(sleep_duration_ms*1000);
			break;

		default:
			debug_warn("invalid TA_* action");
			break;
		}	// switch
	}	// while !is_complete

	atomic_add(&num_active_threads, -1);
	assert2(num_active_threads >= 0);

	return 0;
}


static void multithreaded_torture_test()
{
	int err;

	// this test is randomized; we need deterministic results.
	srand(1);

	static const double TEST_LENGTH = 30.;	// [seconds]
	const double end_time = get_time() + TEST_LENGTH;
	is_complete = false;

	err = lfl_init(&list);
	assert2(err == 0);
	err = lfh_init(&hash, 128);
	assert2(err == 0);
	err = pthread_mutex_init(&mutex, 0);
	assert2(err == 0);

	// spin off test threads (many, to force preemption)
	const uint NUM_THREADS = 16;
	for(uintptr_t i = 0; i < NUM_THREADS; i++)
		pthread_create(0, 0, thread_func, (void*)i);

	// wait until time interval elapsed (if we get that far, all is well).
	while(get_time() < end_time)
		usleep(10*1000);

	// signal and wait for all threads to complete (poor man's barrier -
	// those aren't currently implemented in wpthread).
	is_complete = true;
	while(num_active_threads > 0)
		usleep(5*1000);

	lfl_free(&list);
	lfh_free(&hash);
	err = pthread_mutex_destroy(&mutex);
	assert2(err == 0);
}


static int run_tests()
{
	basic_single_threaded_test();
	multithreaded_torture_test();
	return 0;
}

static int dummy = run_tests();

#endif	// #if PERFORM_SELF_TEST

}	// namespace test