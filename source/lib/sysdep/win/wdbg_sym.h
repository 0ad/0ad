/**
 * =========================================================================
 * File        : wdbg_sym.h
 * Project     : 0 A.D.
 * Description : Win32 stack trace and symbol engine.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WDBG_SYM
#define INCLUDED_WDBG_SYM

struct _EXCEPTION_POINTERS;
extern void wdbg_sym_write_minidump(_EXCEPTION_POINTERS* ep);

#endif	// #ifndef INCLUDED_WDBG_SYM
