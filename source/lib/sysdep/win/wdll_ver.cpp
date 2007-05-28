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

#include "lib/path_util.h"
#include "win.h"
#include "wutil.h"

#if MSC_VERSION
#pragma comment(lib, "version.lib")		// DLL version
#endif


//-----------------------------------------------------------------------------

// helper function that does all the work; caller wraps it and takes care of
// undoing various operations if we fail midway.
static LibError get_ver_impl(const char* module_path, char* out_ver, size_t out_ver_len, void*& mem)
{
#ifndef NDEBUG
	// make sure the file exists (rules out that problem as a cause of
	// GetFileVersionInfoSize failing, since it doesn't SetLastError)
	HMODULE hModule = LoadLibraryEx(module_path, 0, LOAD_LIBRARY_AS_DATAFILE);
	debug_assert(hModule != 0);
	FreeLibrary(hModule);
#endif

	// determine size of and allocate memory for version information.
	DWORD unused;
	const DWORD ver_size = GetFileVersionInfoSize(module_path, &unused);
	if(!ver_size)
		WARN_RETURN(ERR::FAIL);
	mem = malloc(ver_size);
	if(!mem)
		WARN_RETURN(ERR::NO_MEM);

	if(!GetFileVersionInfo(module_path, 0, ver_size, mem))
		WARN_RETURN(ERR::FAIL);

	u16* lang;	// -> 16 bit language ID, 16 bit codepage
	uint lang_len;
	const BOOL ok = VerQueryValue(mem, "\\VarFileInfo\\Translation", (void**)&lang, &lang_len);
	if(!ok || !lang || lang_len != 4)
		WARN_RETURN(ERR::FAIL);

	char subblock[64];
	sprintf(subblock, "\\StringFileInfo\\%04X%04X\\FileVersion", lang[0], lang[1]);
	const char* in_ver;
	uint in_ver_len;
	if(!VerQueryValue(mem, subblock, (void**)&in_ver, &in_ver_len))
		WARN_RETURN(ERR::FAIL);

	strcpy_s(out_ver, out_ver_len, in_ver);
	return INFO::OK;
}

// get version information for the specified DLL.
static LibError get_ver(const char* module_path, char* out_ver, size_t out_ver_len)
{
	LibError ret;

	WIN_SAVE_LAST_ERROR;	// GetFileVersion*, Ver*
	{
		PVOID wasRedirectionEnabled;
		wutil_DisableWow64Redirection(wasRedirectionEnabled);
		{
			void* mem = NULL;
			ret = get_ver_impl(module_path, out_ver, out_ver_len, mem);
			free(mem);
		}
		wutil_RevertWow64Redirection(wasRedirectionEnabled);
	}
	WIN_RESTORE_LAST_ERROR;

	if(ret != INFO::OK)
		out_ver[0] = '\0';
	return ret;
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
LibError wdll_ver_list_add(const char* name)
{
	// not be called before wdll_ver_list_init or after failure
	if(!ver_list_pos)
		WARN_RETURN(ERR::LOGIC);

	// some driver names are stored in the registry without .dll extension.
	// if necessary, copy to new buffer and add it there.
	// note: do not change extension if present; some drivers have a
	// ".sys" extension, so always appending ".dll" is incorrect.
	char buf[MAX_PATH];
	const char* dll_name = name;
	const char* ext = path_extension(name);
	if(ext[0] == '\0')	// no extension
	{
		snprintf(buf, ARRAY_SIZE(buf), "%s.dll", name);
		dll_name = buf;
	}

	// read file version.
	char dll_ver[500] = "unknown";	// enclosed in () below
	(void)get_ver(dll_name, dll_ver, sizeof(dll_ver));
		// if this fails, default is already set and we don't want to abort.

	const ssize_t max_chars_to_write = (ssize_t)ver_list_chars - (ver_list_pos-ver_list_buf) - 10;
		// reserves enough room for subsequent comma and "..." strings.

	// not first time: prepend comma to string (room was reserved above).
	if(ver_list_pos != ver_list_buf)
		ver_list_pos += sprintf(ver_list_pos, ", ");

	// extract filename.
	const char* dll_fn = path_name_only(dll_name);

	int len = snprintf(ver_list_pos, max_chars_to_write, "%s (%s)", dll_fn, dll_ver);
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
