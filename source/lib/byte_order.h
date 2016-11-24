/* Copyright (c) 2015 Wildfire Games
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
 * byte order (endianness) support routines.
 */

#ifndef INCLUDED_BYTE_ORDER
#define INCLUDED_BYTE_ORDER

#include "lib/sysdep/cpu.h"

// detect byte order via predefined macros.
#ifndef BYTE_ORDER
# define LITTLE_ENDIAN 0x4321
# define BIG_ENDIAN    0x1234
# if ARCH_IA32 || ARCH_IA64 || ARCH_AMD64 || ARCH_ALPHA || ARCH_ARM || ARCH_AARCH64 || ARCH_MIPS || defined(__LITTLE_ENDIAN__)
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


#if BYTE_ORDER == BIG_ENDIAN
// convert a little-endian number to/from native byte order.
# define to_le16(x) swap16(x)
# define to_le32(x) swap32(x)
# define to_le64(x) swap64(x)
// convert a big-endian number to/from native byte order.
# define to_be16(x) (x)
# define to_be32(x) (x)
# define to_be64(x) (x)
#else // LITTLE_ENDIAN
// convert a little-endian number to/from native byte order.
# define to_le16(x) (x)
# define to_le32(x) (x)
# define to_le64(x) (x)
// convert a big-endian number to/from native byte order.
# define to_be16(x) swap16(x)
# define to_be32(x) swap32(x)
# define to_be64(x) swap64(x)
#endif

/// read a little-endian number from memory into native byte order.
LIB_API u16 read_le16(const void* p);
LIB_API u32 read_le32(const void* p);	/// see read_le16
LIB_API u64 read_le64(const void* p);	/// see read_le16

/// read a big-endian number from memory into native byte order.
LIB_API u16 read_be16(const void* p);
LIB_API u32 read_be32(const void* p);	/// see read_be16
LIB_API u64 read_be64(const void* p);	/// see read_be16

/// write a little-endian number to memory in native byte order.
LIB_API void write_le16(void* p, u16 x);
LIB_API void write_le32(void* p, u32 x);	/// see write_le16
LIB_API void write_le64(void* p, u64 x);	/// see write_le16

/// write a big-endian number to memory in native byte order.
LIB_API void write_be16(void* p, u16 x);
LIB_API void write_be32(void* p, u32 x);	/// see write_be16
LIB_API void write_be64(void* p, u64 x);	/// see write_be16

/**
 * zero-extend \<size\> (truncated to 8) bytes of little-endian data to u64,
 * starting at address \<p\> (need not be aligned).
 **/
LIB_API u64 movzx_le64(const u8* p, size_t size);
LIB_API u64 movzx_be64(const u8* p, size_t size);

/**
 * sign-extend \<size\> (truncated to 8) bytes of little-endian data to i64,
 * starting at address \<p\> (need not be aligned).
 **/
LIB_API i64 movsx_le64(const u8* p, size_t size);
LIB_API i64 movsx_be64(const u8* p, size_t size);


#if ICC_VERSION
#define swap32 _bswap
#define swap64 _bswap64
#elif MSC_VERSION
extern unsigned short _byteswap_ushort(unsigned short);
extern unsigned long _byteswap_ulong(unsigned long);
extern unsigned __int64 _byteswap_uint64(unsigned __int64);
#pragma intrinsic(_byteswap_ushort)
#pragma intrinsic(_byteswap_ulong)
#pragma intrinsic(_byteswap_uint64)
# define swap16 _byteswap_ushort
# define swap32 _byteswap_ulong
# define swap64 _byteswap_uint64
#elif defined(linux)
# include <asm/byteorder.h>
# if defined(__arch__swab16) && !defined(swap16)
#  define swap16 __arch__swab16
# endif
# if defined(__arch__swab32) && !defined(swap32)
#  define swap32 __arch__swab32
# endif
# if defined(__arch__swab64) && !defined(swap64)
#  define swap64 __arch__swab64
# endif
#endif

#ifndef swap16
LIB_API u16 swap16(const u16 x);
#endif
#ifndef swap32
LIB_API u32 swap32(const u32 x);
#endif
#ifndef swap64
LIB_API u64 swap64(const u64 x);
#endif

#endif	// #ifndef INCLUDED_BYTE_ORDER
