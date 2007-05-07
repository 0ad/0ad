/**
 * =========================================================================
 * File        : dll_ver.cpp
 * Project     : 0 A.D.
 * Description : return DLL version information.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "dll_ver.h"

#include <stdio.h>
#include <stdlib.h>

#include "lib/path_util.h"
#include "win_internal.h"
#include "wutil.h"

#if MSC_VERSION
#pragma comment(lib, "version.lib")		// DLL version
#endif

// helper function that does all the work; caller wraps it and takes care of
// undoing various operations if we fail midway.
static LibError get_ver_impl(const char* module_path, char* out_ver, size_t out_ver_len, void*& mem)
{
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
	WIN_SAVE_LAST_ERROR;	// GetFileVersion*, Ver*

	// WinXP x64 'helpfully' redirects all 32-bit apps' accesses of
	// %windir\\system32\\drivers to %windir\\system32\\drivers\\SysWOW64.
	// that's bad, because the actual drivers are not in the subdirectory.
	// to work around this, we disable the redirection over the duration of
	// this call. if not on a 64-bit OS (i.e. these entry points aren't
	// found), no action need be taken.
	HMODULE hKernel32Dll = LoadLibrary("kernel32.dll");
	BOOL (WINAPI *pWow64DisableWow64FsRedirection)(PVOID*) = 0;
	BOOL (WINAPI *pWow64RevertWow64FsRedirection)(PVOID) = 0;
	*(void**)&pWow64DisableWow64FsRedirection = GetProcAddress(hKernel32Dll, "Wow64DisableWow64FsRedirection");
	*(void**)&pWow64RevertWow64FsRedirection  = GetProcAddress(hKernel32Dll, "Wow64RevertWow64FsRedirection");
	PVOID old_value = 0;
	if(pWow64DisableWow64FsRedirection)
	{
		BOOL ok = pWow64DisableWow64FsRedirection(&old_value);
		WARN_IF_FALSE(ok);
	}

	void* mem = NULL;
	LibError ret = get_ver_impl(module_path, out_ver, out_ver_len, mem);
	free(mem);

	if(pWow64DisableWow64FsRedirection)
	{
		BOOL ok = pWow64RevertWow64FsRedirection(old_value);
		WARN_IF_FALSE(ok);
	}
	FreeLibrary(hKernel32Dll);

	WIN_RESTORE_LAST_ERROR;

	if(ret != INFO::OK)
		out_ver[0] = '\0';
	return ret;
}

//
// build a string containing DLL filename(s) and their version info.
//

static char* dll_list_buf;
static size_t dll_list_chars;
static char* dll_list_pos;

// set output buffer into which DLL names and their versions will be written.
void dll_list_init(char* buf, size_t chars)
{
	dll_list_pos = dll_list_buf = buf;
	dll_list_chars = chars;
}


// read DLL file version and append that and its name to the list.
//
// name should preferably be the complete path to DLL, to make sure
// we don't inadvertently load another one on the library search path.
// we add the .dll extension if necessary.
LibError dll_list_add(const char* name)
{
	// not be called before dll_list_init or after failure
	if(!dll_list_pos)
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
	char dll_ver[128] = "unknown";	// enclosed in () below
	(void)get_ver(dll_name, dll_ver, sizeof(dll_ver));
		// if this fails, default is already set and we don't want to abort.

	const ssize_t max_chars_to_write = (ssize_t)dll_list_chars - (dll_list_pos-dll_list_buf) - 10;
		// reserves enough room for subsequent comma and "..." strings.

	// not first time: prepend comma to string (room was reserved above).
	if(dll_list_pos != dll_list_buf)
		dll_list_pos += sprintf(dll_list_pos, ", ");

	// extract filename.
	const char* dll_fn = path_name_only(dll_name);

	int len = snprintf(dll_list_pos, max_chars_to_write, "%s (%s)", dll_fn, dll_ver);
	// success
	if(len > 0)
	{
		dll_list_pos += len;
		return INFO::OK;
	}

	// didn't fit; complain
	sprintf(dll_list_pos, "...");	// (room was reserved above)
	dll_list_pos = 0;	// poison pill, prevent further calls
	WARN_RETURN(ERR::BUF_SIZE);
}
