#include "config.h"

// converts 4 character string to u32 for easy comparison
// can't pass code as string, and use s[0]..s[3], because
// VC6/7 don't realize the macro is constant
// (it should be useable as a switch{} expression)
//
// these casts are ugly but necessary. u32 is required because u8 << 8 == 0;
// the additional u8 cast ensures each character is treated as unsigned
// (otherwise, they'd be promoted to signed int before the u32 cast,
// which would break things).
#define FOURCC_BE(a,b,c,d) ( ((u32)(u8)a) << 24 | ((u32)(u8)b) << 16 | \
                             ((u32)(u8)c) << 8  | ((u32)(u8)d) << 0  )
#define FOURCC_LE(a,b,c,d) ( ((u32)(u8)a) << 0  | ((u32)(u8)b) << 8  | \
                             ((u32)(u8)c) << 16 | ((u32)(u8)d) << 24 )

#if BYTE_ORDER == BIG_ENDIAN
# define FOURCC FOURCC_BE
#else
# define FOURCC FOURCC_LE
#endif


extern u16 to_le16(u16 x);
extern u32 to_le32(u32 x);
extern u64 to_le64(u64 x);

extern u16 to_be16(u16 x);
extern u32 to_be32(u32 x);
extern u64 to_be64(u64 x);

extern u16 read_le16(const void* p);
extern u32 read_le32(const void* p);
extern u64 read_le64(const void* p);

extern u16 read_be16(const void* p);
extern u32 read_be32(const void* p);
extern u64 read_be64(const void* p);

extern void write_le16(void* p, u16 x);
extern void write_le32(void* p, u32 x);
extern void write_le64(void* p, u64 x);

extern void write_be16(void* p, u16 x);
extern void write_be32(void* p, u32 x);
extern void write_be64(void* p, u64 x);
