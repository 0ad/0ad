#ifndef ALLOC_H
#define ALLOC_H

/*#if defined(_WIN32) && defined(_MEM_DEBUG)
	void *c_alloc(unsigned long size, unsigned long num, const char *file, unsigned long line);
	void *m_alloc(unsigned long size, const char *file, unsigned long line);
	void f_ree(void *ptr);

	#ifdef malloc
	#undef malloc
	#endif

	#ifdef calloc
	#undef calloc
	#endif

	#ifdef free
	#undef free
	#endif


	#define malloc(size) m_alloc(size, __FILE__, __LINE__)
	#define calloc(size, num) c_alloc(size, num, __FILE__, __LINE__)
	#define free(addr) f_ree(addr)
#endif//defined(_WIN32) && defined(_MEM_DEBUG)*/


#if defined (__ALLOC_C)
	#define __ALLOC_EXTERN
#else
	#define __ALLOC_EXTERN extern
#endif
#include <IL/il.h>


__ALLOC_EXTERN mAlloc ialloc_ptr;
__ALLOC_EXTERN mFree  ifree_ptr;



#endif//ALLOC_H
