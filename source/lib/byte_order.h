/**
 * =========================================================================
 * File        : byte_order.h
 * Project     : 0 A.D.
 * Description : byte order (endianness) support routines.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "config.h"


/**
 * convert 4 characters to u32 (at compile time) for easy comparison.
 * output is in native byte order; e.g. FOURCC_LE can be used instead.
 **/
#define FOURCC(a,b,c,d)	// real definition is below
#undef  FOURCC

// implementation rationale:
// - can't pass code as string, and use s[0]..s[3], because
//   VC6/7 don't realize the macro is constant
//   (it should be usable as a switch{} expression)
// - the casts are ugly but necessary. u32 is required because u8 << 8 == 0;
//   the additional u8 cast ensures each character is treated as unsigned
//   (otherwise, they'd be promoted to signed int before the u32 cast,
//   which would break things).

/// big-endian version of FOURCC
#define FOURCC_BE(a,b,c,d) ( ((u32)(u8)a) << 24 | ((u32)(u8)b) << 16 | \
	((u32)(u8)c) << 8  | ((u32)(u8)d) << 0  )

/// little-endian version of FOURCC
#define FOURCC_LE(a,b,c,d) ( ((u32)(u8)a) << 0  | ((u32)(u8)b) << 8  | \
	((u32)(u8)c) << 16 | ((u32)(u8)d) << 24 )

#if BYTE_ORDER == BIG_ENDIAN
# define FOURCC FOURCC_BE
#else
# define FOURCC FOURCC_LE
#endif


/// convert a little-endian number to/from native byte order.
extern u16 to_le16(u16 x);
extern u32 to_le32(u32 x);	/// see to_le16
extern u64 to_le64(u64 x);	/// see to_le16

/// convert a big-endian number to/from native byte order.
extern u16 to_be16(u16 x);
extern u32 to_be32(u32 x);	/// see to_be16
extern u64 to_be64(u64 x);	/// see to_be16

/// read a little-endian number from memory into native byte order.
extern u16 read_le16(const void* p);
extern u32 read_le32(const void* p);	/// see read_le16
extern u64 read_le64(const void* p);	/// see read_le16

/// read a big-endian number from memory into native byte order.
extern u16 read_be16(const void* p);
extern u32 read_be32(const void* p);	/// see read_be16
extern u64 read_be64(const void* p);	/// see read_be16

/// write a little-endian number to memory in native byte order.
extern void write_le16(void* p, u16 x);
extern void write_le32(void* p, u32 x);	/// see write_le16
extern void write_le64(void* p, u64 x);	/// see write_le16

/// write a big-endian number to memory in native byte order.
extern void write_be16(void* p, u16 x);
extern void write_be32(void* p, u32 x);	/// see write_be16
extern void write_be64(void* p, u64 x);	/// see write_be16
