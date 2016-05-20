/* Copyright (C) 2016 Wildfire Games.
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

#define JSGC_GENERATIONAL 1
#define JSGC_USE_EXACT_ROOTING 1

#ifdef _WIN32
# define XP_WIN
# ifndef WIN32
#  define WIN32 // SpiderMonkey expects this
# endif
#endif

// Ignore some harmless warnings
#if GCC_VERSION
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
# pragma GCC diagnostic ignored "-Wredundant-decls"
# pragma GCC diagnostic ignored "-Wundef" // Some versions of GCC will still print warnings (see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=53431).
# pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
# pragma GCC diagnostic ignored "-Wignored-qualifiers"
# pragma GCC diagnostic ignored "-Wextra"
#endif
#if CLANG_VERSION
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wuninitialized"
# pragma clang diagnostic ignored "-Wc++11-extensions"
# pragma clang diagnostic ignored "-Wignored-qualifiers"
# pragma clang diagnostic ignored "-Wmismatched-tags"
// Ugly hack to deal with macro redefinitions from libc++
# ifdef nullptr
#  undef nullptr
# endif
# ifdef decltype
#  undef decltype
# endif
#endif
#if MSC_VERSION
// reduce the warning level for the SpiderMonkey headers
# pragma warning(push, 1)
#endif

#include "jspubtd.h"
#include "jsapi.h"

// restore user flags and re-enable the warnings disabled a few lines above
#if MSC_VERSION
# pragma warning(pop)
#endif
#if CLANG_VERSION
# pragma clang diagnostic pop
#endif
#if GCC_VERSION
# pragma GCC diagnostic pop
#endif

#if MOZJS_MAJOR_VERSION != 31
#error Your compiler is trying to use an incorrect major version of the \
SpiderMonkey library. The only version that works is the one in the \
libraries/spidermonkey/ directory, and it will not work with a typical \
system-installed version. Make sure you have got all the right files and \
include paths.
#endif

#if MOZJS_MINOR_VERSION != 2 && MOZJS_MINOR_VERSION != 4 && MOZJS_MINOR_VERSION != 5
#error Your compiler is trying to use an untested minor version of the \
SpiderMonkey library. If you are a package maintainer, please make sure \
to check very carefully that this version does not change the behaviour \
of the code executed by SpiderMonkey. Different parts of the game (e.g. \
the multiplayer mode) rely on deterministic behaviour of the JavaScript \
engine. A simple way for testing this would be playing a network game \
with one player using the old version and one player using the new \
version. Another way for testing is running replays and comparing the \
final hash (check trac.wildfiregames.com/wiki/Debugging#Replaymode). \
For more information check this link: trac.wildfiregames.com/wiki/Debugging#Outofsync
#endif

class ScriptInterface;

#endif // INCLUDED_SCRIPTTYPES
