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
#include "lib/sysdep/os/win/wposix/wdlfcn.h"

#include "lib/sysdep/os/win/wposix/wposix_internal.h"


static HMODULE HMODULE_from_void(void* handle)
{
	return (HMODULE)handle;
}

static void* void_from_HMODULE(HMODULE hModule)
{
	return (void*)hModule;
}


int dlclose(void* handle)
{
	WARN_IF_FALSE(FreeLibrary(HMODULE_from_void(handle)));
	return 0;
}


char* dlerror()
{
	// the obvious implementation involves sys_StatusDescription, but we
	// don't want to return a pointer to a static array because that's not
	// thread-safe. we therefore check GetLastError directly.
	switch(GetLastError())
	{
	case ERROR_MOD_NOT_FOUND:
		return "module not found";
	case ERROR_PROC_NOT_FOUND:
		return "symbol not found";
	default:
		return "unknown";
	}
}


void* dlopen(const char* so_name, int flags)
{
	ENSURE(!(flags & RTLD_GLOBAL));

	OsPath pathname = Path(so_name).ChangeExtension(L".dll");
	HMODULE hModule = LoadLibraryW(OsString(pathname).c_str());
	return void_from_HMODULE(hModule);
}


void* dlsym(void* handle, const char* sym_name)
{
	HMODULE hModule = HMODULE_from_void(handle);
	void* sym = GetProcAddress(hModule, sym_name);
	ENSURE(sym);
	return sym;
}
