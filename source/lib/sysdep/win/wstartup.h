/**
 * =========================================================================
 * File        : wstartup.h
 * Project     : 0 A.D.
 * Description : windows-specific startup code
 * =========================================================================
 */

// license: GPL; see lib/license.txt

// linking with this component automatically causes winit's functions to
// be called at the appropriate times.
//
// the current implementation manages to trigger winit initialization
// between CRT init and static C++ ctors. that means wpthread etc. APIs
// are safe to use from the ctors and also that winit initializers are
// allowed to use non-stateless CRT functions such as atexit.

#ifndef INCLUDED_WSTARTUP
#define INCLUDED_WSTARTUP

#endif	// #ifndef INCLUDED_WSTARTUP
