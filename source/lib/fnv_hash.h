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
 * File        : fnv_hash.h
 * Project     : 0 A.D.
 * Description : Fowler/Noll/Vo string hash
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_FNV_HASH
#define INCLUDED_FNV_HASH

/**
 * rationale: this algorithm was chosen because it delivers 'good' results
 * for string data and is relatively simple. other good alternatives exist;
 * see Ozan Yigit's hash roundup.
 **/

/**
 * calculate FNV1-A hash.
 *
 * @param buf input buffer.
 * @param len if 0 (default), treat buf as a C-string; otherwise,
 * indicates how many bytes of buffer to hash.
 * @return hash result. note: results are distinct for buffers containing
 * differing amounts of zero bytes because the hash value is seeded.
 **/
extern u32 fnv_hash(const void* buf, size_t len = 0);
/// 64-bit version of fnv_hash.
extern u64 fnv_hash64(const void* buf, size_t len = 0);

/**
 * special version of fnv_hash for strings: first converts to lowercase
 * (useful for comparing mixed-case filenames)
 **/
extern u32 fnv_lc_hash(const char* str, size_t len = 0);

#endif	// #ifndef INCLUDED_FNV_HASH
