/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_SCRIPTTYPES
#define INCLUDED_SCRIPTTYPES

#ifdef _WIN32
# define XP_WIN
# ifndef WIN32
#  define WIN32 // SpiderMonkey expects this
# endif
#endif

// Guess whether the library was compiled with the release-mode or debug-mode ABI
// (for JS_DumpHeap etc)
#if defined(DEBUG) && !defined(WITH_SYSTEM_MOZJS24)
# define MOZJS_DEBUG_ABI 1
#else
# define MOZJS_DEBUG_ABI 0
#endif

// Ignore some harmless warnings triggered by jsapi.h
// Not all of these warnings can be disabled in older versions of GCC.
// The version checking was taken from the manual here:
// 	http://gcc.gnu.org/onlinedocs/
// Choose the version and navigate to "Options to Request or Suppress Warnings"
// or for some flags "Options Controlling C++ Dialect".
#if GCC_VERSION >= 402
# if GCC_VERSION >= 406 // store user flags
#  pragma GCC diagnostic push
# endif
# pragma GCC diagnostic ignored "-Wunused-parameter"
# pragma GCC diagnostic ignored "-Wredundant-decls"
# pragma GCC diagnostic ignored "-Wundef" // Some versions of GCC will still print warnings (see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=53431).
# pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
# if GCC_VERSION >= 403
#  pragma GCC diagnostic ignored "-Wignored-qualifiers"
# endif
#endif
#if MSC_VERSION
// warnings which are also disabled for the files that include this header

// For the transition from JSBool to bool
# pragma warning(disable:4800) // "forcing value to bool 'true' or 'false' (performance warning)
# pragma warning(disable:4805) // '==' : unsafe mix of type 'JSBool' and type 'bool' in operation

// warnings only disabled for the SpiderMonkey headers
# pragma warning(push)
# pragma warning(disable:4480) // "nonstandard extension used: specifying underlying type for enum"
# pragma warning(disable:4100) // "unreferenced formal parameter"
# pragma warning(disable:4512) // "assignment operator could not be generated"
# pragma warning(disable:4265) // "class has virtual functions, but destructor is not virtual"
# pragma warning(disable:4251) // "class 'X' needs to have dll-interface to be used by clients of struct 'Y'"
#endif

#include "jspubtd.h"
#include "jsapi.h"


#if MSC_VERSION
# pragma warning(pop)
#endif
#if GCC_VERSION >= 402
# pragma GCC diagnostic warning "-Wunused-parameter"
# pragma GCC diagnostic warning "-Wredundant-decls"
# pragma GCC diagnostic warning "-Wundef"
# pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
# if GCC_VERSION >= 403
#  pragma GCC diagnostic warning "-Wignored-qualifiers"
# endif
# if GCC_VERSION >= 406
// restore user flags (and we don't have to manually restore the warning levels for GCC >= 4.6)
#  pragma GCC diagnostic pop
# endif
#endif

#if MOZJS_MAJOR_VERSION != 24
#error Your compiler is trying to use an incorrect major version of the SpiderMonkey library.
#error The only version that works is the one in the libraries/spidermonkey/ directory,
#error and it will not work with a typical system-installed version.
#error Make sure you have got all the right files and include paths.
#endif

#if MOZJS_MINOR_VERSION != 2
#error Your compiler is trying to use an untested minor version of the SpiderMonkey library.
#error If you are a package maintainer, please make sure to check very carefully that this version does not change
#error the behaviour of the code executed by SpiderMonkey.
#error Different parts of the game (e.g. the multiplayer mode) rely on deterministic behaviour of the JavaScript engine.
#error A simple way for testing this would be playing a network game with one player using the old version and one player using the new version.
#error Another way for testing is running replays and comparing the final hash (check trac.wildfiregames.com/wiki/Debugging#Replaymode). 
#error For more information check this link: trac.wildfiregames.com/wiki/Debugging#Outofsync
#endif

class ScriptInterface;
class CScriptVal;
class CScriptValRooted;

#endif // INCLUDED_SCRIPTTYPES
