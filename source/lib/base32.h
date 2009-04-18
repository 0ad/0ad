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
 * File        : base32.h
 * Project     : 0 A.D.
 * Description : base32 conversion
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_BASE32
#define INCLUDED_BASE32

/**
 * generate the base32 textual representation of a buffer.
 *
 * @param len size [bytes] of input
 * @param big-endian input data (assumed to be integral number of bytes)
 * @param output string; zero-terminated. must be big enough
 * (i.e. at least ceil(len*CHAR_BIT/5) + 1 chars)
 **/
extern void base32(const size_t len, const u8* in, u8* out);

#endif	// #ifndef INCLUDED_BASE32
