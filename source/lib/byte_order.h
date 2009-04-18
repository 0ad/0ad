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
 * File        : byte_order.h
 * Project     : 0 A.D.
 * Description : byte order (endianness) support routines.
 * =========================================================================
 */

#ifndef INCLUDED_BYTE_ORDER
#define INCLUDED_BYTE_ORDER

#include "lib/sysdep/cpu.h"

// detect byte order via predefined macros.
#ifndef BYTE_ORDER
# define LITTLE_ENDIAN 0x4321
# define BIG_ENDIAN    0x1234
# if ARCH_IA32 || ARCH_IA64 || ARCH_AMD64 || ARCH_ALPHA || ARCH_ARM || ARCH_MIPS || defined(__LITTLE_ENDIAN__)
#  define BYTE_ORDER LITTLE_ENDIAN
# else
#  define BYTE_ORDER BIG_ENDIAN
# endif
#endif


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

/**
 * zero-extend <size> (truncated to 8) bytes of little-endian data to u64,
 * starting at address <p> (need not be aligned).
 **/
extern u64 movzx_le64(const u8* p, size_t size);
extern u64 movzx_be64(const u8* p, size_t size);

/**
 * sign-extend <size> (truncated to 8) bytes of little-endian data to i64,
 * starting at address <p> (need not be aligned).
 **/
extern i64 movsx_le64(const u8* p, size_t size);
extern i64 movsx_be64(const u8* p, size_t size);


#if MSC_VERSION
extern unsigned short _byteswap_ushort(unsigned short);
extern unsigned long _byteswap_ulong(unsigned long);
extern unsigned __int64 _byteswap_uint64(unsigned __int64);
# if !ICC_VERSION	// ICC doesn't need (and warns about) the pragmas
#  pragma intrinsic(_byteswap_ushort)
#  pragma intrinsic(_byteswap_ulong)
#  pragma intrinsic(_byteswap_uint64)
# endif
# define swap16 _byteswap_ushort
# define swap32 _byteswap_ulong
# define swap64 _byteswap_uint64
#elif defined(linux)
# include <asm/byteorder.h>
# ifdef __arch__swab16
#  define swap16 __arch__swab16
# endif
# ifdef __arch__swab32
#  define swap32 __arch__swab32
# endif
# ifdef __arch__swab64
#  define swap64 __arch__swab64
# endif
#endif

#ifndef swap16
extern u16 swap16(const u16 x);
#endif
#ifndef swap32
extern u32 swap32(const u32 x);
#endif
#ifndef swap64
extern u64 swap64(const u64 x);
#endif

#endif	// #ifndef INCLUDED_BYTE_ORDER
