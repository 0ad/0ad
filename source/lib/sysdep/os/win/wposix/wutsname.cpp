/* Copyright (C) 2021 Wildfire Games.
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

#include "lib/sysdep/os/win/wposix/wutsname.h"
#include "lib/sysdep/os/win/wutil.h"	// WinScopedPreserveLastError
#include "lib/sysdep/os/win/wversion.h"	// wversion_Family

#include <sstream>
#include <winternl.h>

/**
 * Taken and modified from: https://stackoverflow.com/questions/32115255/c-how-to-detect-windows-10
 */
int uname(struct utsname* un)
{
	WUTIL_FUNC(pRtlGetVersion, NTSTATUS, (LPOSVERSIONINFOEXW));
	WUTIL_IMPORT_NTDLL(RtlGetVersion , pRtlGetVersion);

	if (!pRtlGetVersion)
		return  -1;

	OSVERSIONINFOEXW osInfo = { sizeof(OSVERSIONINFOEXW) };
	pRtlGetVersion(&osInfo);
	std::ostringstream stream;

	// OS Implementation name
	if (osInfo.dwMajorVersion >= 10)
	{
		// Microsoft kept the 10.0.* version for Windows 11.
		// https://stackoverflow.com/questions/69836878/detecting-windows-11-properly
		// https://stackoverflow.com/questions/68510685/how-to-detect-windows-11-using-delphi-10-3-3/68517744#68517744
		if (osInfo.dwMajorVersion == 10 && osInfo.dwBuildNumber >= 22000)
			stream << "Win11";
		else
			stream << "Win" << osInfo.dwMajorVersion;
	}
	else
		stream << wversion_Family() << "\0";

	sprintf_s(un->sysname, ARRAY_SIZE(un->sysname), "%s", stream.str().c_str());


	// OS Service Pack
	int sp;
	if (swscanf_s(osInfo.szCSDVersion, L"Service Pack %d", &sp) == 1)
		sprintf_s(un->release, ARRAY_SIZE(un->release), "SP %d", sp);
	else
		un->release[0] = '\0';

	// OS Version.
	sprintf_s(un->version, ARRAY_SIZE(un->version), "%lu.%lu.%lu", osInfo.dwMajorVersion, osInfo.dwMinorVersion, osInfo.dwBuildNumber);

	// node name
	{
		WinScopedPreserveLastError s;	// GetComputerName
		DWORD bufSize = sizeof(un->nodename);
		WARN_IF_FALSE(GetComputerNameA(un->nodename, &bufSize));
	}

	// hardware type
	SYSTEM_INFO si;
	GetNativeSystemInfo(&si);
	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		strcpy_s(un->machine, ARRAY_SIZE(un->machine), "x64");
	else
		strcpy_s(un->machine, ARRAY_SIZE(un->machine), "x86");

	return 0;
}
