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
 * graphics card detection on Windows.
 */

#include "precompiled.h"
#include "lib/sysdep/gfx.h"

#include "lib/sysdep/os/win/wdll_ver.h"
#include "lib/sysdep/os/win/wutil.h"
#include "lib/sysdep/os/win/wmi.h"

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
	RETURN_ERR(wmi_GetClass(L"Win32_VideoController", wmiMap));
	swprintf_s(gfx_card, GFX_CARD_LEN, L"%ls", wmiMap[L"Caption"].bstrVal);
	return INFO::OK;
}


// note: this implementation doesn't require OpenGL to be initialized.
static LibError AppendDriverVersionsFromRegistry(std::wstring& versionList)
{
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

	wchar_t dllName[MAX_PATH+1];

	HKEY hkDrivers;
	const wchar_t* key = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\OpenGLDrivers";
	if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hkDrivers) != 0)
		WARN_RETURN(ERR::FAIL);

	// for each subkey (i.e. installed OpenGL driver):
	for(DWORD i = 0; ; i++)
	{
		wchar_t driverName[32];
		DWORD driverNameLength = ARRAY_SIZE(driverName);
		const LONG err = RegEnumKeyExW(hkDrivers, i, driverName, &driverNameLength, 0, 0, 0, 0);
		if(err == ERROR_NO_MORE_ITEMS)
		{
			if(i == 0)
			{
				RegCloseKey(hkDrivers);
				return ERR::NO_SYS;	// NOWARN (ATI and NVidia don't create sub-keys on Windows 7)
			}
			break;
		}
		debug_assert(err == ERROR_SUCCESS);

		HKEY hkDriver;
		if(RegOpenKeyExW(hkDrivers, driverName, 0, KEY_QUERY_VALUE, &hkDriver) == 0)
		{
			DWORD dllNameLength = ARRAY_SIZE(dllName)-5;	// for ".dll"
			if(RegQueryValueExW(hkDriver, L"Dll", 0, 0, (LPBYTE)dllName, &dllNameLength) == 0)
				wdll_ver_Append(dllName, versionList);

			RegCloseKey(hkDriver);
		}
	}

	// for each value:
	// (some old drivers, e.g. S3 Super Savage, store their ICD name in a
	// single REG_SZ value. we therefore include those as well.)
	for(DWORD i = 0; ; i++)
	{
		wchar_t name[100];	// we don't need this, but RegEnumValue fails otherwise.
		DWORD nameLength = ARRAY_SIZE(name);
		DWORD type;
		DWORD dllNameLength = ARRAY_SIZE(dllName)-5;	// for ".dll"
		const DWORD err = RegEnumValueW(hkDrivers, i, name, &nameLength, 0, &type, (LPBYTE)dllName, &dllNameLength);
		if(err == ERROR_NO_MORE_ITEMS)
			break;
		debug_assert(err == ERROR_SUCCESS);
		if(type == REG_SZ)
			wdll_ver_Append(dllName, versionList);
	}

	RegCloseKey(hkDrivers);

	return INFO::OK;
}


static LibError AppendDriverVersionsFromKnownFiles(std::wstring& versionList)
{
	if(!wcsncmp(gfx_card, L"NVIDIA", 6))
	{
		wdll_ver_Append(L"nvoglv64.dll", versionList);
		wdll_ver_Append(L"nvoglv32.dll", versionList);
		wdll_ver_Append(L"nvoglnt.dll", versionList);
	}
	else if(!wcsncmp(gfx_card, L"ATI", 3))
	{
		wdll_ver_Append(L"atioglxx.dll", versionList);
	}
	else if(!wcsncmp(gfx_card, L"Intel", 5))
	{
		wdll_ver_Append(L"igxpdv32.dll", versionList);
	}
	else
		return INFO::CANNOT_HANDLE;
	return INFO::OK;
}


LibError win_get_gfx_info()
{
	LibError err = win_get_gfx_card();

	std::wstring versionList;
	if(AppendDriverVersionsFromRegistry(versionList) != INFO::OK)	// (fails on Windows 7)
	{
		if(AppendDriverVersionsFromKnownFiles(versionList) != INFO::OK)
			versionList = L"(unknown)";
	}
	wcscpy_s(gfx_drv_ver, GFX_DRV_VER_LEN, versionList.c_str());

	return err;
}
