// Windows-specific system detect
//
// Copyright (c) 2004 Jan Wassenberg
//
// This file is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This file is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include "detect.h"
#include "lib.h"
#include "lib/res/file/file.h"	// file_enum

#include "win_internal.h"


#include <stdio.h>
#include <stdlib.h>

#include <set>
#include <string>

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


// these are all delay-loaded - they're not needed if
// system information is never queried.
#if MSC_VERSION
#pragma comment(lib, "version.lib")		// DLL version
#pragma comment(lib, "advapi32.lib")	// registry
#pragma comment(lib, "dsound.lib")		// sound card name
#endif


// EnumDisplayDevices is not available on Win95 or NT.
// try to import it manually here; return -1 if not available.
static BOOL (WINAPI *pEnumDisplayDevicesA)(LPCSTR, DWORD, LPDISPLAY_DEVICEA, DWORD);
static int import_EnumDisplayDevices()
{
	if(!pEnumDisplayDevicesA)
	{
		static HMODULE hUser32Dll = LoadLibrary("user32.dll");
		*(void**)&pEnumDisplayDevicesA = GetProcAddress(hUser32Dll, "EnumDisplayDevicesA");

		// do not free the DLL reference! if this happens to be the first
		// use of user32 (possible if it's delay-loaded or otherwise unused),
		// it would actually be unloaded. that apparently breaks the
		// Windows cursor.
		// unfortunately there's no way to get at the reference count,
		// so this resource leak is unavoidable.
	}

	return pEnumDisplayDevicesA? 0 : -1;
}


// useful for choosing a video mode.
// if we fail, outputs are unchanged (assumed initialized to defaults)
int get_cur_vmode(int* xres, int* yres, int* bpp, int* freq)
{
	// don't use EnumDisplaySettingsW - BoundsChecker reports it causes
	// a memory overrun (even if called as the very first thing, before
	// static CRT initialization).

	DEVMODEA dm;
	memset(&dm, 0, sizeof(dm));
	dm.dmSize = sizeof(dm);
	// dm.dmDriverExtra already set to 0 by memset

	if(!EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &dm))
		return -1;

	if(dm.dmFields & (DWORD)DM_PELSWIDTH && xres)
		*xres = (int)dm.dmPelsWidth;
	if(dm.dmFields & (DWORD)DM_PELSHEIGHT && yres)
		*yres = (int)dm.dmPelsHeight;
	if(dm.dmFields & (DWORD)DM_BITSPERPEL && bpp)
		*bpp  = (int)dm.dmBitsPerPel;
	if(dm.dmFields & (DWORD)DM_DISPLAYFREQUENCY && freq)
		*freq = (int)dm.dmDisplayFrequency;

	return 0;
}


// useful for determining aspect ratio.
// if we fail, outputs are unchanged (assumed initialized to defaults)
int get_monitor_size(int& width_mm, int& height_mm)
{
	// (DC for the primary monitor's entire screen)
	HDC dc = GetDC(0);
	width_mm = GetDeviceCaps(dc, HORZSIZE);
	height_mm = GetDeviceCaps(dc, VERTSIZE);
	ReleaseDC(0, dc);
	return 0;
}


//----------------------------------------------------------------------------
// support routines for getting DLL version
//----------------------------------------------------------------------------

static int get_ver(const char* module_path, char* out_ver, size_t out_ver_len)
{
	WIN_SAVE_LAST_ERROR;	// GetFileVersion*, Ver*

	// determine size of and allocate memory for version information.
	DWORD unused;
	const DWORD ver_size = GetFileVersionInfoSize(module_path, &unused);
	if(!ver_size)
		return -1;
	void* buf = malloc(ver_size);
	if(!buf)
		return ERR_NO_MEM;

	int ret = -1;	// single point of exit (for free())

	if(GetFileVersionInfo(module_path, 0, ver_size, buf))
	{
		u16* lang;	// -> 16 bit language ID, 16 bit codepage
		uint lang_len;
		const BOOL ok = VerQueryValue(buf, "\\VarFileInfo\\Translation", (void**)&lang, &lang_len);
		if(ok && lang && lang_len == 4)
		{
			char subblock[64];
			sprintf(subblock, "\\StringFileInfo\\%04X%04X\\FileVersion", lang[0], lang[1]);
			const char* in_ver;
			uint in_ver_len;
			if(VerQueryValue(buf, subblock, (void**)&in_ver, &in_ver_len))
			{
				strcpy_s(out_ver, out_ver_len, in_ver);
				ret = 0;	// success
			}
		}
	}
	free(buf);

	WIN_RESTORE_LAST_ERROR;

	return ret;
}


//
// build a string containing DLL filename(s) and their version info.
//

static char* dll_list_buf;
static size_t dll_list_chars;
static char* dll_list_pos;

static void dll_list_init(char* buf, size_t chars)
{
	dll_list_pos = dll_list_buf = buf;
	dll_list_chars = chars;
}


// read DLL file version and append that and its name to the list.
// return 0 on success or a negative error code.
//
// name should preferably be the complete path to DLL, to make sure
// we don't inadvertently load another one on the library search path.
// we add the .dll extension if necessary.
static int dll_list_add(const char* name)
{
	// make sure we're allowed to be called.
	if(!dll_list_pos)
	{
		debug_warn("dll_list_add: called before dll_list_init or after failure");
		return -1;
	}

	// some driver names are stored in the registry without .dll extension.
	// if necessary, copy to new buffer and add it there.
	// note: do not change extension if present; some drivers have a
	// ".sys" extension, so always appending ".dll" is incorrect.
	char buf[MAX_PATH];
	const char* dll_name = name;
	const char* ext = strrchr(name, '.');
	if(!ext)
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
	const char* slash = strrchr(dll_name, '\\');
	const char* dll_fn = slash? slash+1 : dll_name;

	int len = snprintf(dll_list_pos, max_chars_to_write, "%s (%s)", dll_fn, dll_ver);
	// success
	if(len > 0)
	{
		dll_list_pos += len;
		return 0;
	}

	// didn't fit; complain
	sprintf(dll_list_pos, "...");	// (room was reserved above)
	dll_list_pos = 0;	// poison pill, prevent further calls
	debug_warn("dll_list_add: not enough room");
	return ERR_BUF_SIZE;
}


//////////////////////////////////////////////////////////////////////////////
//
// graphics card / driver version
//
//////////////////////////////////////////////////////////////////////////////


static int win_get_gfx_card()
{
	// make sure EnumDisplayDevices is available (as pEnumDisplayDevicesA)
	if(import_EnumDisplayDevices() >= 0)
	{
		// loop through all display adapters and put the primary device's
		// identifier in gfx_card.
		// (we don't bother returning a list of all of them because it's
		// likely to be redundant, e.g. "Radeon" and "Radeon Secondary";
		// we're only interested in the 'main' card anyway)
		DISPLAY_DEVICEA dd = { sizeof(DISPLAY_DEVICEA) };
		for(DWORD dev_num = 0; ; dev_num++)
		{
			if(!pEnumDisplayDevicesA(0, dev_num, &dd, 0))
				break;

			// ignore mirror pseudo-devices (installed e.g. by NetMeeting)
			if(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
				continue;

			if(dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
			{
				strcpy_s(gfx_card, ARRAY_SIZE(gfx_card), (const char*)dd.DeviceString);
				return 0;
			}
		}
	}

	return -1;
}


// note: this implementation doesn't require OpenGL to be initialized.
static int win_get_gfx_drv_ver()
{
	if(gfx_drv_ver[0] != '\0')
		return -1;

	// rationale:
	// - we could easily determine the 2d driver via EnumDisplaySettings,
	//   but we want to query the actual OpenGL driver. see
	//   http://www.opengl.org/discussion_boards/ubb/Forum3/HTML/009679.html ;
	//   in short, we need the exact OpenGL driver version because some
	//   driver packs (e.g. Omega) mix and match DLLs.
	// - an alternative implementation would be to enumerate all
	//   DLLs loaded into the process, and check for a glBegin export.
	//   that requires toolhelp/PSAPI (a bit complicated) and telling
	//   ICD/opengl32.dll apart (not future-proof).
	// - therefore, we stick with the OpenGLDrivers approach. since there is
	//   no good way to determine which of the subkeys (e.g. nvoglnt) is
	//   active (several may exist due to previously removed drivers),
	//   we just display all of them. it is obvious from looking at
	//   gfx_card which one is correct; we thus avoid driver-specific
	//   name checks and reporting incorrectly.

	int ret = -1;	// single point of exit (for RegCloseKey)
	DWORD i;
	char drv_name[MAX_PATH+1];

	dll_list_init(gfx_drv_ver, GFX_DRV_VER_LEN);

	HKEY hkOglDrivers;
	const char* key = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\OpenGLDrivers";
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hkOglDrivers) != 0)
		return -1;

	// for each subkey (i.e. set of installed OpenGL drivers):
	for(i = 0; ; i++)
	{
		char set_name[32];
		DWORD set_name_len = ARRAY_SIZE(set_name);
		LONG err = RegEnumKeyEx(hkOglDrivers, i, set_name, &set_name_len, 0, 0,0, 0);
		if(err != ERROR_SUCCESS)	// error or no more items - bail
			break;

		HKEY hkSet;
		if(RegOpenKeyEx(hkOglDrivers, set_name, 0, KEY_QUERY_VALUE, &hkSet) == 0)
		{
			DWORD drv_name_len = ARRAY_SIZE(drv_name)-5;	// for ".dll"
			if(RegQueryValueEx(hkSet, "Dll", 0, 0, (LPBYTE)drv_name, &drv_name_len) == 0)
				ret = dll_list_add(drv_name);

			RegCloseKey(hkSet);
		}
	}	// for each subkey

	// for each value:
	// (some old drivers, e.g. S3 Super Savage, store their ICD name in a
	// single REG_SZ value. we therefore include those as well.)
	for(i = 0; ; i++)
	{
		char value_name[100];	// we don't need this, but RegEnumValue fails otherwise.
		DWORD value_name_len = ARRAY_SIZE(value_name);
		DWORD type;
		DWORD drv_name_len = ARRAY_SIZE(drv_name)-5;	// for ".dll"
		DWORD err = RegEnumValue(hkOglDrivers, i, value_name,&value_name_len,
			0, &type, (LPBYTE)drv_name,&drv_name_len);
		if(err != ERROR_SUCCESS)	// error or no more items - bail
			break;
		if(type == REG_SZ)
			ret = dll_list_add(drv_name);
	}	// for each value

	RegCloseKey(hkOglDrivers);
	return ret;
}


int win_get_gfx_info()
{
	int err1 = win_get_gfx_card();
	int err2 = win_get_gfx_drv_ver();

	// don't exit before trying both
	CHECK_ERR(err1);
	CHECK_ERR(err2);
	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
// sound card / driver version
//
//////////////////////////////////////////////////////////////////////////////

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
static int add_if_oal_dll(const DirEnt* ent, PathPackage* pp, StringSet* dlls)
{
	const char* fn = ent->name;

	// skip non-files.
	if(!DIRENT_IS_DIR(ent))
		return 0;

	// skip if not an OpenAL DLL.
	const size_t len = strlen(fn);
	const bool oal = len >= 7 && !stricmp(fn+len-7, "oal.dll");
	const bool openal = strstr(fn, "OpenAL") != 0;
	if(!oal && !openal)
		return 0;

	// skip if already in StringSet (i.e. has already been dll_list_add-ed)
	std::pair<StringSet::iterator, bool> ret = dlls->insert(fn);
	if(!ret.second)	// insert failed - element already there
		return 0;

	RETURN_ERR(pp_append_file(pp, fn));
	return dll_list_add(pp->path);
}


// find all OpenAL DLLs in a dir (via file_enum and add_if_oal_dll).
// call in library search order (exe dir, then win sys dir); otherwise,
// DLLs in the executable's starting directory hide those of the
// same name in the system directory.
//
// <dir>: no trailing.
static int add_oal_dlls_in_dir(const char* dir, StringSet* dlls)
{
	PathPackage pp;
	RETURN_ERR(pp_set_dir(&pp, dir));

	DirIterator d;
	RETURN_ERR(dir_open(dir, &d));

	DirEnt ent;
	for(;;)	// instead of while to avoid warning
	{
		int err = dir_next_ent(&d, &ent);
		if(err != 0)
			break;
		(void)add_if_oal_dll(&ent, &pp, dlls);
	}

	(void)dir_close(&d);
	return 0;
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


int win_get_snd_info()
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
		return 0;
	}

	// find all DLLs related to OpenAL, retrieve their versions,
	// and store in snd_drv_ver string.
	dll_list_init(snd_drv_ver, SND_DRV_VER_LEN);
	(void)dll_list_add(ds_drv_path);
	StringSet dlls;	// ensures uniqueness
	(void)add_oal_dlls_in_dir(win_exe_dir, &dlls);
	(void)add_oal_dlls_in_dir(win_sys_dir, &dlls);
	return 0;
}
