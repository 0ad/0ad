#ifndef __MEM_H__
#define __MEM_H__

enum MemType
{
	MEM_POOL,
	MEM_HEAP
};

extern void* mem_alloc(size_t size, MemType type = MEM_HEAP, uint align = 1);
extern int mem_free(void* p);

#endif	// #ifndef __MEM_H__
