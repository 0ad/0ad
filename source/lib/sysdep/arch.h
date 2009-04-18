/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

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

// ensure exactly one architecture has been detected
#if (ARCH_IA32+ARCH_IA64+ARCH_AMD64+ARCH_ALPHA+ARCH_ARM+ARCH_MIPS) != 1
# error "architecture not correctly detected (either none or multiple ARCH_* defined)"
#endif

// "X86_X64"-specific code requires either IA-32 or AMD64
#define ARCH_X86_X64 (ARCH_IA32|ARCH_AMD64)

#endif	// #ifndef INCLUDED_ARCH
