#ifndef MEM_H
#define MEM_H

#include "handle.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef void(*MEM_DTOR)(void* p, size_t size, uintptr_t ctx);

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

// returns 0 if the handle is invalid
extern void* mem_get_ptr(Handle h, size_t* size = 0);


#ifdef __cplusplus
}
#endif

#endif	// #ifndef MEM_H
