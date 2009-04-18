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
 * File        : mem_util.h
 * Project     : 0 A.D.
 * Description : memory allocator helper routines.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_MEM_UTIL
#define INCLUDED_MEM_UTIL

LIB_API bool mem_IsPageMultiple(uintptr_t x);

LIB_API size_t mem_RoundUpToPage(size_t size);
LIB_API size_t mem_RoundUpToAlignment(size_t size);


// very thin wrapper on top of sys/mman.h that makes the intent more obvious
// (its commit/decommit semantics are difficult to tell apart)
LIB_API LibError mem_Reserve(size_t size, u8** pp);
LIB_API LibError mem_Release(u8* p, size_t size);
LIB_API LibError mem_Commit(u8* p, size_t size, int prot);
LIB_API LibError mem_Decommit(u8* p, size_t size);
LIB_API LibError mem_Protect(u8* p, size_t size, int prot);


// note: element memory is used to store a pointer to the next free element.
// rationale for the function-based interface: a class encapsulating the
// freelist pointer would force each header to include mem_util.h;
// instead, implementations need only declare a void* pointer.
LIB_API void mem_freelist_AddToFront(void*& freelist, void* el);
LIB_API void* mem_freelist_Detach(void*& freelist);

#endif	// #ifndef INCLUDED_MEM_UTIL
