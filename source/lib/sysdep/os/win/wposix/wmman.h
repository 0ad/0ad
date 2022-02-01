/* Copyright (C) 2022 Wildfire Games.
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

#ifndef INCLUDED_WMMAN
#define INCLUDED_WMMAN

//
// <sys/mman.h>
//

// mmap prot flags
#define PROT_NONE   0x00
#define PROT_READ	0x01
#define PROT_WRITE	0x02
#define PROT_EXEC   0x04

// mmap flags
#define MAP_SHARED	0x01	// writes change the underlying file
#define MAP_PRIVATE	0x02	// writes do not affect the file (copy-on-write)
#define MAP_FIXED	0x04
// .. non-portable
#define MAP_ANONYMOUS 0x10	// backed by the pagefile; fd should be -1
#define MAP_NORESERVE 0x20	// see below

// note: we need a means of only "reserving" virtual address ranges
// for the fixed-address expandable array mechanism. the non-portable
// MAP_NORESERVE flag says that no space in the page file need be reserved.
// the caller can still try to access the entire mapping, but might get
// SIGBUS if there isn't enough room to commit a page. Linux currently
// doesn't commit mmap-ed regions anyway, but we specify this flag to
// make sure of that in the future.

#define MAP_FAILED ((void*)intptr_t(-1))

extern void* mmap(void* start, size_t len, int prot, int flags, int fd, off_t offset);
extern int munmap(void* start, size_t len);

extern int mprotect(void* addr, size_t len, int prot);

// convert POSIX PROT_* flags to their Win32 PAGE_* enumeration equivalents.
unsigned MemoryProtectionFromPosix(int prot);

#endif	// #ifndef INCLUDED_WMMAN
