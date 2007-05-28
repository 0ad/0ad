/**
 * =========================================================================
 * File        : winit.h
 * Project     : 0 A.D.
 * Description : windows-specific module init and shutdown mechanism
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WINIT
#define INCLUDED_WINIT

// overview:
// participating modules store function pointer(s) to their init and/or
// shutdown function in a specific COFF section. the sections are
// grouped according to the desired notification and the order in which
// functions are to be called (useful if one module depends on another).
// they are then gathered by the linker and arranged in alphabetical order.
// placeholder variables in the sections indicate where the series of
// functions begins and ends for a given notification time.
// at runtime, all of the function pointers between the markers are invoked.
//
// details:
// the section names are of the format ".LIB${type}{group}".
// {type} is I for initialization- or S for shutdown functions.
// {group} is [0, 9]; all functions in a group are called before those of
//   the next higher group, but order within the group is undefined.
//   (this is because the linker sorts sections alphabetically but doesn't
//   specify the order in which object files are processed.)
//
// example:
// #pragma SECTION_INIT(7))
// WINIT_REGISTER_FUNC(wtime_init);
// #pragma FORCE_INCLUDE(wtime_init)
// #pragma SECTION_SHUTDOWN(3))
// WINIT_REGISTER_FUNC(wtime_shutdown);
// #pragma FORCE_INCLUDE(wtime_shutdown)
// #pragma SECTION_RESTORE
//
// rationale:
// several methods of module init are possible: (see Large Scale C++ Design)
// - on-demand initialization: each exported function would have to check
//   if init already happened. that would be brittle and hard to verify.
// - singleton: variant of the above, but not applicable to a
//   procedural interface (and quite ugly to boot).
// - registration: static constructors call a central notification function.
//   module dependencies would be quite difficult to express - this would
//   require a graph or separate lists for each priority (clunky).
//   worse, a fatal flaw is that other C++ constructors may depend on the
//   modules we are initializing and already have run. there is no way
//   to influence ctor call order between separate source files, so
//   this is out of the question.
// - linker-based registration: same as above, but the linker takes care
//   of assembling various functions into one sorted table. the list of
//   init functions is available before C++ ctors have run. incidentally,
//   zero runtime overhead is incurred. unfortunately, this approach is
//   MSVC-specific. however, the MS CRT uses a similar method for its
//   init, so this is expected to remain supported.

// macros to simplify usage.
// notes:
// - #pragma cannot be packaged in macros due to expansion rules.
// - __declspec(allocate) would be tempting, since that could be
//   wrapped in WINIT_REGISTER_FUNC. unfortunately it inexplicably cannot
//   cope with split string literals (e.g. "ab" "c"). that disqualifies
//   it, since we want to hide the section name behind a macro, which
//   would require the abovementioned merging.

// note: init functions are called before _cinit and MUST NOT use
// any stateful CRT functions (e.g. atexit)!
#define SECTION_INIT(group)     data_seg(".WINIT$I" #group)
#define SECTION_SHUTDOWN(group) data_seg(".WINIT$S" #group)
#define SECTION_RESTORE data_seg()
// use to make sure the link-stage optimizer doesn't discard the
// function pointers (happens on VC8)
#define FORCE_INCLUDE(id) comment(linker, "/include:_p"#id)

#define WINIT_REGISTER_FUNC(func)\
	static LibError func(void);\
	EXTERN_C LibError (*p##func)(void) = func

/**
 * call each registered function.
 * these are invoked by wstartup at the appropriate times.
 **/
extern void winit_CallInitFunctions();
extern void winit_CallShutdownFunctions();

#endif	// #ifndef INCLUDED_WINIT
