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

#include <string.h>

#include "lib.h"
#include "sysdep/debug.h"
#include "nommgr.h"
	// some functions here are called from within mmgr; disable its hooks
	// so that our allocations don't cause infinite recursion.


// needed when writing crashlog
static const size_t LOG_CHARS = 16384;
wchar_t debug_log[LOG_CHARS];
wchar_t* debug_log_pos = debug_log;

// write to memory buffer (fast)
void debug_wprintf_mem(const wchar_t* fmt, ...)
{
	const ssize_t chars_left = (ssize_t)LOG_CHARS - (debug_log_pos-debug_log);
	assert2(chars_left >= 0);

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


//////////////////////////////////////////////////////////////////////////////
// storage for and construction of strings describing a symbol
//////////////////////////////////////////////////////////////////////////////

// tightly pack strings within one large buffer. we never need to free them,
// since the program structure / addresses can never change.
static const size_t STRING_BUF_SIZE = 64*KiB;
static char* string_buf;
static char* string_buf_pos;


// used in simplify_stl_name.
// TODO: check strcpy safety
#define REPLACE(what, with)\
	else if(!strncmp(src, (what), sizeof(what)-1))\
	{\
		src += sizeof(what)-1-1;/* see preincrement rationale*/\
		strcpy(dst, (with));\
		dst += sizeof(with)-1;\
	}
#define STRIP(what)\
	else if(!strncmp(src, (what), sizeof(what)-1))\
	{\
		src += sizeof(what)-1-1;/* see preincrement rationale*/\
	}

// reduce complicated STL names to human-readable form (in place).
// e.g. "std::basic_string<char, char_traits<char>, std::allocator<char> >" =>
//  "string". algorithm: strip undesired strings in one pass (fast).
// called from symbol_string_build.
//
// see http://www.bdsoft.com/tools/stlfilt.html and
// http://www.moderncppdesign.com/publications/better_template_error_messages.html
static void simplify_stl_name(char* name)
{
	// used when stripping everything inside a < > to continue until
	// the final bracket is matched (at the original nesting level).
	int nesting = 0;

	const char* src = name-1;	// preincremented; see below.
	char* dst = name;

	// for each character: (except those skipped as parts of strings)
	for(;;)
	{
		int c = *(++src);
			// preincrement rationale: src++ with no further changes would
			// require all comparisons to subtract 1. incrementing at the
			// end of a loop would require a goto, instead of continue
			// (there are several paths through the loop, for speed).
			// therefore, preincrement. when skipping strings, subtract
			// 1 from the offset (since src is advanced directlry after).

		// end of string reached - we're done.
		if(c == '\0')
		{
			*dst = '\0';
			break;
		}

		// we're stripping everything inside a < >; eat characters
		// until final bracket is matched (at the original nesting level).
		if(nesting)
		{
			if(c == '<')
				nesting++;
			else if(c == '>')
			{
				nesting--;
				assert(nesting >= 0);
			}
			continue;
		}

		// start if chain (REPLACE and STRIP use else if)
		if(0) {}
		else if(!strncmp(src, "::_Node", 7))
		{
			// add a space if not already preceded by one
			// (prevents replacing ">::_Node>" with ">>")
			if(src != name && src[-1] != ' ')
				*dst++ = ' ';
			src += 7;
		}
		REPLACE("unsigned short", "u16")
		REPLACE("unsigned int", "uint")
		REPLACE("unsigned __int64", "u64")
		STRIP(",0> ")
		// early out: all tests after this start with s, so skip them
		else if(c != 's')
		{
			*dst++ = c;
			continue;
		}
		REPLACE("std::_List_nod", "list")
		REPLACE("std::_Tree_nod", "map")
		REPLACE("std::basic_string<char,", "string<")
		REPLACE("std::basic_string<unsigned short,", "wstring<")
		STRIP("std::char_traits<char>,")
		STRIP("std::char_traits<unsigned short>,")
		STRIP("std::_Tmap_traits")
		STRIP("std::_Tset_traits")
		else if(!strncmp(src, "std::allocator<", 15))
		{
			// remove preceding comma (if present)
			if(src != name && src[-1] == ',')
				dst--;
			src += 15;
			// strip everything until trailing > is matched
			assert(nesting == 0);
			nesting = 1;
		}
		else if(!strncmp(src, "std::less<", 10))
		{
			// remove preceding comma (if present)
			if(src != name && src[-1] == ',')
				dst--;
			src += 10;
			// strip everything until trailing > is matched
			assert(nesting == 0);
			nesting = 1;
		}
		STRIP("std::")
		else
			*dst++ = c;
	}
}


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
		simplify_stl_name(string+len);
	}

	return string;
}


//////////////////////////////////////////////////////////////////////////////
// cache, mapping symbol address to its description string.
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
