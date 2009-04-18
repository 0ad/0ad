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

/**
 * =========================================================================
 * File        : handle.h
 * Project     : 0 A.D.
 * Description : forward declaration of Handle (reduces dependencies)
 * =========================================================================
 */

#ifndef INCLUDED_HANDLE
#define INCLUDED_HANDLE

#include "lib/file/vfs/vfs_path.h"

/**
 * `handle' representing a reference to a resource (sound, texture, etc.)
 *
 * 0 is the (silently ignored) invalid handle value; < 0 is an error code.
 *
 * this is 64 bits because we want tags to remain unique. (tags are a
 * counter that disambiguate several subsequent uses of the same
 * resource array slot). 32-bit handles aren't enough because the index
 * field requires at least 12 bits, thus leaving only about 512K possible
 * tag values.
 **/
typedef i64 Handle;

#endif	// #ifndef INCLUDED_HANDLE
