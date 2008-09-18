/**
 * =========================================================================
 * File        : debug.h
 * Project     : 0 A.D.
 * Description : platform-independent debug support code.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_DEBUG
#define INCLUDED_DEBUG

// this module provides platform-independent debug facilities, useful for
// diagnosing and reporting program errors.
// - a symbol engine provides access to compiler-generated debug information and
//   can also give a stack trace including local variables;
// - our more powerful assert() replacement gives a stack trace so
//   that the underlying problem becomes apparent;
// - the output routines make for platform-independent logging and
//   crashlogs with "last-known activity" reporting.


/**
 * trigger a breakpoint when reached/"called".
 * if defined as a macro, the debugger can break directly into the
 * target function instead of one frame below it as with a conventional
 * call-based implementation.
 **/
#if MSC_VERSION
# define debug_break __debugbreak	// intrinsic "function"
#else
extern void debug_break();
#endif


//-----------------------------------------------------------------------------
// output
//-----------------------------------------------------------------------------

/**
 * write a formatted string to the debug channel, subject to filtering
 * (see below). implemented via debug_puts - see performance note there.
 *
 * @param format string and varargs; see printf.
 **/
LIB_API void debug_printf(const char* fmt, ...);
LIB_API void debug_printf(const wchar_t* fmt, ...);


/**
 * translates and displays the given strings in a dialog.
 * this is typically only used when debug_display_error has failed or
 * is unavailable because that function is much more capable.
 * implemented via sys_display_msg; see documentation there.
 **/
LIB_API void debug_display_msgw(const wchar_t* caption, const wchar_t* msg);

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
LIB_API ErrorReaction debug_display_error(const wchar_t* description,
	size_t flags, size_t skip, void* context,
	const char* file, int line, const char* func,
	u8* suppress);

/**
 * convenience version, in case the advanced parameters aren't needed.
 * macro instead of providing overload/default values for C compatibility.
 **/
#define DEBUG_DISPLAY_ERROR(text) debug_display_error(text, 0, 0,0, __FILE__,__LINE__,__func__, 0)


//
// filtering
//

/**
 * debug output is very useful, but "too much of a good thing can kill you".
 * we don't want to require different LOGn() macros that are enabled
 * depending on "debug level", because changing that entails lengthy
 * compiles and it's too coarse-grained. instead, we require all
 * strings to start with "tag_string|" (exact case and no quotes;
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
LIB_API void debug_filter_add(const char* tag);

/**
 * in future, discard output with the given tag.
 * no effect if not currently added.
 **/
LIB_API void debug_filter_remove(const char* tag);

/**
 * clear all filter state; equivalent to debug_filter_remove for
 * each tag that was debug_filter_add-ed.
 **/
LIB_API void debug_filter_clear();

/**
 * indicate if the given text would be printed.
 * useful for a series of debug_printfs - avoids needing to add a tag to
 * each of their format strings.
 **/
LIB_API bool debug_filter_allows(const char* text);


/**
 * write to memory buffer (fast)
 * used for "last activity" reporting in the crashlog.
 *
 * @param format string and varags; see printf.
 **/
LIB_API void debug_wprintf_mem(const wchar_t* fmt, ...);

/**
 * write an error description and all logs into crashlog.txt
 * (in unicode format).
 *
 * @param text description of the error (including stack trace);
 * typically generated by debug_error_message_build.
 *
 * @return LibError; ERR::REENTERED if reentered via recursion or
 * multithreading (not allowed since an infinite loop may result).
 **/
LIB_API LibError debug_write_crashlog(const wchar_t* text);


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
 * completely eliminate the problem; replacing 0 literals with LIB_API
 * volatile variables fools VC7 but isn't guaranteed to be free of overhead.
 * we therefore just squelch the warning (unfortunately non-portable).
 * this duplicates the code from debug_assert to avoid compiler warnings about
 * constant conditions.
 **/
#define debug_warn(expr) \
STMT(\
	static u8 suppress__;\
	switch(debug_assert_failed(expr, &suppress__, __FILE__, __LINE__, __func__))\
	{\
	case ER_BREAK:\
		debug_break();\
		break;\
	default:\
		break;\
	}\
)


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
LIB_API ErrorReaction debug_assert_failed(const char* assert_expr,
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
LIB_API ErrorReaction debug_warn_err(LibError err,
	u8* suppress,
	const char* file, int line, const char* func);


/**
 * suppress (prevent from showing) the next error dialog for a
 * specific LibError.
 *
 * rationale: for edge cases in some functions, warnings are raised in
 * addition to returning an error code. self-tests deliberately trigger
 * these cases and check for the latter but shouldn't cause the former.
 * we therefore need to squelch them.
 *
 * @param err the LibError to skip. if the next error to be raised matches
 * this, it is skipped. otherwise, we raise a warning to help catch
 * erroneous usage. either way, the skip request is reset afterwards.
 *
 * note: this is thread-safe, but to prevent confusion, only one
 * concurrent skip request is allowed.
 */
LIB_API void debug_skip_next_err(LibError err);

/**
 * same as debug_skip_next_err, but for asserts.
 * note that this is implemented in terms of it, so only one assert or
 * error skip request may be active at a time.
 */
LIB_API void debug_skip_assert();


//-----------------------------------------------------------------------------
// symbol access
//-----------------------------------------------------------------------------

namespace ERR
{
	const LibError SYM_NO_STACK_FRAMES_FOUND = -100400;
	const LibError SYM_UNRETRIEVABLE_STATIC  = -100401;
	const LibError SYM_UNRETRIEVABLE         = -100402;
	const LibError SYM_TYPE_INFO_UNAVAILABLE = -100403;
	const LibError SYM_INTERNAL_ERROR        = -100404;
	const LibError SYM_UNSUPPORTED           = -100405;
	const LibError SYM_CHILD_NOT_FOUND       = -100406;
	// this limit is to prevent infinite recursion.
	const LibError SYM_NESTING_LIMIT         = -100407;
	// this limit is to prevent large symbols (e.g. arrays or linked lists)
	// from taking up all available output space.
	const LibError SYM_SINGLE_SYMBOL_LIMIT   = -100408;
}

namespace INFO
{
	// one of the dump_sym* functions decided not to output anything at
	// all (e.g. for member functions in UDTs - we don't want those).
	// therefore, skip any post-symbol formatting (e.g. ) as well.
	const LibError SYM_SUPPRESS_OUTPUT       = +100409;
}


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
 * @return LibError; INFO::OK iff any information was successfully
 * retrieved and stored.
 **/
LIB_API LibError debug_resolve_symbol(void* ptr_of_interest, char* sym_name, char* file, int* line);

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
 * @return LibError; ERR::REENTERED if reentered via recursion or
 * multithreading (not allowed since static data is used).
 **/
LIB_API LibError debug_dump_stack(wchar_t* buf, size_t max_chars, size_t skip, void* context);


//-----------------------------------------------------------------------------
// helper functions (used by implementation)
//-----------------------------------------------------------------------------

/**
 * [system-dependent] write a string to the debug channel.
 * this can be quite slow (~1 ms)! On Windows, it uses OutputDebugString
 * (entails context switch), otherwise stdout+fflush (waits for IO).
 **/
LIB_API void debug_puts(const char* text);

/**
 * return address of the Nth function on the call stack.
 *
 * @param skip number of stack frames (i.e. functions on call stack) to skip.
 * @param context platform-specific representation of execution state
 * (e.g. Win32 CONTEXT). if not NULL, tracing starts there; this is useful
 * for exceptions. otherwise, tracing starts from the current call stack.
 * @return address of Nth function
 *
 * note: this does not access debug symbols and is therefore quite fast.
 **/
LIB_API void* debug_get_nth_caller(size_t skip, void* context);

/**
 * check if a pointer appears to be totally invalid.
 *
 * this check is not authoritative (the pointer may be "valid" but incorrect)
 * but can be used to filter out obviously wrong values in a portable manner.
 *
 * @param p pointer
 * @return 1 if totally bogus, otherwise 0.
 **/
LIB_API int debug_is_pointer_bogus(const void* p);

/// does the given pointer appear to point to code?
LIB_API bool debug_is_code_ptr(void* p);

/// does the given pointer appear to point to the stack?
LIB_API bool debug_is_stack_ptr(void* p);


/**
 * inform the debugger of the current thread's name.
 *
 * (threads are easier to keep apart when they are identified by
 * name rather than TID.)
 **/
LIB_API void debug_set_thread_name(const char* name);


/**
 * holds memory for an error message.
 **/
struct ErrorMessageMem
{
	// rationale:
	// - error messages with stack traces require a good deal of memory
	//   (dozens of KB). static buffers of that size are undesirable.
 	// - the heap may be corrupted, so don't use malloc. allocator.h's
	//   page_aligned_malloc (implemented via mmap) should be safe.
	// - alloca is a bit iffy (the stack may be maxed out), non-portable and
	//   complicates the code because it can't be allocated by a subroutine.
	// - this method is probably slow, but error messages aren't built often.
	//   if necessary, first try malloc and use mmap if that fails.
	void* pa_mem;
};

/**
 * free memory from the error message.
 *
 * @param ErrorMessageMem*
 **/
LIB_API void debug_error_message_free(ErrorMessageMem* emm);

/**
 * build a string describing the given error.
 *
 * this is a helper function used by debug_dump_stack and is made available
 * so that the self-test doesn't have to display the error dialog.
 *
 * @param description: general description of the problem.
 * @param fn_only filename (no path) of source file that triggered the error.
 * @param line, func: exact position of the error.
 * @param skip, context: see debug_dump_stack.
 * @param emm memory for the error message. caller should allocate
 * stack memory and set alloc_buf*; if not, there will be no
 * fallback in case heap alloc fails. should be freed via
 * debug_error_message_free when no longer needed.
 **/
LIB_API const wchar_t* debug_error_message_build(
	const wchar_t* description,
	const char* fn_only, int line, const char* func,
	size_t skip, void* context,
	ErrorMessageMem* emm);

#endif	// #ifndef INCLUDED_DEBUG
