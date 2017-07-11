/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// Note: This used to use BFD to get more useful debugging information.
// (See SVN r8270 if you want that code.)
// That requires binutils at runtime, which hits some bugs and library versioning
// issues on some Linux distros.
// The debugging info reported by this code with BFD wasn't especially useful,
// and it's easy enough to get people to run in gdb if we want a proper backtrace.
// So we now go with the simple approach of not using BFD.

#include "precompiled.h"

#include "lib/sysdep/sysdep.h"
#include "lib/debug.h"

#if OS_ANDROID

// Android NDK doesn't support backtrace()
// TODO: use unwind.h or similar?

void* debug_GetCaller(void* UNUSED(context), const wchar_t* UNUSED(lastFuncToSkip))
{
	return NULL;
}

Status debug_DumpStack(wchar_t* UNUSED(buf), size_t UNUSED(max_chars), void* UNUSED(context), const wchar_t* UNUSED(lastFuncToSkip))
{
	return ERR::NOT_SUPPORTED;
}

Status debug_ResolveSymbol(void* UNUSED(ptr_of_interest), wchar_t* UNUSED(sym_name), wchar_t* UNUSED(file), int* UNUSED(line))
{
	return ERR::NOT_SUPPORTED;
}

#else

#include <execinfo.h>

void* debug_GetCaller(void* UNUSED(context), const wchar_t* UNUSED(lastFuncToSkip))
{
	// bt[0] == this function
	// bt[1] == our caller
	// bt[2] == the first caller they are interested in
	// HACK: we currently don't support lastFuncToSkip (would require debug information),
	// instead just returning the caller of the function calling us
	void *bt[3];
	int bt_size = backtrace(bt, 3);
	if (bt_size < 3)
	    return NULL;
	return bt[2];
}

Status debug_DumpStack(wchar_t* buf, size_t max_chars, void* UNUSED(context), const wchar_t* UNUSED(lastFuncToSkip))
{
	static const size_t N_FRAMES = 16;
	void *bt[N_FRAMES];
	int bt_size = 0;
	wchar_t *bufpos = buf;
	wchar_t *bufend = buf + max_chars;

	bt_size = backtrace(bt, ARRAY_SIZE(bt));

	// Assumed max length of a single print-out
	static const size_t MAX_OUT_CHARS = 1024;

	for (size_t i = 0; (int)i < bt_size && bufpos+MAX_OUT_CHARS < bufend; i++)
	{
		wchar_t file[DEBUG_FILE_CHARS];
		wchar_t symbol[DEBUG_SYMBOL_CHARS];
		int line;
		int len;

		if (debug_ResolveSymbol(bt[i], symbol, file, &line) == 0)
		{
			if (file[0])
				len = swprintf(bufpos, MAX_OUT_CHARS, L"(%p) %ls:%d %ls\n", bt[i], file, line, symbol);
			else
				len = swprintf(bufpos, MAX_OUT_CHARS, L"(%p) %ls\n", bt[i], symbol);
		}
		else
		{
			len = swprintf(bufpos, MAX_OUT_CHARS, L"(%p)\n", bt[i]);
		}

		if (len < 0)
		{
			// MAX_OUT_CHARS exceeded, realistically this was caused by some
			// mindbogglingly long symbol name... replace the end with an
			// ellipsis and a newline
			memcpy(&bufpos[MAX_OUT_CHARS-6], L"...\n", 5*sizeof(wchar_t));
			len = MAX_OUT_CHARS;
		}

		bufpos += len;
	}

	return INFO::OK;
}

Status debug_ResolveSymbol(void* ptr_of_interest, wchar_t* sym_name, wchar_t* file, int* line)
{
	if (sym_name)
		*sym_name = 0;
	if (file)
		*file = 0;
	if (line)
		*line = 0;

	char** symbols = backtrace_symbols(&ptr_of_interest, 1);
	if (symbols)
	{
		swprintf_s(sym_name, DEBUG_SYMBOL_CHARS, L"%hs", symbols[0]);
		free(symbols);

		// (Note that this will usually return a pretty useless string,
		// because we compile with -fvisibility=hidden and there won't be
		// any exposed symbols for backtrace_symbols to report.)

		return INFO::OK;
	}
	else
	{
		return ERR::FAIL;
	}
}

#endif

void debug_SetThreadName(char const* UNUSED(name))
{
    // Currently unimplemented
}
