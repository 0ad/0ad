/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Fowler/Noll/Vo string hash
 */

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
