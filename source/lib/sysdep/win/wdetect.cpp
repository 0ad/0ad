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
#include "lib/res/file.h"	// file_enum

#include "win_internal.h"

#include <assert.h>
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
#ifdef _MSC_VER
#pragma comment(lib, "version.lib")		// DLL version
#pragma comment(lib, "advapi32.lib")	// registry
#pragma comment(lib, "dsound.lib")		// sound card name
#endif



static int get_ver(const char* module, char* out_ver, size_t out_ver_len)
{
	WIN_SAVE_LAST_ERROR;	// GetFileVersion*, Ver*

	// allocate as much mem as required
	DWORD unused;
	const DWORD ver_size = GetFileVersionInfoSize(module, &unused);
	if(!ver_size)
		return -1;
	void* const buf = malloc(ver_size);
	if(!buf)
		return ERR_NO_MEM;

	// from here on, we set and later return this variable -
	// can't return directly, since we've allocated memory.
	int ret = -1;

	if(GetFileVersionInfo(module, 0, ver_size, buf))
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
				strncpy(out_ver, in_ver, out_ver_len);
				ret = 0;	// success
			}
		}
	}
	free(buf);

	WIN_RESTORE_LAST_ERROR;

	return ret;
}


// EnumDisplayDevices is not available on Win95 or NT.
// try to import it manually here; return -1 if not available.
static BOOL (WINAPI *pEnumDisplayDevicesA)(void*, DWORD, void*, DWORD);
static int import_EnumDisplayDevices()
{
	if(!pEnumDisplayDevicesA)
	{
		static HMODULE hUser32Dll = LoadLibrary("user32.dll");
		*(void**)&pEnumDisplayDevicesA = GetProcAddress(hUser32Dll, "EnumDisplayDevicesA");
//		FreeLibrary(hUser32Dll);
			// make sure the reference is released so BoundsChecker
			// doesn't complain. it won't actually be unloaded anyway -
			// there is at least one other reference.
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

	if(dm.dmFields & DM_PELSWIDTH && xres)
		*xres = (int)dm.dmPelsWidth;
	if(dm.dmFields & DM_PELSHEIGHT && yres)
		*yres = (int)dm.dmPelsHeight;
	if(dm.dmFields & DM_BITSPERPEL && bpp)
		*bpp  = (int)dm.dmBitsPerPel;
	if(dm.dmFields & DM_DISPLAYFREQUENCY && freq)
		*freq = (int)dm.dmDisplayFrequency;

	return 0;
}


// useful for determining aspect ratio.
// if we fail, outputs are unchanged (assumed initialized to defaults)
int get_monitor_size(int& width_mm, int& height_mm)
{
	HDC dc = GetDC(0);	// dc for entire screen

	 width_mm = GetDeviceCaps(dc, HORZSIZE);
	height_mm = GetDeviceCaps(dc, VERTSIZE);

	ReleaseDC(0, dc);
	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
// graphics card / driver version
//
//////////////////////////////////////////////////////////////////////////////


// get the name of the OpenGL driver DLL (used to determine driver version).
// see: http://www.opengl.org/discussion_boards/ubb/Forum3/HTML/009679.html
// implementation doesn't currently require OpenGL to be ready for use.
//
// an alternative would be to enumerate all DLLs loaded into the process,
// and check for a glBegin export. this requires OpenGL to be initialized,
// though - the DLLs aren't loaded at startup. it'd also be a bit of work
// to sort out MCD, ICD, and opengl32.dll.
static int get_ogl_drv_name(char* const ogl_drv_name, const size_t max_name_len)
{
	// need single point of exit so that we can close all keys; return this.
	int ret = -1;

	HKEY hkOglDrivers;
	const char* key = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\OpenGLDrivers";
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_ENUMERATE_SUB_KEYS, &hkOglDrivers) != 0)
		return -1;

	// we just use the first entry. it might be wrong on dual-graphics card
	// systems, but I don't see a better way to do it. there's no other
	// occurence of the OpenGL driver name in the registry on my system.
	char key_name[32];
	DWORD key_name_len = sizeof(key_name);
	if(RegEnumKeyEx(hkOglDrivers, 0, key_name, &key_name_len, 0, 0, 0, 0) == 0)
	{
		HKEY hkClass;
		if(RegOpenKeyEx(hkOglDrivers, key_name, 0, KEY_QUERY_VALUE, &hkClass) == 0)
		{
			DWORD size = (DWORD)max_name_len-5;	// -5 for ".dll"
			if(RegQueryValueEx(hkClass, "Dll", 0, 0, (LPBYTE)ogl_drv_name, &size) == 0)
			{
				// add .dll to filename, if not already there
				char* ext = strrchr(ogl_drv_name, '.');
				if(!ext || stricmp(ext, ".dll") != 0)
					strcat(ogl_drv_name, ".dll");

				ret = 0;	// success
			}

			RegCloseKey(hkClass);
		}
	}

	RegCloseKey(hkOglDrivers);

	return ret;
}


static int win_get_gfx_card()
{
	if(gfx_card[0] != '\0')
		return -1;

	// make sure EnumDisplayDevices is available (as pEnumDisplayDevicesA)
	if(import_EnumDisplayDevices() >= 0)
	{
		DISPLAY_DEVICEA dev = { sizeof(dev) };
		if(pEnumDisplayDevicesA(0, 0, &dev, 0))
		{
			strncpy(gfx_card, (const char*)dev.DeviceString, GFX_CARD_LEN-1);
			return 0;
		}
	}

	return -1;
}


static int win_get_gfx_drv_ver()
{
	if(gfx_drv_ver[0] != '\0')
		return -1;

	// note: getting the 2d driver name can be done with EnumDisplaySettings,
	// but we want the actual OpenGL driver. see discussion linked above;
	// the summary is, 2d driver version may differ from the OpenGL driver.
	char ogl_drv_name[MAX_PATH];
	CHECK_ERR(get_ogl_drv_name(ogl_drv_name, sizeof(ogl_drv_name)));
	CHECK_ERR(get_ver(ogl_drv_name, gfx_drv_ver, GFX_DRV_VER_LEN));
	return 0;
}


int win_get_gfx_info()
{
	win_get_gfx_card(); 
	win_get_gfx_drv_ver();
	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
// sound card / driver version
//
//////////////////////////////////////////////////////////////////////////////

// note: OpenAL alGetString is worthless: it only returns OpenAL API version
// and renderer (e.g. "Software").


//
// write list of sound driver DLLs and their versions into snd_drv_ver.
//
// be careful to respect library search order: DLLs in the executable's
// starting directory hide those of the same name in the system directory.
//

static std::set<std::string> dlls_already_added;

static char* snd_drv_ver_pos = snd_drv_ver;
	// driver strings will be appended here.

// if we haven't seen this DLL yet, read its file version and
// append that and its name to the list.
//
// dll_path: complete path to DLL (ensures we don't inadvertently
// load another one that's on the library search path). no trailing '\\'.
static void list_add_dll(const char* dll_path)
{
	// read file version.
	char dll_ver[32];
	if(get_ver(dll_path, dll_ver, sizeof(dll_ver)) < 0)
		strcpy(dll_ver, "unknown version");

	// get filename for "already added" check.
	const char* dll_fn = strrchr(dll_path, '\\')+1;
	assert(dll_fn != (const char*)1);	// fires if dll_path was a filename

	// make sure it hasn't been added yet.
	if(dlls_already_added.find(dll_fn) != dlls_already_added.end())
		return;
	dlls_already_added.insert(dll_fn);

	// not first time: prepend comma to string.
	if(snd_drv_ver_pos != snd_drv_ver)
		snd_drv_ver_pos += sprintf(snd_drv_ver_pos, ", ");
	snd_drv_ver_pos += sprintf(snd_drv_ver_pos, "%s (%s)", dll_fn, dll_ver);
}


// check_if_oal_dll needs to prepend directory to the filename it gets
// (for list_add_dll). it appends filename to directory in a buffer allocated
// list_check_dir (more efficient, less memory use than copying).
struct PathInfo
{
	const char* path;
	char* end;
	size_t remaining;
};

// if this file is an OpenAL DLL, add it to our list.
// (match "*oal.dll" and "*OpenAL*", as with OpenAL router's search).
static int check_if_oal_dll(const char* fn, const struct stat* s, const uintptr_t user)
{
	PathInfo* pi = (PathInfo*)user;
	strncpy(pi->end, fn, pi->remaining);

	const size_t len = strlen(fn);
	const bool oal = len >= 7 && !stricmp(fn+len-7, "oal.dll");
	const bool openal = strstr(fn, "OpenAL") != 0;
	if(oal || openal)
		list_add_dll(pi->path);
	return 0;	// continue calling
}


// find all OpenAL DLLs in a dir (via file_enum and check_if_oal_dll).
// call in library search order (exe dir, then win sys dir).
//
// dir: no trailing '\\'.
static void list_check_dir(const char* dir)
{
	char path[MAX_PATH+1]; path[MAX_PATH] = '\0';
	const int len = snprintf(path, MAX_PATH, "%s\\", dir);
	if(len < 0)
	{
		assert(0);
		return;
	}
	const PathInfo pi = { path, path+len, MAX_PATH-len };
	file_enum(path, check_if_oal_dll, (uintptr_t)&pi);
}


// free memory used while building the list;
// required before again building a new list.
static void list_free_mem()
{
	dlls_already_added.clear();
	snd_drv_ver_pos = snd_drv_ver;
}




// path to DS3D driver; filled by callback, used when checking version.
static char ds_drv_name[MAX_PATH+1];

// store sound card name and path to DirectSound driver.
// called for each DirectSound driver, but aborts after first valid driver.
static BOOL CALLBACK ds_enum(void* guid, const char* description, const char* module, void* ctx)
{
	// skip first (dummy) entry, where description == "Primary Sound Driver".
	if(module[0] == '\0')
		return TRUE;	// continue calling

	// stick with the first "driver name" (sound card) we get;
	// in case there are several, we assume this is the one we want.

	strncpy(snd_card, description, SND_CARD_LEN-1);

	// store DirectSound driver name for version check later
	// (it's in "win_sys_dir\drivers\").
	snprintf(ds_drv_name, MAX_PATH, "%s\\drivers\\%s", win_sys_dir, module);

	return FALSE;	// stop calling
}


int win_get_snd_info()
{
	// get sound card name
	if(DirectSoundEnumerateA((LPDSENUMCALLBACKA)ds_enum, (void*)0) != DS_OK)
		debug_warn("DirectSoundEnumerate failed");

	// find all DLLs related to OpenAL, retrieve their versions,
	// and store in snd_drv_ver string.
	list_add_dll(ds_drv_name);
	list_check_dir(win_exe_dir);
	list_check_dir(win_sys_dir);
	list_free_mem();

	return 0;
}
