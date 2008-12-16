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
