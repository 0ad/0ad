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
 * sound card detection on Windows.
 */

#include "precompiled.h"
#include "lib/sysdep/snd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <set>

#include "lib/file/file_system.h"
#include "lib/sysdep/os/win/wdll_ver.h"
#include "lib/sysdep/os/win/wversion.h"
#include "lib/sysdep/os/win/wutil.h"
#include "lib/sysdep/os/win/wmi.h"


static bool IsOpenAlDllName(const Path::String& name)
{
	// (matches "*oal.dll" and "*OpenAL*", as with OpenAL router's search)
	return name.find(L"oal.dll") != Path::String::npos || name.find(L"OpenAL") != Path::String::npos;
}

// ensures each OpenAL DLL is only listed once (even if present in several
// directories on our search path).
typedef std::set<Path::String> StringSet;

// find all OpenAL DLLs in a dir.
// call in library search order (exe dir, then win sys dir); otherwise,
// DLLs in the executable's starting directory hide those of the
// same name in the system directory.
static void add_oal_dlls_in_dir(const OsPath& path, StringSet& dlls, VersionList& versionList)
{
	CFileInfos files;
	(void)GetDirectoryEntries(path, &files, 0);
	for(size_t i = 0; i < files.size(); i++)
	{
		const Path::String name = files[i].Name().string();
		if(!IsOpenAlDllName(name))
			continue;

		// already in StringSet (i.e. has already been wdll_ver_list_add-ed)
		std::pair<StringSet::iterator, bool> ret = dlls.insert(name);
		if(!ret.second)	// insert failed - element already there
			continue;

		wdll_ver_Append(path / name, versionList);
	}
}


//-----------------------------------------------------------------------------
// DirectSound driver version

// we've seen audio problems caused by buggy DirectSound drivers (because
// OpenAL can use them in its implementation), so their version should be
// retrieved as well. the only way I know of is to enumerate all DS devices.
//
// unfortunately this fails with Vista's DS emulation - it returns some
// GUID crap as the module name. to avoidc crashing when attempting to get
// the version info for that bogus driver path, we'll skip this code there.
// (delay-loading dsound.dll eliminates any overhead)

static OsPath directSoundDriverPath;

// store sound card name and path to DirectSound driver.
// called for each DirectSound driver, but aborts after first valid driver.
static BOOL CALLBACK DirectSoundCallback(void* guid, const wchar_t* UNUSED(description),
	const wchar_t* module, void* UNUSED(cbData))
{
	// skip first dummy entry (description == "Primary Sound Driver")
	if(guid == NULL)
		return TRUE;	// continue calling

	// note: $system\\drivers is not in LoadLibrary's search list,
	// so we have to give the full pathname.
	directSoundDriverPath = wutil_SystemPath()/"drivers" / module;

	// we assume the first "driver name" (sound card) is the one we want;
	// stick with that and stop calling.
	return FALSE;
}

static const OsPath& GetDirectSoundDriverPath()
{
#define DS_OK 0
	typedef BOOL (CALLBACK* LPDSENUMCALLBACKW)(void*, const wchar_t*, const wchar_t*, void*);
	HMODULE hDsoundDll = LoadLibraryW(L"dsound.dll");
	WUTIL_FUNC(pDirectSoundEnumerateW, HRESULT, (LPDSENUMCALLBACKW, void*));
	WUTIL_IMPORT(hDsoundDll, DirectSoundEnumerateW, pDirectSoundEnumerateW);
	if(pDirectSoundEnumerateW)
	{
		HRESULT ret = pDirectSoundEnumerateW(DirectSoundCallback, (void*)0);
		ENSURE(ret == DS_OK);
	}
	FreeLibrary(hDsoundDll);

	return directSoundDriverPath;
}

//-----------------------------------------------------------------------------

Status win_get_snd_info()
{
	WmiInstances instances;
	RETURN_STATUS_IF_ERR(wmi_GetClassInstances(L"Win32_SoundDevice", instances));
	std::set<std::wstring> names;	// get rid of duplicate "High Definition Audio Device" entries
	for(WmiInstances::iterator it = instances.begin(); it != instances.end(); ++it)
	{
		if((*it)[L"Availability"].intVal != 8)	// not offline
			names.insert(std::wstring((*it)[L"ProductName"].bstrVal));
	}
	wchar_t* pos = snd_card;
	for(std::set<std::wstring>::const_iterator it = names.begin(); it != names.end(); ++it)
	{
		const int ret = swprintf_s(pos, SND_CARD_LEN-(pos-snd_card), L"%ls; ", it->c_str());
		if(ret > 0)
			pos += ret;
	}

	// find all DLLs related to OpenAL and retrieve their versions.
	VersionList versionList;
	if(wversion_Number() < WVERSION_VISTA)
		wdll_ver_Append(GetDirectSoundDriverPath(), versionList);
	StringSet dlls;	// ensures uniqueness
	(void)add_oal_dlls_in_dir(wutil_ExecutablePath(), dlls, versionList);
	(void)add_oal_dlls_in_dir(wutil_SystemPath(), dlls, versionList);
	wcscpy_s(snd_drv_ver, SND_DRV_VER_LEN, versionList.c_str());

	return INFO::OK;
}
