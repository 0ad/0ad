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
 * File        : wdbg_heap.h
 * Project     : 0 A.D.
 * Description : improved debug heap using MS CRT
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WDBG_HEAP
#define INCLUDED_WDBG_HEAP

// this module provides a more convenient interface to the MS CRT's
// debug heap checks. it also hooks into allocations to record the
// caller/owner information without requiring macros (which break code
// using placement new or member functions called free).

/**
 * enable or disable manual and automatic heap validity checking.
 * (enabled by default during critical_init.)
 **/
LIB_API void wdbg_heap_Enable(bool);

/**
 * check heap integrity.
 * errors are reported by the CRT or via debug_DisplayError.
 * no effect if called between wdbg_heap_Enable(false) and the next
 * wdbg_heap_Enable(true).
 **/
LIB_API void wdbg_heap_Validate(void);

/**
 * @return the total number of alloc and realloc operations thus far.
 * used by the in-game profiler.
 **/
LIB_API intptr_t wdbg_heap_NumberOfAllocations();

#endif	// #ifndef INCLUDED_WDBG_HEAP
