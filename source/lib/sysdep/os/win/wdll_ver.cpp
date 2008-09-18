/**
 * =========================================================================
 * File        : wdll_ver.cpp
 * Project     : 0 A.D.
 * Description : return DLL version information.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

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

static LibError ReadVersionString(const OsPath& modulePathname_, char* out_ver, size_t out_ver_len)
{
	WinScopedPreserveLastError s;	// GetFileVersion*, Ver*
	WinScopedDisableWow64Redirection noRedirect;

	const std::string modulePathname = modulePathname_.external_file_string();

	// determine size of and allocate memory for version information.
	DWORD unused;
	const DWORD ver_size = GetFileVersionInfoSize(modulePathname.c_str(), &unused);
	if(!ver_size)
	{
		// check if the failure is due to not finding modulePathname
		// (necessary since GetFileVersionInfoSize doesn't SetLastError)
		HMODULE hModule = LoadLibraryEx(modulePathname.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);
		FreeLibrary(hModule);
		const LibError err = hModule? ERR::_1 : ERR::_2;
		WARN_RETURN(err);
	}

	shared_ptr<u8> mem = Allocate(ver_size);
	if(!GetFileVersionInfo(modulePathname.c_str(), 0, ver_size, mem.get()))
		WARN_RETURN(ERR::_3);

	u16* lang;	// -> 16 bit language ID, 16 bit codepage
	UINT lang_len;
	const BOOL ok = VerQueryValue(mem.get(), "\\VarFileInfo\\Translation", (void**)&lang, &lang_len);
	if(!ok || !lang || lang_len != 4)
		WARN_RETURN(ERR::_4);

	char subblock[64];
	sprintf(subblock, "\\StringFileInfo\\%04X%04X\\FileVersion", lang[0], lang[1]);
	const char* in_ver;
	UINT in_ver_len;
	if(!VerQueryValue(mem.get(), subblock, (void**)&in_ver, &in_ver_len))
		WARN_RETURN(ERR::_5);

	strcpy_s(out_ver, out_ver_len, in_ver);
	return INFO::OK;
}


void wdll_ver_Append(const OsPath& pathname, std::string& list)
{
	// pathname may not have an extension (e.g. driver names from the
	// registry). note that always appending ".dll" would be incorrect
	// since some have ".sys" extension.
	OsPath modulePathname(pathname);
	if(fs::extension(modulePathname).empty())
		modulePathname = fs::change_extension(modulePathname, ".dll");
	const std::string moduleName(modulePathname.leaf());

	// read file version.
	// (note: we can ignore the return value since the default
	// text has already been set)
	char versionString[500] = "unknown";	// enclosed in () below
	(void)ReadVersionString(modulePathname, versionString, ARRAY_SIZE(versionString));

	if(!list.empty())
		list += ", ";
	
	list += moduleName;
	list += " (";
	list += versionString;
	list += ")";
}
