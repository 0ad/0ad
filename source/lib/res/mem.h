/**
* =========================================================================
* File        : mem.h
* Project     : 0 A.D.
* Description : wrapper that treats memory as a "resource", i.e.
*             : guarantees its lifetime and automatic release.
*
* @author Jan.Wassenberg@stud.uni-karlsruhe.de
* =========================================================================
*/

/*
* Copyright (c) 2003 Jan Wassenberg
*
* Redistribution and/or modification are also permitted under the
* terms of the GNU General Public License as published by the
* Free Software Foundation (version 2 or later, at your option).
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef MEM_H__
#define MEM_H__

#include "handle.h"


typedef void (*MEM_DTOR)(void* p, size_t size, uintptr_t ctx);

// mem_alloc flags
enum
{
	// RES_*

	MEM_ZERO = 0x1000
};

extern void* mem_alloc(size_t size, size_t align = 1, uint flags = 0, Handle* ph = 0);

#define mem_free(p) mem_free_p((void*&)p)
extern LibError mem_free_p(void*& p);

extern LibError mem_free_h(Handle& hm);

// returns 0 if the handle is invalid
extern void* mem_get_ptr(Handle h, size_t* size = 0);

extern LibError mem_get(Handle hm, u8** pp, size_t* psize);


extern Handle mem_wrap(void* p, size_t size, uint flags, void* raw_p, size_t raw_size, MEM_DTOR dtor, uintptr_t ctx, void* owner);

// exception to normal resource shutdown: must not be called before
// h_mgr_shutdown! (this is because h_mgr calls us to free memory, which
// requires the pointer -> Handle index still be in place)
extern void mem_shutdown(void);



#endif	// #ifndef MEM_H__
