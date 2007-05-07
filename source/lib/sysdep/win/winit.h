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

// register functions to be called before libc init, before main,
// or after atexit.
//
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
// {type} is C for pre-libc init, I for pre-main init, or
//   T for terminators (last of the atexit handlers).
// {group} is [B, Y]; all functions in a group are called before those of
//   the next (alphabetically) higher group, but order within the group is
//   undefined. this is because the linker sorts sections alphabetically,
//   but doesn't specify the order in which object files are processed.
//   another consequence is that groups A and Z must not be used!
//   (data placed there might end up outside the start/end markers)
//
// example:
// #pragma SECTION_PRE_LIBC(G))
// WIN_REGISTER_FUNC(wtime_init);
// #pragma FORCE_INCLUDE(wtime_init)
// #pragma SECTION_POST_ATEXIT(D))
// WIN_REGISTER_FUNC(wtime_shutdown);
// #pragma FORCE_INCLUDE(wtime_shutdown)
// #pragma SECTION_RESTORE
//
// rationale:
// several methods of module init are possible: (see Large Scale C++ Design)
// - on-demand initialization: each exported function would have to check
//   if init already happened. that would be brittle and hard to verify.
// - singleton: variant of the above, but not applicable to a
//   procedural interface (and quite ugly to boot).
// - registration: NLSO constructors call a central notification function.
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
//   wrapped in WIN_REGISTER_FUNC. unfortunately it inexplicably cannot
//   cope with split string literals (e.g. "ab" "c"). that disqualifies
//   it, since we want to hide the section name behind a macro, which
//   would require the abovementioned merging.

// note: the purpose of pre-libc init (with the resulting requirement that
// no CRT functions be used during init!) is to allow the use of the
// initialized module in static ctors.
#define SECTION_PRE_LIBC(group)    data_seg(".LIB$C" #group)
#define SECTION_PRE_MAIN(group)    data_seg(".LIB$I" #group)
#define SECTION_POST_ATEXIT(group) data_seg(".LIB$T" #group)
#define SECTION_RESTORE data_seg()
// use to make sure the link-stage optimizer doesn't discard the
// function pointers (happens on VC8)
#define FORCE_INCLUDE(id) comment(linker, "/include:_p"#id)

#define WIN_REGISTER_FUNC(func)\
	static LibError func(void);\
	EXTERN_C LibError (*p##func)(void) = func

/**
 * call each registered function.
 * these are invoked by wstartup at the appropriate times.
 **/
extern void winit_CallPreLibcFunctions();
extern void winit_CallPreMainFunctions();
extern void winit_CallShutdownFunctions();

#endif	// #ifndef INCLUDED_WINIT
