#ifndef __TYPES_H__
#define __TYPES_H__

// defines instead of typedefs so we can #undef conflicting decls

#define uint unsigned int
#define ulong unsigned long

#define int8 signed char
#define int16 short
#define int32 long

#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned long		// compatibility with Win32 DWORD
#define u64 unsigned __int64

#endif // #ifndef __TYPES_H__
