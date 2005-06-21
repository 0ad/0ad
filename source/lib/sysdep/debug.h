// platform-independent debug interface
// Copyright (c) 2002-2005 Jan Wassenberg
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

#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#include "sysdep.h"		// ErrorReaction
#ifdef _WIN32
# include "win/wdbg.h"
#else
# include "unix/udbg.h"
#endif


// check heap integrity (independently of mmgr).
// errors are reported by the CRT, e.g. via assert.
extern void debug_check_heap(void);

extern void debug_disable_leak_reporting();


//////////////////////////////////////////////////////////////////////////////
//
// assert
//

// notify the user that an assertion failed; displays a
// stack trace with local variables.
// returns one of UserErrorReaction.
extern enum ErrorReaction debug_assert_failed(const char* source_file, int line, const char* assert_expr);

// recommended use: assert2(expr && "descriptive string")
#define assert2(expr)\
STMT(\
	static unsigned char suppress__ = 0x55;\
	if(suppress__ == 0x55 && !(expr))\
		switch(debug_assert_failed(__FILE__, __LINE__, #expr))\
		{\
		case ER_SUPPRESS:\
			suppress__ = 0xaa;\
			break;\
		case ER_BREAK:\
			debug_break();\
			break;\
		}\
)


//////////////////////////////////////////////////////////////////////////////
//
// output
//

// write to the debugger output window (may take ~1 ms!)
extern void debug_printf(const char* fmt, ...);
extern void debug_wprintf(const wchar_t* fmt, ...);

// write to memory buffer (fast)
// used for "last activity" reporting in the crashlog.
extern void debug_wprintf_mem(const wchar_t* fmt, ...);

// warn of unexpected state. less error-prone than assert(!"text");
#define debug_warn(str) assert2(0 && (str))

// TODO
extern int debug_write_crashlog(const wchar_t* text);


//////////////////////////////////////////////////////////////////////////////
//
// breakpoints
//

// trigger a breakpoint when reached/"called".
// defined as a macro by the platform-specific header above; this allows
// breaking directly into the target function, instead of one frame
// below it as with a conventional call-based implementation.
//#define debug_break()


// sometimes mmgr's 'fences' (making sure padding before and after the
// allocation remains intact) aren't enough to catch hard-to-find
// memory corruption bugs. another tool is to trigger a debug exception
// when the later to be corrupted variable is accessed; the problem should
// then become apparent.
// the VC++ IDE provides such 'breakpoints', but can only detect write access.
// additionally, it can't resolve symbols in Release mode (where this would
// be most useful), so we provide a breakpoint API.

// values chosen to match IA-32 bit defs, so compiler can optimize.
// this isn't required, it'll work regardless.
enum DbgBreakType
{
	DBG_BREAK_CODE       = 0,	// execute
	DBG_BREAK_DATA_WRITE = 1,	// write
	DBG_BREAK_DATA       = 3	// read or write
};

// arrange for a debug exception to be raised when <addr> is accessed
// according to <type>.
// for simplicity, the length (range of bytes to be checked) is derived
// from addr's alignment, and is typically 1 machine word.
// breakpoints are a limited resource (4 on IA-32); abort and
// return ERR_LIMIT if none are available.
extern int debug_set_break(void* addr, DbgBreakType type);

// remove all breakpoints that were set by debug_set_break.
// important, since these are a limited resource.
extern int debug_remove_all_breaks();


//////////////////////////////////////////////////////////////////////////////
//
// symbol access
//

// TODO: rationale+comments

const size_t DBG_SYMBOL_LEN = 1000;
const size_t DBG_FILE_LEN = 100;

extern void* debug_get_nth_caller(uint n);

extern int debug_resolve_symbol(void* ptr_of_interest, char* sym_name, char* file, int* line);

extern const wchar_t* debug_dump_stack(wchar_t* buf, size_t max_chars, uint skip, void* context);

#endif	// #ifndef DEBUG_H_INCLUDED
