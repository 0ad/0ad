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

// note: the BFD stuff *could* be used on other platforms, if we saw the
// need for it.

#include "precompiled.h"

#include "lib/sysdep/sysdep.h"
#include "lib/debug.h"

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

void debug_SetThreadName(char const* UNUSED(name))
{
	// Currently unimplemented
}
