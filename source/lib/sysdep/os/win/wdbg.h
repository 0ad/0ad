/**
 * =========================================================================
 * File        : wdbg.h
 * Project     : 0 A.D.
 * Description : Win32 debug support code.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WDBG
#define INCLUDED_WDBG

/**
 * same as debug_printf except that some type conversions aren't supported
 * (in particular, no floating point).
 *
 * this function does not allocate memory from the CRT heap, which makes it
 * safe to use from an allocation hook.
 **/
LIB_API void wdbg_printf(const char* fmt, ...);

/**
 * similar to debug_assert but safe to use during critical init or
 * while under the heap or dbghelp locks.
 **/
#define wdbg_assert(expr) STMT(if(!(expr)) debug_break();)

#endif	// #ifndef INCLUDED_WDBG
