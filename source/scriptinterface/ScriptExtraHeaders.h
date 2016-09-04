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

#ifndef INCLUDED_SCRIPTEXTRAHEADERS
#define INCLUDED_SCRIPTEXTRAHEADERS

// Includes occasionally-used SpiderMonkey headers for typed arrays and debug API,
// with appropriate tweaks to fix warnings and build errors. (Most code should
// just include ScriptTypes.h directly to get the standard jsapi.h.)

#include "scriptinterface/ScriptTypes.h"

// Ignore some harmless warnings
#if GCC_VERSION
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
# pragma GCC diagnostic ignored "-Wredundant-decls"
# pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif
#if CLANG_VERSION
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmismatched-tags"
#endif
#if MSC_VERSION
// reduce the warning level for the SpiderMonkey headers
# pragma warning(push, 1)
#endif

// Redefine signbit to fix build error in GCC
#ifndef signbit
# define signbit std::signbit
#endif

#include "jsfriendapi.h"
#include "js/Conversions.h"
#include "js/GCAPI.h"
#include "js/StructuredClone.h"

#undef signbit

#if MSC_VERSION
# pragma warning(pop)
#endif
#if CLANG_VERSION
# pragma clang diagnostic pop
#endif
#if GCC_VERSION
# pragma GCC diagnostic pop
#endif

#endif // INCLUDED_SCRIPTEXTRAHEADERS
