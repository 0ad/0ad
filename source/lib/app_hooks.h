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
 * hooks to allow customization / app-specific behavior.
 */

/*

Background
----------

This codebase is shared between several projects, each with differing needs.
Some of them have e.g. complicated i18n/translation facilities and require
all text output to go through it; others strive to minimize size and
therefore do not want to include that.
Since commenting things out isn't an option with shared source and
conditional compilation is ugly, we bridge the differences via "hooks".
These are functions whose behavior is expected to differ between projects;
they are defined by the app, registered here and called from lib code via
our trampolines.


Introduction
------------

This module provides a clean interface for other code to call hooks
and allows the app to register them. It also defines default stub
implementations.


Usage
-----

In the simplest case, the stubs are already acceptable. Otherwise,
you need to implement a new version of some hooks, fill an
AppHooks struct with pointers to those functions (zero the rest),
and call app_hooks_update.


Adding New Functions
--------------------

Several steps are needed (see below for rationale):
0) HOOKNAME is the name of the desired procedure (e.g. "bundle_logs")
1) add a 'trampoline' (user visible function) declaration to this header
   (typically named ah_HOOKNAME)
2) add the corresponding implementation, i.e. call to ah.HOOKNAME
3) add a default implementation of the new functionality
   (typically named def_HOOKNAME)
4) add HOOKNAME member to struct AppHooks declaration
5) set HOOKNAME member to def_HOOKNAME in initialization of
   'struct AppHooks ah'
6) add HOOKNAME to list in app_hooks_update code


Rationale
---------

While X-Macros would reduce the amount of work needed when adding new
functions, they confuse static code analysis and VisualAssist X
(the function definitions are not visible to them).
We prefer convenience during usage instead of in the rare cases
where new app hook functions are defined.

note: an X-Macro would define the app hook as such:
extern const wchar_t*, translate, (const wchar_t* text), (text), return)
.. and in its various invocations perform the above steps automatically.

*/

#ifndef INCLUDED_APP_HOOKS
#define INCLUDED_APP_HOOKS

#include "lib/os_path.h"

// trampolines for user code to call the hooks. they encapsulate
// the details of how exactly to do this.

/**
 * override default decision on using OpenGL extensions relating to
 * texture upload.
 *
 * this should call ogl_tex_override to disable/force their use if the
 * current card/driver combo respectively crashes or
 * supports it even though the extension isn't advertised.
 *
 * the default implementation works but is hardwired in code and therefore
 * not expandable.
 **/
extern void ah_override_gl_upload_caps();

/**
 * return path to directory into which crash dumps should be written.
 *
 * must be callable at any time - in particular, before VFS init.
 * paths are typically relative to sys_ExecutablePathname.
 *
 * @return path ending with directory separator (e.g. '/').
 **/
extern const OsPath& ah_get_log_dir();

/**
 * gather all app-related logs/information and write it to file.
 *
 * used when writing a crash log so that all relevant info is in one file.
 *
 * the default implementation attempts to gather 0ad data, but is
 * fail-safe (doesn't complain if file not found).
 *
 * @param f file into which to write.
 **/
extern void ah_bundle_logs(FILE* f);

/**
 * translate text to the current locale.
 *
 * @param text to translate.
 * @return pointer to localized text; must be freed via translate_free.
 *
 * the default implementation just returns the pointer unchanged.
 **/
extern const wchar_t* ah_translate(const wchar_t* text);

/**
 * free text that was returned by translate.
 *
 * @param text to free.
 *
 * the default implementation does nothing.
 **/
extern void ah_translate_free(const wchar_t* text);

/**
 * write text to the app's log.
 *
 * @param text to write.
 *
 * the default implementation uses stdout.
 **/
extern void ah_log(const wchar_t* text);

/**
 * display an error dialog, thus overriding sys_display_error.
 *
 * @param text error message.
 * @param flags see DebugDisplayErrorFlags.
 * @return ErrorReactionInternal.
 *
 * the default implementation just returns ERI_NOT_IMPLEMENTED, which
 * causes the normal sys_display_error to be used.
 **/
extern ErrorReactionInternal ah_display_error(const wchar_t* text, size_t flags);


/**
 * holds a function pointer (allowed to be NULL) for each hook.
 * passed to app_hooks_update.
 **/
struct AppHooks
{
	void (*override_gl_upload_caps)();
	const OsPath& (*get_log_dir)();
	void (*bundle_logs)(FILE* f);
	const wchar_t* (*translate)(const wchar_t* text);
	void (*translate_free)(const wchar_t* text);
	void (*log)(const wchar_t* text);
	ErrorReactionInternal (*display_error)(const wchar_t* text, size_t flags);
};

/**
 * update the app hook function pointers.
 *
 * @param ah AppHooks struct. any of its function pointers that are non-zero
 * override the previous function pointer value
 * (these default to the stub hooks which are functional but basic).
 **/
LIB_API void app_hooks_update(AppHooks* ah);

/**
 * was the app hook changed via app_hooks_update from its default value?
 *
 * @param offset_in_struct byte offset within AppHooks (determined via
 * offsetof) of the app hook function pointer.
 **/
extern bool app_hook_was_redefined(size_t offset_in_struct);
// name is identifier of the function pointer within AppHooks to test.
#define AH_IS_DEFINED(name) app_hook_was_redefined(offsetof(AppHooks, name))

#endif	// #ifndef INCLUDED_APP_HOOKS
