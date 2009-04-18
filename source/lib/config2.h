/**
 * =========================================================================
 * File        : config2.h
 * Project     : 0 A.D.
 * Description : compile-time configuration for isolated spots
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_CONFIG2
#define INCLUDED_CONFIG2

// rationale: a centralized header makes it much easier to see what all
// can be changed. it is assumed that only a few modules will need
// configuration choices, so rebuilding them all is acceptable.
// use config.h when settings must apply to the entire project.

// allow use of RDTSC for raw tick counts (otherwise, the slower but
// more reliable on MP systems wall-clock will be used).
#ifndef CONFIG2_TIMER_ALLOW_RDTSC
# define CONFIG2_TIMER_ALLOW_RDTSC 1
#endif

// this enables/disables the actual checking done by OverrunProtector
// (quite slow, entailing mprotect() before/after each access).
// define to 1 here or in the relevant module if you suspect mem corruption.
// we provide this option because OverrunProtector requires some changes to
// the object being wrapped, and we want to leave those intact but not
// significantly slow things down except when needed.
#ifndef CONFIG2_ALLOCATORS_OVERRUN_PROTECTION
# define CONFIG2_ALLOCATORS_OVERRUN_PROTECTION 0
#endif

// zero-copy IO means all clients share the cached buffer; changing their
// contents is forbidden. this flag causes the buffers to be marked as
// read-only via MMU (writes would cause an exception), which takes a
// bit of extra time.
#ifndef CONFIG2_CACHE_READ_ONLY
#define CONFIG2_CACHE_READ_ONLY 1
#endif

// enable the wsdl emulator in Windows builds.
//
// NOTE: the official SDL distribution has two problems on Windows:
// - it specifies "/defaultlib:msvcrt.lib". this is troublesome because
//   multiple heaps are active; errors result when allocated blocks are
//   (for reasons unknown) passed to a different heap to be freed.
//   one workaround is to add "/nodefaultlib:msvcrt.lib" to the linker
//   command line in debug configurations.
// - it doesn't support color hardware mouse cursors and clashes with
//   cursor.cpp's efforts by resetting the mouse cursor after movement.
#ifndef CONFIG2_WSDL
# define CONFIG2_WSDL 1
#endif

#endif	// #ifndef INCLUDED_CONFIG2
