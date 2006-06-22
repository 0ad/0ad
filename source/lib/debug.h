/**
 * =========================================================================
 * File        : debug.h
 * Project     : 0 A.D.
 * Description : platform-independent debug support code.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

#include "lib.h" // STMT
#if OS_WIN
# include "lib/sysdep/win/wdbg.h"
#else
# include "lib/sysdep/unix/udbg.h"
#endif

/**

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

**/

//-----------------------------------------------------------------------------
// debug memory allocator
//-----------------------------------------------------------------------------

/**
 * check heap integrity (independently of mmgr).
 * errors are reported by the CRT or via debug_display_error.
 **/
extern void debug_heap_check(void);

enum DebugHeapChecks
{
	/**
	 * no automatic checks. (default)
	 **/
	DEBUG_HEAP_NONE   = 0,

	/**
	 * basic automatic checks when deallocating.
	 **/
	DEBUG_HEAP_NORMAL = 1,

	/**
	 * all automatic checks on every memory API call. this is really
	 * slow (x100), but reports errors closer to where they occurred.
	 **/
	DEBUG_HEAP_ALL    = 2
};

/**
 * call at any time; from then on, the specified checks will be performed.
 * if not called, the default is DEBUG_HEAP_NONE, i.e. do nothing.
 **/
extern void debug_heap_enable(DebugHeapChecks what);


//-----------------------------------------------------------------------------
// output
//-----------------------------------------------------------------------------

/**
 * write a formatted string to the debug channel, subject to filtering
 * (see below). implemented via debug_puts - see performance note there.
 *
 * @param format string and varargs; see printf.
 **/
extern void debug_printf(const char* fmt, ...);

/// note: this merely converts to a MBS and calls debug_printf.
extern void debug_wprintf(const wchar_t* fmt, ...);


/**
 * translates and displays the given strings in a dialog.
 * this is typically only used when debug_display_error has failed or
 * is unavailable because that function is much more capable.
 * implemented via sys_display_msgw; see documentation there.
 **/
extern void debug_display_msgw(const wchar_t* caption, const wchar_t* msg);

/// flags to customize debug_display_error behavior
enum DebugDisplayErrorFlags
{
	/**
	 * disallow the Continue button. used e.g. if an exception is fatal.
	 **/
	DE_NO_CONTINUE    = 1,

	/**
	 * enable the Suppress button. set automatically by debug_display_error if
	 * it receives a non-NULL suppress pointer. a flag is necessary because
	 * the sys_display_error interface doesn't get that pointer.
	 * rationale for automatic setting: this may prevent someone from
	 * forgetting to specify it, and disabling Suppress despite having
	 * passed a non-NULL pointer doesn't make much sense.
	 **/
	DE_ALLOW_SUPPRESS = 2,

	/**
	 * do not trigger a breakpoint inside debug_display_error; caller
	 * will take care of this if ER_BREAK is returned. this is so that the
	 * debugger can jump directly into the offending function.
	 **/
	DE_MANUAL_BREAK   = 4
};

/**
 * value for suppress flag once set by debug_display_error.
 * rationale: this value is fairly distinctive and helps when
 * debugging the symbol engine.
 * initial value is 0 rather that another constant; this avoids
 * allocating .rdata space.
 **/
const u8 DEBUG_SUPPRESS = 0xAB;

/**
 * choices offered by the shared error dialog
 **/
enum ErrorReaction
{
	/**
	 * ignore, continue as if nothing happened.
	 * note: value doesn't start at 0 because that is interpreted as a
	 * DialogBoxParam failure.
	 **/
	ER_CONTINUE = 1,

	/**
	 * trigger breakpoint, i.e. enter debugger.
	 * only returned if DE_MANUAL_BREAK was passed; otherwise,
	 * debug_display_error will trigger a breakpoint itself.
	 **/
	ER_BREAK,

	/**
	 * ignore and do not report again.
	 * note: non-persistent; only applicable during this program run.
	 * acted on by debug_display_error; never returned to caller.
	 **/
	ER_SUPPRESS,

	/**
	 * exit the program immediately.
	 * acted on by debug_display_error; never returned to caller.
	 **/
	ER_EXIT,

	/**
	 * special return value for the display_error app hook stub to indicate
	 * that it has done nothing and that the normal sys_display_error
	 * implementation should be called instead.
	 * acted on by debug_display_error; never returned to caller.
	 **/
	ER_NOT_IMPLEMENTED
};

/**
 * display an error dialog with a message and stack trace.
 *
 * @param description text to show.
 * @param flags: see DebugDisplayErrorFlags.
 * @param context, skip: see debug_dump_stack.
 * @param file, line, func: location of the error (typically passed as
 * __FILE__, __LINE__, __func__ from a macro)
 * @param suppress pointer to a caller-allocated flag that can be used to
 * suppress this error. if NULL, this functionality is skipped and the
 * "Suppress" dialog button will be disabled.
 * note: this flag is read and written exclusively here; caller only
 * provides the storage. values: see DEBUG_SUPPRESS above.
 * @return ErrorReaction (user's choice: continue running or stop?)
 **/
extern ErrorReaction debug_display_error(const wchar_t* description,
	uint flags, uint skip, void* context,
	const char* file, int line, const char* func,
	u8* suppress);

/**
 * convenience version, in case the advanced parameters aren't needed.
 * macro instead of providing overload/default values for C compatibility.
 **/
#define DISPLAY_ERROR(text) debug_display_error(text, 0, 0,0, __FILE__,__LINE__,__func__, 0)


//
// filtering
//

/**
 * debug output is very useful, but "too much of a good thing can kill you".
 * we don't want to require different LOGn() macros that are enabled
 * depending on "debug level", because changing that entails lengthy
 * compiles and it's too coarse-grained. instead, we require all
 * strings to start with "tag_string:" (exact case and no quotes;
 * the alphanumeric-only <tag_string> identifies output type).
 * they are then subject to filtering: only if the tag has been
 * "added" via debug_filter_add is the appendant string displayed.
 *
 * this approach is easiest to implement and is fine because we control
 * all logging code. LIMODS falls from consideration since it's not
 * portable and too complex.
 *
 * notes:
 * - filter changes only affect subsequent debug_*printf calls;
 *   output that didn't pass the filter is permanently discarded.
 * - strings not starting with a tag are always displayed.
 * - debug_filter_* can be called at any time and from the debugger.

 * in future, allow output with the given tag to proceed.
 * no effect if already added.
 **/
extern void debug_filter_add(const char* tag);

/**
 * in future, discard output with the given tag.
 * no effect if not currently added.
 **/
extern void debug_filter_remove(const char* tag);

/**
 * clear all filter state; equivalent to debug_filter_remove for
 * each tag that was debug_filter_add-ed.
 **/
extern void debug_filter_clear();


/**
 * write to memory buffer (fast)
 * used for "last activity" reporting in the crashlog.
 *
 * @param format string and varags; see printf.
 **/
extern void debug_wprintf_mem(const wchar_t* fmt, ...);

/**
 * write all logs and <text> out to crashlog.txt (unicode format).
 **/
extern LibError debug_write_crashlog(const wchar_t* text);


//-----------------------------------------------------------------------------
// debug_assert
//-----------------------------------------------------------------------------

/**
 * make sure the expression <expr> evaluates to non-zero. used to validate
 * invariants in the program during development and thus gives a
 * very helpful warning if something isn't going as expected.
 * sprinkle these liberally throughout your code!
 *
 * recommended use is debug_assert(expression && "descriptive string") -
 * the string can pass more information about the problem on to whomever
 * is seeing the error.
 *
 * rationale: we call this "debug_assert" instead of "assert" for the
 * following reasons:
 * - consistency (everything here is prefixed with debug_) and
 * - to avoid inadvertent use of the much less helpful built-in CRT assert.
 *   if we were to override assert, it would be difficult to tell whether
 *   user source has included <assert.h> (possibly indirectly via other
 *   headers) and thereby stomped on our definition.
 **/
#define debug_assert(expr) \
STMT(\
	static u8 suppress__;\
	if(!(expr))\
	{\
		switch(debug_assert_failed(#expr, &suppress__, __FILE__, __LINE__, __func__))\
		{\
		case ER_BREAK:\
			debug_break();\
			break;\
		default:\
			break;\
		}\
	}\
)

/**
 * show a dialog to make sure unexpected states in the program are noticed.
 * this is less error-prone than "debug_assert(0 && "text");" and avoids
 * "conditional expression is constant" warnings. we'd really like to
 * completely eliminate the problem; replacing 0 literals with extern
 * volatile variables fools VC7 but isn't guaranteed to be free of overhead.
 * we therefore just squelch the warning (unfortunately non-portable).
 **/
#define debug_warn(str) debug_assert((str) && 0)


/**
 * if (LibError)err indicates an function failed, display the error dialog.
 * used by CHECK_ERR et al., which wrap function calls and automatically
 * warn user and return to caller.
 **/
#define DEBUG_WARN_ERR(err)\
STMT(\
	static u8 suppress__;\
	switch(debug_warn_err(err, &suppress__, __FILE__, __LINE__, __func__))\
	{\
	case ER_BREAK:\
		debug_break();\
		break;\
	default:\
		break;\
	}\
)


/**
 * called when a debug_assert fails;
 * notifies the user via debug_display_error.
 *
 * @param assert_expr the expression that failed; typically passed as
 * #expr in the assert macro.
 * @param suppress see debug_display_error.
 * @param file, line source file name and line number of the spot that failed
 * @param func name of the function containing it
 * @return ErrorReaction (user's choice: continue running or stop?)
 **/
extern ErrorReaction debug_assert_failed(const char* assert_expr,
	u8* suppress,
	const char* file, int line, const char* func);

/**
 * called when a DEBUG_WARN_ERR indicates an error occurred;
 * notifies the user via debug_display_error.
 *
 * @param err LibError value indicating the error that occurred
 * @param suppress see debug_display_error.
 * @param file, line source file name and line number of the spot that failed
 * @param func name of the function containing it
 * @return ErrorReaction (user's choice: continue running or stop?)
 **/
extern ErrorReaction debug_warn_err(LibError err,
	u8* suppress,
	const char* file, int line, const char* func);


//-----------------------------------------------------------------------------
// breakpoints
//-----------------------------------------------------------------------------

/**
 * trigger a breakpoint when reached/"called".
 * defined as a macro by the platform-specific header above; this allows
 * breaking directly into the target function, instead of one frame
 * below it as with a conventional call-based implementation.
 **/
//#define debug_break()	// not defined here; see above


/**
 * sometimes mmgr's 'fences' (making sure padding before and after the
 * allocation remains intact) aren't enough to catch hard-to-find
 * memory corruption bugs. another tool is to trigger a debug exception
 * when the later to be corrupted variable is accessed; the problem should
 * then become apparent.
 * the VC++ IDE provides such 'breakpoints', but can only detect write access.
 * additionally, it can't resolve symbols in Release mode (where this would
 * be most useful), so we provide a breakpoint API.

 * (values chosen to match IA-32 bit defs, so compiler can optimize.
 * this isn't required; it'll work regardless.)
 **/
enum DbgBreakType
{
	DBG_BREAK_CODE       = 0,	/// execute
	DBG_BREAK_DATA_WRITE = 1,	/// write
	DBG_BREAK_DATA       = 3	/// read or write
};

/**
 * arrange for a debug exception to be raised when the
 * indicated memory is accessed.
 *
 * @param addr memory address
 * for simplicity, the length (range of bytes to be checked) is derived
 * from addr's alignment, and is typically 1 machine word.
 * @param type the type of access to watch for (see DbgBreakType)
 * @return LibError; ERR_LIMIT if no more breakpoints are available
 * (they are a limited resource - only 4 on IA-32).
 **/
extern LibError debug_set_break(void* addr, DbgBreakType type);

/**
 * remove all breakpoints that were set by debug_set_break.
 * important, since these are a limited resource.
 **/
extern LibError debug_remove_all_breaks();


//-----------------------------------------------------------------------------
// symbol access
//-----------------------------------------------------------------------------

/**
 * maximum number of characters (including trailing \0) written to
 * user's buffers by debug_resolve_symbol.
 **/
const size_t DBG_SYMBOL_LEN = 1000;
const size_t DBG_FILE_LEN = 100;

/**
 * read and return symbol information for the given address.
 *
 * NOTE: the PDB implementation is rather slow (~500us).
 *
 * @param ptr_of_interest address of symbol (e.g. function, variable)
 * @param sym_name optional out; size >= DBG_SYMBOL_LEN chars;
 * receives symbol name returned via debug info.
 * @param file optional out; size >= DBG_FILE_LEN chars; receives
 * base name only (no path; see rationale in wdbg_sym) of
 * source file containing the symbol.
 * @param line optional out; receives source file line number of symbol.
 *
 * note: all of the output parameters are optional; we pass back as much
 * information as is available and desired.
 * @return LibError; INFO_OK iff any information was successfully
 * retrieved and stored.
 **/
extern LibError debug_resolve_symbol(void* ptr_of_interest, char* sym_name, char* file, int* line);

/**
 * write a complete stack trace (including values of local variables) into
 * the specified buffer.
 *
 * @param buf target buffer
 * @param max_chars of buffer (should be several thousand)
 * @param skip number of stack frames (i.e. functions on call stack) to skip.
 * this prevents error-reporting functions like debug_assert_failed from
 * cluttering up the trace.
 * @param context platform-specific representation of execution state
 * (e.g. Win32 CONTEXT). if not NULL, tracing starts there; this is useful
 * for exceptions. otherwise, tracing starts from the current call stack.
 * @return buf for convenience; writes an error string into it if
 * something goes wrong.
 *
 * not reentrant! (pointer to buf is stored in static variable)
 **/
extern const wchar_t* debug_dump_stack(wchar_t* buf, size_t max_chars, uint skip, void* context);


//-----------------------------------------------------------------------------
// helper functions (used by implementation)
//-----------------------------------------------------------------------------

/**
 * [system-dependent] write a string to the debug channel.
 * this can be quite slow (~1 ms)! On Windows, it uses OutputDebugString
 * (entails context switch), otherwise stdout+fflush (waits for IO).
 **/
extern void debug_puts(const char* text);

/**
 * return address of the Nth function on the call stack.
 *
 * used by mmgr to determine what function requested each allocation;
 * this is fast enough to allow that.
 *
 * @param skip number of stack frames (i.e. functions on call stack) to skip.
 * @param context platform-specific representation of execution state
 * (e.g. Win32 CONTEXT). if not NULL, tracing starts there; this is useful
 * for exceptions. otherwise, tracing starts from the current call stack.
 * @return address of Nth function
 **/
extern void* debug_get_nth_caller(uint skip, void* context);

/**
 * check if a pointer appears to be totally invalid.
 *
 * this check is not authoritative (the pointer may be "valid" but incorrect)
 * but can be used to filter out obviously wrong values in a portable manner.
 *
 * @param p pointer
 * @return 1 if totally bogus, otherwise 0.
 **/
extern int debug_is_pointer_bogus(const void* p);

/// does the given pointer appear to point to code?
extern bool debug_is_code_ptr(void* p);

/// does the given pointer appear to point to the stack?
extern bool debug_is_stack_ptr(void* p);


/**
 * set the current thread's name; it will be returned by subsequent calls to
 * debug_get_thread_name.
 *
 * if supported on this platform, the debugger is notified of the new name;
 * it will be displayed there instead of just the handle.
 *
 * @param name identifier string for thread. MUST remain valid throughout
 * the entire program; best to pass a string literal. allocating a copy
 * would be quite a bit more work due to cleanup issues.
 **/
extern void debug_set_thread_name(const char* name);

/**
 * return current thread's name.
 *
 * @return thread name, or NULL if one hasn't been assigned yet
 * via debug_set_thread_name.
 **/
extern const char* debug_get_thread_name();


/**
 * call at exit to avoid some leaks.
 * not strictly necessary.
 **/
extern void debug_shutdown();

#endif	// #ifndef DEBUG_H_INCLUDED
