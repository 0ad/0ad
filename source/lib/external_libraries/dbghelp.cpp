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

#include "precompiled.h"

#if OS_WIN

#include "lib/sysdep/sysdep.h"
#include "lib/sysdep/os/win/wutil.h"
#include "lib/external_libraries/dbghelp.h"

// define extension function pointers
extern "C"
{
#define FUNC(ret, name, params) ret (__stdcall *p##name) params;
#include "lib/external_libraries/dbghelp_funcs.h"
#undef FUNC
}

void dbghelp_ImportFunctions()
{
	// for reasons unknown, LoadLibrary first checks the Dropbox shell
	// extension's directory (instead of "The directory from which the
	// application loaded.") and then the system directory, whose
	// dbghelp.dll is too old. we therefore specify the full path
	// to our executable directory, which contains a newer dbghelp.dll.
	const OsPath pathname = sys_ExecutablePathname().Parent()/"dbghelp.dll";
	HMODULE hDbghelp = LoadLibraryW(OsString(pathname).c_str());
	ENSURE(hDbghelp);
#define FUNC(ret, name, params) p##name = (ret (__stdcall*) params)GetProcAddress(hDbghelp, #name);
#include "lib/external_libraries/dbghelp_funcs.h"
#undef FUNC

	// if this function is missing, the DLL is too old.
	ENSURE(pSymInitializeW);
}

#endif // OS_WIN
