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

#include "win_internal.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// these are all delay-loaded - they're not needed if these
// routines are never called (to return system info).
#ifdef _MSC_VER
#pragma comment(lib, "version.lib")		// DLL version
#pragma comment(lib, "advapi32.lib")	// registry
#pragma comment(lib, "dsound.lib")		// sound card name
#endif


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


static int win_get_gfx_card()
{
	if(gfx_card[0] != '\0')
		return -1;

	// make sure EnumDisplayDevices is available (as pEnumDisplayDevicesA)
	if(import_EnumDisplayDevices() < 0)
		return -1;

	DISPLAY_DEVICEA dev = { sizeof(dev) };
	if(!pEnumDisplayDevicesA(0, 0, &dev, 0))
		return -1;

	strncpy(gfx_card, (const char*)dev.DeviceString, GFX_CARD_LEN-1);
	return 0;
}


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


int win_get_gfx_drv_ver()
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




// note: OpenAL alGetString is worthless: it only returns OpenAL API version
// and renderer (e.g. "Software").


// problem when including dsound.h: subwtype.h
// (included via d3dtypes.h and dsound.h) isn't present on VC6/7 hybrid.
// we're only using one DirectSound function ATM, so we'll declare it here.

#if 0
// mmsystem.h is necessary for dsound.h; we cut out unnecessary junk
# define MMNODRV         // Installable driver support
# define MMNOSOUND       // Sound support
//# define MMNOWAVE      // Waveform support
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
typedef BOOL (CALLBACK* LPDSENUMCALLBACKA)(void*, const char*, const char*, void*);
extern "C" __declspec(dllimport) HRESULT WINAPI DirectSoundEnumerateA(LPDSENUMCALLBACKA, void*);
#define DS_OK 0
#endif

static char ds_drv_name[MAX_PATH+1];

static int __stdcall ds_enum(void* guid, const char* description, const char* module, void* ctx)
{
	// skip if description == "Primary Sound Driver"
	if(module[0] == '\0')
		return true;	// continue calling

	// stick with the first "driver name" (sound card) we get;
	// in case there are several, we assume this is the one we want.

	strncpy(snd_card, description, SND_CARD_LEN-1);

	// .. DirectSound driver is in "$system\drivers\";
	//    save its path for version check later.
	snprintf(ds_drv_name, MAX_PATH, "%s\\drivers\\%s", win_sys_dir, module);

	return false;	// stop calling
}

static char* snd_drv_ver_pos = snd_drv_ver;

// module: complete path or filename only (if on default library search path)
static void add_drv(const char* module)
{
	char ver[32];
	if(get_ver(module, ver, sizeof(ver)) < 0)
		strcpy(ver, "unknown version");

	const char* module_fn = strrchr(module, '\\');
	if(!module_fn)
		module_fn = module;
	else
		module_fn++;

	// not first time: prepend comma
	if(snd_drv_ver_pos != snd_drv_ver)
		snd_drv_ver_pos += sprintf(snd_drv_ver_pos, ", ");
	snd_drv_ver_pos += sprintf(snd_drv_ver_pos, "%s (%s)", module_fn, ver);
}


int win_get_snd_info()
{
	if(DirectSoundEnumerateA((LPDSENUMCALLBACKA)ds_enum, (void*)0) != DS_OK)
		debug_warn("DirectSoundEnumerate failed");

	// find all DLLs related to OpenAL and retrieve their versions.
	// (search system dir for *oal.dll and *OpenAL*,
	// which is also how the router finds implementations).
	add_drv(ds_drv_name);
	DIR* dir = opendir(win_sys_dir);
	if(!dir)
	{
		assert(0);
		return -1;
	}
	while(dirent* ent = readdir(dir))
	{
		const char* fn = ent->d_name;
		const size_t len = strlen(fn);

		const bool oal = len > 7 && !stricmp(fn+len-7, "oal.dll");
		const bool openal = strstr(fn, "OpenAL") != 0;
		if(oal || openal)
			add_drv(fn);
	}
	closedir(dir);

	return 0;
}
