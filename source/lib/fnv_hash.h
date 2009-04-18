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
