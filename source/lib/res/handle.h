/**
 * =========================================================================
 * File        : handle.h
 * Project     : 0 A.D.
 * Description : forward declaration of Handle (reduces dependencies)
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_HANDLE
#define INCLUDED_HANDLE

// 0 = invalid handle value; < 0 is an error code.
// 64 bits, because we want tags to remain unique: tag overflow may
// let handle use errors slip through, or worse, cause spurious errors.
// with 32 bits, we'd need >= 12 for the index, leaving < 512K tags -
// not a lot.
typedef i64 Handle;

#endif	// #ifndef INCLUDED_HANDLE
