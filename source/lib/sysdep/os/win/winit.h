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
 * windows-specific module init and shutdown mechanism
 */

#ifndef INCLUDED_WINIT
#define INCLUDED_WINIT

/*

Overview
--------

This facility allows registering init and shutdown functions with only
one line of code and zero runtime overhead. It provides for dependencies
between modules, allowing groups of functions to run before others.


Details
-------

Participating modules store function pointer(s) to their init and/or
shutdown function in a specific COFF section. The sections are
grouped according to the desired notification and the order in which
functions are to be called (useful if one module depends on another).
They are then gathered by the linker and arranged in alphabetical order.
Placeholder variables in the sections indicate where the series of
functions begins and ends for a given notification time.
At runtime, all of the function pointers between the markers are invoked.


Example
-------

(at file scope:)
WINIT_REGISTER_MAIN_INIT(InitCallback);


Rationale
---------

Several methods of module init are possible: (see Large Scale C++ Design)
- on-demand initialization: each exported function would have to check
  if init already happened. that would be brittle and hard to verify.
- singleton: variant of the above, but not applicable to a
  procedural interface (and quite ugly to boot).
- registration: static constructors call a central notification function.
  module dependencies would be quite difficult to express - this would
  require a graph or separate lists for each priority (clunky).
  worse, a fatal flaw is that other C++ constructors may depend on the
  modules we are initializing and already have run. there is no way
  to influence ctor call order between separate source files, so
  this is out of the question.
- linker-based registration: same as above, but the linker takes care
  of assembling various functions into one sorted table. the list of
  init functions is available before C++ ctors have run. incidentally,
  zero runtime overhead is incurred. unfortunately, this approach is
  MSVC-specific. however, the MS CRT uses a similar method for its
  init, so this is expected to remain supported.

*/


//-----------------------------------------------------------------------------
// section declarations

// section names are of the format ".WINIT${type}{group}".
// {type} is I for initialization- or S for shutdown functions.
// {group} is [0, 9] - see below.
// note: __declspec(allocate) requires declaring segments in advance via
// #pragma section.
#pragma section(".WINIT$I$", read)
#pragma section(".WINIT$I0", read)
#pragma section(".WINIT$I1", read)
#pragma section(".WINIT$I2", read)
#pragma section(".WINIT$I6", read)
#pragma section(".WINIT$I7", read)
#pragma section(".WINIT$IZ", read)
#pragma section(".WINIT$S$", read)
#pragma section(".WINIT$S0", read)
#pragma section(".WINIT$S1", read)
#pragma section(".WINIT$S6", read)
#pragma section(".WINIT$S7", read)
#pragma section(".WINIT$S8", read)
#pragma section(".WINIT$SZ", read)
#pragma comment(linker, "/merge:.WINIT=.rdata")


//-----------------------------------------------------------------------------
// Function groups

// to allow correct ordering of module init in the face of dependencies,
// we introduce 'groups'. all functions in one are called before those in
// the next higher group, but order within the group is undefined.
// (this is because the linker sorts sections alphabetically but doesn't
// specify the order in which object files are processed.)

// these macros register a function to be called at the given time.
// usage: invoke at file scope, passing a function identifier/symbol.
// rationale:
// - __declspec(allocate) requires section declarations, but allows users to
//   write only one line (instead of needing an additional #pragma data_seg)
// - fixed groups instead of passing a group number are more clear and
//   encourage thinking about init order. (__declspec(allocate) requires
//   a single string literal anyway and doesn't support string merging)
// - why EXTERN_C and __pragma? VC8's link-stage optimizer believes
//   the static function pointers defined by WINIT_REGISTER_* to be unused;
//   unless action is taken, they would be removed. to prevent this, we
//   forcibly include the function pointer symbols. this means the variable
//   must be extern, not static. the linker needs to know the decorated
//   symbol name, so we disable mangling via EXTERN_C.

// very early init; must not fail, since error handling code *crashes*
// if called before these have completed.
#define WINIT_REGISTER_CRITICAL_INIT(func)   __pragma(comment(linker, "/include:" STRINGIZE(DECORATED_NAME(p##func)))) static Status func(); EXTERN_C __declspec(allocate(".WINIT$I0")) Status (*p##func)(void) = func

// meant for modules with dependents but whose init is complicated and may
// raise error/warning messages (=> can't go in WINIT_REGISTER_CRITICAL_INIT)
#define WINIT_REGISTER_EARLY_INIT(func)      __pragma(comment(linker, "/include:" STRINGIZE(DECORATED_NAME(p##func)))) static Status func(); EXTERN_C __declspec(allocate(".WINIT$I1")) Status (*p##func)(void) = func

// available for dependents of WINIT_REGISTER_EARLY_INIT-modules that
// must still come before WINIT_REGISTER_MAIN_INIT.
#define WINIT_REGISTER_EARLY_INIT2(func)     __pragma(comment(linker, "/include:" STRINGIZE(DECORATED_NAME(p##func)))) static Status func(); EXTERN_C __declspec(allocate(".WINIT$I2")) Status (*p##func)(void) = func

// most modules will go here unless they are often used or
// have many dependents.
#define WINIT_REGISTER_MAIN_INIT(func)       __pragma(comment(linker, "/include:" STRINGIZE(DECORATED_NAME(p##func)))) static Status func(); EXTERN_C __declspec(allocate(".WINIT$I6")) Status (*p##func)(void) = func

// available for any modules that may need to come after
// WINIT_REGISTER_MAIN_INIT (unlikely)
#define WINIT_REGISTER_LATE_INIT(func)       __pragma(comment(linker, "/include:" STRINGIZE(DECORATED_NAME(p##func)))) static Status func(); EXTERN_C __declspec(allocate(".WINIT$I7")) Status (*p##func)(void) = func

#define WINIT_REGISTER_EARLY_SHUTDOWN(func)  __pragma(comment(linker, "/include:" STRINGIZE(DECORATED_NAME(p##func)))) static Status func(); EXTERN_C __declspec(allocate(".WINIT$S0")) Status (*p##func)(void) = func
#define WINIT_REGISTER_EARLY_SHUTDOWN2(func) __pragma(comment(linker, "/include:" STRINGIZE(DECORATED_NAME(p##func)))) static Status func(); EXTERN_C __declspec(allocate(".WINIT$S1")) Status (*p##func)(void) = func
#define WINIT_REGISTER_MAIN_SHUTDOWN(func)   __pragma(comment(linker, "/include:" STRINGIZE(DECORATED_NAME(p##func)))) static Status func(); EXTERN_C __declspec(allocate(".WINIT$S6")) Status (*p##func)(void) = func
#define WINIT_REGISTER_LATE_SHUTDOWN(func)   __pragma(comment(linker, "/include:" STRINGIZE(DECORATED_NAME(p##func)))) static Status func(); EXTERN_C __declspec(allocate(".WINIT$S7")) Status (*p##func)(void) = func
#define WINIT_REGISTER_LATE_SHUTDOWN2(func)  __pragma(comment(linker, "/include:" STRINGIZE(DECORATED_NAME(p##func)))) static Status func(); EXTERN_C __declspec(allocate(".WINIT$S8")) Status (*p##func)(void) = func

//-----------------------------------------------------------------------------

/**
 * call each registered function.
 *
 * if this is called before CRT initialization, callbacks must not use any
 * non-stateless CRT functions such as atexit. see wstartup.h for the
 * current status on this issue.
 **/
extern void winit_CallInitFunctions();
extern void winit_CallShutdownFunctions();

#endif	// #ifndef INCLUDED_WINIT
