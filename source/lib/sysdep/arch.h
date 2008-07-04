/**
 * =========================================================================
 * File        : arch.h
 * Project     : 0 A.D.
 * Description : CPU architecture detection.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_ARCH
#define INCLUDED_ARCH

// detect target CPU architecture via predefined macros
// .. IA-32
#if defined(_M_IX86) || defined(i386) || defined(_X86_)
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
#if defined(_M_X64) || defined(__amd64__) || defined(__amd64)
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
// .. MIPS
#if defined(__MIPS__) || defined(__mips__) || defined(__mips)
# define ARCH_MIPS 1
#else
# define ARCH_MIPS 0
#endif

#endif	// #ifndef INCLUDED_ARCH
