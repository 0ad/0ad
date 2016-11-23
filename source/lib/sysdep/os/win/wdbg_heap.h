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
 * improved debug heap using MS CRT
 */

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
LIB_API void wdbg_heap_Validate();

/**
 * @return the total number of alloc and realloc operations thus far.
 * used by the in-game profiler.
 **/
LIB_API intptr_t wdbg_heap_NumberOfAllocations();

#endif	// #ifndef INCLUDED_WDBG_HEAP
