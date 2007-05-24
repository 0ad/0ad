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
#include "lib/res/file/file.h"
#include "dll_ver.h"	// dll_list_*
#include "win.h"
#include "wutil.h"


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
EXTERN_C __declspec(dllimport) HRESULT WINAPI DirectSoundEnumerateA(LPDSENUMCALLBACKA, void*);
#endif

#if MSC_VERSION
#pragma comment(lib, "dsound.lib")
#endif


// note: OpenAL alGetString is worthless: it only returns OpenAL API version
// and renderer (e.g. "Software").

// indicate if this directory entry is an OpenAL DLL.
// (matches "*oal.dll" and "*OpenAL*", as with OpenAL router's search)
static LibError IsOpenAlDll(const DirEnt* ent)
{
	// not a file
	if(!DIRENT_IS_DIR(ent))
		return false;

	// name doesn't match
	const size_t len = strlen(ent->name);
	const bool oal = len >= 7 && !strcasecmp(ent->name+len-7, "oal.dll");
	const bool openal = strstr(ent->name, "OpenAL") != 0;
	if(!oal && !openal)
		return false;

	return true;
}

// ensures each OpenAL DLL is only listed once (even if present in several
// directories on our search path).
typedef std::set<std::string> StringSet;

// find all OpenAL DLLs in a dir (via file_enum and IsOpenAlDll).
// call in library search order (exe dir, then win sys dir); otherwise,
// DLLs in the executable's starting directory hide those of the
// same name in the system directory.
//
// <dir>: no trailing.
static LibError add_oal_dlls_in_dir(const char* dir, StringSet* dlls)
{
	// note: dll_list_add requires the full DLL path but DirEnt only gives us
	// the name. for efficiency, we append this via PathPackage.
	PathPackage pp;
	RETURN_ERR(path_package_set_dir(&pp, dir));

	DirIterator d;
	RETURN_ERR(dir_open(dir, &d));

	for(;;)	// instead of while to avoid warning
	{
		DirEnt ent;
		LibError err = dir_next_ent(&d, &ent);
		if(err != INFO::OK)
			break;

		if(!IsOpenAlDll(&ent))
			continue;

		// already in StringSet (i.e. has already been dll_list_add-ed)
		std::pair<StringSet::iterator, bool> ret = dlls->insert(ent.name);
		if(!ret.second)	// insert failed - element already there
			continue;

		(void)path_package_append_file(&pp, ent.name);
		(void)dll_list_add(pp.path);
	}

	(void)dir_close(&d);
	return INFO::OK;
}


// DS3D driver path; filled by ds_enum, used by win_get_snd_info.
// side effect: remains zeroed if there's no sound card installed.
static char ds_drv_path[MAX_PATH+1];

// store sound card name and path to DirectSound driver.
// called for each DirectSound driver, but aborts after first valid driver.
static BOOL CALLBACK ds_enum(void* guid, const char* description,
	const char* module, void* UNUSED(ctx))
{
	// skip first dummy entry (description == "Primary Sound Driver")
	if(guid == NULL)
		return TRUE;	// continue calling

	strcpy_s(snd_card, SND_CARD_LEN, description);

	// note: $system\\drivers is not in LoadLibrary's search list,
	// so we have to give the full pathname.
	snprintf(ds_drv_path, ARRAY_SIZE(ds_drv_path), "%s\\drivers\\%s", win_sys_dir, module);

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
		return INFO::OK;
	}

	// find all DLLs related to OpenAL, retrieve their versions,
	// and store in snd_drv_ver string.
	dll_list_init(snd_drv_ver, SND_DRV_VER_LEN);
	(void)dll_list_add(ds_drv_path);
	StringSet dlls;	// ensures uniqueness
	(void)add_oal_dlls_in_dir(win_exe_dir, &dlls);
	(void)add_oal_dlls_in_dir(win_sys_dir, &dlls);
	return INFO::OK;
}
