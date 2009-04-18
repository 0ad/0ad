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

// Hacked version to minimise dependencies

// convenience type (shorter defs than stdint uintN_t)

#ifndef __TYPES_H__
#define __TYPES_H__

//#include "posix.h"
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef unsigned short wchar_t;

// defines instead of typedefs so we can #undef conflicting decls

// Workaround: GCC won't parse constructor-casts with multi-word types, while
// visual studio will. Define uint/long to a namespaced one-word typedef.
typedef unsigned long PS_ulong;
typedef unsigned int PS_uint;
#define ulong PS_ulong
#define uint PS_uint

#define i8 int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t


// the standard only guarantees 16 bits.
// we use this for memory offsets and ranges, so it better be big enough.
#ifdef SIZE_MAX		// nested #if to avoid ICC warning if not defined
# if SIZE_MAX < 32
#  error "check size_t and SIZE_MAX - too small?"
# endif
#endif
	

#endif // #ifndef __TYPES_H__
