//
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
#include "lockless.h"


// note: a 486 or later processor is required since we use CMPXCHG.
// there's no feature flag we can check, and the ia32 code doesn't
// bother detecting anything < Pentium, so this'll crash and burn if
// run on 386. we could replace cmpxchg with a simple mov (since 386
// CPUs aren't MP-capable), but it's not worth the trouble.

__declspec(naked) bool __cdecl CAS_(uintptr_t* location, uintptr_t expected, uintptr_t new_value)
{
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
- does hp0 (private, static) need to be in TLS? or is per-"find()" ok?
- memory barriers where?
*/




#define K 2
#define R 10

typedef void* Key;

// for stack-allocated retired_nodes array
static const uint MAX_THREADS = 32;

static intptr_t active_threads;


// Nodes are internal to this module. having callers add those directly would
// be more convenient but risky, since they might change <next> and <key>,
// or not allocate via malloc (necessary since Nodes are garbage-collected
// and allowing user-specified destructors would be more work).
//
// to still allow storing arbitrary user data without requiring an
// additional memory alloc per node, we append <user_size> bytes to the
// end of the Node structure; this is what is returned by find.


		// this is exposed to users of the lock-free data structures.
		//
		//
		// rationale: to avoid unnecessary mem allocs and increase locality,
		// we want to store user data in the Node itself. an alternative would
		// be to pass user_data_size in bytes to insert(), and have find() return a
		// pointer to this user data. however, that interface is less obvious, and
		// users will often want to set the data immediately after insertion
		// (which would require either a find(), or returning a pointer during
		// insertion - ugly).

struct Node
{
	Node* next;
	Key key;

	// <user_size> bytes are allocated here at the caller's discretion.
};

static Node* node_alloc(size_t additional_bytes)
{
	return (Node*)calloc(1, sizeof(Node) + additional_bytes);
}

static void node_free(Node* n)
{
	free(n);
}

static void* node_user_data(Node* n)
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

	void* hp[K];
	uintptr_t active;	// used as bool, but set by CAS

	Node* retired_nodes[R];
	size_t num_retired_nodes;
};

static TLS* tls_list = 0;


// (called from pthread dtor; registered in tls_init)
static void tls_retire(void* tls_)
{
	TLS* tls = (TLS*)tls_;

	// our hazard pointers are no longer in use
	for(size_t i = 0; i < K; i++)
		tls->hp[i] = 0;

	tls->active = 0;

	atomic_add(&active_threads, -1);
	assert2(active_threads >= 0);
}


static void tls_init()
{
	int ret = pthread_key_create(&tls_key, tls_retire);
	assert2(ret == 0);
}


static TLS* tls_alloc()
{
	atomic_add(&active_threads, 1);

	TLS* tls;

	// try to reuse a retired TLS slot
	for(tls = tls_list; tls; tls = tls->next)
		// succeeded in reactivating one
		if(CAS(&tls->active, 0, 1))
			goto have_tls;

	// no unused slots available - allocate another
	{
	tls = (TLS*)calloc(1, sizeof(TLS));
	tls->active = 1;
	// .. and insert at front of list (wait free since # threads is finite)
	TLS* old_tls_list;
	do
	{
		old_tls_list = tls_list;
		tls->next = old_tls_list;
	}
	while(!CAS(&tls_list, old_tls_list, tls));
	}

have_tls:
	int ret = pthread_setspecific(tls_key, tls);
	assert2(ret == 0);
	return tls;
}


static TLS* tls_get()
{
	int ret = pthread_once(&tls_once, tls_init);
	assert2(ret == 0);

	// already allocated - return it
	TLS* tls = (TLS*)pthread_getspecific(tls_key);
	if(tls)
		return tls;

	return tls_alloc();
}


// call via reference count - when last data structure is no longer in use,
// we can free all TLS info.
static void tls_shutdown()
{
	int ret = pthread_key_delete(tls_key);
	assert2(ret == 0);

	while(tls_list)
	{
		TLS* tls = tls_list;
		tls_list = tls_list->next;
		free(tls);
	}
}


//////////////////////////////////////////////////////////////////////////////
//
// "Safe Memory Reclamation for Lock-Free Objects" via hazard pointers
//
//////////////////////////////////////////////////////////////////////////////

static bool is_node_referenced(Node* node, void** hps, size_t num_hps)
{
	for(size_t i = 0; i < num_hps; i++)
		if(hps[i] == node)
			return true;

	return false;
}


// "Scan"
static void release_unreferenced_nodes(TLS* tls)
{
	// required for head/tail below; guaranteed by callers.
	assert2(tls->num_retired_nodes != 0);

	//
	// build array of all active (non-NULL) hazard pointers (more efficient
	// than walking through tls_list on every is_node_referenced call)
	//
try_again:
	const size_t max_hps = (active_threads+3) * K;
		// allow for creating a few additional threads during the loop
	void** hps = (void**)alloca(max_hps * sizeof(void*));
	size_t num_hps = 0;
	// for each participating thread:
	for(TLS* t = tls_list; t; t = t->next)
		// for each of its non-NULL hazard pointers:
		for(int i = 0; i < K-1; i++)
		{
			void* hp = t->hp[i];
			if(!hp)
				continue;

			// many threads were created after choosing max_hps =>
			// start over. not expected to happen.
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
		if(is_node_referenced(node, hps, num_hps))
			head++;
		else
		{
			node_free(node);

			*head = *tail;	// if last element, no-op
			tail--;
			tls->num_retired_nodes--;
		}
	}
}


// "HelpScan"
// if a TLS slot with retired Nodes happens not to be reused,
// we can still release that memory.
static void clear_old_retired_lists(TLS* tls)
{
	for(TLS* t = tls_list; t; t = t->next)
	{
		// succeeded in reactivating one
		if(!CAS(&t->active, 0, 1))
			continue;

		// no locking needed because no one is using <t>
		// (it was retired and can't be reactivated until active = 0)

		while(t->num_retired_nodes > 0)
		{
			Node* node = t->retired_nodes[--t->num_retired_nodes];
			tls->retired_nodes[tls->num_retired_nodes++] = node;
			if(tls->num_retired_nodes >= R)
				release_unreferenced_nodes(tls);
		}

		tls->active = 0;
	}
}


static void retire_node(Node* node)
{
	TLS* tls = tls_get();

	tls->retired_nodes[tls->num_retired_nodes++] = node;
	if(tls->num_retired_nodes >= R)
	{
		release_unreferenced_nodes(tls);
		clear_old_retired_lists(tls);
	}
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




static void lfl_init(void** phead)
{
	*phead = 0;
	// TODO: refcount for module shutdown
}


// call when list is no longer needed; may still hold references
static void lfl_free(void** phead)
{
	// TODO: refcount for module shutdown

	// TODO: is this safe?
	Node* cur = *((Node**)phead);
	while(cur)
	{
		retire_node(cur);
		cur = cur->next;
	}
}


// "Find"
// look for a given key in the list; return true with <pos>
static bool list_lookup(void** phead, Key key, ListPos* pos)
{
	TLS* tls = tls_get();
	void** hp0 = &tls->hp[0];	// protects cur
	void** hp1 = &tls->hp[1];	// protects *pprev

try_again:
	pos->pprev = (Node**)phead;
		// linearization point of erase and find if list is empty.
		// already protected by virtue of being the root node.
	pos->cur = *pos->pprev;

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

			retire_node(pos->cur);
			pos->cur = next;
		}
		else
		{
			// (see above)
			if(*pos->pprev != pos->cur)
				goto try_again;

			// the nodes are sorted in ascending key order, so we've either
			// found <key>, or it's not in the list.
			const Key cur_key = pos->cur->key;
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
void* lfl_find(void** phead, Key key)
{
	ListPos* pos = (ListPos*)alloca(sizeof(ListPos));
	if(!list_lookup(phead, key, pos))
		return 0;
	return node_user_data(pos->cur);
}


// insert into list in order of increasing key. ensures items are unique
// by first checking if already in the list. returns 0 if out of memory,
// otherwise a pointer to "user data" attached to <key>. the optional
// <was_inserted> return variable indicates whether <key> was newly added.
void* lfl_insert(void** phead, Key key, size_t additional_bytes, int* was_inserted)
{
	TLS* tls = tls_get();
	ListPos* pos = (ListPos*)alloca(sizeof(ListPos));

	Node* node = 0;
	if(was_inserted)
		*was_inserted = 0;

try_again:
	// already in list - return it and leave <was_inserted> 'false'
	if(list_lookup(phead, key, pos))
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
		Node* node = node_alloc(additional_bytes);
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
int lfl_erase(void** phead, Key key)
{
	TLS* tls = tls_get();
	ListPos* pos = (ListPos*)alloca(sizeof(ListPos));

try_again:
	// not found in list - abort.
	if(!list_lookup(phead, key, pos))
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
		retire_node(pos->cur);
	// failed: another thread removed cur after it was marked above.
	// call list_lookup to ensure # non-released nodes < # threads.
	else
		list_lookup(phead, key, pos);
	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
// lock-free hash table
//
//////////////////////////////////////////////////////////////////////////////

static const size_t SZ = 32;
static Node* T[SZ];
static size_t h(Key key)
{
	return ((uintptr_t)key) % SZ;
}


void* lfh_find(Key key)
{
	void** phead = (void**)&T[h(key)];
	return lfl_find(phead, key);
}

void* lfh_insert(Key key, size_t additional_bytes, int* was_inserted)
{
	void** phead = (void**)&T[h(key)];
	return lfl_insert(phead, key, additional_bytes, was_inserted);
}

int lfh_erase(Key key)
{
	void** phead = (void**)&T[h(key)];
	return lfl_erase(phead, key);
}


//////////////////////////////////////////////////////////////////////////////
//
// built-in self test
//
//////////////////////////////////////////////////////////////////////////////

static int test()
{
	void* head = 0;
	lfl_init(&head);

	const uint ENTRIES = 10;

	Key key = (Key)0x1000;
	int sig = 10;
	for(uint i = 0; i < ENTRIES; i++)
	{
		void* user_data = lfl_insert(&head, (u8*)key+i, sizeof(int), 0);
		assert2(user_data != 0);

		*(int*)user_data = sig+i;
	}

	for(uint i = 0; i < ENTRIES; i++)
	{
		debug_out("looking for key: %p sig: %d", (u8*)key+i, sig+i);
		void* user_data = lfl_find(&head, (u8*)key+i);
		assert2(user_data != 0);
		assert2(*(int*)user_data == sig+i);
	}

	return 0;
}


static int dummy = test();