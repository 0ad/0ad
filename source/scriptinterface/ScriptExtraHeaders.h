/* Copyright (C) 2013 Wildfire Games.
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

#ifndef INCLUDED_SCRIPTEXTRAHEADERS
#define INCLUDED_SCRIPTEXTRAHEADERS

// Includes occasionally-used SpiderMonkey headers for typed arrays and debug API,
// with appropriate tweaks to fix warnings and build errors. (Most code should
// just include ScriptTypes.h directly to get the standard jsapi.h.)

#include "scriptinterface/ScriptTypes.h"

// Ignore some harmless warnings
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
# pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
# if GCC_VERSION >= 407
#  pragma GCC diagnostic ignored "-Wunused-local-typedefs" // caused by js/debug.h
# endif
#endif
#if MSC_VERSION
# pragma warning(push)
# pragma warning(disable:4100) // "unreferenced formal parameter"
# pragma warning(disable:4800) // "forcing value to bool 'true' or 'false' (performance warning)"
#endif

// Redefine signbit to fix build error in GCC
#ifndef signbit
# define signbit std::signbit
#endif

#include "jsfriendapi.h"
#include "jsdbgapi.h"
#include "js/GCAPI.h"

#undef signbit

#if MSC_VERSION
# pragma warning(pop)
#endif
#if GCC_VERSION >= 402
# pragma GCC diagnostic warning "-Wunused-parameter"
# pragma GCC diagnostic warning "-Wredundant-decls"
# pragma GCC diagnostic warning "-Wnon-virtual-dtor"
# if GCC_VERSION >= 406
// restore user flags (and we don't have to manually restore the warning levels for GCC >= 4.6)
#  pragma GCC diagnostic pop
# endif
#endif

#endif // INCLUDED_SCRIPTEXTRAHEADERS
