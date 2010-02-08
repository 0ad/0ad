/* Copyright (c) 2010 Wildfire Games
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

/**
 * mark a function local variable or parameter as unused and avoid
 * the corresponding compiler warning.
 * use inside the function body, e.g. void f(int x) { UNUSED2(x); }
 **/
#define UNUSED2(param) (void)param;

/**
 * mark a function parameter as unused and avoid
 * the corresponding compiler warning.
 * wrap around the parameter name, e.g. void f(int UNUSED(x))
 **/
#define UNUSED(param)


/**
"unreachable code" helpers

unreachable lines of code are often the source or symptom of subtle bugs.
they are flagged by compiler warnings; however, the opposite problem -
erroneously reaching certain spots (e.g. due to missing return statement)
is worse and not detected automatically.

to defend against this, the programmer can annotate their code to
indicate to humans that a particular spot should never be reached.
however, that isn't much help; better is a sentinel that raises an
error if if it is actually reached. hence, the UNREACHABLE macro.

ironically, if the code guarded by UNREACHABLE works as it should,
compilers may flag the macro's code as unreachable. this would
distract from genuine warnings, which is unacceptable.

even worse, compilers differ in their code checking: GCC only complains if
non-void functions end without returning a value (i.e. missing return
statement), while VC checks if lines are unreachable (e.g. if they are
preceded by a return on all paths).

our implementation of UNREACHABLE solves this dilemna as follows:
- on GCC: call abort(); since it has the noreturn attributes, the
  "non-void" warning disappears.
- on VC: avoid generating any code. we allow the compiler to assume the
  spot is actually unreachable, which incidentally helps optimization.
  if reached after all, a crash usually results. in that case, compile with
  CONFIG_PARANOIA, which will cause an error message to be displayed.

this approach still allows for the possiblity of automated
checking, but does not cause any compiler warnings.
**/
#define UNREACHABLE	// actually defined below.. this is for
# undef UNREACHABLE	// CppDoc's benefit only.

// 1) final build: optimize assuming this location cannot be reached.
//    may crash if that turns out to be untrue, but removes checking overhead.
#if CONFIG_FINAL
# define UNREACHABLE ASSUME_UNREACHABLE
// 2) normal build:
#else
//    a) normal implementation: includes "abort", which is declared with
//       noreturn attribute and therefore avoids GCC's "execution reaches
//       end of non-void function" warning.
# if !MSC_VERSION || ICC_VERSION || CONFIG_PARANOIA
#  define UNREACHABLE\
	STMT(\
		debug_assert(0);	/* hit supposedly unreachable code */\
		abort();\
	)
//    b) VC only: don't generate any code; squelch the warning and optimize.
# else
#  define UNREACHABLE ASSUME_UNREACHABLE
# endif
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
// used to give a unique name (per file) to types made by cassert.
// we can't prepend __FILE__ to make it globally unique - the filename
// may be enclosed in quotes. PASTE3_HIDDEN__ is needed to make sure
// __LINE__ is expanded correctly.
#define PASTE3_HIDDEN__(a, b, c) a ## b ## c
#define PASTE3__(a, b, c) PASTE3_HIDDEN__(a, b, c)
#define UID__  PASTE3__(LINE_, __LINE__, _)
#define UID2__ PASTE3__(LINE_, __LINE__, _2)

/**
 * compile-time debug_assert. causes a compile error if the expression
 * evaluates to zero/false.
 *
 * no runtime overhead; may be used anywhere, including file scope.
 * especially useful for testing sizeof types.
 *
 * @param expression that is expected to evaluate to non-zero at compile-time.
 **/
#define cassert(expr) typedef static_assert_<(expr)>::type UID__
template<bool> struct static_assert_;
template<> struct static_assert_<true>
{
	typedef int type;
};

/**
 * compile-time debug_assert. causes a compile error if the expression
 * evaluates to zero/false.
 *
 * no runtime overhead; may be used anywhere, including file scope.
 * especially useful for testing sizeof types.
 *
 * this version has a less helpful error message, but redefinition doesn't
 * trigger warnings.
 *
 * @param expression that is expected to evaluate to non-zero at compile-time.
 **/
#define cassert2(expr) extern u8 CASSERT_FAILURE[1][(expr)]


// copied from boost::noncopyable; this definition avoids warnings when
// an exported class derives from noncopyable.

namespace noncopyable_  // protection from unintended ADL
{
	class noncopyable
	{
	protected:
		noncopyable() {}
		~noncopyable() {}
	private:  // emphasize the following members are private
		noncopyable(const noncopyable&);
		const noncopyable& operator=(const noncopyable&);
	};
}

typedef noncopyable_::noncopyable noncopyable;

// this form avoids ICC 11 W4 warnings about non-virtual dtors and
// suppression of the copy assignment operator.
#define NONCOPYABLE(className)\
private:\
	className(const className&);\
	const className& operator=(const className&)

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

#endif	// #ifndef INCLUDED_CODE_ANNOTATION
