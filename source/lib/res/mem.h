#ifndef MEM_H
#define MEM_H

#include "h_mgr.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef void(*MEM_DTOR)(void* const p, const size_t size, const uintptr_t ctx);
	// VC6 requires const here if the actual implementations define as such.

// mem_alloc flags
enum
{
	// RES_*

	MEM_ZERO = 0x1000
};

extern void* mem_alloc(size_t size, size_t align = 1, uint flags = 0, Handle* ph = 0);

#define mem_free(p) mem_free_p((void*&)p)
extern int mem_free_p(void*& p);

extern int mem_free_h(Handle& hm);

// create a H_MEM handle of type MEM_USER,
// and assign it the specified memory range.
// dtor is called when the handle is freed, if non-NULL.
extern Handle mem_assign(void* p, size_t size, uint flags = 0, MEM_DTOR dtor = 0, uintptr_t ctx = 0);

extern int mem_assign_user(Handle hm, void* user_p, size_t user_size);

// returns 0 if the handle is invalid
extern void* mem_get_ptr(Handle h, size_t* size = 0);


#ifdef __cplusplus
}
#endif

#endif	// #ifndef MEM_H
