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

#ifndef INCLUDED_SPIDERMONKEY
#define INCLUDED_SPIDERMONKEY

// Master header for the SpiderMonkey Javascript library.
//
// Include this instead of accessing any <js*.h> headers directly.
// Rationale: they require an OS macro to be set (XP_*).
// Since this is specific to SpiderMonkey, we don't want to saddle
// another obscure header with it (would make reuse harder).
// Instead, do it here where the purpose is clear.
//
// This doesn't go in ScriptingHost.h because some other files
// (notably precompiled.h and i18n/ScriptInterface.cpp) want only these
// definitions without pulling in the whole of ScriptingHost.

// (Actually that XP_* stuff is handled by the newer scriptinterface
// code instead; this file no longer does anything interesting)

#include "scriptinterface/ScriptTypes.h"

#include "JSUtil.h"

// Make JS debugging a little easier by automatically naming GC roots
// Don't simply #define NAME_ALL_GC_ROOTS, because jsapi.h is horridly broken
#ifndef NDEBUG
# define JS_AddRoot(cx, rp) JS_AddNamedRoot((cx), (rp), __FILE__ )
#endif

#endif // INCLUDED_SPIDERMONKEY
