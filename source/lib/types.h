// convenience type (shorter defs than stdint uintN_t)

#ifndef __TYPES_H__
#define __TYPES_H__

#include "posix.h"

// defines instead of typedefs so we can #undef conflicting decls

#define ulong unsigned long

#define uint unsigned int

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
#if defined(SIZE_MAX) && SIZE_MAX < 32
#error "check size_t and SIZE_MAX - too small?"
#endif
	

#endif // #ifndef __TYPES_H__
