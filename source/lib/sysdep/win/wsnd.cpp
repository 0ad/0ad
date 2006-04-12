/**
 * =========================================================================
 * File        : wsdl.cpp
 * Project     : 0 A.D.
 * Description : sound card detection on Windows.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2004 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "precompiled.h"

#include <string>
#include <set>

#include <stdio.h>
#include <stdlib.h>

#include "lib/sysdep/snd.h"
#include "dll_ver.h"	// dll_list_*
#include "win_internal.h"
#include "lib/res/file/file.h"

// DirectSound header
// HACK: workaround for "subwtype.h not found" errors on VC6/7 hybrid.
// (subwtype.h <- d3dtypes.h <- dsound.h)
// since we're only using one DS function (DirectSoundEnumerate),
// we forward-declare it rather than fix/mess with the system headers.
#if 0
// mmsystem.h is necessary for dsound.h; we cut out unnecessary junk
# define MMNODRV         // Installable driver support
# define MMNOSOUND       // Sound support
//# define MMNOWAVE        // Waveform support
# define MMNOMIDI        // MIDI support
# define MMNOAUX         // Auxiliary audio support
# define MMNOMIXER       // Mixer support
# define MMNOTIMER       // Timer support
# define MMNOJOY         // Joystick support
# define MMNOMCI         // MCI support
# define MMNOMMIO        // Multimedia file I/O support
# define MMNOMMSYSTEM    // General MMSYSTEM functions
# include <MMSystem.h>
# define DIRECTSOUND_VERSION 0x0500
# include <dsound.h>
#else
# define DS_OK 0
typedef BOOL (CALLBACK* LPDSENUMCALLBACKA)(void*, const char*, const char*, void*);
extern "C" __declspec(dllimport) HRESULT WINAPI DirectSoundEnumerateA(LPDSENUMCALLBACKA, void*);
#endif

#if MSC_VERSION
#pragma comment(lib, "dsound.lib")
#endif


// note: OpenAL alGetString is worthless: it only returns OpenAL API version
// and renderer (e.g. "Software").

// ensures each OpenAL DLL is only listed once (even if present in several
// directories on our search path).
typedef std::set<std::string> StringSet;

// if this directory entry is an OpenAL DLL, add it to our list.
// (matches "*oal.dll" and "*OpenAL*", as with OpenAL router's search)
// called by add_oal_dlls_in_dir.
//
// note: we need the full DLL path for dll_list_add but DirEnt only gives us
// the name. for efficiency, we append this in a PathPackage allocated by
// add_oal_dlls_in_dir.
static LibError add_if_oal_dll(const DirEnt* ent, PathPackage* pp, StringSet* dlls)
{
	const char* fn = ent->name;

	// skip non-files.
	if(!DIRENT_IS_DIR(ent))
		return ERR_OK;

	// skip if not an OpenAL DLL.
	const size_t len = strlen(fn);
	const bool oal = len >= 7 && !stricmp(fn+len-7, "oal.dll");
	const bool openal = strstr(fn, "OpenAL") != 0;
	if(!oal && !openal)
		return ERR_OK;

	// skip if already in StringSet (i.e. has already been dll_list_add-ed)
	std::pair<StringSet::iterator, bool> ret = dlls->insert(fn);
	if(!ret.second)	// insert failed - element already there
		return ERR_OK;

	RETURN_ERR(pp_append_file(pp, fn));
	return dll_list_add(pp->path);
}


// find all OpenAL DLLs in a dir (via file_enum and add_if_oal_dll).
// call in library search order (exe dir, then win sys dir); otherwise,
// DLLs in the executable's starting directory hide those of the
// same name in the system directory.
//
// <dir>: no trailing.
static LibError add_oal_dlls_in_dir(const char* dir, StringSet* dlls)
{
	PathPackage pp;
	RETURN_ERR(pp_set_dir(&pp, dir));

	DirIterator d;
	RETURN_ERR(dir_open(dir, &d));

	DirEnt ent;
	for(;;)	// instead of while to avoid warning
	{
		LibError err = dir_next_ent(&d, &ent);
		if(err != ERR_OK)
			break;
		(void)add_if_oal_dll(&ent, &pp, dlls);
	}

	(void)dir_close(&d);
	return ERR_OK;
}


// DS3D driver path; filled by ds_enum, used by win_get_snd_info.
// side effect: remains zeroed if there's no sound card installed.
static char ds_drv_path[MAX_PATH+1];

// store sound card name and path to DirectSound driver.
// called for each DirectSound driver, but aborts after first valid driver.
static BOOL CALLBACK ds_enum(void* UNUSED(guid), const char* description,
							 const char* module, void* UNUSED(ctx))
{
	// skip first (dummy) entry, where description == "Primary Sound Driver".
	if(module[0] == '\0')
		return TRUE;	// continue calling

	strcpy_s(snd_card, SND_CARD_LEN, description);
	snprintf(ds_drv_path, ARRAY_SIZE(ds_drv_path), "%s\\drivers\\%s", win_sys_dir, module);
	// note: this directory is not in LoadLibrary's search list,
	// so we have to give the full pathname.

	// we assume the first "driver name" (sound card) is the one we want;
	// stick with that and stop calling.
	return FALSE;
}


LibError win_get_snd_info()
{
	// get sound card name and DS driver path.
	if(DirectSoundEnumerateA((LPDSENUMCALLBACKA)ds_enum, (void*)0) != DS_OK)
		debug_warn("DirectSoundEnumerate failed");

	// there are apparently no sound card/drivers installed; so indicate.
	// (the code below would fail and not produce reasonable output)
	if(ds_drv_path[0] == '\0')
	{
		strcpy_s(snd_card, SND_CARD_LEN, "(none)");
		strcpy_s(snd_drv_ver, SND_DRV_VER_LEN, "(none)");
		return ERR_OK;
	}

	// find all DLLs related to OpenAL, retrieve their versions,
	// and store in snd_drv_ver string.
	dll_list_init(snd_drv_ver, SND_DRV_VER_LEN);
	(void)dll_list_add(ds_drv_path);
	StringSet dlls;	// ensures uniqueness
	(void)add_oal_dlls_in_dir(win_exe_dir, &dlls);
	(void)add_oal_dlls_in_dir(win_sys_dir, &dlls);
	return ERR_OK;
}
