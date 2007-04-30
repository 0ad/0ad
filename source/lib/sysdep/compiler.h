#ifndef INCLUDED_COMPILER
#define INCLUDED_COMPILER


// pass "omit frame pointer" setting on to the compiler
#if MSC_VERSION
# if CONFIG_OMIT_FP
#  pragma optimize("y", on)
# else
#  pragma optimize("y", off)
# endif
#elif GCC_VERSION
// TODO
#endif


// try to define _W64, if not already done
// (this is useful for catching pointer size bugs)
#ifndef _W64
# if MSC_VERSION
#  define _W64 __w64
# elif GCC_VERSION
#  define _W64 __attribute__((mode (__pointer__)))
# else
#  define _W64
# endif
#endif


// C99-like restrict (non-standard in C++, but widely supported in various forms).
//
// May be used on pointers. May also be used on member functions to indicate
// that 'this' is unaliased (e.g. "void C::m() RESTRICT { ... }").
// Must not be used on references - GCC supports that but VC doesn't.
//
// We call this "RESTRICT" to avoid conflicts with VC's __declspec(restrict),
// and because it's not really the same as C99's restrict.
//
// To be safe and satisfy the compilers' stated requirements: an object accessed
// by a restricted pointer must not be accessed by any other pointer within the
// lifetime of the restricted pointer, if the object is modified.
// To maximise the chance of optimisation, any pointers that could potentially
// alias with the restricted one should be marked as restricted too.
//
// It would probably be a good idea to write test cases for any code that uses
// this in an even very slightly unclear way, in case it causes obscure problems
// in a rare compiler due to differing semantics.
//
// .. we made g++ claim to support C99 by defining __STDC_VERSION__ in the
//    makefiles, but that's wrong and it doesn't have the restrict keyword.
//    use the extension __restrict__ instead.
#if GCC_VERSION
# define RESTRICT __restrict__
// .. already available in C99 as 'restrict'
#elif HAVE_C99
# define RESTRICT restrict
// .. VC8 provides __restrict
#elif MSC_VERSION >= 1400
# define RESTRICT __restrict
// .. ICC supports the keyword 'restrict' when run with the /Qrestrict option,
//    but it always also supports __restrict__ or __restrict to be compatible
//    with GCC/MSVC, so we'll use the underscored version. One of {GCC,MSC}_VERSION
//    should have been defined in addition to ICC_VERSION, so we should be using
//    one of the above cases (unless it's an old VS7.1-emulating ICC).
#elif ICC_VERSION
# error ICC_VERSION defined without either GCC_VERSION or an adequate MSC_VERSION
// .. unsupported; remove it from code
#else
# define RESTRICT
#endif


// C99 __func__
// .. already available; need do nothing
#if HAVE_C99
// .. VC supports something similar
#elif MSC_VERSION
# define __func__ __FUNCTION__
// .. unsupported; remove it from code
#else
# define __func__ "(unknown)"
#endif


// tell the compiler that the code at/following this macro invocation is
// unreachable. this can improve optimization and avoid warnings.
//
// this macro should not generate any fallback code; it is merely the
// compiler-specific backend for lib.h's UNREACHABLE.
// #define it to nothing if the compiler doesn't support such a hint.
#if MSC_VERSION
# define ASSUME_UNREACHABLE __assume(0)
#else
# define ASSUME_UNREACHABLE
#endif


#endif	// #ifndef INCLUDED_COMPILER
