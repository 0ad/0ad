#ifndef __MEM_H__
#define __MEM_H__

#include "res.h"

// include memory mappings because the VFS may return either a chunk of
// memory, or the mapped file, and a client doesn't know the difference.

enum MemType
{
	MEM_POOL,
	MEM_HEAP,
	MEM_MAPPED
};


struct MEM
{
	void* p;
	size_t size;

	MemType type;

	union
	{
		size_t ofs;		// MEM_POOL only
		void* org_p;	// MEM_HEAP only
		int fd;			// MEM_MAPPED only
	};
};

extern void* mem_alloc(size_t size, uint align = 1, MemType type = MEM_HEAP, int fd = -1, Handle* ph = 0);
extern int mem_free(void* p);

// faster than mem_free(void*) - no scan of open handles for the pointer
extern int mem_free(Handle hm);

#endif	// #ifndef __MEM_H__
