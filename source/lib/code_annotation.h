/* Copyright (c) 2019 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * macros for code annotation.
 */

#ifndef INCLUDED_CODE_ANNOTATION
#define INCLUDED_CODE_ANNOTATION

#include "lib/sysdep/compiler.h"
#include "lib/sysdep/arch.h"	// ARCH_AMD64

/**
 * mark a function parameter as unused and avoid
 * the corresponding compiler warning.
 * wrap around the parameter name, e.g. void f(int UNUSED(x))
 **/
#define UNUSED(param)

/**
 * mark a function local variable or parameter as unused and avoid
 * the corresponding compiler warning.
 * note that UNUSED is not applicable to variable definitions that
 * involve initialization, nor is it sufficient in cases where
 * an argument is unused only in certain situations.
 * example: void f(int x) { ASSERT(x == 0); UNUSED2(x); }
 * this asserts in debug builds and avoids warnings in release.
 **/
#if HAVE_C99 && GCC_VERSION	// _Pragma from C99, unused from GCC
# define UNUSED2(param) _Pragma("unused " #param)
#elif ICC_VERSION
// ICC 12 still doesn't recognize pragma unused, casting to void
// isn't sufficient, and self-assignment doesn't work for references.
# define UNUSED2(param) do{ if(&param) {} } while(false)
#else
# define UNUSED2(param) ((void)(param))
#endif


/**
 * indicate a function will not throw any synchronous exceptions,
 * thus hopefully generating smaller and more efficient code.
 *
 * must be placed BEFORE return types because "The [VC++] compiler
 * ignores, without warning, any __declspec keywords placed after *".
 * such syntax is apparently also legal in GCC, per the example
 * "__attribute__((noreturn)) void d0 (void)".
 *
 * example:
 * NOTHROW_DECLARE void function();
 * NOTHROW_DEFINE void function() {}
 **/
#if GCC_VERSION
# define NOTHROW_DECLARE __attribute__((nothrow))
# define NOTHROW_DEFINE	// not supported for definitions
#elif MSC_VERSION
// Kevin Frei, 2006-03-23: "I work on the Visual C++ compiler team,
// and agree completely with Paul Parks: don't use throw(), because
// there's a chance that we'll eventually implement it according to the standard".
# define NOTHROW_DECLARE __declspec(nothrow)
# define NOTHROW_DEFINE	__declspec(nothrow)
#else
// don't use throw() because it might result in ADDITIONAL checks
// (the standard mandates calling unexpected())
# define NOTHROW_DECLARE
# define NOTHROW_DEFINE
#endif


/**
 * mark a function as noreturn for static analyzer purposes.
 * currently only for clang-analyzer.
 */
#if __has_feature(attribute_analyzer_noreturn)
# define ANALYZER_NORETURN __attribute__((analyzer_noreturn))
#else
# define ANALYZER_NORETURN
#endif


/**
 * "unreachable code" helpers
 *
 * unreachable lines of code are often the source or symptom of subtle bugs.
 * they are flagged by compiler warnings; however, the opposite problem -
 * erroneously reaching certain spots (e.g. due to missing return statement)
 * is worse and not detected automatically.
 *
 * to defend against this, the programmer can annotate their code to
 * indicate to humans that a particular spot should never be reached.
 * however, that isn't much help; better is a sentinel that raises an
 * error if if it is actually reached. hence, the UNREACHABLE macro.
 *
 * ironically, if the code guarded by UNREACHABLE works as it should,
 * compilers may flag the macro's code as unreachable. this would
 * distract from genuine warnings, which is unacceptable.
 *
 * even worse, compilers differ in their code checking: GCC only complains if
 * non-void functions end without returning a value (i.e. missing return
 * statement), while VC checks if lines are unreachable (e.g. if they are
 * preceded by a return on all paths).
 *
 * the implementation below enables optimization and automated checking
 * without raising warnings.
 **/
#define UNREACHABLE	// actually defined below.. this is for
# undef UNREACHABLE	// CppDoc's benefit only.

// this macro should not generate any fallback code; it is merely the
// compiler-specific backend for UNREACHABLE.
// #define it to nothing if the compiler doesn't support such a hint.
#define HAVE_ASSUME_UNREACHABLE 1
#if MSC_VERSION && !ICC_VERSION // (ICC ignores this)
# define ASSUME_UNREACHABLE __assume(0)
#elif GCC_VERSION
# define ASSUME_UNREACHABLE __builtin_unreachable()
#else
# define ASSUME_UNREACHABLE
# undef HAVE_ASSUME_UNREACHABLE
# define HAVE_ASSUME_UNREACHABLE 0
#endif

// compiler supports ASSUME_UNREACHABLE => allow it to assume the code is
// never reached (improves optimization at the cost of undefined behavior
// if the annotation turns out to be incorrect).
#if HAVE_ASSUME_UNREACHABLE && !CONFIG_ENABLE_CHECKS
# define UNREACHABLE ASSUME_UNREACHABLE
// otherwise (or if CONFIG_ENABLE_CHECKS is set), add a user-visible
// warning if the code is reached. note that abort() fails to stop
// ICC from warning about the lack of a return statement, so we
// use an infinite loop instead.
#else
# define UNREACHABLE\
	STMT(\
		DEBUG_WARN_ERR(ERR::LOGIC);	/* hit supposedly unreachable code */\
		for(;;){};\
	)
#endif

/**
convenient specialization of UNREACHABLE for switch statements whose
default can never be reached. example usage:
int x;
switch(x % 2)
{
	case 0: break;
	case 1: break;
	NODEFAULT;
}
**/
#define NODEFAULT default: UNREACHABLE


// generate a symbol containing the line number of the macro invocation.
// used to give a unique name (per file) to types or variables.
// we can't prepend __FILE__ to make it globally unique - the filename
// may be enclosed in quotes. PASTE3_HIDDEN__ is needed to make sure
// __LINE__ is expanded correctly.
#define PASTE3_HIDDEN__(a, b, c) a ## b ## c
#define PASTE3__(a, b, c) PASTE3_HIDDEN__(a, b, c)
#define UID__  PASTE3__(LINE_, __LINE__, _)
#define UID2__ PASTE3__(LINE_, __LINE__, _2)


//-----------------------------------------------------------------------------
// cassert

/**
 * Compile-time assertion. Causes a compile error if the expression
 * evaluates to zero/false.
 *
 * No runtime overhead; may be used anywhere, including file scope.
 * Especially useful for testing sizeof types.
 *
 * @param expr Expression that is expected to evaluate to non-zero at compile-time.
 **/
#define cassert(expr) static_assert((expr), #expr)


/**
 * Indicates that a class is noncopyable (usually due to const or reference
 * members, or because the class works as a singleton).
 *
 * For example:
 *
 * @code
 * class ClassName {
 *   NONCOPYABLE(ClassName);
 * public: // etc.
 * };
 * @endcode
 *
 * This is preferable to inheritance from boost::noncopyable because it avoids
 * ICC 11 W4 warnings about non-virtual dtors and suppression of the copy
 * assignment operator.
 */
#define NONCOPYABLE(className) \
	className(const className&) = delete; \
	className& operator=(const className&) = delete

/**
 * Indicates that move semantics can be used, so that a NONCOPYABLE class can still be assigned by taking over the reference to the value.
 * Make sure to use the macro with the necessary access modifier.
 */
#define MOVABLE(className) \
	className(className&&) = default; \
	className& operator=(className&&) = default

#if ICC_VERSION
# define ASSUME_ALIGNED(ptr, multiple) __assume_aligned(ptr, multiple)
#else
# define ASSUME_ALIGNED(ptr, multiple)
#endif

// annotate printf-style functions for compile-time type checking.
// fmtpos is the index of the format argument, counting from 1 or
// (if it's a non-static class function) 2; the '...' is assumed
// to come directly after it.
#if GCC_VERSION
# define PRINTF_ARGS(fmtpos) __attribute__ ((format (printf, fmtpos, fmtpos+1)))
# define VPRINTF_ARGS(fmtpos) __attribute__ ((format (printf, fmtpos, 0)))
# if CONFIG_DEHYDRA
#  define WPRINTF_ARGS(fmtpos) __attribute__ ((user("format, w, printf, " #fmtpos ", +1")))
# else
#  define WPRINTF_ARGS(fmtpos) /* not currently supported in GCC */
# endif
# define VWPRINTF_ARGS(fmtpos) /* not currently supported in GCC */
#else
# define PRINTF_ARGS(fmtpos)
# define VPRINTF_ARGS(fmtpos)
# define WPRINTF_ARGS(fmtpos)
# define VWPRINTF_ARGS(fmtpos)
// TODO: support _Printf_format_string_ for VC9+
#endif

// annotate vararg functions that expect to end with an explicit NULL
#if GCC_VERSION
# define SENTINEL_ARG __attribute__ ((sentinel))
#else
# define SENTINEL_ARG
#endif

/**
 * prevent the compiler from reordering loads or stores across this point.
 **/
#if ICC_VERSION
# define COMPILER_FENCE __memory_barrier()
#elif MSC_VERSION
# include <intrin.h>
# pragma intrinsic(_ReadWriteBarrier)
# define COMPILER_FENCE _ReadWriteBarrier()
#elif GCC_VERSION
# define COMPILER_FENCE asm volatile("" : : : "memory")
#else
# define COMPILER_FENCE
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
// .. GCC
#if GCC_VERSION
# define RESTRICT __restrict__
// .. VC8 provides __restrict
#elif MSC_VERSION
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


//
// number of array elements
//

// (function taking a reference to an array and returning a pointer to
// an array of characters. it's only declared and never defined; we just
// need it to determine n, the size of the array that was passed.)
template<typename T, size_t n> char (*ArraySizeDeducer(T (&)[n]))[n];

// (although requiring C++, this method is much better than the standard
// sizeof(name) / sizeof(name[0]) because it doesn't compile when a
// pointer is passed, which can easily happen under maintenance.)
#define ARRAY_SIZE(name) (sizeof(*ArraySizeDeducer(name)))


// C99-style __func__
// .. newer GCC already have it
#if GCC_VERSION
// nothing need be done
// .. MSVC have __FUNCTION__
#elif MSC_VERSION
# define __func__ __FUNCTION__
// .. unsupported
#else
# define __func__ "(unknown)"
#endif


// extern "C", but does the right thing in pure-C mode
#if defined(__cplusplus)
# define EXTERN_C extern "C"
#else
# define EXTERN_C extern
#endif


#if MSC_VERSION
# define INLINE __forceinline
#else
# define INLINE inline
#endif


#if MSC_VERSION
# define CALL_CONV __cdecl
#else
# define CALL_CONV
#endif


#if MSC_VERSION && !ARCH_AMD64
# define DECORATED_NAME(name) _##name
#else
# define DECORATED_NAME(name) name
#endif


// workaround for preprocessor limitation: macro args aren't expanded
// before being pasted.
#define STRINGIZE2(id) # id
#define STRINGIZE(id) STRINGIZE2(id)

// for widening non-literals (e.g. __FILE__)
// note: C99 says __func__ is a magic *variable*, and GCC doesn't allow
// widening it via preprocessor.
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)

// TODO: Replace this with [[fallthrough]] once we support C++17
#if __has_cpp_attribute(fallthrough) || defined(__cplusplus) && __cplusplus >= 201703L
# define FALLTHROUGH [[fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough)
# define FALLTHROUGH [[gnu::fallthrough]]
#elif __has_cpp_attribute(clang::fallthrough)
# define FALLTHROUGH [[clang::fallthrough]]
#else
# define FALLTHROUGH
// TODO: Maybe use __fallthrough for the MSVC code analyzer (also figure out if we need to add some switch when switching to a newer version of VS that supports [[fallthrough]]
#endif

#endif	// #ifndef INCLUDED_CODE_ANNOTATION
