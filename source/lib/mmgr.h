// memory manager and tracker
//
// Copyright (c) 2005 Jan Wassenberg
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


// !!!!!!!!!! see user guide / documentation at end of file !!!!!!!!!!


// provide for completely disabling the memory manager
// (e.g. when using other debug packages)
//
// note: checking here messes up include guard detection, but we need to
// cover both the guarded part (constants+externs) and the macros.
#ifdef USE_MMGR

#ifndef	MMGR_H__
#define	MMGR_H__

#include "types.h"

//
// optional additional checks, enabled via mmgr_set_options.
// these slow down the application; see 'digging deeper' above.
//

// log all allocation/deallocation operations undertaken.
const uint MMGR_LOG_ALL       = 0x01;

// validate all allocations on every memory API call. slow!
const uint MMGR_VALIDATE_ALL  = 0x02;

// fill the user-visible part of each allocation with a certain pattern
// on alloc and free. this is required for unused memory tracking.
const uint MMGR_FILL          = 0x04;

// log all enter/exit into our main functions. if there's an
// unmatched pair in the log, we know where a crash occurred.
const uint MMGR_TRACE         = 0x08;

// when calling global operator delete, where we can't pass owner
// information via macro, find out who called us via stack trace
// and resolve the symbol via debug information.
// more informative, but adds 500µs to every call.
const uint MMGR_RESOLVE_OWNER = 0x10;

// force each log line to be written directly to disk. slow!
// use only when the application is crashing, to make sure all
// available information is written out.
const uint MMGR_FLUSH_LOG     = 0x20;

// an alias that includes all of the above.
const uint MMGR_ALL           = 0xff;

// return the current options unchanged.
const uint MMGR_QUERY         = ~0;

extern uint mmgr_set_options(uint);


// break when a certain allocation is created/reallocated/freed:
extern void mmgr_break_on_alloc(uint count);
extern void mmgr_break_on_realloc(const void*);
extern void mmgr_break_on_free(const void*);

// "proactive" validation: (see 'digging deeper' above)
extern bool mmgr_is_valid_ptr(const void*);
extern bool mmgr_are_all_valid(void);

// write a report file
extern void mmgr_write_report(void);
extern void mmgr_write_leak_report(void);

//
// our wrappers for C++ memory handling functions
//

// note that all line numbers are int, for compatibility with external
// overloaded operator new.

extern void* mmgr_malloc_dbg (size_t size,             const char* file, int line, const char* func);
extern void* mmgr_calloc_dbg (size_t num, size_t size, const char* file, int line, const char* func);
extern void* mmgr_realloc_dbg(void* p, size_t size,    const char* file, int line, const char* func);
extern void  mmgr_free_dbg   (void* p,                 const char* file, int line, const char* func);

extern char* mmgr_strdup_dbg(const char*,                  const char* file, int line, const char* func);
extern wchar_t* mmgr_wcsdup_dbg(const wchar_t*,            const char* file, int line, const char* func);
extern char* mmgr_getcwd_dbg(char*, size_t,                const char* file, int line, const char* func);


// .. global operator new (to catch allocs from STL/external libs)
extern void* operator new  (size_t size);
extern void* operator new[](size_t size);
// .. override commonly used global operator new overload (done e.g. by MFC),
//    in case someone hasn't included this file
extern void* operator new  (size_t size, const char* file, int line);
extern void* operator new[](size_t size, const char* file, int line);
// .. called by our global operator new hook macro
extern void* operator new  (size_t size, const char* file, int line, const char* func);
extern void* operator new[](size_t size, const char* file, int line, const char* func);
// .. global operator delete
extern void operator delete  (void* p) throw();
extern void operator delete[](void* p) throw();
// .. corresponding delete for first overloaded new,
//    only called if exception raised during ctor
extern void operator delete  (void* p, const char* file, int line) throw();
extern void operator delete[](void* p, const char* file, int line) throw();
// .. corresponding delete for our overloaded new,
//    only called if exception raised during ctor
extern void operator delete  (void* p, const char* file, int line, const char* func) throw();
extern void operator delete[](void* p, const char* file, int line, const char* func) throw();


#endif	// #ifdef MMGR_H__


//
// hook macros
//

#include "nommgr.h"

// get rid of __FUNCTION__ unless we know the compiler supports it.
// (note: don't define if built-in - compiler will raise a warning)
#if !defined(_MSC_VER) && !defined(__GNUC__)
#define	__FUNCTION__ 0
#endif

#define	new new(__FILE__, __LINE__, __FUNCTION__)
// hooking delete and setting global owner variables/pushing them on a stack
// isn't thread-safe and can be fooled with destructor chains.
// we instead rely on the call stack (works with VC and GCC)
#define	malloc(size)      mmgr_malloc_dbg (size,    __FILE__,__LINE__,__FUNCTION__)
#define	calloc(num, size) mmgr_calloc_dbg (num,size,__FILE__,__LINE__,__FUNCTION__)
#define	realloc(p,size)   mmgr_realloc_dbg(p,size,  __FILE__,__LINE__,__FUNCTION__)
#define	free(p)           mmgr_free_dbg   (p,       __FILE__,__LINE__,__FUNCTION__)

#define strdup(p)              mmgr_strdup_dbg(p,       __FILE__,__LINE__,__FUNCTION__)
#define wcsdup(p)              mmgr_wcsdup_dbg(p,       __FILE__,__LINE__,__FUNCTION__)
#define getcwd(p,size)         mmgr_getcwd_dbg(p, size, __FILE__,__LINE__,__FUNCTION__)

#endif	// #ifdef USE_MMGR


// purpose and history
// -------------------

// our goal is to expose any memory handling bugs in the
// application as early as possible, by triggering
// breakpoints when a problem is detected.
// if all options are on, we can spot the following:
//   memory leaks, double-free, allocation over/underruns,
//   unused memory, and use-after-free.
//
// this code started life as Paul Nettle's memory manager (available
// at http://www.fluidstudios.com), and has been completely overhauled.
// in particular, it is now thread-safe and modularized;
// duplicated code has been eliminated.
//
//
// instructions for integrating into your project
// ----------------------------------------------
//
// 1) #include this from all project source files [that will allocate memory].
// 2) all system headers must be #include-d before this header, so that
//    we don't mess with any of their local operator new/delete.
// 3) if project source/headers also use local operator new/delete, #include
//    "nommgr.h" before that spot, and re-#include "mmgr.h" afterwards.
//
// 4) if using MFC:
//    - set linker option /FORCE - works around conflict between our global
//      operator new and that of MFC. be sure to check for other errors.
//    - remove any #define new DEBUG_NEW from all source files.
//
//
// effects
// -------
//
// many bugs are caught and announced with no further changes
// required, due to integrity checks inside the allocator.
//
// at exit, three report files are generated: a listing of leaks,
// various statistics (e.g. total unused memory), and the log.
// this lists (depending on settings) all allocations, enter/exit
// indications for our functions, and failure notifications.
//
//
// digging deeper
// --------------
//
// when tracking down hard-to-find bugs, more stringent checks can be
// activated via mmgr_set_option, or by changing the initial value of
// options in mmgr.cpp. however, they slow down the app considerably
// and need not always be enabled. see option declarations below.
//
// you can also change padding_size in mmgr.cpp at compile-time to provide
// more safety vs. overruns, at the cost of wasting lots of memory per
// allocation (which must also be cleared). this is only done in
// PARANOIA builds, because overruns seldom 'skip' padding.
//
// finally, you can induce memory allocations to fail a certain percentage
// of the time - this tests your application's error handling.
// adjust the RANDOM_FAILURE #define in mmgr.cpp.
//
//
// fixing your bugs
// ----------------
//
// if this code crashes or fails an assert, it is most likely due to a bug
// in your application. consult the current Alloc for information;
// search the log for its address to determine what operation failed,
// and what piece of code owns the allocation.
//
// if the cause isn't visible (i.e. the error is reported after the fact),
// you can try activating the more stringent checks to catch the problem
// earlier. you may also call the validation routines at checkpoints
// in your code to narrow the cause down. if all else fails, break on
// the allocation number to see what's happening.
//
// good luck!
