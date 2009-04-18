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

/**
 * =========================================================================
 * File        : wsdl.cpp
 * Project     : 0 A.D.
 * Description : sound card detection on Windows.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "lib/sysdep/snd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <set>

#include "lib/path_util.h"
#include "wdll_ver.h"
#include "win.h"
#include "wutil.h"
#include "wmi.h"


static LibError IsOpenAlDllName(const std::string& name)
{
	// (matches "*oal.dll" and "*OpenAL*", as with OpenAL router's search)
	return name.find("oal.dll") != std::string::npos || name.find("OpenAL") != std::string::npos;
}

// ensures each OpenAL DLL is only listed once (even if present in several
// directories on our search path).
typedef std::set<std::string> StringSet;

// find all OpenAL DLLs in a dir.
// call in library search order (exe dir, then win sys dir); otherwise,
// DLLs in the executable's starting directory hide those of the
// same name in the system directory.
static void add_oal_dlls_in_dir(const OsPath& path, StringSet& dlls, std::string& versionList)
{
	for(OsDirectoryIterator it(path); it != OsDirectoryIterator(); ++it)
	{
		if(!fs::is_regular(it->status()))
			continue;

		const OsPath& pathname = it->path();
		const std::string& name = pathname.leaf();
		if(!IsOpenAlDllName(name))
			continue;

		// already in StringSet (i.e. has already been wdll_ver_list_add-ed)
		std::pair<StringSet::iterator, bool> ret = dlls.insert(name);
		if(!ret.second)	// insert failed - element already there
			continue;

		wdll_ver_Append(pathname, versionList);
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

static char directSoundDriverPath[MAX_PATH+1];

// store sound card name and path to DirectSound driver.
// called for each DirectSound driver, but aborts after first valid driver.
static BOOL CALLBACK DirectSoundCallback(void* guid, const char* UNUSED(description),
	const char* module, void* UNUSED(cbData))
{
	// skip first dummy entry (description == "Primary Sound Driver")
	if(guid == NULL)
		return TRUE;	// continue calling

	// note: $system\\drivers is not in LoadLibrary's search list,
	// so we have to give the full pathname.
	snprintf(directSoundDriverPath, ARRAY_SIZE(directSoundDriverPath), "%s\\drivers\\%s", win_sys_dir, module);

	// we assume the first "driver name" (sound card) is the one we want;
	// stick with that and stop calling.
	return FALSE;
}

static const char* GetDirectSoundDriverPath()
{
#define DS_OK 0
	typedef BOOL (CALLBACK* LPDSENUMCALLBACKA)(void*, const char*, const char*, void*);
	typedef HRESULT (WINAPI *PDirectSoundEnumerateA)(LPDSENUMCALLBACKA, void*);
	HMODULE hDsoundDll = LoadLibrary("dsound.dll");
	PDirectSoundEnumerateA pDirectSoundEnumerateA = (PDirectSoundEnumerateA)GetProcAddress(hDsoundDll, "DirectSoundEnumerateA");
	if(pDirectSoundEnumerateA)
	{
		HRESULT ret = pDirectSoundEnumerateA(DirectSoundCallback, (void*)0);
		debug_assert(ret == DS_OK);
	}
	FreeLibrary(hDsoundDll);

	return directSoundDriverPath;
}

//-----------------------------------------------------------------------------

LibError win_get_snd_info()
{
	WmiMap wmiMap;
	if(wmi_GetClass("Win32_SoundDevice", wmiMap) == INFO::OK)
		sprintf_s(snd_card, SND_CARD_LEN, "%ls", wmiMap[L"ProductName"].bstrVal);

	// find all DLLs related to OpenAL and retrieve their versions.
	std::string versionList;
	if(wutil_WindowsVersion() < WUTIL_VERSION_VISTA)
		wdll_ver_Append(GetDirectSoundDriverPath(), versionList);
	StringSet dlls;	// ensures uniqueness
	(void)add_oal_dlls_in_dir(win_exe_dir, dlls, versionList);
	(void)add_oal_dlls_in_dir(win_sys_dir, dlls, versionList);
	strcpy_s(snd_drv_ver, SND_DRV_VER_LEN, versionList.c_str());

	return INFO::OK;
}
