#ifndef __TYPES_H__
#define __TYPES_H__


// defines instead of typedefs so we can #undef conflicting decls


#define ulong unsigned long

#define uint unsigned int

#define i8 signed char
#define i16 short
#define i32 long

#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned long		// long to match Win32 DWORD

#if defined(_MSC_VER) || defined(__INTEL_COMPILER) || defined(__LCC__)
#define i64 __int64
#define u64 unsigned __int64
#elif defined(__GNUC__) || defined(__MWERKS__) || defined(__SUNPRO_C) || defined(__DMC__)
#define i64 long long
#define u64 unsigned long long
#else
#error "TODO: port u64"
#endif


#define int8 i8
#define int16 i16
#define int32 i32

#include <stddef.h>
#ifdef _MSC_VER
# ifndef _UINTPTR_T_DEFINED
#  define _UINTPTR_T_DEFINED
#  define uintptr_t u32
# endif	// _UINTPTR_T_DEFINED
# ifndef _INTPTR_T_DEFINED
#  define _INTPTR_T_DEFINED
#  define intptr_t i32
# endif	// _INTPTR_T_DEFINED
#else	// !_MSC_VER
# include <stdint.h>
#endif	// _MSC_VER


#endif // #ifndef __TYPES_H__
