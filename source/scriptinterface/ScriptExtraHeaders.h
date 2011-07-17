/* Copyright (C) 2011 Wildfire Games.
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
#if GCC_VERSION >= 402 // (older GCCs don't support this pragma)
# pragma GCC diagnostic ignored "-Wunused-parameter"
# pragma GCC diagnostic ignored "-Wredundant-decls"
#endif
#if MSC_VERSION
# pragma warning(push)
# pragma warning(disable:4100) // "unreferenced formal parameter"
# pragma warning(disable:4800) // "forcing value to bool 'true' or 'false' (performance warning)"
#endif

// Redefine signbit to fix build error in GCC
#define signbit std::signbit

#include "js/jstypedarray.h"
#include "js/jsdbgapi.h"

#undef signbit

#if MSC_VERSION
# pragma warning(pop)
#endif
#if GCC_VERSION >= 402
# pragma GCC diagnostic ignored "-Wunused-parameter"
# pragma GCC diagnostic warning "-Wredundant-decls"
#endif

#endif // INCLUDED_SCRIPTEXTRAHEADERS
