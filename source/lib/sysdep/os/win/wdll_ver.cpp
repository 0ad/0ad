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
