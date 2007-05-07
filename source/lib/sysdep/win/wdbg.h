/**
 * =========================================================================
 * File        : wdbg.h
 * Project     : 0 A.D.
 * Description : Win32 debug support code and exception handler.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WDBG
#define INCLUDED_WDBG

#if HAVE_MS_ASM
# define debug_break() __asm { int 3 }
#else
# error "port this or define to implementation function"
#endif

// internal use only:
extern void wdbg_set_thread_name(const char* name);

// see rationale at definition.
struct _EXCEPTION_POINTERS;
extern long __stdcall wdbg_exception_filter(_EXCEPTION_POINTERS* ep);

#endif	// #ifndef INCLUDED_WDBG
