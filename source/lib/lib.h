// Copyright (c) 2003 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#ifndef LIB_H
#define LIB_H

#include <stddef.h>
#include <assert.h>

#include "config.h"
#include "posix.h"
#include "types.h"

#include "sysdep/sysdep.h"

#include "sdl.h"	// for endian stuff


// tell STL not to generate exceptions, if compiling without exceptions
// (usually done for performance reasons).
#ifdef CONFIG_DISABLE_EXCEPTIONS
# ifdef _WIN32
#  define _HAS_EXCEPTIONS 0
# else
#  define STL_NO_EXCEPTIONS
# endif
#endif




#define STMT(STMT_code__) do { STMT_code__; } while(0)

// must not be used before main entered! (i.e. not from NLS constructors / functions)
#define ONCE(ONCE_code__)\
STMT(\
/*	static pthread_mutex_t ONCE_mutex__ = PTHREAD_MUTEX_INITIALIZER;\
	if(pthread_mutex_trylock(&ONCE_mutex__) == 0)\*/\
	static bool ONCE_done__ = false;\
	if(!ONCE_done__)\
	{\
		ONCE_done__ = true;\
		ONCE_code__;\
	}\
)

// note: UINT_MAX is necessary when testing a Handle value and
// also returning Handle. the negative value (error return)
// is guaranteed to fit into an int, but we need to "mask"
// it to avoid VC cast-to-smaller-type warnings.

#ifdef _WIN32
#define CHECK_ERR(func)\
STMT(\
	int err__ = (int)((func) & UINT_MAX);\
	if(err__ < 0)\
	{\
		assert(0 && "FYI: CHECK_ERR reports that a function failed."\
		            "feel free to ignore or suppress this warning.");\
		return err__;\
	}\
)
#else
#define CHECK_ERR(func)\
STMT(\
	int err__ = (int)((func) & UINT_MAX);\
	if(err__ < 0)\
	{\
		debug_out("%s:%d: FYI: CHECK_ERR reports that a function failed."\
		            "feel free to ignore or suppress this warning.\n", __FILE__, __LINE__);\
		return err__;\
	}\
)
#endif



// useful because VC may return 0 on failure, instead of throwing.
// this wraps the exception handling, and creates a NULL pointer on failure.
#define SAFE_NEW(type, ptr)\
	type* ptr;\
	try\
	{\
		ptr = new type();\
	}\
	catch(std::bad_alloc)\
	{\
		ptr = 0;\
	}


// superassert
// recommended use: assert2(expr && "descriptive string")
#define assert2(expr)\
{\
	static int suppress__ = 0;\
	if(!suppress__ && !expr)\
		switch(debug_assert_failed(__FILE__, __LINE__, #expr))\
		{\
		case 1:\
			suppress__ = 1;\
			break;\
		case 2:\
			debug_break();\
			break;\
		}\
}



enum LibError
{
	//
	// lib + res
	//

	ERR_INVALID_PARAM   = -1000,
	ERR_INVALID_HANDLE  = -1001,
	ERR_NO_MEM          = -1002,

	// try again later
	ERR_AGAIN           = -1003,

	// fixed limit exceeded
	ERR_LIMIT           = -1004,

	// system doesn't support required API(s)
	ERR_NO_SYS          = -1005,

	// feature not currently implemented (will probably change)
	ERR_NOT_IMPLEMENTED = -1006,

	// feature won't be supported
	ERR_NOT_SUPPORTED   = -1007,

	// file contents are damaged
	ERR_CORRUPTED       = -1008,

	ERR_UNKNOWN_FORMAT  = -1009,

	ERR_TIMED_OUT       = -1010,

	//
	// file + vfs
	//

	ERR_FILE_NOT_FOUND  = -1100,
	ERR_PATH_NOT_FOUND  = -1101,
	ERR_DIR_END         = -1102,
	ERR_EOF             = -1103,
	ERR_PATH_LENGTH     = -1104,
	ERR_NOT_FILE        = -1105,
	ERR_FILE_ACCESS     = -1106,
	ERR_IO              = -1107,


	ERR_TEX_FMT_INVALID = -1200,


	ERR_LAST
};




#ifndef MIN
#define MIN(a, b) (((a) < (b))? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b))? (a) : (b))
#endif


// For insertion into code, to avoid unused warnings:
#define UNUSED(param) (void)param;
// For using in parameters ("void f(int UNUSEDPARAM(x))"),
// to avoid 'unreferenced formal parameter' warnings:
#define UNUSEDPARAM(param)


#define ARRAY_SIZE(name) (sizeof(name) / sizeof(name[0]))


//
// compile-time assert, especially useful for testing sizeof().
// no runtime overhead; may be used anywhere, including file scope.
//

// generate a symbol containing the line number of the macro invocation.
// used to give a unique name (per file) to types made by cassert.
// we can't prepend __FILE__ to make it globally unique - the filename
// may be enclosed in quotes. need the 2 macro expansions to make sure
// __LINE__ is expanded correctly.
#define MAKE_UID2__(l) LINE_ ## l
#define MAKE_UID1__(l) MAKE_UID2__(l)
#define UID__ MAKE_UID1__(__LINE__)

// more descriptive error message, but may cause a struct redefinition
// warning if used from the same line in different files.
#define cassert(expr) struct UID__ { int CASSERT_FAILURE: (expr); }

// less helpful error message, but redefinition doesn't trigger warnings.
#define cassert2(expr) extern char CASSERT_FAILURE[1][(expr)]

// note: alternative method in C++: specialize a struct only for true;
// using it will raise 'incomplete type' errors if instantiated with false.

#ifdef _WIN32

#define debug_warn(str) assert(0 && (str))

#else

#define debug_warn(str) debug_out("Debug Warning Fired at %s:%d: %s\n", __FILE__, __LINE__, str)

#endif

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






const size_t KB = 1ul << 10;
const size_t MB = 1ul << 20;


#ifdef _WIN32
#define DIR_SEP '\\'
#else
#define DIR_SEP '/'
#endif


#define BIT(n) (1ul << (n))




// call from main as early as possible.
extern void lib_init();


enum CallConvention	// number of parameters and convention
{
	CC_CDECL_0,
	CC_CDECL_1,
#ifdef _WIN32
	CC_STDCALL_0,
	CC_STDCALL_1,
#endif

	CC_UNUSED	// get rid of trailing comma when !_WIN32
};


// more powerful atexit: registers an exit handler, with 0 or 1 parameters.
// callable before libc initialized, and frees up the real atexit table.
// stdcall convention is provided on Windows to call APIs (e.g. WSACleanup).
// for these to be called at exit, lib_main must be invoked after _cinit.
extern int atexit2(void* func, uintptr_t arg, CallConvention cc = CC_CDECL_1);

// no parameters, cdecl (CC_CDECL_0)
extern int atexit2(void* func);
inline int atexit2(void (*func)())
{
	return atexit2((void *)func);
}











// FNV1-A hash - good for strings.
// if len = 0 (default), treat buf as a C-string;
// otherwise, hash <len> bytes of buf.
extern u32 fnv_hash(const void* buf, const size_t len = 0);
extern u64 fnv_hash64(const void* buf, const size_t len);

// hash (currently FNV) of a filename
typedef u32 FnHash;


#ifndef min
inline int min(int a, int b)
{
	return (a < b)? a : b;
}

inline int max(int a, int b)
{
	return (a > b)? a : b;
}
#endif

extern u16 addusw(u16 x, u16 y);
extern u16 subusw(u16 x, u16 y);


extern u16 read_le16(const void* p);
extern u32 read_le32(const void* p);
extern u16 read_be16(const void* p);
extern u32 read_be32(const void* p);


extern bool is_pow2(long n);

// return -1 if not an integral power of 2,
// otherwise the base2 logarithm
extern int ilog2(const int n);

// return log base 2, rounded up.
extern uint log2(uint x);


extern uintptr_t round_up(uintptr_t val, uintptr_t multiple);

extern u16 fp_to_u16(double in);

// big endian!
extern void base32(const int len, const u8* in, u8* out);


// case-insensitive check if string <s> matches the pattern <w>,
// which may contain '?' or '*' wildcards. if so, return 1, otherwise 0.
extern int match_wildcard(const char* s, const char* w);


// design goals:
// fast (including startup time)
// portable
// reusable across projects (i.e. no dependency on "parent" that calls modules)


#endif	// #ifndef LIB_H
