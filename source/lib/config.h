/**
 * =========================================================================
 * File        : config.h
 * Project     : 0 A.D.
 * Description : user-specified compile-time configuration.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_CONFIG
#define INCLUDED_CONFIG

// the config macros are always defined; their values (1 or 0) are
// tested with #if instead of #ifdef.
// this protects user code from typos such as #ifdef _MSC_VEER, which
// would silently remove code. instead, we will at least get "test of
// undefined macro" warnings here. not including this header also triggers
// such warnings, but won't happen often because we're included from PCH.
//
// allow override via compiler settings by checking #ifndef.

// omit frame pointers - stack frames will not be generated, which
// improves performance but makes walking the stack harder.
// this is acted upon by #pragmas in sysdep.h.
#ifndef CONFIG_OMIT_FP
# ifdef NDEBUG
#  define CONFIG_OMIT_FP 1
# else
#  define CONFIG_OMIT_FP 0
# endif
#endif

// (only applicable if ARCH_IA32) 64-bit values will be returned in EDX:EAX.
// this chiefly affects ia32_rdtsc. if not set, a safer but slower fallback
// will be used that doesn't assume anything about return convention.
#ifndef CONFIG_RETURN64_EDX_EAX
# define CONFIG_RETURN64_EDX_EAX 1
#endif

// allow use of RDTSC for raw tick counts (otherwise, the slower but
// more reliable on MP systems wall-clock will be used).
#ifndef CONFIG_TIMER_ALLOW_RDTSC
# define CONFIG_TIMER_ALLOW_RDTSC 1
#endif

// this enables/disables the actual checking done by OverrunProtector-s
// (quite slow, entailing mprotect() before/after each access).
// define to 1 here or in the relevant module if you suspect mem corruption.
// we provide this option because OverrunProtector requires some changes to
// the object being wrapped, and we want to leave those intact but not
// significantly slow things down except when needed.
#ifndef CONFIG_OVERRUN_PROTECTION
# define CONFIG_OVERRUN_PROTECTION 0
#endif

// zero-copy IO means all clients share the cached buffer; changing their
// contents is forbidden. this flag causes the buffers to be marked as
// read-only via MMU (writes would cause an exception), which takes a
// bit of extra time.
#ifndef CONFIG_READ_ONLY_CACHE
#define CONFIG_READ_ONLY_CACHE 1
#endif

// enable memory tracking (slow). see mmgr.cpp.
#ifndef CONFIG_USE_MMGR
# define CONFIG_USE_MMGR 0
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
#ifndef CONFIG_USE_WSDL
# define CONFIG_USE_WSDL 1
#endif

// enable additional debug checks (very slow).
#ifndef CONFIG_PARANOIA
# define CONFIG_PARANOIA 0
#endif

// final release; disables some safety checks.
#ifndef CONFIG_FINAL
# define CONFIG_FINAL 0
#endif

// enable trace output for low-level code - various functions will
// debug_printf when they are entered/exited. note that the appropriate
// TRACEn tags must be debug_filter_add-ed for this to have any effect.
#ifndef CONFIG_TRACE
# define CONFIG_TRACE 0
#endif

// try to prevent any exceptions from being thrown - even by the C++
// standard library. useful only for performance tests.
#ifndef CONFIG_DISABLE_EXCEPTIONS
# define CONFIG_DISABLE_EXCEPTIONS 0
#endif

// precompiled headers (affects what precompiled.h pulls in; see there)
#ifndef CONFIG_ENABLE_PCH
# define CONFIG_ENABLE_PCH 1
#endif

#endif	// #ifndef INCLUDED_CONFIG
