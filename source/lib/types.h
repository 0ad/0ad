/**
 * =========================================================================
 * File        : types.h
 * Project     : 0 A.D.
 * Description : convenient type aliases (shorter than stdint.h's uintN_t)
 * =========================================================================
 */

// license: GPL; see lib/license.txt

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
