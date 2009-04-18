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
 * File        : ia32_memcpy.h
 * Project     : 0 A.D.
 * Description : interface to various IA-32 functions (written in asm)
 * =========================================================================
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
