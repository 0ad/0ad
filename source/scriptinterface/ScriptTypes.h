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
#include "js/Initialization.h"

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

class ScriptInterface;

#endif // INCLUDED_SCRIPTTYPES
