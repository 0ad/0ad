// platform-independent debug code
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

#include "precompiled.h"

#include <stdarg.h>
#include <string.h>

#include "lib.h"
#include "debug.h"
#include "debug_stl.h"
#include "posix.h"
// some functions here are called from within mmgr; disable its hooks
// so that our allocations don't cause infinite recursion.
#include "nommgr.h"
#include "self_test.h"
#include "app_hooks.h"


// needed when writing crashlog
static const size_t LOG_CHARS = 16384;
wchar_t debug_log[LOG_CHARS];
wchar_t* debug_log_pos = debug_log;

// write to memory buffer (fast)
void debug_wprintf_mem(const wchar_t* fmt, ...)
{
	const ssize_t chars_left = (ssize_t)LOG_CHARS - (debug_log_pos-debug_log);
	debug_assert(chars_left >= 0);

	// potentially not enough room for the new string; throw away the
	// older half of the log. we still protect against overflow below.
	if(chars_left < 512)
	{
		const size_t copy_size = sizeof(wchar_t) * LOG_CHARS/2;
		wchar_t* const middle = &debug_log[LOG_CHARS/2];
		memcpy2(debug_log, middle, copy_size);
		memset(middle, 0, copy_size);
		debug_log_pos -= LOG_CHARS/2;	// don't assign middle (may leave gap)
	}

	// write into buffer (in-place)
	va_list args;
	va_start(args, fmt);
	int len = vswprintf(debug_log_pos, chars_left-2, fmt, args);
	va_end(args);
	if(len < 0)
	{
		debug_warn("vswprintf failed");
		return;
	}
	debug_log_pos += len+2;
	wcscpy(debug_log_pos-2, L"\r\n");	// safe
}



// need to shoehorn printf-style variable params into
// the OutputDebugString call.
// - don't want to split into multiple calls - would add newlines to output.
// - fixing Win32 _vsnprintf to return # characters that would be written,
//   as required by C99, looks difficult and unnecessary. if any other code
//   needs that, implement GNU vasprintf.
// - fixed size buffers aren't nice, but much simpler than vasprintf-style
//   allocate+expand_until_it_fits. these calls are for quick debug output,
//   not loads of data, anyway.

// max # characters (including \0) output by debug_(w)printf in one call.
static const int MAX_CHARS = 512;


// rationale: static data instead of std::set to allow setting at any time.
// we store FNV hash of tag strings for fast comparison; collisions are
// extremely unlikely and can only result in displaying more/less text.
static const uint MAX_TAGS = 20;
static u32 tags[MAX_TAGS];
static uint num_tags;

void debug_filter_add(const char* tag)
{
	const u32 hash = fnv_hash(tag);

	// make sure it isn't already in the list
	for(uint i = 0; i < MAX_TAGS; i++)
		if(tags[i] == hash)
			return;

	// too many already?
	if(num_tags == MAX_TAGS)
	{
		debug_warn("increase MAX_TAGS");
		return;
	}

	tags[num_tags++] = hash;
}

void debug_filter_remove(const char* tag)
{
	const u32 hash = fnv_hash(tag);

	for(uint i = 0; i < MAX_TAGS; i++)
		// found it
		if(tags[i] == hash)
		{
			// replace with last element (avoid holes)
			tags[i] = tags[MAX_TAGS-1];
			num_tags--;

			// can only happen once, so we're done.
			return;
		}
}

void debug_filter_clear()
{
	for(uint i = 0; i < MAX_TAGS; i++)
		tags[i] = 0;
}

static bool filter_allows(const char* text)
{
	uint i;
	for(i = 0; ; i++)
	{
		// no | found => no tag => should always be displayed
		if(text[i] == ' ' || text[i] == '\0')
			return true;
		if(text[i] == '|' && i != 0)
			break;
	}

	const u32 hash = fnv_hash(text, i);

	// check if entry allowing this tag is found
	for(i = 0; i < MAX_TAGS; i++)
		if(tags[i] == hash)
			return true;

	return false;
}


void debug_printf(const char* fmt, ...)
{
	char buf[MAX_CHARS]; buf[ARRAY_SIZE(buf)-1] = '\0';

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, MAX_CHARS-1, fmt, ap);
	va_end(ap);

	if(filter_allows(buf))
		debug_puts(buf);
}

void debug_wprintf(const wchar_t* fmt, ...)
{
	wchar_t wcs_buf[MAX_CHARS]; wcs_buf[ARRAY_SIZE(wcs_buf)-1] = '\0';

	va_list ap;
	va_start(ap, fmt);
	vswprintf(wcs_buf, MAX_CHARS-1, fmt, ap);
	va_end(ap);

	// convert wchar_t to UTF-8.
	//
	// rationale: according to fwide(3) and assorted manpage, FILEs are in
	// single character or in wide character mode. When a FILE is in
	// single character mode, wide character writes will fail, and no
	// conversion is done automatically. Thus the manual conversion.
	//
	// it's done here (instead of in OS-specific debug_putws) because
	// filter_allow requires the conversion also.
	//
	// jw: MSDN wcstombs dox say 2 bytes per wchar is enough.
	// not sure about this; to be on the safe side, we check for overflow.
	const size_t MAX_BYTES = MAX_CHARS*2;
	char mbs_buf[MAX_BYTES]; mbs_buf[MAX_BYTES-1] = '\0';
	size_t bytes_written = wcstombs(mbs_buf, wcs_buf, MAX_BYTES);
	// .. error
	if(bytes_written == (size_t)-1)
		debug_warn("invalid wcs character encountered");
	// .. exact fit, make sure it's 0-terminated
	if(bytes_written == MAX_BYTES)
		mbs_buf[MAX_BYTES-1] = '\0';
	// .. paranoia: overflow is impossible
	debug_assert(bytes_written <= MAX_BYTES);

	if(filter_allows(mbs_buf))
		debug_puts(mbs_buf);
}


//-----------------------------------------------------------------------------

LibError debug_write_crashlog(const wchar_t* text)
{
	// note: we go through some gyrations here (strcpy+strcat) to avoid
	// dependency on file code (vfs_path_append).
	char N_path[PATH_MAX];
	strcpy_s(N_path, ARRAY_SIZE(N_path), ah_get_log_dir());
	strcat_s(N_path, ARRAY_SIZE(N_path), "crashlog.txt");
	FILE* f = fopen(N_path, "w");
	if(!f)
	{
		DISPLAY_ERROR(L"debug_write_crashlog: unable to open file");
		return ERR_FILE_ACCESS;
	}

	fputwc(0xfeff, f);	// BOM
	fwprintf(f, L"%ls\n", text);
	fwprintf(f, L"\n\n====================================\n\n");

	// allow user to bundle whatever information they want
	ah_bundle_logs(f);

	fwprintf(f, L"Last known activity:\n\n %ls\n", debug_log);

	fclose(f);
	return ERR_OK;
}



//////////////////////////////////////////////////////////////////////////////
//
// storage for and construction of strings describing a symbol
//
//////////////////////////////////////////////////////////////////////////////

// tightly pack strings within one large buffer. we never need to free them,
// since the program structure / addresses can never change.
static const size_t STRING_BUF_SIZE = 64*KiB;
static char* string_buf;
static char* string_buf_pos;

static const char* symbol_string_build(void* symbol, const char* name, const char* file, int line)
{
	// maximum bytes allowed per string (arbitrary).
	// needed to prevent possible overflows.
	const size_t STRING_MAX = 1000;

	if(!string_buf)
	{
		string_buf = (char*)malloc(STRING_BUF_SIZE);
		if(!string_buf)
		{
			debug_warn("failed to allocate string_buf");
			return 0;
		}
		string_buf_pos = string_buf;
	}

	// make sure there's enough space for a new string
	char* string = string_buf_pos;
	if(string + STRING_MAX >= string_buf + STRING_BUF_SIZE)
	{
		debug_warn("increase STRING_BUF_SIZE");
		return 0;
	}

	// user didn't know name/file/line. attempt to resolve from debug info.
	char name_buf[DBG_SYMBOL_LEN];
	char file_buf[DBG_FILE_LEN];
	if(!name || !file || !line)
	{
		int line_buf;
		(void)debug_resolve_symbol(symbol, name_buf, file_buf, &line_buf);

		// only override the original parameters if value is meaningful;
		// otherwise, stick with what we got, even if 0.
		// (obviates test of return value; correctly handles partial failure).
		if(name_buf[0])
			name = name_buf;
		if(file_buf[0])
			file = file_buf;
		if(line_buf)
			line = line_buf;
	}

	// file and line are available: write them
	int len;
	if(file && line)
	{
		// strip path from filename (long and irrelevant)
		const char* filename_only = file;
		const char* slash = strrchr(file, DIR_SEP);
		if(slash)
			filename_only = slash+1;

		len = snprintf(string, STRING_MAX-1, "%s:%05d ", filename_only, line);
	}
	// only address is known
	else
		len = snprintf(string, STRING_MAX-1, "%p ", symbol);

	// append symbol name
	if(name)
	{
		snprintf(string+len, STRING_MAX-1-len, "%s", name);
		stl_simplify_name(string+len);
	}

	return string;
}


//////////////////////////////////////////////////////////////////////////////
//
// cache, mapping symbol address to its description string.
//
//////////////////////////////////////////////////////////////////////////////

// note: we don't want to allocate a new string for every symbol -
// that would waste lots of memory. instead, when a new address is first
// encountered, allocate a string describing it, and store for later use.

// hash table entry; valid iff symbol != 0. the string pointer must remain
// valid until the cache is shut down.
struct Symbol
{
	void* symbol;
	const char* string;
};

static const uint MAX_SYMBOLS = 2048;
static Symbol* symbols;
static uint total_symbols;


static uint hash_jumps;

// strip off lower 2 bits, since it's unlikely that 2 symbols are
// within 4 bytes of one another.
static uint hash(void* symbol)
{
	const uintptr_t address = (uintptr_t)symbol;
	return (uint)( (address >> 2) % MAX_SYMBOLS );
}


// algorithm: hash lookup with linear probing.
static const char* symbol_string_from_cache(void* symbol)
{
	// hash table not initialized yet, nothing to find
	if(!symbols)
		return 0;

	uint idx = hash(symbol);
	for(;;)
	{
		Symbol* c = &symbols[idx];

		// not in table
		if(!c->symbol)
			return 0;
		// found
		if(c->symbol == symbol)
			return c->string;

		idx = (idx+1) % MAX_SYMBOLS;
	}
}


// associate <string> (must remain valid) with <symbol>, for
// later calls to symbol_string_from_cache.
static void symbol_string_add_to_cache(const char* string, void* symbol)
{
	if(!symbols)
	{
		// note: must be zeroed to set each Symbol to "invalid"
		symbols = (Symbol*)calloc(MAX_SYMBOLS, sizeof(Symbol));
		if(!symbols)
			debug_warn("failed to allocate symbols");
	}

	// hash table is completely full (guard against infinite loop below).
	// if this happens, the string won't be cached - nothing serious.
	if(total_symbols >= MAX_SYMBOLS)
	{
		debug_warn("increase MAX_SYMBOLS");
		return;
	}
	total_symbols++;

	// find Symbol slot in hash table
	Symbol* c;
	uint idx = hash(symbol);
	for(;;)
	{
		c = &symbols[idx];

		// found an empty slot
		if(!c->symbol)
			break;

		idx = (idx+1) % MAX_SYMBOLS;
		hash_jumps++;
	}

	// commit Symbol information
	c->symbol  = symbol;
	c->string = string;

	string_buf_pos += strlen(string)+1;
}




const char* debug_get_symbol_string(void* symbol, const char* name, const char* file, int line)
{
	// return it if already in cache
	const char* string = symbol_string_from_cache(symbol);
	if(string)
		return string;

	// try to build a new string
	string = symbol_string_build(symbol, name, file, line);
	if(!string)
		return 0;

	symbol_string_add_to_cache(string, symbol);

	return string;
}


//-----------------------------------------------------------------------------
// output
//-----------------------------------------------------------------------------

// translates and displays the given strings in a dialog.
// this is typically only used when debug_display_error has failed or
// is unavailable because that function is much more capable.
// implemented via sys_display_msgw; see documentation there.
void debug_display_msgw(const wchar_t* caption, const wchar_t* msg)
{
	sys_display_msgw(ah_translate(caption), ah_translate(msg));
}


// display the error dialog. shows <description> along with a stack trace.
// context and skip are as with debug_dump_stack.
// flags: see DisplayErrorFlags. file and line indicate where the error
// occurred and are typically passed as __FILE__, __LINE__.
ErrorReaction debug_display_error(const wchar_t* description,
	int flags, uint skip, void* context, const char* file, int line)
{
	if(!file || file[0] == '\0')
		file = "unknown";
	if(line <= 0)
		line = 0;

	// translate
	description = ah_translate(description);

	// display in output window; double-click will navigate to error location.
	const char* slash = strrchr(file, DIR_SEP);
	const char* filename = slash? slash+1 : file;
	debug_wprintf(L"%hs(%d): %ls\n", filename, line, description);

	// allocate memory for the stack trace. this needs to be quite large,
	// so preallocating is undesirable. it must work even if the heap is
	// corrupted (since that's an error we might want to display), so
	// we cannot rely on the heap alloc alone. what we do is try malloc,
	// fall back to alloca if it failed, and give up after that.
	wchar_t* text = 0;
	size_t max_chars = 256*KiB;
	// .. try allocating from heap
	void* heap_mem = malloc(max_chars*sizeof(wchar_t));
	text = (wchar_t*)heap_mem;
	// .. heap alloc failed; try allocating from stack
	if(!text)
	{
		max_chars = 128*KiB;	// (stack limit is usually 1 MiB)
		text = (wchar_t*)alloca(max_chars*sizeof(wchar_t));
	}

	// alloc succeeded; proceed
	if(text)
	{
		static const wchar_t fmt[] = L"%ls\r\n\r\nCall stack:\r\n\r\n";
		int len = swprintf(text, max_chars, fmt, description);
		// paranoia - only dump stack if this string output succeeded.
		if(len >= 0)
		{
			if(!context)
				skip++;	// skip this frame
			debug_dump_stack(text+len, max_chars-len, skip, context);
		}
	}
	else
		text = L"(insufficient memory to display error message)";

	debug_write_crashlog(text);
	ErrorReaction er = sys_display_error(text, flags);

	// note: debug_break-ing here to make sure the app doesn't continue
	// running is no longer necessary. debug_display_error now determines our
	// window handle and is modal.

	// handle "break" request unless the caller wants to (doing so here
	// instead of within the dlgproc yields a correct call stack)
	if(er == ER_BREAK && !(flags & DE_MANUAL_BREAK))
	{
		debug_break();
		er = ER_CONTINUE;
	}

	free(heap_mem);	// no-op if not allocated from heap
		// after debug_break to ease debugging, but before exit to avoid leak.

	// exit requested. do so here to disburden callers.
	if(er == ER_EXIT)
	{
		// disable memory-leak reporting to avoid a flood of warnings
		// (lots of stuff will leak since we exit abnormally).
		debug_heap_enable(DEBUG_HEAP_NONE);
#if CONFIG_USE_MMGR
		mmgr_set_options(0);
#endif

		exit(EXIT_FAILURE);
	}

	return er;
}


// notify the user that an assertion failed; displays a stack trace with
// local variables.
ErrorReaction debug_assert_failed(const char* expr,
	const char* file, int line, const char* func)
{
	// for edge cases in some functions, warnings (=asserts) are raised in
	// addition to returning an error code. self-tests deliberately trigger
	// these cases and check for the latter but shouldn't cause the former.
	// we therefore squelch them here.
	// (note: don't do so in lib.h's CHECK_ERR or debug_assert to reduce
	// compile-time dependency on self_test.h)
	if(self_test_active)
		return ER_CONTINUE;

	// __FILE__ evaluates to the full path (albeit without drive letter)
	// which is rather long. we only display the base name for clarity.
	const char* slash = strrchr(file, DIR_SEP);
	const char* base_name = slash? slash+1 : file;

	uint skip = 1; void* context = 0;
	wchar_t buf[400];
	swprintf(buf, ARRAY_SIZE(buf), L"Assertion failed at %hs:%d (%hs): \"%hs\"", base_name, line, func, expr);
	return debug_display_error(buf, DE_ALLOW_SUPPRESS|DE_MANUAL_BREAK, skip,context, base_name,line);
}


ErrorReaction debug_warn_err(LibError err,
	const char* file, int line, const char* func)
{
	// for edge cases in some functions, warnings (=asserts) are raised in
	// addition to returning an error code. self-tests deliberately trigger
	// these cases and check for the latter but shouldn't cause the former.
	// we therefore squelch them here.
	// (note: don't do so in lib.h's CHECK_ERR or debug_assert to reduce
	// compile-time dependency on self_test.h)
	if(self_test_active)
		return ER_CONTINUE;

	// __FILE__ evaluates to the full path (albeit without drive letter)
	// which is rather long. we only display the base name for clarity.
	const char* slash = strrchr(file, DIR_SEP);
	const char* base_name = slash? slash+1 : file;

	uint skip = 1; void* context = 0;
	wchar_t buf[400];
	char err_buf[200]; error_description_r(err, err_buf, ARRAY_SIZE(err_buf));
	swprintf(buf, ARRAY_SIZE(buf), L"Function call failed at %hs:%d (%hs): return value was %d (%hs)", base_name, line, func, err, err_buf);
	return debug_display_error(buf, DE_ALLOW_SUPPRESS|DE_MANUAL_BREAK, skip,context, base_name,line);
}


//-----------------------------------------------------------------------------
// thread naming
//-----------------------------------------------------------------------------

// when debugging multithreading problems, logging the currently running
// thread is helpful; a user-specified name is easier to remember than just
// the thread handle. to that end, we provide a robust TLS mechanism that is
// much safer than the previous method of hijacking TIB.pvArbitrary.
//
// note: on Win9x thread "IDs" are pointers to the TIB xor-ed with an
// obfuscation value calculated at boot-time.
//
// __declspec(thread) et al. are now available on VC and newer GCC but we
// implement TLS manually (via pthread_setspecific) to ensure compatibility.

static pthread_key_t tls_key;
static pthread_once_t tls_once = PTHREAD_ONCE_INIT;


// provided for completeness and to avoid displaying bogus resource leaks.
static void tls_shutdown()
{
	WARN_ERR(pthread_key_delete(tls_key));
	tls_key = 0;
}


// (called via pthread_once from debug_set_thread_name)
static void tls_init()
{
	WARN_ERR(pthread_key_create(&tls_key, 0));	// no dtor

	// note: do not use atexit; this may be called before _cinit.
}


// set the current thread's name; it will be returned by subsequent calls to
// debug_get_thread_name.
//
// the string pointed to by <name> MUST remain valid throughout the
// entire program; best to pass a string literal. allocating a copy
// would be quite a bit more work due to cleanup issues.
//
// if supported on this platform, the debugger is notified of the new name;
// it will be displayed there instead of just the handle.
void debug_set_thread_name(const char* name)
{
	WARN_ERR(pthread_once(&tls_once, tls_init));

	WARN_ERR(pthread_setspecific(tls_key, name));

#if OS_WIN
	wdbg_set_thread_name(name);
#endif
}


// return the pointer assigned by debug_set_thread_name or 0 if
// that hasn't been done yet for this thread.
const char* debug_get_thread_name()
{
	return (const char*)pthread_getspecific(tls_key);
}




void debug_shutdown()
{
	tls_shutdown();
}
