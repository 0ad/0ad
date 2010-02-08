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
 * interface to various IA-32 functions (written in asm)
 */

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
