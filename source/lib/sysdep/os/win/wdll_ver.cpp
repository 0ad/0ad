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

/*
 * return DLL version information.
 */

#include "precompiled.h"
#include "wdll_ver.h"

#include <stdio.h>
#include <stdlib.h>

#include "win.h"
#include "wutil.h"

#include "lib/allocators/shared_ptr.h"

#if MSC_VERSION
#pragma comment(lib, "version.lib")		// DLL version
#endif


//-----------------------------------------------------------------------------

static LibError ReadVersionString(const fs::wpath& modulePathname_, wchar_t* out_ver, size_t out_ver_len)
{
	WinScopedPreserveLastError s;	// GetFileVersion*, Ver*
	WinScopedDisableWow64Redirection noRedirect;

	const std::wstring modulePathname = modulePathname_.string();

	// determine size of and allocate memory for version information.
	DWORD unused;
	const DWORD ver_size = GetFileVersionInfoSizeW(modulePathname.c_str(), &unused);	// [bytes]
	if(!ver_size)
	{
		// check if the failure is due to not finding modulePathname
		// (necessary since GetFileVersionInfoSize doesn't SetLastError)
		HMODULE hModule = LoadLibraryExW(modulePathname.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);
		if(!hModule)
			WARN_RETURN(ERR::FAIL);	// file not found
		FreeLibrary(hModule);
		return ERR::NOT_SUPPORTED;	// NOWARN (module apparently lacks version information)
	}

	shared_ptr<u8> mem = Allocate(ver_size);
	if(!GetFileVersionInfoW(modulePathname.c_str(), 0, ver_size, mem.get()))
		WARN_RETURN(ERR::_3);

	u16* lang;	// -> 16 bit language ID, 16 bit codepage
	UINT lang_len;
	const BOOL ok = VerQueryValueW(mem.get(), L"\\VarFileInfo\\Translation", (void**)&lang, &lang_len);
	if(!ok || !lang || lang_len != 4)
		WARN_RETURN(ERR::_4);

	wchar_t subblock[64];
	swprintf_s(subblock, ARRAY_SIZE(subblock), L"\\StringFileInfo\\%04X%04X\\FileVersion", lang[0], lang[1]);
	const wchar_t* in_ver;
	UINT in_ver_len;
	if(!VerQueryValueW(mem.get(), subblock, (void**)&in_ver, &in_ver_len))
		WARN_RETURN(ERR::_5);

	wcscpy_s(out_ver, out_ver_len, in_ver);
	return INFO::OK;
}


void wdll_ver_Append(const fs::wpath& pathname, std::wstring& list)
{
	// pathname may not have an extension (e.g. driver names from the
	// registry). note that always appending ".dll" would be incorrect
	// since some have ".sys" extension.
	fs::wpath modulePathname(pathname);
	if(fs::extension(modulePathname).empty())
		modulePathname = fs::change_extension(modulePathname, L".dll");
	const std::wstring moduleName(modulePathname.leaf());

	// read file version.
	// (note: we can ignore the return value since the default
	// text has already been set)
	wchar_t versionString[500] = L"unknown";	// enclosed in () below
	(void)ReadVersionString(modulePathname, versionString, ARRAY_SIZE(versionString));

	if(!list.empty())
		list += L", ";
	
	list += moduleName;
	list += L" (";
	list += versionString;
	list += L")";
}
