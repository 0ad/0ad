#ifndef __TYPES_H__
#define __TYPES_H__


// defines instead of typedefs so we can #undef conflicting decls


#define ulong unsigned long

#define uint unsigned int

#define int8 signed char
#define int16 short
#define int32 long

#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned long		// long to match Win32 DWORD

#ifdef _MSC_VER
#define u64 unsigned __int64
#else
#define u64 unsigned long long
#endif



#include <stddef.h>
#ifdef _MSC_VER
#ifndef _UINTPTR_T_DEFINED
#define _UINTPTR_T_DEFINED
#define uintptr_t u32
#endif	// _UINTPTR_T_DEFINED
#else
#include <stdint.h>
#endif	// _MSC_VER


#endif // #ifndef __TYPES_H__
