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
