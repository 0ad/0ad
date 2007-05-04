#ifndef INCLUDED_IA32_MEMCPY
#define INCLUDED_IA32_MEMCPY

#ifdef __cplusplus
extern "C" {
#endif

extern void ia32_memcpy_init();

extern void* ia32_memcpy(void* RESTRICT dst, const void* RESTRICT src, size_t nbytes);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef INCLUDED_IA32_MEMCPY
