/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"
#include "wdlfcn.h"

#include "wposix_internal.h"


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
	BOOL ok = FreeLibrary(HMODULE_from_void(handle));
	WARN_RETURN_IF_FALSE(ok);
	return 0;
}


char* dlerror()
{
	return 0;
}


void* dlopen(const char* so_name, int flags)
{
	debug_assert(!(flags & RTLD_GLOBAL));

	fs::path pathname = fs::change_extension(so_name, ".dll");
	HMODULE hModule = LoadLibrary(pathname.string().c_str());
	debug_assert(hModule);
	return void_from_HMODULE(hModule);
}


void* dlsym(void* handle, const char* sym_name)
{
	HMODULE hModule = HMODULE_from_void(handle);
	void* sym = GetProcAddress(hModule, sym_name);
	debug_assert(sym);
	return sym;
}
