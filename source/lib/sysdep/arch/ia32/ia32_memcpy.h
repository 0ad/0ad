/**
 * =========================================================================
 * File        : ia32_memcpy.h
 * Project     : 0 A.D.
 * Description : interface to various IA-32 functions (written in asm)
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_IA32_MEMCPY
#define INCLUDED_IA32_MEMCPY

#ifdef __cplusplus
extern "C" {
#endif

/**
 * drop-in replacement for POSIX memcpy.
 * highly optimized for Athlon and Pentium III microarchitectures;
 * significantly outperforms VC7.1 memcpy and memcpy_amd.
 * for details, see "Speeding Up Memory Copy".
 **/
extern void* ia32_memcpy(void* RESTRICT dst, const void* RESTRICT src, size_t size);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef INCLUDED_IA32_MEMCPY
