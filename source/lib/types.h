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
 * File        : types.h
 * Project     : 0 A.D.
 * Description : convenient type aliases (shorter than stdint.h's uintN_t)
 * =========================================================================
 */

#ifndef INCLUDED_TYPES
#define INCLUDED_TYPES

#include "posix/posix_types.h"

// defines instead of typedefs so we can #undef conflicting decls

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
//#ifdef SIZE_MAX
//# if SIZE_MAX < 0xFFFFFFFF
//#  error "check size_t and SIZE_MAX - too small?"
//# endif
//#endif

#endif // #ifndef INCLUDED_TYPES
