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
// file_make_full_native_path is needed when bundling game data files.
#include "lib/res/file/file.h"

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
		memcpy(debug_log, middle, copy_size);
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
		debug_warn("debug_wprintf_mem: vswprintf failed");
		return;
	}
	debug_log_pos += len+2;
	wcscpy(debug_log_pos-2, L"\r\n");	// safe
}




// convert contents of file <in_filename> from char to wchar_t and
// append to <out> file. used by debug_write_crashlog.
static void cat_atow(FILE* out, const char* in_filename)
{
	FILE* in = fopen(in_filename, "rb");
	if(!in)
	{
		fwprintf(out, L"(unavailable)");
		return;
	}

	const size_t buf_size = 1024;
	char buf[buf_size+1]; // include space for trailing '\0'

	while(!feof(in))
	{
		size_t bytes_read = fread(buf, 1, buf_size, in);
		if(!bytes_read)
			break;
		buf[bytes_read] = 0;	// 0-terminate
		fwprintf(out, L"%hs", buf);
	}

	fclose(in);
}


int debug_write_crashlog(const wchar_t* text)
{
	const wchar_t divider[] = L"\n\n====================================\n\n";
#define WRITE_DIVIDER fwprintf(f, divider);

	FILE* f = fopen("crashlog.txt", "w");
	if(!f)
		return -1;

	fputwc(0xfeff, f);	// BOM

	fwprintf(f, L"%ls\n", text);
	WRITE_DIVIDER

	// for user convenience, bundle all logs into this file:
	char N_path[PATH_MAX];
	fwprintf(f, L"System info:\n\n");
	(void)file_make_full_native_path("../logs/system_info.txt", N_path);
	cat_atow(f, N_path);
	WRITE_DIVIDER
	fwprintf(f, L"Main log:\n\n");
	(void)file_make_full_native_path("../logs/mainlog.html", N_path);
	cat_atow(f, N_path);
	WRITE_DIVIDER

	fwprintf(f, L"Last known activity:\n\n %ls\n", debug_log);

	fclose(f);
	return 0;
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

ErrorReaction display_error(const wchar_t* description, int flags,
	uint skip, void* context, const char* file, int line)
{
	if(!file || file[0] == '\0')
		file = "unknown";
	if(line <= 0)
		line = 0;

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
	ErrorReaction er = display_error_impl(text, flags);

	// note: debug_break-ing here to make sure the app doesn't continue
	// running is no longer necessary. display_error now determines our
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
ErrorReaction debug_assert_failed(const char* file, int line, const char* expr)
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
	wchar_t buf[200];
	swprintf(buf, ARRAY_SIZE(buf), L"Assertion failed in %hs, line %d: \"%hs\"", base_name, line, expr);
	return display_error(buf, DE_ALLOW_SUPPRESS|DE_MANUAL_BREAK, skip, context, base_name, line);
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
