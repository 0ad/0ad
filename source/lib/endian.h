// map to intrinsics on VC
#ifdef _MSC_VER

#include <stdlib.h>

#define bswap16 _byteswap_ushort
#define bswap32 _byteswap_ulong
#define bswap64 _byteswap_uint64

// otherwise, define our own functions
#else

extern u16 bswap16(u16);
extern u32 bswap32(u32);
extern u64 bswap64(u64);

#endif

extern void bswap32(const u8* data, int cnt);