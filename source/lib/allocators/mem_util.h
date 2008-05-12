/**
 * =========================================================================
 * File        : mem_util.h
 * Project     : 0 A.D.
 * Description : memory allocator helper routines.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_MEM_UTIL
#define INCLUDED_MEM_UTIL

extern bool mem_IsPageMultiple(uintptr_t x);

extern size_t mem_RoundUpToPage(size_t size);
extern size_t mem_RoundUpToAlignment(size_t size);


// very thin wrapper on top of sys/mman.h that makes the intent more obvious
// (its commit/decommit semantics are difficult to tell apart)
extern LibError mem_Reserve(size_t size, u8** pp);
extern LibError mem_Release(u8* p, size_t size);
extern LibError mem_Commit(u8* p, size_t size, int prot);
extern LibError mem_Decommit(u8* p, size_t size);
extern LibError mem_Protect(u8* p, size_t size, int prot);


// note: element memory is used to store a pointer to the next free element.
// rationale for the function-based interface: a class encapsulating the
// freelist pointer would force each header to include mem_util.h;
// instead, implementations need only declare a void* pointer.
extern void mem_freelist_AddToFront(void*& freelist, void* el);
extern void* mem_freelist_Detach(void*& freelist);

#endif	// #ifndef INCLUDED_MEM_UTIL
