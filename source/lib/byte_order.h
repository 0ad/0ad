#include "sdl.h"

// converts 4 character string to u32 for easy comparison
// can't pass code as string, and use s[0]..s[3], because
// VC6/7 don't realize the macro is constant
// (it should be useable as a switch{} expression)
//
// these casts are ugly but necessary. u32 is required because u8 << 8 == 0;
// the additional u8 cast ensures each character is treated as unsigned
// (otherwise, they'd be promoted to signed int before the u32 cast,
// which would break things).
#if SDL_BYTE_ORDER == SDL_BIG_ENDIAN
#define FOURCC(a,b,c,d) ( ((u32)(u8)a) << 24 | ((u32)(u8)b) << 16 | \
	((u32)(u8)c) << 8  | ((u32)(u8)d) << 0  )
#else
#define FOURCC(a,b,c,d) ( ((u32)(u8)a) << 0  | ((u32)(u8)b) << 8  | \
	((u32)(u8)c) << 16 | ((u32)(u8)d) << 24 )
#endif


extern u16 read_le16(const void* p);
extern u32 read_le32(const void* p);
extern u16 read_be16(const void* p);
extern u32 read_be32(const void* p);
