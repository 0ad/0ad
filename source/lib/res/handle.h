#ifndef HANDLE_H__
#define HANDLE_H__

#include "lib/types.h"

// define type Handle. split out of h_mgr.h to reduce dependencies.

// 0 = invalid handle value; < 0 is an error code.
// 64 bits, because we want tags to remain unique: tag overflow may
// let handle use errors slip through, or worse, cause spurious errors.
// with 32 bits, we'd need >= 12 for the index, leaving < 512K tags -
// not a lot.
typedef i64 Handle;
#define HANDLE_DEFINED

#endif	// #ifndef HANDLE_H__
