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

#ifndef NDEBUG
	// make sure the file exists (rules out that problem as a cause of
	// GetFileVersionInfoSize failing, since it doesn't SetLastError)
	HMODULE hModule = LoadLibraryEx(modulePathname.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);
	debug_assert(hModule != 0);
	FreeLibrary(hModule);
#endif

	// determine size of and allocate memory for version information.
	DWORD unused;
	const DWORD ver_size = GetFileVersionInfoSize(modulePathname.c_str(), &unused);
	if(!ver_size)
		WARN_RETURN(ERR::FAIL);

	shared_ptr<u8> mem = Allocate(ver_size);
	if(!GetFileVersionInfo(modulePathname.c_str(), 0, ver_size, mem.get()))
		WARN_RETURN(ERR::FAIL);

	u16* lang;	// -> 16 bit language ID, 16 bit codepage
	UINT lang_len;
	const BOOL ok = VerQueryValue(mem.get(), "\\VarFileInfo\\Translation", (void**)&lang, &lang_len);
	if(!ok || !lang || lang_len != 4)
		WARN_RETURN(ERR::FAIL);

	char subblock[64];
	sprintf(subblock, "\\StringFileInfo\\%04X%04X\\FileVersion", lang[0], lang[1]);
	const char* in_ver;
	UINT in_ver_len;
	if(!VerQueryValue(mem.get(), subblock, (void**)&in_ver, &in_ver_len))
		WARN_RETURN(ERR::FAIL);

	strcpy_s(out_ver, out_ver_len, in_ver);
	return INFO::OK;
}

//
// build a string containing DLL filename(s) and their version info.
//

static char* ver_list_buf;
static size_t ver_list_chars;
static char* ver_list_pos;

// set output buffer into which DLL names and their versions will be written.
void wdll_ver_list_init(char* buf, size_t chars)
{
	ver_list_pos = ver_list_buf = buf;
	ver_list_chars = chars;
}


// read DLL file version and append that and its name to the list.
//
// name should preferably be the complete path to DLL, to make sure
// we don't inadvertently load another one on the library search path.
// we add the .dll extension if necessary.
LibError wdll_ver_list_add(const OsPath& pathname)
{
	// not be called before wdll_ver_list_init or after failure
	if(!ver_list_pos)
		WARN_RETURN(ERR::LOGIC);

	// pathname may not have an extension (e.g. driver names from the
	// registry). note that always appending ".dll" would be incorrect
	// since some have ".sys" extension.
	OsPath modulePathname(pathname);
	if(fs::extension(modulePathname).empty())
		modulePathname = fs::change_extension(modulePathname, ".dll");

	// read file version.
	// (note: we can ignore the return value since the default
	// text has already been set)
	char versionString[500] = "unknown";	// enclosed in () below
	(void)ReadVersionString(modulePathname, versionString, ARRAY_SIZE(versionString));

	// reserve enough room for subsequent comma and "..." strings.
	const ssize_t max_chars_to_write = (ssize_t)ver_list_chars - (ver_list_pos-ver_list_buf) - 10;
		
	// not first time: prepend comma to string (room was reserved above).
	if(ver_list_pos != ver_list_buf)
		ver_list_pos += sprintf(ver_list_pos, ", ");

	// extract filename.
	const std::string moduleName(modulePathname.leaf());
	int len = snprintf(ver_list_pos, max_chars_to_write, "%s (%s)", moduleName.c_str(), versionString);
	// success
	if(len > 0)
	{
		ver_list_pos += len;
		return INFO::OK;
	}

	// didn't fit; complain
	sprintf(ver_list_pos, "...");	// (room was reserved above)
	ver_list_pos = 0;	// poison pill, prevent further calls
	WARN_RETURN(ERR::BUF_SIZE);
}
