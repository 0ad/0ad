/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * memory allocator helper routines.
 */

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
