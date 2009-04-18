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

#define XP_WIN

#include <jsapi.h>
#include <jsatom.h>
#ifndef NDEBUG
# include <jsdbgapi.h>
#endif

// Make JS debugging a little easier by automatically naming GC roots
// Don't simply #define NAME_ALL_GC_ROOTS, because jsapi.h is horridly broken
#ifndef NDEBUG
# define JS_AddRoot(cx, rp) JS_AddNamedRoot((cx), (rp), __FILE__ )
#endif
