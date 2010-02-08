/* Copyright (c) 2010 Wildfire Games
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

#include "precompiled.h"

#if OS_WIN

#include "dbghelp.h"

// define extension function pointers
extern "C"
{
#define FUNC(ret, name, params) ret (__stdcall *p##name) params;
#include "dbghelp_funcs.h"
#undef FUNC
}

void dbghelp_ImportFunctions()
{
	HMODULE hDbghelp = LoadLibraryW(L"dbghelp.dll");
	debug_assert(hDbghelp);
#define FUNC(ret, name, params) p##name = (ret (__stdcall*) params)GetProcAddress(hDbghelp, #name);
#include "dbghelp_funcs.h"
#undef FUNC
}

#endif // OS_WIN
