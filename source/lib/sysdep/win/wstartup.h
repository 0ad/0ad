/**
 * =========================================================================
 * File        : wstartup.h
 * Project     : 0 A.D.
 * Description : windows-specific entry point and startup code
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WSTARTUP
#define INCLUDED_WSTARTUP

// entry points (normal and without SEH wrapper; see definition)
EXTERN_C int entry();
EXTERN_C int entry_noSEH();

#endif	// #ifndef INCLUDED_WSTARTUP
