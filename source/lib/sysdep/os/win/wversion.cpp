/* Copyright (c) 2014 Wildfire Games
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

#include "precompiled.h"
#include "lib/sysdep/os/win/wversion.h"

#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/winit.h"

WINIT_REGISTER_EARLY_INIT(wversion_Init);


static wchar_t windowsVersionString[20];
static size_t windowsVersion;	// see WVERSION_*


const wchar_t* wversion_Family()
{
	ENSURE(windowsVersion != 0);
	switch(windowsVersion)
	{
	case WVERSION_2K:
		return L"Win2k";
	case WVERSION_XP:
		return L"WinXP";
	case WVERSION_XP64:
		return L"WinXP64";
	case WVERSION_VISTA:
		return L"Vista";
	case WVERSION_7:
		return L"Win7";
	case WVERSION_8:
		return L"Win8";
	case WVERSION_8_1:
		return L"Win8.1";
	case WVERSION_10:
		return L"Win10";
	default:
		return L"Windows";
	}
}


const wchar_t* wversion_String()
{
	ENSURE(windowsVersionString[0] != '\0');
	return windowsVersionString;
}

size_t wversion_Number()
{
	ENSURE(windowsVersion != 0);
	return windowsVersion;
}


static Status wversion_Init()
{
	// note: don't use GetVersion[Ex] because it gives the version of the
	// emulated OS when running an app with compatibility shims enabled.
	HKEY hKey;
	if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		DWORD size = sizeof(windowsVersionString);
		(void)RegQueryValueExW(hKey, L"CurrentVersion", 0, 0, (LPBYTE)windowsVersionString, &size);

		unsigned major = 0, minor = 0;
		// ICC 11.1.082 generates incorrect code for the following:
		// const int ret = swscanf_s(windowsVersionString, L"%u.%u", &major, &minor);
		std::wstringstream ss(windowsVersionString);
		ss >> major;
		wchar_t dot;
		ss >> dot;
		ENSURE(dot == '.');
		ss >> minor;

		ENSURE(4 <= major && major <= 0xFF);
		ENSURE(minor <= 0xFF);
		windowsVersion = (major << 8) | minor;

		RegCloseKey(hKey);
	}
	else
		DEBUG_WARN_ERR(ERR::LOGIC);

	return INFO::OK;
}
