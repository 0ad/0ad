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

#include "config.h"

#include "misc.h"

#include "sysdep/sysdep.h"


// yikes! avoid template warning spew on VC6
#if _MSC_VER <= 1200
#pragma warning(disable:4786)
#endif


#define STMT(STMT_code__) do { STMT_code__; } while(0)

// must not be used before main entered! (i.e. not from NLS constructors / functions)
#define ONCE(ONCE_code__)\
STMT(\
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;\
	static bool ONCE_done__ = false;\
	if(pthread_mutex_trylock(&(mutex)) == 0 && !ONCE_done__)\
	{\
		ONCE_done__ = true;\
		ONCE_code__;\
	}\
)


#define CHECK_ERR(func)\
STMT(\
	int err = (int)(func);\
	if(err < 0)\
		return err;\
)


enum LibError
{
	ERR_INVALID_HANDLE = -1000,
	ERR_NO_MEM         = -1001,
	ERR_EOF            = -1002,	// attempted to read beyond EOF
	ERR_INVALID_PARAM  = -1003,
	ERR_FILE_NOT_FOUND = -1004,
	ERR_PATH_NOT_FOUND = -1005,

	ERR_LAST
};


#ifndef MIN
#define MIN(a, b) (((a) < (b))? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b))? (a) : (b))
#endif


#define UNUSED(param) (void)param;



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
#define cassert(expr) struct UID__ { int CASSERT_FAILURE: (expr); };

// less helpful error message, but redefinition doesn't trigger warnings.
#define cassert2(expr) extern char CASSERT_FAILURE[1][(expr)];

// note: alternative method in C++: specialize a struct only for true;
// using it will raise 'incomplete type' errors if instantiated with false.




#define debug_warn(str) assert(0 && (str))




// converts 4 character string to u32 for easy comparison
// can't pass code as string, and use s[0]..s[3], because
// VC6/7 don't realize the macro is constant
// (it should be useable as a switch{} expression)
#ifdef BIG_ENDIAN
#define FOURCC(a,b,c,d) ( ((u32)a << 24) | ((u32)b << 16) | \
	((u32)c << 8 ) | ((u32)d << 0 ) )
#else
#define FOURCC(a,b,c,d) ( ((u32)a << 0 ) | ((u32)b << 8 ) | \
	((u32)c << 16) | ((u32)d << 24) )
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

#include "posix.h"



#endif	// #ifndef LIB_H
