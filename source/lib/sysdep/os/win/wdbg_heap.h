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
