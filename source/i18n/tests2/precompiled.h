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

#ifndef NDEBUG

#include <vector>
#include <hash_map>
#include <string>
#include <sstream>
#include <map>

/*
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK ,__FILE__, __LINE__)
#define   malloc(s)         _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   calloc(c, s)      _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   realloc(p, s)     _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _expand(p, s)     _expand_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   free(p)           _free_dbg(p, _NORMAL_BLOCK)
*/
#endif

// Random stuff:

#define SDL_LIL_ENDIAN 1234
#define SDL_BIG_ENDIAN 4321

#define SDL_BYTEORDER SDL_LIL_ENDIAN

#include <cassert>
#define cassert(x) extern char cassert__##__LINE__ [x]
#define debug_warn(x) assert(0&&x)
#define debug_assert assert

#define XP_WIN
#define MSC_VERSION _MSC_VER
#define UNUSED(x)

#include "ps/Errors.h"

#include <cstdio>
#define swprintf _snwprintf
