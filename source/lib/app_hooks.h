// Hooks to allow application-specific behavior
// Copyright (c) 2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

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
and call set_app_hooks.

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

// override default decision on using OpenGL extensions relating to
// texture upload. this should call ogl_tex_override to disable/force
// their use if the current card/driver combo respectively crashes or
// supports it even though the extension isn't advertised.
//
// default implementation works but is hardwired in code and therefore
// not expandable.
FUNC(void, override_gl_upload_caps, (void), (), (void))

// return full native path of the directory into which crashdumps should be
// written. must end with directory separator (e.g. '/').
// if implementing via static storage, be sure to guarantee reentrancy
// (e.g. by only filling the string once).
// must be callable at any time - in particular, before VFS init.
// this means file_make_full_native_path cannot be used; it is best
// to specify a path relative to sys_get_executable_name.
FUNC(const char*, get_log_dir, (void), (), return)

// gather all app-related logs/information and write it into <f>.
// used when writing a crashlog so that all relevant info is in one file.
//
// default implementation gathers 0ad data but is fail-safe.
FUNC(void, bundle_logs, (FILE* f), (f), (void))

// return localized version of <text> if i18n functionality is available.
//
// default implementation just returns the pointer unchanged.
FUNC(const wchar_t*, translate, (const wchar_t* text), (text), return)

// write <text> to the app's log.
//
// default implementation uses stdout.
FUNC(void, log, (const wchar_t* text), (text), (void))

#endif	// #ifdef FUNC


//-----------------------------------------------------------------------------
// normal header part

#ifndef APP_HOOKS_H__
#define APP_HOOKS_H__

// holds a function pointer for each hook. passed to set_app_hooks.
struct AppHooks
{
#define FUNC(ret, name, params, param_names, call_prefix) ret (*name) params;
#include "app_hooks.h"
#undef FUNC

	// used to safely terminate initializer list
	int dummy;
};

// register the specified hook function pointers. any of them that
// are non-zero override the previous function pointer value
// (these default to the stub hooks which are functional but basic).
extern void set_app_hooks(AppHooks* ah);


// trampolines used by lib code to call the hooks. they encapsulate
// the details of how exactly to do this.
#define FUNC(ret, name, params, param_names, call_prefix) extern ret ah_##name params;
#include "app_hooks.h"
#undef FUNC

#endif	// #ifndef APP_HOOKS_H__
