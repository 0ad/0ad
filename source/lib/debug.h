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

#include "sysdep/sysdep.h"		// ErrorReaction
#ifdef _WIN32
# include "sysdep/win/wdbg.h"
#else
# include "sysdep/unix/udbg.h"
#endif

/*

[KEEP IN SYNC WITH WIKI]

overview
--------

this module provides platform-independent debug facilities, useful for
diagnosing and reporting program errors.
- a symbol engine provides access to compiler-generated debug information and
  can also give a stack trace including local variables;
- the breakpoint API enables stopping when a given address is
  executed, read or written to (as specified);
- a hook into the system's memory allocator can optionally check for and
  report heap corruption;
- our more powerful assert() replacement gives a stack trace so
  that the underlying problem becomes apparent;
- the output routines make for platform-independent logging and
  crashlogs with "last-known activity" reporting.


usage
-----

please see the detailed comments below on how to use the individual features.
much of this is only helpful if you explicity ask for it!


rationale
---------

much of this functionality already exists within the VC7 IDE/debugger.
motivation for this code is as follows:
- we want a consistent interface for all platforms;
- limitations(*) in the VC variants should be fixed;
- make debugging as easy as possible.

* mostly pertaining to Release mode - e.g. symbols cannot be resolved
even if debug information is present and assert dialogs are useless.

*/

//-----------------------------------------------------------------------------
// debug memory allocator
//-----------------------------------------------------------------------------

// check heap integrity (independently of mmgr).
// errors are reported by the CRT or via display_error.
extern void debug_heap_check(void);

enum DebugHeapChecks
{
	// no automatic checks. (default)
	DEBUG_HEAP_NONE   = 0,

	// basic automatic checks when deallocating.
	DEBUG_HEAP_NORMAL = 1,

	// all automatic checks on every memory API call. this is really
	// slow (x100), but reports errors closer to where they occurred.
	DEBUG_HEAP_ALL    = 2
};

// call at any time; from then on, the specified checks will be performed.
// if not called, the default is DEBUG_HEAP_NONE, i.e. do nothing.
extern void debug_heap_enable(DebugHeapChecks what);


//-----------------------------------------------------------------------------
// debug_assert
//-----------------------------------------------------------------------------

// make sure the expression <expr> evaluates to non-zero. used to validate
// invariants in the program during development and thus gives a
// very helpful warning if something isn't going as expected.
// sprinkle these liberally throughout your code!
//
// recommended use is debug_assert(expression && "descriptive string") -
// the string can pass more information about the problem on to whomever
// is seeing the error.
//
// rationale: 0x55 and 0xaa have distinctive bit patterns and thus
// help debug the symbol engine.
#define debug_assert(expr)\
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

// rationale: we call our assert "debug_assert" for the following reasons:
// - consistency (everything here is prefixed with debug_) and
// - to avoid inadvertent use of the much less helpful built-in CRT assert.
//   if we were to override assert, it would be difficult to tell whether
//   user source has included <assert.h> (possibly indirectly via other
//   headers) and thereby stomped on our definition.

// called when an assertion has failed; notifies the user via display_error.
extern enum ErrorReaction debug_assert_failed(const char* source_file, int line, const char* assert_expr);


//-----------------------------------------------------------------------------
// output
//-----------------------------------------------------------------------------

// show a dialog to make sure unexpected states in the program are noticed.
// this is less error-prone than debug_assert(!"text");
#define debug_warn(str) debug_assert(0 && (str))

// write to the debugger output window (may take ~1 ms!)
extern void debug_printf(const char* fmt, ...);
extern void debug_wprintf(const wchar_t* fmt, ...);

// write to memory buffer (fast)
// used for "last activity" reporting in the crashlog.
extern void debug_wprintf_mem(const wchar_t* fmt, ...);

// write all logs and <text> out to crashlog.txt (unicode format).
// currently all filenames are hardcoded; this may change.
extern int debug_write_crashlog(const wchar_t* text);


//-----------------------------------------------------------------------------
// breakpoints
//-----------------------------------------------------------------------------

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

// (values chosen to match IA-32 bit defs, so compiler can optimize.
// this isn't required, it'll work regardless.)
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
// breakpoints are a limited resource (4 on IA-32); if none are
// available, we return ERR_LIMIT.
extern int debug_set_break(void* addr, DbgBreakType type);

// remove all breakpoints that were set by debug_set_break.
// important, since these are a limited resource.
extern int debug_remove_all_breaks();


//-----------------------------------------------------------------------------
// symbol access
//-----------------------------------------------------------------------------

// maximum number of characters (including trailing \0) written to
// user's buffers by debug_resolve_symbol.
const size_t DBG_SYMBOL_LEN = 1000;
const size_t DBG_FILE_LEN = 100;

// read and return symbol information for the given address. all of the
// output parameters are optional; we pass back as much information as is
// available and desired. return 0 iff any information was successfully
// retrieved and stored.
// sym_name and file must hold at least the number of chars above;
// file is the base name only, not path (see rationale in wdbg_sym).
// the PDB implementation is rather slow (~500µs).
extern int debug_resolve_symbol(void* ptr_of_interest, char* sym_name, char* file, int* line);

// write a complete stack trace (including values of local variables) into
// the specified buffer. if <context> is nonzero, it is assumed to be a
// platform-specific representation of execution state (e.g. Win32 CONTEXT)
// and tracing starts there; this is useful for exceptions.
// otherwise, tracing starts at the current stack position, and the given
// number of stack frames (i.e. functions) above the caller are skipped.
// this prevents functions like debug_assert_failed from
// cluttering up the trace. returns the buffer for convenience.
extern const wchar_t* debug_dump_stack(wchar_t* buf, size_t max_chars, uint skip, void* context);


//-----------------------------------------------------------------------------
// helper functions (used by implementation)
//-----------------------------------------------------------------------------

// abstraction of all STL iterators used by debug_stl.
typedef const u8* (*DebugIterator)(void* internal, size_t el_size);

// return address of the Nth function on the call stack.
// if <context> is nonzero, it is assumed to be a platform-specific
// representation of execution state (e.g. Win32 CONTEXT) and tracing
// starts there; this is useful for exceptions.
// otherwise, tracing starts at the current stack position, and the given
// number of stack frames (i.e. functions) above the caller are skipped.
// used by mmgr to determine what function requested each allocation;
// this is fast enough to allow that.
extern void* debug_get_nth_caller(uint skip, void* context);

// return 1 if the pointer appears to be totally bogus, otherwise 0.
// this check is not authoritative in that the pointer may in truth
// be invalid regardless of the return value here, but can be used
// to filter out obviously wrong values in a portable manner.
extern int debug_is_bogus_pointer(const void* p);

#endif	// #ifndef DEBUG_H_INCLUDED
