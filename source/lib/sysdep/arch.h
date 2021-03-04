/* Copyright (C) 2021 Wildfire Games.
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
 * CPU architecture detection.
 */

#ifndef INCLUDED_ARCH
#define INCLUDED_ARCH

// detect target CPU architecture via predefined macros
// .. IA-32
#if defined(_M_IX86) || defined(__X86__) || defined(_X86_) || defined(__i386__) || defined(__i386) || defined(i386)
# define ARCH_IA32 1
#else
# define ARCH_IA32 0
#endif
// .. IA-64
#if defined(_M_IA64) || defined(__ia64__)
# define ARCH_IA64 1
#else
# define ARCH_IA64 0
#endif
// .. AMD64
#if defined(_M_X64) || defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
# define ARCH_AMD64 1
#else
# define ARCH_AMD64 0
#endif
// .. Alpha
#if defined(_M_ALPHA) || defined(__alpha__) || defined(__alpha)
# define ARCH_ALPHA 1
#else
# define ARCH_ALPHA 0
#endif
// .. ARM
#if defined(__arm__)
# define ARCH_ARM 1
#else
# define ARCH_ARM 0
#endif
// .. AArch64 (ARM64)
#if defined(__aarch64__)
# define ARCH_AARCH64 1
#else
# define ARCH_AARCH64 0
#endif
// .. PowerPC64 (PPC64)
#if defined(__powerpc64__)
# define ARCH_PPC64 1
#else
# define ARCH_PPC64 0
#endif
// .. MIPS
#if defined(__MIPS__) || defined(__mips__) || defined(__mips)
# define ARCH_MIPS 1
#else
# define ARCH_MIPS 0
#endif
// .. E2K (MCST Elbrus 2000)
#if defined(__e2k__)
# define ARCH_E2K 1
#else
# define ARCH_E2K 0
#endif

// ensure exactly one architecture has been detected
#if (ARCH_IA32+ARCH_IA64+ARCH_AMD64+ARCH_ALPHA+ARCH_ARM+ARCH_AARCH64+ARCH_MIPS+ARCH_E2K+ARCH_PPC64) != 1
# error "architecture not correctly detected (either none or multiple ARCH_* defined)"
#endif

// "X86_X64"-specific code requires either IA-32 or AMD64
#define ARCH_X86_X64 (ARCH_IA32|ARCH_AMD64)

#endif	// #ifndef INCLUDED_ARCH
