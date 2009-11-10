/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
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
 * paths are typically relative to sys_get_executable_name.
 *
 * @return path ending with directory separator (e.g. '/').
 **/
extern const fs::wpath& ah_get_log_dir();

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
 * @return ErrorReaction.
 *
 * the default implementation just returns ER_NOT_IMPLEMENTED, which
 * causes the normal sys_display_error to be used.
 **/
extern ErrorReaction ah_display_error(const wchar_t* text, size_t flags);


/**
 * holds a function pointer (allowed to be NULL) for each hook.
 * passed to app_hooks_update.
 **/
struct AppHooks
{
	void (*override_gl_upload_caps)();
	const fs::wpath& (*get_log_dir)();
	void (*bundle_logs)(FILE* f);
	const wchar_t* (*translate)(const wchar_t* text);
	void (*translate_free)(const wchar_t* text);
	void (*log)(const wchar_t* text);
	ErrorReaction (*display_error)(const wchar_t* text, size_t flags);
};

/**
 * update the app hook function pointers.
 *
 * @param ah AppHooks struct. any of its function pointers that are non-zero
 * override the previous function pointer value
 * (these default to the stub hooks which are functional but basic).
 **/
extern void app_hooks_update(AppHooks* ah);

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
