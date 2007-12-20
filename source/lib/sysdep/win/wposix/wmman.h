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
#define MAP_ANONYMOUS 0x10 
#define MAP_NORESERVE 0x20

// note: we need a means of only "reserving" virtual address ranges
// for the fixed-address expandable array mechanism. the non-portable
// MAP_NORESERVE flag says that no space in the page file need be reserved.
// the caller can still try to access the entire mapping, but might get
// SIGBUS if there isn't enough room to commit a page. Linux currently
// doesn't commit mmap-ed regions anyway, but we specify this flag to
// make sure of that in the future.

#define MAP_FAILED ((void*)(intptr_t)-1L)

extern void* mmap(void* start, size_t len, int prot, int flags, int fd, off_t offset);
extern int munmap(void* start, size_t len);

extern int mprotect(void* addr, size_t len, int prot); 

#endif	// #ifndef INCLUDED_WMMAN
