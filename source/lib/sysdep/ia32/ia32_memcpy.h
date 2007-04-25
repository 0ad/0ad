#ifndef INCLUDED_IA32_MEMCPY
#define INCLUDED_IA32_MEMCPY

#include "lib/sysdep/sysdep.h"

extern "C" {

extern void ia32_memcpy_init();

#define RESTRICT __restrict 
extern void* ia32_memcpy(void* RESTRICT dst, const void* RESTRICT src, size_t nbytes);

}

#endif	// #ifndef INCLUDED_IA32_MEMCPY
