/**
 * =========================================================================
 * File        : app_hooks.h
 * Project     : 0 A.D.
 * Description : hooks to allow customization / app-specific behavior.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
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

*/

// X macros that define the individual hooks. All function pointers,
// struct contents, trampoline functions etc. are automatically
// generated from them to ease maintenance.
// When adding a new hook, you need only update this and write a
// default (stub) implementation.
//
// params:
// - ret: return value type
// - name: function name identifier
// - params: parameter declarations, used when declaring the function;
//   enclosed in parentheses.
// - param_names: names of parameters, used when calling the function;
//   enclosed in parentheses.
// - call_prefix: precedes the call to this function.
//   must be (without quotes) '(void)' if ret is void, else 'return'.
//   this is to allow generating trampoline functions with or without
//   a return value.
#ifdef FUNC

// for convenience; less confusing than FUNC(void, [..], (void))
#define VOID_FUNC(name, params, param_names)\
	FUNC(void, name, params, param_names, (void))

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
VOID_FUNC(override_gl_upload_caps, (void), ())

/**
 * return path to directory into which crash dumps should be written.
 *
 * if implementing via static storage, be sure to guarantee reentrancy
 * (e.g. by only filling the string once).
 * must be callable at any time - in particular, before VFS init.
 * this means file_make_full_native_path cannot be used; it is best
 * to specify a path relative to sys_get_executable_name.
 *
 * @return full native path; must end with directory separator (e.g. '/').
 **/
FUNC(const char*, get_log_dir, (void), (), return)

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
VOID_FUNC(bundle_logs, (FILE* f), (f))

/**
 * translate text to the current locale.
 *
 * @param text to translate.
 * @return pointer to localized text; must be freed via translate_free.
 *
 * the default implementation just returns the pointer unchanged.
 **/
FUNC(const wchar_t*, translate, (const wchar_t* text), (text), return)

/**
 * free text that was returned by translate.
 *
 * @param text to free.
 *
 * the default implementation does nothing.
 **/
VOID_FUNC(translate_free, (const wchar_t* text), (text))

/**
 * write text to the app's log.
 *
 * @param text to write.
 *
 * the default implementation uses stdout.
 **/
VOID_FUNC(log, (const wchar_t* text), (text))

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
FUNC(ErrorReaction, display_error, (const wchar_t* text, uint flags), (text, flags), return)

#undef VOID_FUNC

#endif	// #ifdef FUNC


//-----------------------------------------------------------------------------
// normal header part

#ifndef APP_HOOKS_H__
#define APP_HOOKS_H__

/**
 * holds a function pointer for each hook. passed to app_hooks_update.
 **/
struct AppHooks
{
#define FUNC(ret, name, params, param_names, call_prefix) ret (*name) params;
#include "app_hooks.h"
#undef FUNC

	// used to safely terminate initializer list
	int dummy;
};

/**
 * update the app hook function pointers.
 *
 * @param ah AppHooks struct. any of its function pointers that are non-zero
 * override the previous function pointer value
 * (these default to the stub hooks which are functional but basic).
 **/
extern void app_hooks_update(AppHooks* ah);


// trampolines used by lib code to call the hooks. they encapsulate
// the details of how exactly to do this.
#define FUNC(ret, name, params, param_names, call_prefix) extern ret ah_##name params;
#include "app_hooks.h"
#undef FUNC

#endif	// #ifndef APP_HOOKS_H__
