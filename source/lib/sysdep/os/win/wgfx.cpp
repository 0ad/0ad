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

/*
 * graphics card detection on Windows.
 */

#include "precompiled.h"
#include "lib/sysdep/gfx.h"

#include "wdll_ver.h"
#include "win.h"
#include "wmi.h"

#if MSC_VERSION
#pragma comment(lib, "advapi32.lib")	// registry
#endif


// useful for choosing a video mode.
// if we fail, outputs are unchanged (assumed initialized to defaults)
LibError gfx_get_video_mode(int* xres, int* yres, int* bpp, int* freq)
{
	DEVMODEA dm;
	memset(&dm, 0, sizeof(dm));
	dm.dmSize = sizeof(dm);
	// dm.dmDriverExtra already set to 0 by memset

	// (Win2k: don't use EnumDisplaySettingsW - BoundsChecker reports it causes
	// a memory overrun, even if called as the very first thing before
	// static CRT initialization.)
	if(!EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &dm))
		WARN_RETURN(ERR::FAIL);

	// EnumDisplaySettings is documented to set the values of the following:
	const DWORD expectedFlags = DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL|DM_DISPLAYFREQUENCY|DM_DISPLAYFLAGS;
	debug_assert((dm.dmFields & expectedFlags) == expectedFlags);

	if(xres)
		*xres = (int)dm.dmPelsWidth;
	if(yres)
		*yres = (int)dm.dmPelsHeight;
	if(bpp)
		*bpp  = (int)dm.dmBitsPerPel;
	if(freq)
		*freq = (int)dm.dmDisplayFrequency;

	return INFO::OK;
}


// useful for determining aspect ratio.
// if we fail, outputs are unchanged (assumed initialized to defaults)
LibError gfx_get_monitor_size(int& width_mm, int& height_mm)
{
	// (DC for the primary monitor's entire screen)
	HDC dc = GetDC(0);
	width_mm = GetDeviceCaps(dc, HORZSIZE);
	height_mm = GetDeviceCaps(dc, VERTSIZE);
	ReleaseDC(0, dc);
	return INFO::OK;
}


//-----------------------------------------------------------------------------

static LibError win_get_gfx_card()
{
	WmiMap wmiMap;
	RETURN_ERR(wmi_GetClass("Win32_VideoController", wmiMap));
	sprintf_s(gfx_card, GFX_CARD_LEN, "%ls", wmiMap[L"Caption"].bstrVal);
	return INFO::OK;
}


// note: this implementation doesn't require OpenGL to be initialized.
static LibError win_get_gfx_drv_ver()
{
	// don't overwrite existing information
	if(gfx_drv_ver[0] != '\0')
		return INFO::SKIPPED;

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

	DWORD i;
	char drv_name[MAX_PATH+1];
	std::string versionList;

	HKEY hkOglDrivers;
	const char* key = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\OpenGLDrivers";
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hkOglDrivers) != 0)
		WARN_RETURN(ERR::FAIL);

	// for each subkey (i.e. set of installed OpenGL drivers):
	for(i = 0; ; i++)
	{
		char set_name[32];
		DWORD set_name_len = ARRAY_SIZE(set_name);
		const LONG err = RegEnumKeyEx(hkOglDrivers, i, set_name, &set_name_len, 0, 0,0, 0);
		if(err == ERROR_NO_MORE_ITEMS)
			break;
		debug_assert(err == ERROR_SUCCESS);

		HKEY hkSet;
		if(RegOpenKeyEx(hkOglDrivers, set_name, 0, KEY_QUERY_VALUE, &hkSet) == 0)
		{
			DWORD drv_name_len = ARRAY_SIZE(drv_name)-5;	// for ".dll"
			if(RegQueryValueEx(hkSet, "Dll", 0, 0, (LPBYTE)drv_name, &drv_name_len) == 0)
				wdll_ver_Append(drv_name, versionList);

			RegCloseKey(hkSet);
		}
	}

	// for each value:
	// (some old drivers, e.g. S3 Super Savage, store their ICD name in a
	// single REG_SZ value. we therefore include those as well.)
	for(i = 0; ; i++)
	{
		char value_name[100];	// we don't need this, but RegEnumValue fails otherwise.
		DWORD value_name_len = ARRAY_SIZE(value_name);
		DWORD type;
		DWORD drv_name_len = ARRAY_SIZE(drv_name)-5;	// for ".dll"
		const DWORD err = RegEnumValue(hkOglDrivers, i, value_name, &value_name_len, 0, &type, (LPBYTE)drv_name, &drv_name_len);
		if(err == ERROR_NO_MORE_ITEMS)
			break;
		debug_assert(err == ERROR_SUCCESS);
		if(type == REG_SZ)
			wdll_ver_Append(drv_name, versionList);
	}

	RegCloseKey(hkOglDrivers);

	strcpy_s(gfx_drv_ver, GFX_DRV_VER_LEN, versionList.c_str());
	return INFO::OK;
}


LibError win_get_gfx_info()
{
	LibError err1 = win_get_gfx_card();
	LibError err2 = win_get_gfx_drv_ver();

	// don't exit before trying both
	RETURN_ERR(err1);
	RETURN_ERR(err2);
	return INFO::OK;
}
