/**
 * =========================================================================
 * File        : wseh.h
 * Project     : 0 A.D.
 * Description : Win32 debug support code and exception handler.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WSEH
#define INCLUDED_WSEH

extern LONG WINAPI wseh_ExceptionFilter(EXCEPTION_POINTERS* ep);

EXTERN_C int wseh_EntryPoint();

#endif	// #ifndef INCLUDED_WSEH
